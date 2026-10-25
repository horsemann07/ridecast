/* ============================================================================
 * File        : bsp_uart.c
 * Description : ESP-IDF 5.5 UART implementation of portable BSP UART API
 * Author      : Jha Raghav
 * Date        : 2026-10-24
 * ============================================================================
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Description:
 *   Implements platform-neutral UART operations using ESP-IDF UART drivers.
 *   When enabled, one port-local FreeRTOS task consumes native ESP-IDF UART
 *   event queues; no native RTOS types appear in bsp_uart.h.
 *
 * Integration:
 *   Build this file only for ESP targets; add esp_driver_uart and freertos
 *   to the BSP component dependencies. Do not share ports with ESP console.
 *
 * Requirements:
 *   ESP-IDF 5.5, FreeRTOS, BSP status definitions.
 * ===========================================================================
 * Change Log (newest first):
 * Date          Author          Description
 * ------------  --------------  ----------------------------------------------
 * 2026-10-24    Jha Raghav      Reconciled handle, callbacks, async lifecycle
 * 2026-10-23    Jha Raghav      Initial ESP UART draft (date to be confirmed)
 * ============================================================================ */

#include "bsp_uart.h"
#include "bsp_config.h"

#include <limits.h>
#include <string.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"


/* -------------------------------------------------------------------------- */
/* UART Configuration                                                         */
/* -------------------------------------------------------------------------- */

/* Enable or disable asynchronous UART operations (0U: disabled, 1U: enabled). */
#ifndef configUART_ENABLE_ASYNC
    #define configUART_ENABLE_ASYNC (1U)
#endif

/* Configure the native UART TX and RX buffer size in bytes. */
#ifndef configUART_RXTX_BUFFER_SIZE
    #define configUART_RXTX_BUFFER_SIZE (1024U)
#endif

/* Configure the asynchronous UART event queue depth. */
#ifndef configUART_ASYNC_EVENT_QUEUE_LEN
    #define configUART_ASYNC_EVENT_QUEUE_LEN (16U)
#endif

/* Configure the asynchronous UART worker stack size in bytes. */
#ifndef configUART_TASK_STACK_BYTES
    #define configUART_TASK_STACK_BYTES (4096U)
#endif

/* Configure the asynchronous UART worker task priority. */
#ifndef configUART_TASK_PRIORITY
    #define configUART_TASK_PRIORITY (8U)
#endif

/* Configure the UART event polling interval in milliseconds. */
#ifndef configUART_POLL_MS
    #define configUART_POLL_MS (10U)
#endif

/* Configure the UART worker shutdown timeout in milliseconds. */
#ifndef configUART_SHUTDOWN_MS
    #define configUART_SHUTDOWN_MS (1000U)
#endif

/* -------------------------------------------------------------------------- */
/* UART Compile-Time Configuration Validation                                 */
/* -------------------------------------------------------------------------- */

/* Restrict asynchronous UART configuration to supported Boolean values. */
#if ((configUART_ENABLE_ASYNC != 0U) && (configUART_ENABLE_ASYNC != 1U))
    #error "configUART_ENABLE_ASYNC must be 0 or 1"
#endif

/* Ensure the UART driver buffer meets the minimum required size. */
#if (configUART_RXTX_BUFFER_SIZE < 256U)
    #error "configUART_RXTX_BUFFER_SIZE must be at least 256 bytes"
#endif

/* Validate event queue depth and polling interval when async UART is enabled. */
#if (configUART_ENABLE_ASYNC == 1U)
    #if (configUART_ASYNC_EVENT_QUEUE_LEN < 1U)
        #error "configUART_ASYNC_EVENT_QUEUE_LEN must be nonzero"
    #endif

    #if (configUART_POLL_MS < 1U)
        #error "configUART_POLL_MS must be nonzero"
    #endif
#endif

/* -------------------------------------------------------------------------- */
/* UART Module-Local Configuration Aliases                                    */
/* -------------------------------------------------------------------------- */

/* Enable asynchronous UART operations. */
#define UART_ENABLE_ASYNC          configUART_ENABLE_ASYNC

/* Native UART TX and RX buffer size in bytes. */
#define UART_RXTX_BUFFER_SIZE      configUART_RXTX_BUFFER_SIZE

/* Asynchronous UART event queue depth. */
#define UART_ASYNC_EVENT_QUEUE_LEN configUART_ASYNC_EVENT_QUEUE_LEN

/* UART worker task stack size in bytes. */
#define UART_TASK_STACK_BYTES      configUART_TASK_STACK_BYTES

/* UART worker task priority. */
#define UART_TASK_PRIORITY         configUART_TASK_PRIORITY

/* UART event polling interval in milliseconds. */
#define UART_POLL_MS               configUART_POLL_MS

/* UART worker shutdown timeout in milliseconds. */
#define UART_SHUTDOWN_MS           configUART_SHUTDOWN_MS

#define UART_RX_CHUNK_BYTES        configUART_RX_CHUNK_BYTES
#define UART_RX_TIMEOUT_SYMBOLS    configUART_RX_TIMEOUT_SYMBOLS

#if ((UART_RX_CHUNK_BYTES == 0U) || (UART_RX_CHUNK_BYTES > 1024U))
    #error "UART RX chunk size must be between 1 and 1024"
#endif

#if (UART_RX_TIMEOUT_SYMBOLS > 126U)
    #error "UART RX timeout symbols must not exceed 126"
#endif
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */

/**
 * @brief Maintains the private runtime state of a UART hardware port.
 *
 * @details Stores UART ownership, configuration, synchronization resources,
 * transfer state, error counters, and optional asynchronous worker resources.
 *
 * @note Runtime state is protected by the per-port mutex, while port
 * reservation is synchronized using the global reservation spinlock.
 */
typedef struct
{
    bspUartHandle_t* owner;  /**< Application handle owning the UART port. */
    bspUartConfig_t config;  /**< Active UART configuration. */
    SemaphoreHandle_t mutex; /**< Mutex protecting the port runtime state. */
    bool reserved;           /**< Port reservation state. */
    bool initialized;        /**< UART initialization state. */
    bool stopping;           /**< UART shutdown-in-progress flag. */
    bool rx_busy;            /**< Active asynchronous RX state. */
    size_t rx_count;         /**< Latest recorded RX byte count. */
    size_t tx_count;         /**< Accumulated transmitted byte count. */
    uint32_t rx_errors;      /**< Accumulated RX error count. */
    uint32_t tx_errors;      /**< Accumulated TX error count. */
    bspUartRxCallback_t rx_callback; /**< Registered RX callback. */
    void* rx_callback_context; /**< Application-defined RX callback context. */
    QueueHandle_t event_queue;
} uart_port_state_t;

/* -------------------------------------------------------------------------- */
/* Private UART Runtime Variables                                             */
/* -------------------------------------------------------------------------- */

/* Maintain one statically allocated runtime state for each native UART port. */
static uart_port_state_t s_ports[UART_NUM_MAX];

/* Protect UART port reservation against concurrent initialization attempts. */
static portMUX_TYPE s_claim_lock = portMUX_INITIALIZER_UNLOCKED;

/* -------------------------------------------------------------------------- */

/* ============================================================================
 * PRIVATE FUNCTIONS DECLARATIONS
 * ========================================================================== */

/* -------------------------------------------------------------------------- */
/* Private UART Function Declarations                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Convert a millisecond timeout into FreeRTOS ticks.
 *
 * @param[in] ms Timeout in milliseconds.
 *
 * @return Converted timeout in RTOS ticks.
 *
 * @note Positive timeouts are rounded upward to the nearest tick.
 * The result is limited to a value below portMAX_DELAY.
 */
static TickType_t ticks_from_ms(uint32_t ms);

/* -------------------------------------------------------------------------- */

/**
 * @brief Validate a BSP UART port identifier.
 *
 * @param[in] port UART port identifier to validate.
 *
 * @retval true The port identifier is supported.
 * @retval false The port identifier is outside the supported range.
 */
static bool valid_port(bspUartPort_t port);

/* -------------------------------------------------------------------------- */

/**
 * @brief Convert a BSP UART configuration into a native port identifier.
 *
 * @param[in] cfg Pointer to a valid UART configuration.
 *
 * @return Corresponding ESP-IDF UART port identifier.
 *
 * @pre cfg must not be NULL.
 */
static uart_port_t uart_id(const bspUartConfig_t* cfg);

/* -------------------------------------------------------------------------- */

/**
 * @brief Determine whether the UART operating mode supports transmission.
 *
 * @param[in] mode Configured UART operating mode.
 *
 * @retval true Transmission is enabled.
 * @retval false Transmission is disabled.
 */
static bool tx_enabled(bspUartMode_t mode);

/* -------------------------------------------------------------------------- */

/**
 * @brief Determine whether the UART operating mode supports reception.
 *
 * @param[in] mode Configured UART operating mode.
 *
 * @retval true Reception is enabled.
 * @retval false Reception is disabled.
 */
static bool rx_enabled(bspUartMode_t mode);

/* -------------------------------------------------------------------------- */

/**
 * @brief Validate a BSP UART configuration.
 *
 * @param[in] cfg Pointer to the UART configuration to validate.
 *
 * @retval RET_ERR_STS_OK Configuration is valid.
 * @retval RET_ERR_STS_INVALID_PARAM Configuration contains invalid values.
 * @retval RET_ERR_STS_NOT_SUPPORTED Configuration requests unsupported features.
 *
 * @note Validation does not modify the UART hardware or runtime state.
 */
static ret_err_sts_t validate_config(const bspUartConfig_t* cfg);

/* -------------------------------------------------------------------------- */

/**
 * @brief Convert a BSP GPIO identifier into an ESP-IDF pin value.
 *
 * @param[in] pin BSP GPIO identifier or BSP_UART_PIN_NO_CHANGE.
 *
 * @return Native GPIO number or UART_PIN_NO_CHANGE.
 */
static int gpio_or_unchanged(uint32_t pin);

/* -------------------------------------------------------------------------- */

/**
 * @brief Apply a validated UART configuration to the native driver.
 *
 * @param[in] cfg Pointer to a previously validated UART configuration.
 *
 * @retval ESP_OK Configuration applied successfully.
 * @return ESP-IDF error code if a hardware configuration operation fails.
 *
 * @pre cfg must not be NULL.
 *
 * @note Configuration includes communication parameters, GPIO routing,
 * signal inversion, and the optional receive threshold.
 */
static esp_err_t apply_config(const bspUartConfig_t* cfg);

/* -------------------------------------------------------------------------- */

/**
 * @brief Validate ownership and acquire the mutex of a UART instance.
 *
 * @param[in] handle Pointer to the application-owned UART handle.
 * @param[out] result Receives the associated UART runtime state pointer.
 *
 * @retval RET_ERR_STS_OK UART validated and mutex acquired.
 * @retval RET_ERR_STS_INVALID_PARAM Invalid handle, output pointer, or port.
 * @retval RET_ERR_STS_NOT_INITIALIZED UART is unavailable or not owned
 * by the requesting handle.
 * @retval RET_ERR_STS_FAIL Mutex acquisition failed.
 *
 * @warning On success, the caller must release the mutex using
 * release_port().
 *
 * @note The function does not return with the mutex held on failure.
 */
static ret_err_sts_t acquire_port(const bspUartHandle_t* handle, uart_port_state_t** result);

/* -------------------------------------------------------------------------- */

/**
 * @brief Release the mutex associated with a UART runtime instance.
 *
 * @param[in] port Pointer to the UART runtime state.
 *
 * @return None.
 *
 * @pre port must be valid and its mutex must be held by the calling task.
 */
static void release_port(uart_port_state_t* port);

/* -------------------------------------------------------------------------- */

/**
 * @brief Convert an ESP-IDF driver result into a BSP status code.
 *
 * @param[in] err ESP-IDF error code to convert.
 *
 * @retval RET_ERR_STS_OK Native operation succeeded.
 * @retval RET_ERR_STS_TIMEOUT Native operation timed out.
 * @retval RET_ERR_STS_INVALID_PARAM Native operation received invalid arguments.
 * @retval RET_ERR_STS_FAIL Other native driver failure.
 */
static ret_err_sts_t map_driver_error(esp_err_t err);
/* -------------------------------------------------------------------------- */
/* ============================================================================
 * PRIVATE FUNCTIONS DEFINITIONS
 * ========================================================================== */

/* -------------------------------------------------------------------------- */
/* Millisecond to RTOS Tick Conversion                                        */
/* -------------------------------------------------------------------------- */

static TickType_t ticks_from_ms(uint32_t ms)
{
    uint64_t t;

    /* Preserve nonblocking behavior for a zero timeout. */
    if(ms == 0U)
    {
        return (TickType_t)0;
    }

    /*
     * Use 64-bit arithmetic to prevent overflow during conversion.
     * Round positive millisecond values upward to a whole tick.
     */
    t = (((uint64_t)ms * (uint64_t)configTICK_RATE_HZ) + 999ULL) / 1000ULL;

    /* Ensure a positive timeout does not become zero ticks. */
    if(t == 0ULL)
    {
        t = 1ULL;
    }

    /*
     * Reserve portMAX_DELAY for the FreeRTOS infinite-wait value.
     * Limit finite timeouts to the largest permitted tick count.
     */
    if(t >= (uint64_t)portMAX_DELAY)
    {
        t = (uint64_t)portMAX_DELAY - 1ULL;
    }

    return (TickType_t)t;
}

/* -------------------------------------------------------------------------- */
/* UART Port Validation                                                       */
/* -------------------------------------------------------------------------- */

static bool valid_port(bspUartPort_t port)
{
    /*
     * Validate the requested port against both the native ESP-IDF
     * port count and the maximum identifier exposed by the BSP.
     */
    return (((unsigned)port < (unsigned)UART_NUM_MAX) &&
            ((unsigned)port <= (unsigned)eBspUartPort7));
}

/* -------------------------------------------------------------------------- */
/* Native UART Port Identifier                                                */
/* -------------------------------------------------------------------------- */

static uart_port_t uart_id(const bspUartConfig_t* cfg)
{
    /* Convert the validated BSP UART identifier to its native type. */
    return (uart_port_t)cfg->port;
}

/* -------------------------------------------------------------------------- */
/* UART Transmission Mode Check                                               */
/* -------------------------------------------------------------------------- */

static bool tx_enabled(bspUartMode_t mode)
{
    /* Transmission is supported in TX-only and combined TX/RX modes. */
    return ((mode == eBspUartModeTx) || (mode == eBspUartModeTxRx));
}

/* -------------------------------------------------------------------------- */
/* UART Reception Mode Check                                                  */
/* -------------------------------------------------------------------------- */

static bool rx_enabled(bspUartMode_t mode)
{
    /* Reception is supported in RX-only and combined TX/RX modes. */
    return ((mode == eBspUartModeRx) || (mode == eBspUartModeTxRx));
}

/* -------------------------------------------------------------------------- */
/* UART Configuration Validation                                              */
/* -------------------------------------------------------------------------- */

static ret_err_sts_t validate_config(const bspUartConfig_t* cfg)
{
    /*
     * Validate the configuration pointer, hardware port identifier,
     * and baud rate before accessing the remaining parameters.
     */
    if((cfg == NULL) || (!valid_port(cfg->port)) || (cfg->baudrate == 0U) ||
       (cfg->baudrate > (uint32_t)INT_MAX))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Reject unsupported enumeration values before they are
     * converted into native ESP-IDF UART configuration values.
     */
    if((cfg->wordLength < eBspUartWordLength5) || (cfg->wordLength > eBspUartWordLength8) ||
       (cfg->parity > eBspUartParityEven) || (cfg->stopBits > eBspUartStopBitsTwo) ||
       (cfg->mode > eBspUartModeTxRx) || (cfg->flowControl > eBspUartFlowControlRtsCts))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Reject optional BSP features that do not have a supported
     * implementation in the selected native UART driver.
     */
    if((cfg->dmaMode != eBspUartDmaDisabled) || (cfg->fifoSize != 0U) ||
       (cfg->oversampling != 0U))
    {
        return RET_ERR_STS_NOT_SUPPORTED;
    }

    /* Validate the hardware RX FIFO threshold when explicitly configured. */
    if((cfg->rxThreshold > 0U) && (cfg->rxThreshold >= 128U))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Require explicit RTS and CTS pin assignments when
     * hardware flow control is enabled.
     */
    if((cfg->flowControl == eBspUartFlowControlRtsCts) &&
       ((cfg->uartRtsPin == BSP_UART_PIN_NO_CHANGE) || (cfg->uartCtsPin == BSP_UART_PIN_NO_CHANGE)))
    {
        /* For portability, require explicit pins when enabling RTS/CTS. */
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Collect the configured GPIO identifiers for consistent validation.
     * BSP_UART_PIN_NO_CHANGE is accepted without numeric conversion.
     */
    const uint32_t pins[4] = { cfg->uartTxPin, cfg->uartRxPin, cfg->uartRtsPin,
                               cfg->uartCtsPin };

    for(size_t i = 0U; i < 4U; i++)
    {
        /*
         * Reject GPIO values that cannot be represented by the
         * signed integer type expected by the native driver.
         */
        if((pins[i] != BSP_UART_PIN_NO_CHANGE) && (pins[i] > (uint32_t)INT_MAX))
        {
            return RET_ERR_STS_INVALID_PARAM;
        }
    }

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */
/* Native GPIO Identifier Conversion                                          */
/* -------------------------------------------------------------------------- */

static int gpio_or_unchanged(uint32_t pin)
{
    /*
     * Preserve the BSP no-change sentinel.
     * Convert an explicitly configured GPIO to the native integer type.
     */
    return (pin == BSP_UART_PIN_NO_CHANGE) ? UART_PIN_NO_CHANGE : (int)pin;
}

/* -------------------------------------------------------------------------- */
/* Native UART Configuration                                                  */
/* -------------------------------------------------------------------------- */

static esp_err_t apply_config(const bspUartConfig_t* cfg)
{
    uart_config_t native_cfg = { 0 };
    esp_err_t err;
    uint32_t inversions = UART_SIGNAL_INV_DISABLE;

    /* Map the validated baud rate into the native UART configuration. */
    native_cfg.baud_rate = (int)cfg->baudrate;

    /* Translate the BSP word length into the corresponding UART data bits. */
    switch(cfg->wordLength)
    {
        case eBspUartWordLength5:
            native_cfg.data_bits = UART_DATA_5_BITS;
            break;

        case eBspUartWordLength6:
            native_cfg.data_bits = UART_DATA_6_BITS;
            break;

        case eBspUartWordLength7:
            native_cfg.data_bits = UART_DATA_7_BITS;
            break;

        default:
            /* The validated default word length is eight data bits. */
            native_cfg.data_bits = UART_DATA_8_BITS;
            break;
    }

    /* Configure parity according to the requested communication format. */
    native_cfg.parity = UART_PARITY_DISABLE;

    if(cfg->parity == eBspUartParityOdd)
    {
        native_cfg.parity = UART_PARITY_ODD;
    }
    else if(cfg->parity == eBspUartParityEven)
    {
        native_cfg.parity = UART_PARITY_EVEN;
    }

    /* Select the configured number of UART stop bits. */
    native_cfg.stop_bits =
    (cfg->stopBits == eBspUartStopBitsTwo) ? UART_STOP_BITS_2 : UART_STOP_BITS_1;

    /* Enable hardware flow control only when explicitly configured. */
    native_cfg.flow_ctrl = (cfg->flowControl == eBspUartFlowControlRtsCts) ?
                           UART_HW_FLOWCTRL_CTS_RTS :
                           UART_HW_FLOWCTRL_DISABLE;

    /*
     * Apply the configured receive threshold.
     * A zero configuration selects the driver's default BSP threshold.
     */
    native_cfg.rx_flow_ctrl_thresh =
    (cfg->rxThreshold == 0U) ? (uint8_t)64U : (uint8_t)cfg->rxThreshold;

    /* Use the native driver's default UART clock source. */
    native_cfg.source_clk = UART_SCLK_DEFAULT;

    /*
     * Configure native UART communication parameters first.
     * Stop configuration immediately when the driver reports failure.
     */
    err = uart_param_config(uart_id(cfg), &native_cfg);

    if(err != ESP_OK)
    {
        return err;
    }

    /* Apply the requested GPIO routing after parameter configuration. */
    err = uart_set_pin(uart_id(cfg), gpio_or_unchanged(cfg->uartTxPin),
                       gpio_or_unchanged(cfg->uartRxPin), gpio_or_unchanged(cfg->uartRtsPin),
                       gpio_or_unchanged(cfg->uartCtsPin));

    if(err != ESP_OK)
    {
        return err;
    }

    /* Construct the optional TX and RX signal inversion mask. */
    if(cfg->invertTx)
    {
        inversions |= UART_SIGNAL_TXD_INV;
    }

    if(cfg->invertRx)
    {
        inversions |= UART_SIGNAL_RXD_INV;
    }

    /*
     * Apply signal inversion after configuring the UART pins.
     * An explicitly configured RX threshold is applied only
     * when the inversion operation succeeds.
     */
    err = uart_set_line_inverse(uart_id(cfg), inversions);

    if((err == ESP_OK) && (cfg->rxThreshold != 0U))
    {
        err = uart_set_rx_full_threshold(uart_id(cfg), cfg->rxThreshold);
    }

    return err;
}

/* -------------------------------------------------------------------------- */
/* UART Port Acquisition                                                      */
/* -------------------------------------------------------------------------- */

static ret_err_sts_t acquire_port(const bspUartHandle_t* handle, uart_port_state_t** result)
{
    uart_port_state_t* port;

    /*
     * Validate the caller handle, output pointer, and port identifier
     * before indexing the global UART runtime state array.
     */
    if((handle == NULL) || (result == NULL) || (!valid_port(handle->config.port)))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Resolve the internal state associated with the requested UART port. */
    port = &s_ports[(unsigned)handle->config.port];

    /* A missing port mutex indicates that initialization is unavailable. */
    if(port->mutex == NULL)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /*
     * Acquire exclusive access to the UART runtime state.
     * The caller is responsible for releasing this mutex on success.
     */
    if(xSemaphoreTake(port->mutex, portMAX_DELAY) != pdTRUE)
    {
        return RET_ERR_STS_FAIL;
    }

    /*
     * Verify that initialization completed, shutdown is not active,
     * and the requesting handle owns the UART instance.
     */
    if((!port->initialized) || port->stopping || (port->owner != handle))
    {
        (void)xSemaphoreGive(port->mutex);
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /* Publish the validated runtime state while retaining the mutex. */
    *result = port;

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */
/* UART Port Release                                                          */
/* -------------------------------------------------------------------------- */

static void release_port(uart_port_state_t* port)
{
    /*
     * Release exclusive access to the UART runtime state.
     * The calling task must previously have acquired the port mutex.
     */
    (void)xSemaphoreGive(port->mutex);
}

/* -------------------------------------------------------------------------- */
/* Native Driver Error Mapping                                                */
/* -------------------------------------------------------------------------- */

static ret_err_sts_t map_driver_error(esp_err_t err)
{
    /* Map successful native operations to the BSP success status. */
    if(err == ESP_OK)
    {
        return RET_ERR_STS_OK;
    }

    /* Preserve timeout information for application-level handling. */
    if(err == ESP_ERR_TIMEOUT)
    {
        return RET_ERR_STS_TIMEOUT;
    }

    /* Convert native argument validation failures to BSP errors. */
    if(err == ESP_ERR_INVALID_ARG)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Map all remaining driver failures to the generic BSP error. */
    return RET_ERR_STS_FAIL;
}


/* ============================================================================
 * UART PUBLIC API IMPLEMENTATION
 * ========================================================================== */

ret_err_sts_t bspUartInit(bspUartHandle_t* handle)
{
    ret_err_sts_t result;
    esp_err_t err;
    uart_port_state_t* portState;

    if(handle == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    result = validate_config(&handle->config);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    portState = &s_ports[(unsigned)handle->config.port];

    /* Reserve the hardware UART port. */
    portENTER_CRITICAL(&s_claim_lock);

    if(portState->reserved)
    {
        portEXIT_CRITICAL(&s_claim_lock);
        return RET_ERR_STS_ALREADY_INIT;
    }

    portState->reserved = true;

    portEXIT_CRITICAL(&s_claim_lock);

    portState->owner  = handle;
    portState->config = handle->config;

    portState->mutex = xSemaphoreCreateMutex();

    if(portState->mutex == NULL)
    {
        result = RET_ERR_STS_NO_MEM;
        goto init_failed;
    }

    /*
     * Install ESP-IDF UART driver.
     *
     * The native event queue belongs to the BSP, but the
     * application RX thread consumes it through bspUartProcessRx().
     *
     * No additional BSP task is created.
     */
    err = uart_driver_install(uart_id(&portState->config), (int)UART_RXTX_BUFFER_SIZE,
                              (int)UART_RXTX_BUFFER_SIZE, (int)UART_ASYNC_EVENT_QUEUE_LEN,
                              &portState->event_queue, 0);

    if(err != ESP_OK)
    {
        result = map_driver_error(err);
        goto init_failed;
    }

    err = apply_config(&portState->config);

    if(err != ESP_OK)
    {
        result = map_driver_error(err);
        goto driver_failed;
    }

#if (UART_ENABLE_ASYNC == 1U)

    /*
     * Trigger RX activity on an idle gap even when the
     * received data is smaller than the FIFO threshold.
     */
    err = uart_set_rx_timeout(uart_id(&portState->config), (uint8_t)UART_RX_TIMEOUT_SYMBOLS);

    if(err != ESP_OK)
    {
        result = map_driver_error(err);
        goto driver_failed;
    }

#endif

    portState->initialized = true;

    return RET_ERR_STS_OK;

driver_failed:
    (void)uart_driver_delete(uart_id(&portState->config));

init_failed:
    if(portState->mutex != NULL)
    {
        vSemaphoreDelete(portState->mutex);
    }

    portENTER_CRITICAL(&s_claim_lock);
    (void)memset(portState, 0, sizeof(*portState));
    portEXIT_CRITICAL(&s_claim_lock);

    return result;
}
/* -------------------------------------------------------------------------- */
ret_err_sts_t bspUartDeInit(bspUartHandle_t* handle)
{
    uart_port_state_t* portState;
    ret_err_sts_t result;

    if((handle == NULL) || (!valid_port(handle->config.port)))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    portState = &s_ports[(unsigned)handle->config.port];

    if(portState->mutex == NULL)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(xSemaphoreTake(portState->mutex, portMAX_DELAY) != pdTRUE)
    {
        return RET_ERR_STS_FAIL;
    }

    if((portState->owner != handle) || (!portState->initialized))
    {
        release_port(portState);
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /*
     * The application RX thread must already have returned.
     * Prevent new BSP operations during hardware teardown.
     */
    portState->stopping = true;

    release_port(portState);

    result = map_driver_error(uart_driver_delete(uart_id(&portState->config)));

    if(result != RET_ERR_STS_OK)
    {
        /* Retain state for a possible subsequent cleanup attempt. */
        return result;
    }

    vSemaphoreDelete(portState->mutex);

    portENTER_CRITICAL(&s_claim_lock);
    (void)memset(portState, 0, sizeof(uart_port_state_t));
    portEXIT_CRITICAL(&s_claim_lock);

    return RET_ERR_STS_OK;
}
/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartSendAsync(bspUartHandle_t* handle, const uint8_t* buffer, size_t length)
{
    uart_port_state_t* portState;
    ret_err_sts_t result;
    int written;

    /*
     * Validate the transmit buffer and length before accessing
     * the UART driver.
     *
     * The length must fit the signed integer range used by
     * the native UART write result.
     */
    if((buffer == NULL) || (length == 0U) || (length > (size_t)INT_MAX))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Validate ownership and acquire the port mutex.
     * The mutex protects the shared UART runtime information.
     */
    result = acquire_port(handle, &portState);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Reject transmission when the configured UART mode disables TX. */
    if(!tx_enabled(portState->config.mode))
    {
        release_port(portState);
        return RET_ERR_STS_NOT_SUPPORTED;
    }

    /*
     * Submit the requested bytes to the ESP-IDF UART driver.
     *
     * Successful submission does not indicate that all bytes
     * have completed physical transmission.
     */
    written = uart_write_bytes(uart_id(&portState->config), buffer, length);

    /* Update the number of bytes accepted by the native driver. */
    if(written > 0)
    {
        portState->tx_count += (size_t)written;
    }

    /*
     * Record failures when the driver rejects the request
     * or accepts fewer bytes than requested.
     */
    if((written < 0) || ((size_t)written != length))
    {
        portState->tx_errors++;
    }

    /* Release the port mutex before returning the operation status. */
    release_port(portState);

    return (written == (int)length) ? RET_ERR_STS_OK : RET_ERR_STS_TX_FAILED;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartWaitTxDone(bspUartHandle_t* handle, uint32_t timeout_ms)
{
    uart_port_state_t* port;

    /*
     * Validate the UART handle and acquire exclusive access
     * before querying the native transmitter state.
     */
    ret_err_sts_t result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Transmission completion is unavailable when TX is disabled. */
    if(!tx_enabled(port->config.mode))
    {
        release_port(port);
        return RET_ERR_STS_NOT_SUPPORTED;
    }

    /*
     * Convert the application timeout to RTOS ticks and
     * wait for the UART transmitter to finish sending data.
     */
    result = map_driver_error(
    uart_wait_tx_done(uart_id(&port->config), ticks_from_ms(timeout_ms)));

    /* Maintain the transmit error counter when completion fails. */
    if(result != RET_ERR_STS_OK)
    {
        port->tx_errors++;
    }

    /* Release the mutex on both success and failure paths. */
    release_port(port);

    return result;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t
bspUartSendSync(bspUartHandle_t* handle, const uint8_t* data, size_t length, uint32_t timeout_ms)
{
    /*
     * Use the common transmit implementation to submit the
     * requested bytes to the native UART driver.
     */
    ret_err_sts_t result = bspUartSendAsync(handle, data, length);

    /*
     * Wait for physical transmission completion only when
     * the complete buffer was accepted successfully.
     */
    if(result == RET_ERR_STS_OK)
    {
        result = bspUartWaitTxDone(handle, timeout_ms);
    }

    /* Propagate the submission or completion operation status. */
    return result;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartReceiveSync(bspUartHandle_t* handle,
                                 uint8_t* buffer,
                                 size_t buffer_len,
                                 size_t* rx_len,
                                 uint32_t timeout_ms)
{
    uart_port_state_t* port;
    ret_err_sts_t result;
    int got;

    /*
     * Initialize the output length before any operation.
     * A valid output pointer receives zero if validation
     * or the underlying UART receive operation fails.
     */
    if(rx_len != NULL)
    {
        *rx_len = 0U;
    }

    /*
     * Validate the destination buffer, output length pointer,
     * and requested transfer size.
     */
    if((buffer == NULL) || (rx_len == NULL) || (buffer_len == 0U) ||
       (buffer_len > (size_t)UINT32_MAX))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Validate the UART instance and acquire its runtime mutex. */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Reject reception when the configured UART mode disables RX. */
    if(!rx_enabled(port->config.mode))
    {
        release_port(port);
        return RET_ERR_STS_NOT_SUPPORTED;
    }

    /*
     * Prevent synchronous reception from overlapping an
     * active asynchronous receive operation.
     */
    if(port->rx_busy)
    {
        release_port(port);
        return RET_ERR_STS_BUSY;
    }

    /*
     * Read available bytes into the caller-owned buffer.
     * The native operation may return fewer bytes than requested.
     */
    got = uart_read_bytes(uart_id(&port->config), buffer, (uint32_t)buffer_len,
                          ticks_from_ms(timeout_ms));

    if(got < 0)
    {
        /* Record the native receive failure. */
        port->rx_errors++;
        result = RET_ERR_STS_RX_FAILED;
    }
    else
    {
        /*
         * Publish the actual number of bytes received.
         * Zero bytes or a partial read are retained as valid results.
         */
        *rx_len        = (size_t)got;
        port->rx_count = (size_t)got;
        result         = RET_ERR_STS_OK;
    }

    /* Release the UART mutex before returning to the caller. */
    release_port(port);

    return result;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t
bspUartSetRxCallback(bspUartHandle_t* handle, bspUartRxCallback_t callback, void* userContext)
{
    uart_port_state_t* port;

    /* Validate UART ownership and synchronize callback registration. */
    ret_err_sts_t result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

#if (UART_ENABLE_ASYNC == 1U)

    /*
     * Callback registration must not change while a receive
     * operation is using the current callback information.
     */
    if(port->rx_busy)
    {
        result = RET_ERR_STS_BUSY;
    }
    else
    {
        /*
         * Store the callback and its application-defined context.
         * Both values are updated while the port mutex is held.
         */
        port->rx_callback         = callback;
        port->rx_callback_context = userContext;
    }

#else

    /*
     * Callback registration is unavailable when asynchronous
     * UART support is disabled at compile time.
     */
    (void)callback;
    (void)userContext;
    result = RET_ERR_STS_NOT_SUPPORTED;

#endif

    /* Release the mutex after completing the registration request. */
    release_port(port);

    return result;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t
bspUartSetTxCallback(bspUartHandle_t* handle, bspUartTxCallback_t callback, void* userContext)
{
    uart_port_state_t* port;

    /*
     * Validate ownership and ensure the UART instance is initialized
     * before reporting the supported interface capabilities.
     */
    ret_err_sts_t result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * TX completion callback registration is not implemented.
     * Explicitly mark the unused parameters to avoid warnings.
     */
    (void)callback;
    (void)userContext;

    /* Release the port mutex before returning the unsupported status. */
    release_port(port);

    return RET_ERR_STS_NOT_SUPPORTED;
}

/* -------------------------------------------------------------------------- */
/**
 * @brief Process one available UART RX chunk.
 *
 * @details
 * Receives native UART events and transfers available bytes
 * to the registered callback in the caller's thread.
 *
 * A partially filled buffer is delivered immediately rather
 * than waiting for an exact number of bytes.
 */
ret_err_sts_t bspUartProcessRx(bspUartHandle_t* handle, uint32_t timeout_ms)
{
#if (UART_ENABLE_ASYNC == 1U)
    uart_port_state_t* port;
    ret_err_sts_t result;
    uart_event_t event = { 0 };

    QueueHandle_t eventQueue;
    uart_port_t uartId;

    bspUartRxCallback_t callback = NULL;
    void* callbackContext        = NULL;

    uint8_t rxChunk[UART_RX_CHUNK_BYTES];

    size_t available = 0U;
    size_t bytesToRead;
    int received = 0;
    BaseType_t hasEvent;

    /*
     * Acquire the port to validate ownership and copy the
     * event queue and native port identifier.
     */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    if(!rx_enabled(port->config.mode))
    {
        release_port(port);
        return RET_ERR_STS_NOT_SUPPORTED;
    }

    if(port->event_queue == NULL)
    {
        release_port(port);
        return RET_ERR_STS_NOT_READY;
    }

    if(port->rx_busy)
    {
        release_port(port);
        return RET_ERR_STS_BUSY;
    }

    eventQueue = port->event_queue;
    uartId     = uart_id(&port->config);

    release_port(port);

    /*
     * Wait without holding the BSP mutex.
     *
     * The application must stop this processing thread before
     * calling bspUartDeInit(), which deletes the event queue.
     */
    hasEvent = xQueueReceive(eventQueue, &event, ticks_from_ms(timeout_ms));

    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * Handle hardware RX errors.
     * Corrupted bytes must not reach the application parser.
     */
    if(hasEvent == pdTRUE)
    {
        if((event.type == UART_FIFO_OVF) || (event.type == UART_BUFFER_FULL) ||
           (event.type == UART_PARITY_ERR) || (event.type == UART_FRAME_ERR) ||
           (event.type == UART_BREAK))
        {
            port->rx_errors++;

            (void)uart_flush_input(uartId);
            (void)xQueueReset(eventQueue);

            if((event.type == UART_FIFO_OVF) || (event.type == UART_BUFFER_FULL))
            {
                result = RET_ERR_STS_BUFFER_OVERFLOW;
            }
            else
            {
                result = RET_ERR_STS_FRAME_ERROR;
            }

            release_port(port);
            return result;
        }
    }

    /*
     * Check the driver's RX ring buffer.
     *
     * This check also runs when no event was received, allowing
     * buffered data to be drained without requiring another event.
     */
    if(uart_get_buffered_data_len(uartId, &available) != ESP_OK)
    {
        port->rx_errors++;

        release_port(port);
        return RET_ERR_STS_RX_FAILED;
    }

    if(available == 0U)
    {
        release_port(port);
        return RET_ERR_STS_OK;
    }

    bytesToRead = available;

    if(bytesToRead > sizeof(rxChunk))
    {
        bytesToRead = sizeof(rxChunk);
    }

    /*
     * The BSP processing call delivers one bounded chunk.
     * The next application RX loop iteration processes any
     * remaining buffered data.
     */
    received = uart_read_bytes(uartId, rxChunk, (uint32_t)bytesToRead, (TickType_t)0);

    if(received < 0)
    {
        port->rx_errors++;

        release_port(port);
        return RET_ERR_STS_RX_FAILED;
    }

    if(received > 0)
    {
        port->rx_count = (size_t)received;

        callback        = port->rx_callback;
        callbackContext = port->rx_callback_context;
    }

    release_port(port);

    /*
     * Execute the callback outside the BSP mutex.
     *
     * rxChunk remains valid until the callback returns.
     * The application must copy the data if it needs to
     * retain it.
     */
    if((received > 0) && (callback != NULL))
    {
        callback(RET_ERR_STS_OK, rxChunk, (size_t)received, callbackContext);
    }

    return RET_ERR_STS_OK;

#else
    (void)handle;
    (void)timeout_ms;

    return RET_ERR_STS_NOT_SUPPORTED;
#endif
}

/* -------------------------------------------------------------------------- */
ret_err_sts_t bspUartReadAsync(bspUartHandle_t* handle, uint8_t* buffer, size_t length)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    if((buffer == NULL) || (length == 0U))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    release_port(port);

    /*
     * Exact-length asynchronous receive is not supported
     * by the application-driven RX processing mode.
     */
    return RET_ERR_STS_NOT_SUPPORTED;
}

/* -------------------------------------------------------------------------- */
ret_err_sts_t bspUartCancelRx(bspUartHandle_t* handle)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * The application-driven processing mode does not arm
     * persistent asynchronous receive operations.
     */
    release_port(port);

    return RET_ERR_STS_OK;
}
/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartFlushRx(bspUartHandle_t* handle)
{
    uart_port_state_t* port;

    /* Validate and lock the UART before modifying buffered RX data. */
    ret_err_sts_t result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * Do not discard incoming data while an asynchronous
     * receive operation owns a destination buffer.
     */
    if(port->rx_busy)
    {
        release_port(port);
        return RET_ERR_STS_BUSY;
    }

    /* Request the native UART driver to discard buffered input. */
    result = map_driver_error(uart_flush_input(uart_id(&port->config)));

#if (UART_ENABLE_ASYNC == 1U)

    /*
     * Clear pending UART event notifications after a successful flush
     * to avoid processing events associated with discarded data.
     */
    if(result == RET_ERR_STS_OK)
    {
        (void)xQueueReset(port->event_queue);
    }

#endif

    /* Release the UART mutex before returning the flush status. */
    release_port(port);

    return result;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartFlushTx(bspUartHandle_t* handle)
{
    uart_port_state_t* port;

    /*
     * Validate the UART instance and acquire its mutex
     * before processing the requested control operation.
     */
    ret_err_sts_t result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * Discarding pending TX data is not implemented by this BSP.
     * No native UART transmit state is modified.
     */
    release_port(port);

    return RET_ERR_STS_NOT_SUPPORTED;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartGetStatus(bspUartHandle_t* handle, bspUartStatus_t* status)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    /* Validate the caller-provided destination structure. */
    if(status == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Acquire the UART mutex to obtain a consistent snapshot
     * of the BSP-managed runtime status and statistics.
     */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * Populate the initialization and receive activity state.
     *
     * A successful port acquisition indicates that the instance
     * is available for normal BSP operations.
     */
    status->initialized = true;
    status->rxBusy      = port->rx_busy;

    /*
     * Query native TX completion without waiting.
     * An incomplete TX operation is reported as busy.
     */
    status->txBusy = tx_enabled(port->config.mode) &&
                     (uart_wait_tx_done(uart_id(&port->config), 0) != ESP_OK);

    /* Copy the maintained transfer and error statistics. */
    status->rxCount      = port->rx_count;
    status->txCount      = port->tx_count;
    status->rxErrorCount = port->rx_errors;
    status->txErrorCount = port->tx_errors;

    /* Release the mutex after completing the status snapshot. */
    release_port(port);

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartIsRxBusy(bspUartHandle_t* handle, bool* busy)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    /* Validate the output pointer before attempting to write the result. */
    if(busy == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Acquire the port mutex before accessing shared RX state. */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * Report whether an asynchronous receive operation
     * is currently registered as active.
     */
    *busy = port->rx_busy;

    /* Release the mutex after retrieving the state. */
    release_port(port);

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartIsTxBusy(bspUartHandle_t* handle, bool* busy)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    /* Validate the destination pointer for the transmit state. */
    if(busy == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Synchronize access to the UART before querying TX completion. */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * Poll the native UART transmitter without blocking.
     * Report busy only when TX is enabled and completion
     * has not been reported by the driver.
     */
    *busy = tx_enabled(port->config.mode) &&
            (uart_wait_tx_done(uart_id(&port->config), 0) != ESP_OK);

    /* Release the port mutex after reading the TX state. */
    release_port(port);

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartGetRxCount(bspUartHandle_t* handle, size_t* count)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    /* Ensure that the caller supplied a valid output pointer. */
    if(count == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Lock the UART instance before accessing its receive statistics. */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Return the receive byte count maintained by the BSP. */
    *count = port->rx_count;

    /* Release the mutex after copying the counter value. */
    release_port(port);

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartGetTxCount(bspUartHandle_t* handle, size_t* count)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    /* Validate the output pointer for the transmit byte count. */
    if(count == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Synchronize access to the maintained transmit statistics. */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Return the number of bytes accepted by the transmit path. */
    *count = port->tx_count;

    /* Release the port mutex before returning the result. */
    release_port(port);

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartSetConfig(bspUartHandle_t* handle, const bspUartConfig_t* config)
{
    uart_port_state_t* port;
    bspUartConfig_t previous;

    /*
     * Validate the requested configuration before accessing
     * or modifying the currently active UART instance.
     */
    ret_err_sts_t result = validate_config(config);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Acquire exclusive access to the UART configuration and state. */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * Prevent changes to the physical UART identity and owner.
     * Runtime reconfiguration is restricted to the existing instance.
     */
    if((config->port != port->config.port) || (config->owner != port->config.owner))
    {
        result = RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Do not reconfigure UART parameters while an RX operation
     * is active or transmitted data remains pending.
     */
    else if(port->rx_busy || (uart_wait_tx_done(uart_id(&port->config), 0) != ESP_OK))
    {
        result = RET_ERR_STS_BUSY;
    }
    else
    {
        /*
         * Preserve the previous configuration so the driver
         * can attempt recovery if the new settings are rejected.
         */
        previous = port->config;

        if(apply_config(config) == ESP_OK)
        {
            /*
             * Publish the new configuration only after the
             * native UART configuration operation succeeds.
             */
            port->config   = *config;
            handle->config = *config;
            result         = RET_ERR_STS_OK;
        }
        else
        {
            /*
             * Attempt to restore the previous hardware configuration.
             * Recovery is best-effort; failure requires higher-level
             * handling or reinitialization.
             */
            (void)apply_config(&previous);
            result = RET_ERR_STS_CONFIG_FAILED;
        }
    }

    /* Release the UART mutex regardless of reconfiguration result. */
    release_port(port);

    return result;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartGetConfig(bspUartHandle_t* handle, bspUartConfig_t* config)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    /* Validate the destination structure before copying configuration. */
    if(config == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Acquire the port mutex to prevent concurrent
     * configuration changes while copying the stored values.
     */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Copy the active BSP configuration into caller-owned storage. */
    *config = port->config;

    /* Release the mutex after completing the configuration snapshot. */
    release_port(port);

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartSetBaudrate(bspUartHandle_t* handle, bspUartBaudrate_t baudrate)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    /*
     * Reject zero baud rate and values outside the signed
     * integer range supported by the native UART interface.
     */
    if((baudrate == 0U) || (baudrate > (uint32_t)INT_MAX))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Acquire the port mutex before inspecting active transfer state. */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * Changing the baud rate while receiving or transmitting
     * could invalidate the current communication operation.
     */
    if(port->rx_busy || (uart_wait_tx_done(uart_id(&port->config), 0) != ESP_OK))
    {
        result = RET_ERR_STS_BUSY;
    }
    else
    {
        /* Request the native driver to apply the new baud rate. */
        result = map_driver_error(uart_set_baudrate(uart_id(&port->config), baudrate));

        /*
         * Update both cached configuration copies only when
         * the hardware driver accepts the requested baud rate.
         */
        if(result == RET_ERR_STS_OK)
        {
            port->config.baudrate   = baudrate;
            handle->config.baudrate = baudrate;
        }
    }

    /* Release the mutex after the baud rate operation completes. */
    release_port(port);

    return result;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartGetBaudrate(bspUartHandle_t* handle, bspUartBaudrate_t* baudrate)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    /* Validate the caller-provided destination pointer. */
    if(baudrate == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Acquire the UART mutex so the baud rate cannot be
     * changed concurrently while reading the cached value.
     */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Return the configured baud rate maintained by the BSP. */
    *baudrate = port->config.baudrate;

    /* Release the mutex after retrieving the configuration value. */
    release_port(port);

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartIoctl(bspUartHandle_t* handle, bspUartIoctlRequest_t req, void* arg)
{
    uart_port_state_t* port;

    /*
     * Validate UART ownership and ensure the instance is operational
     * before evaluating extension control requests.
     */
    ret_err_sts_t result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * No IOCTL extension requests are currently implemented.
     * Explicitly acknowledge the unused request parameters.
     */
    (void)req;
    (void)arg;

    /* Release the port mutex before reporting unsupported functionality. */
    release_port(port);

    return RET_ERR_STS_NOT_SUPPORTED;
}

/* -------------------------------------------------------------------------- */
