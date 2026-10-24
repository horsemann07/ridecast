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
/* UART Compile-Time Configuration                                            */
/* -------------------------------------------------------------------------- */

/* Enable or disable asynchronous UART operations (0U: disabled, 1U: enabled). */
#ifndef bspCONFIG_UART_ENABLE_ASYNC
    #define bspCONFIG_UART_ENABLE_ASYNC (1U)
#endif

/* Configure the native UART TX and RX buffer size in bytes. */
#ifndef bspCONFIG_UART_RXTX_BUFFER_SIZE
    #define bspCONFIG_UART_RXTX_BUFFER_SIZE (1024U)
#endif

/* Configure the asynchronous UART event queue depth. */
#ifndef bspCONFIG_UART_ASYNC_EVENT_QUEUE_LEN
    #define bspCONFIG_UART_ASYNC_EVENT_QUEUE_LEN (16U)
#endif

/* Configure the asynchronous UART worker stack size in bytes. */
#ifndef bspCONFIG_UART_TASK_STACK_BYTES
    #define bspCONFIG_UART_TASK_STACK_BYTES (4096U)
#endif

/* Configure the asynchronous UART worker task priority. */
#ifndef bspCONFIG_UART_TASK_PRIORITY
    #define bspCONFIG_UART_TASK_PRIORITY (8U)
#endif

/* Configure the UART event polling interval in milliseconds. */
#ifndef bspCONFIG_UART_POLL_MS
    #define bspCONFIG_UART_POLL_MS (10U)
#endif

/* Configure the UART worker shutdown timeout in milliseconds. */
#ifndef bspCONFIG_UART_SHUTDOWN_MS
    #define bspCONFIG_UART_SHUTDOWN_MS (1000U)
#endif

/* -------------------------------------------------------------------------- */
/* UART Compile-Time Configuration Validation                                 */
/* -------------------------------------------------------------------------- */

/* Restrict asynchronous UART configuration to supported Boolean values. */
#if ((bspCONFIG_UART_ENABLE_ASYNC != 0U) && (bspCONFIG_UART_ENABLE_ASYNC != 1U))
    #error "bspCONFIG_UART_ENABLE_ASYNC must be 0 or 1"
#endif

/* Ensure the UART driver buffer exceeds the hardware FIFO capacity. */
#if (bspCONFIG_UART_RXTX_BUFFER_SIZE < 256U)
    #error "UART driver buffer must exceed the target hardware FIFO"
#endif

/* Validate event queue depth and polling interval when async UART is enabled. */
#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)
    #if (bspCONFIG_UART_ASYNC_EVENT_QUEUE_LEN < 1U) || (bspCONFIG_UART_POLL_MS < 1U)
        #error "UART event depth and poll interval must be nonzero"
    #endif
#endif

/* -------------------------------------------------------------------------- */
/* Private UART Type Definitions                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Stores completion information for an asynchronous UART receive.
 *
 * @details Captures callback information and received data before the
 * active RX state is cleared. The callback is invoked outside the port mutex.
 *
 * @note The receive buffer remains owned by the application.
 */
#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)
typedef struct
{
    bspUartRxCallback_t callback; /**< Registered RX completion callback. */
    void* callback_context;       /**< Application-defined callback context. */
    uint8_t* buffer;              /**< Application-owned receive buffer. */
    size_t length;   /**< Number of bytes received at completion. */
    size_t received; /**< Additional receive progress storage. */
} uart_rx_completion_t;
#endif

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
#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)
    uint8_t* rx_buffer;        /**< Active application-owned RX buffer. */
    size_t rx_capacity;        /**< Active RX buffer capacity in bytes. */
    size_t rx_received; /**< Bytes accumulated in the active RX operation. */
    QueueHandle_t event_queue; /**< Native ESP-IDF UART event queue. */
    SemaphoreHandle_t worker_stopped; /**< Worker shutdown acknowledgement semaphore. */
    TaskHandle_t worker; /**< Asynchronous UART worker task handle. */
#endif
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
/* Private Asynchronous UART Function Declarations                            */
/* -------------------------------------------------------------------------- */

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

/**
 * @brief Capture and reset an active asynchronous receive operation.
 *
 * @param[in,out] port Pointer to the UART runtime state.
 * @param[out] done Receives the callback information, receive buffer,
 * and number of bytes accumulated.
 *
 * @return None.
 *
 * @pre The calling task must hold the UART port mutex.
 *
 * @note This function does not invoke the application callback.
 * Callback execution is performed after releasing the mutex.
 */
static void complete_rx_locked(uart_port_state_t* port, uart_rx_completion_t* done);

/* -------------------------------------------------------------------------- */

/**
 * @brief Process asynchronous UART events for one hardware port.
 *
 * @param[in] param Pointer to the UART runtime state.
 *
 * @return None.
 *
 * @details
 * Waits for UART events, processes receive data, handles receive
 * errors, and invokes application callbacks after releasing the
 * port mutex.
 *
 * During shutdown, the worker acknowledges the stop request and
 * suspends until the deinitializing task deletes it.
 *
 * @note The function is intended to execute as a FreeRTOS task.
 */
static void uart_async_thread(void* param);

#endif /* (bspCONFIG_UART_ENABLE_ASYNC == 1U) */

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

/* -------------------------------------------------------------------------- */
/* Asynchronous UART Private Functions                                        */
/* -------------------------------------------------------------------------- */

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

/* -------------------------------------------------------------------------- */
/* Asynchronous RX Completion State                                           */
/* -------------------------------------------------------------------------- */

static void complete_rx_locked(uart_port_state_t* port, uart_rx_completion_t* done)
{
    /*
     * Capture the registered callback, application context, and
     * receive buffer before clearing the active operation.
     */
    done->callback         = port->rx_callback;
    done->callback_context = port->rx_callback_context;
    done->buffer           = port->rx_buffer;
    done->length           = port->rx_received;

    /* Preserve the final received byte count for status queries. */
    port->rx_count = port->rx_received;

    /*
     * Clear the active RX state while the port mutex is held.
     * Callback execution is deferred until the caller releases the lock.
     */
    port->rx_buffer   = NULL;
    port->rx_capacity = 0U;
    port->rx_received = 0U;
    port->rx_busy     = false;
}

/* -------------------------------------------------------------------------- */
/* Asynchronous UART Event Worker                                             */
/* -------------------------------------------------------------------------- */

static void uart_async_thread(void* param)
{
    uart_port_state_t* port = (uart_port_state_t*)param;
    const uart_port_t id    = (uart_port_t)(port - &s_ports[0]);
    uart_event_t event;

    /* Continue processing UART events until shutdown is requested. */
    for(;;)
    {
        bool received_event;
        bool completed                 = false;
        ret_err_sts_t completed_status = RET_ERR_STS_OK;
        uart_rx_completion_t done      = { 0 };
        size_t available               = 0U;

        /*
         * Wait for a UART event with a bounded timeout.
         * Periodic wakeups also permit polling for buffered RX data
         * and checking the worker shutdown request.
         */
        received_event = (xQueueReceive(port->event_queue, &event,
                                        ticks_from_ms(bspCONFIG_UART_POLL_MS)) == pdTRUE);

        /* Synchronize access to the UART receive and shutdown state. */
        (void)xSemaphoreTake(port->mutex, portMAX_DELAY);

        if(port->stopping)
        {
            /*
             * Acknowledge shutdown only after leaving the protected
             * state and stop processing additional UART events.
             */
            (void)xSemaphoreGive(port->mutex);
            (void)xSemaphoreGive(port->worker_stopped);

            /* Remain parked until the deinitializing task deletes us. */
            vTaskSuspend(NULL);

            /*
             * Prevent the worker from returning if execution is
             * unexpectedly resumed after the shutdown request.
             */
            for(;;)
            {
                vTaskDelay(portMAX_DELAY);
            }
        }

        /*
         * Handle UART receive errors before consuming more data.
         * Framing, parity, break, and buffer overflow events invalidate
         * the pending receive data.
         */
        if(received_event &&
           ((event.type == UART_FIFO_OVF) || (event.type == UART_BUFFER_FULL) ||
            (event.type == UART_PARITY_ERR) || (event.type == UART_FRAME_ERR) ||
            (event.type == UART_BREAK)))
        {
            /* Record the receive error and discard affected buffered data. */
            port->rx_errors++;

            /*
             * Do not deliver potentially corrupt bytes on a subsequent
             * RX request after framing, parity, break, or overflow errors.
             */
            (void)uart_flush_input(id);
            (void)xQueueReset(port->event_queue);

            /* Distinguish receive overflow from other UART line errors. */
            if((event.type == UART_FIFO_OVF) || (event.type == UART_BUFFER_FULL))
            {
                completed_status = RET_ERR_STS_BUFFER_OVERFLOW;
            }
            else
            {
                completed_status = RET_ERR_STS_FRAME_ERROR;
            }

            /*
             * Capture and clear any active receive operation.
             * The application callback is invoked after unlocking.
             */
            if(port->rx_busy)
            {
                complete_rx_locked(port, &done);
                completed = true;
            }
        }

        /*
         * Process buffered receive data only when an asynchronous
         * operation is active and the driver reports available bytes.
         */
        else if(port->rx_busy && (uart_get_buffered_data_len(id, &available) == ESP_OK) &&
                (available != 0U))
        {
            /* Limit the read to the remaining application buffer capacity. */
            size_t wanted = port->rx_capacity - port->rx_received;

            if(available < wanted)
            {
                wanted = available;
            }

            /* Limit the transfer length to the native driver's size range. */
            if(wanted > (size_t)UINT32_MAX)
            {
                wanted = (size_t)UINT32_MAX;
            }

            /* Read buffered UART data without blocking the worker. */
            int read_count = uart_read_bytes(id, &port->rx_buffer[port->rx_received],
                                             (uint32_t)wanted, (TickType_t)0);

            if(read_count < 0)
            {
                /*
                 * Record the native receive failure and complete
                 * the pending operation with an error status.
                 */
                port->rx_errors++;
                completed_status = RET_ERR_STS_RX_FAILED;
                complete_rx_locked(port, &done);
                completed = true;
            }
            else if(read_count > 0)
            {
                /* Update the accumulated receive length and status counter. */
                port->rx_received += (size_t)read_count;
                port->rx_count = port->rx_received;

                /*
                 * Complete the receive only when the requested
                 * number of bytes has been accumulated.
                 */
                if(port->rx_received == port->rx_capacity)
                {
                    complete_rx_locked(port, &done);
                    completed = true;
                }
            }
        }

        /* Release the port mutex before executing application code. */
        (void)xSemaphoreGive(port->mutex);

        /*
         * Invoke the captured callback only for a completed operation.
         * Executing outside the critical region avoids holding the
         * port mutex during application callback processing.
         */
        if(completed && (done.callback != NULL))
        {
            done.callback(completed_status, done.buffer, done.length, done.callback_context);
        }
    }
}

/* -------------------------------------------------------------------------- */

#endif /* (bspCONFIG_UART_ENABLE_ASYNC == 1U) */


/* ============================================================================
 * UART PUBLIC API IMPLEMENTATION
 * ========================================================================== */

ret_err_sts_t bspUartInit(bspUartHandle_t* handle)
{
    ret_err_sts_t result;
    esp_err_t err;
    uart_port_state_t* port;

    /* Validate the caller-provided handle before accessing its members. */
    if(handle == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Validate the complete UART configuration before reserving the
     * hardware resource or allocating any RTOS objects.
     */
    result = validate_config(&handle->config);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Select the internal runtime state associated with the UART port. */
    port = &s_ports[(unsigned)handle->config.port];

    /*
     * Protect the port reservation against concurrent initialization
     * attempts from different application tasks.
     *
     * The critical section is limited to the reservation operation.
     * No memory allocation or blocking operation occurs while held.
     */
    portENTER_CRITICAL(&s_claim_lock);

    if(port->reserved)
    {
        portEXIT_CRITICAL(&s_claim_lock);
        return RET_ERR_STS_ALREADY_INIT;
    }

    port->reserved = true;

    portEXIT_CRITICAL(&s_claim_lock);

    /*
     * Store the application handle and validated configuration.
     * Subsequent UART operations use these values to identify
     * the owner and access the hardware instance.
     */
    port->owner  = handle;
    port->config = handle->config;

    /*
     * Create a per-port mutex to serialize access to runtime state
     * and UART driver operations performed by application tasks.
     */
    port->mutex = xSemaphoreCreateMutex();

    if(port->mutex == NULL)
    {
        result = RET_ERR_STS_NO_MEM;
        goto fail;
    }

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

    /*
     * Create a synchronization object used by the asynchronous
     * worker to acknowledge completion of its shutdown sequence.
     */
    port->worker_stopped = xSemaphoreCreateBinary();

    if(port->worker_stopped == NULL)
    {
        result = RET_ERR_STS_NO_MEM;
        goto fail;
    }

#endif

    /*
     * Install the native UART driver and configure its TX/RX buffers.
     *
     * When asynchronous support is enabled, request an ESP-IDF
     * event queue for the UART event processing worker.
     */
    err = uart_driver_install(uart_id(&port->config), (int)bspCONFIG_UART_RXTX_BUFFER_SIZE,
                              (int)bspCONFIG_UART_RXTX_BUFFER_SIZE,
#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)
                              (int)bspCONFIG_UART_ASYNC_EVENT_QUEUE_LEN, &port->event_queue,
#else
                              0, NULL,
#endif
                              0);

    if(err != ESP_OK)
    {
        /* Map the native driver error and release allocated resources. */
        result = map_driver_error(err);
        goto fail;
    }

    /*
     * Apply the validated communication settings and GPIO routing.
     *
     * If configuration fails after driver installation,
     * the installed UART driver must also be released.
     */
    err = apply_config(&port->config);

    if(err != ESP_OK)
    {
        result = map_driver_error(err);
        goto fail_driver;
    }

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

    /*
     * Create the UART event processing task only after the
     * hardware driver and port configuration are available.
     *
     * The port runtime state is passed as the task argument.
     */
    if(xTaskCreate(uart_async_thread, "bsp_uart", bspCONFIG_UART_TASK_STACK_BYTES,
                   port, bspCONFIG_UART_TASK_PRIORITY, &port->worker) != pdPASS)
    {
        result = RET_ERR_STS_NO_MEM;
        goto fail_driver;
    }

#endif

    /*
     * Mark initialization successful only after all mandatory
     * resources have been created and configured.
     */
    port->initialized = true;

    return RET_ERR_STS_OK;

    /*
     * Driver cleanup path.
     *
     * This label is reached only after successful driver
     * installation followed by a later initialization failure.
     */
fail_driver:

    (void)uart_driver_delete(uart_id(&port->config));

    /*
     * Common initialization failure cleanup.
     *
     * Release resources created before the failure and restore
     * the runtime state for a subsequent initialization attempt.
     */
fail:

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

    if(port->worker_stopped != NULL)
    {
        vSemaphoreDelete(port->worker_stopped);
    }

#endif

    if(port->mutex != NULL)
    {
        vSemaphoreDelete(port->mutex);
    }

    /*
     * Initialization is serialized by the reservation flag.
     * Clear the state to release ownership of the UART instance.
     */
    memset(port, 0, sizeof(*port));

    return result;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartDeInit(bspUartHandle_t* handle)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    /* Validate the handle and port number before accessing runtime state. */
    if((handle == NULL) || (!valid_port(handle->config.port)))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Resolve the runtime state associated with the requested port. */
    port = &s_ports[(unsigned)handle->config.port];

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

    /*
     * Prevent the UART worker from invoking its own shutdown routine.
     * Waiting for self-termination would prevent normal completion.
     */
    if(xTaskGetCurrentTaskHandle() == port->worker)
    {
        return RET_ERR_STS_INVALID_STATE;
    }

#endif

    /*
     * The mutex must exist before deinitialization can synchronize
     * with other UART operations.
     *
     * This check also supports another deinitialization attempt
     * after a previous shutdown timeout.
     */
    if(port->mutex == NULL)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /* Acquire exclusive access before modifying the shutdown state. */
    if(xSemaphoreTake(port->mutex, portMAX_DELAY) != pdTRUE)
    {
        return RET_ERR_STS_FAIL;
    }

    /*
     * Ensure that the requesting handle owns the UART instance
     * and that the port has completed initialization.
     */
    if((port->owner != handle) || (!port->initialized))
    {
        release_port(port);
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /*
     * Prevent new UART operations while shutdown is in progress.
     * Release the mutex before waiting for the worker to stop.
     */
    port->stopping = true;

    release_port(port);

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

    /*
     * Wait for the asynchronous worker to acknowledge shutdown.
     *
     * On timeout, retain the stopping state so normal UART
     * operations remain rejected until shutdown is retried.
     */
    if(xSemaphoreTake(port->worker_stopped, ticks_from_ms(bspCONFIG_UART_SHUTDOWN_MS)) != pdTRUE)
    {
        /* Keep stopping=true. A subsequent DeInit retries safely; normal
         * UART operations remain rejected while shutdown is pending. */
        return RET_ERR_STS_TIMEOUT;
    }

    /*
     * The worker has acknowledged that it no longer accesses
     * the port runtime state. Reclaim its RTOS task resources.
     */
    vTaskDelete(port->worker);

#endif

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

    /*
     * Preserve completion information for an outstanding RX operation.
     *
     * The application callback must be invoked after releasing
     * the port mutex to avoid callback-induced deadlocks.
     */
    uart_rx_completion_t cancelled = { 0 };

    /* Synchronize access to the pending receive state. */
    (void)xSemaphoreTake(port->mutex, portMAX_DELAY);

    bool pending_rx = port->rx_busy;

    if(pending_rx)
    {
        complete_rx_locked(port, &cancelled);
    }

    release_port(port);

    /*
     * Notify the application about the cancelled receive.
     *
     * The callback is not permitted to recursively deinitialize
     * the same UART instance while cleanup is in progress.
     */
    if(pending_rx && (cancelled.callback != NULL))
    {
        cancelled.callback(RET_ERR_STS_CANCELLED, cancelled.buffer,
                           cancelled.length, cancelled.callback_context);
    }

#endif

    /*
     * Uninstall the native UART driver after the asynchronous
     * worker has stopped accessing its event queue.
     */
    result = map_driver_error(uart_driver_delete(uart_id(&port->config)));

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

    /* Release the worker shutdown synchronization object. */
    vSemaphoreDelete(port->worker_stopped);

#endif

    /* Release the per-port synchronization resource. */
    vSemaphoreDelete(port->mutex);

    /*
     * Clear ownership, configuration, and runtime state.
     * The UART port becomes available for future initialization.
     */
    memset(port, 0, sizeof(*port));

    return result;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartSendAsync(bspUartHandle_t* handle, const uint8_t* buffer, size_t length)
{
    uart_port_state_t* port;
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
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Reject transmission when the configured UART mode disables TX. */
    if(!tx_enabled(port->config.mode))
    {
        release_port(port);
        return RET_ERR_STS_NOT_SUPPORTED;
    }

    /*
     * Submit the requested bytes to the ESP-IDF UART driver.
     *
     * Successful submission does not indicate that all bytes
     * have completed physical transmission.
     */
    written = uart_write_bytes(uart_id(&port->config), buffer, length);

    /* Update the number of bytes accepted by the native driver. */
    if(written > 0)
    {
        port->tx_count += (size_t)written;
    }

    /*
     * Record failures when the driver rejects the request
     * or accepts fewer bytes than requested.
     */
    if((written < 0) || ((size_t)written != length))
    {
        port->tx_errors++;
    }

    /* Release the port mutex before returning the operation status. */
    release_port(port);

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

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

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

ret_err_sts_t bspUartReadAsync(bspUartHandle_t* handle, uint8_t* buffer, size_t length)
{
    uart_port_state_t* port;
    ret_err_sts_t result;

    /*
     * Validate the application-owned receive buffer.
     * The buffer must remain available until the receive
     * operation completes or is cancelled.
     */
    if((buffer == NULL) || (length == 0U))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Validate the UART handle and synchronize RX state modification. */
    result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

    /* Verify that the current UART configuration permits reception. */
    if(!rx_enabled(port->config.mode))
    {
        result = RET_ERR_STS_NOT_SUPPORTED;
    }

    /*
     * Require a registered receive callback before accepting
     * a new asynchronous operation.
     */
    else if(port->rx_callback == NULL)
    {
        result = RET_ERR_STS_NOT_READY;
    }

    /* Prevent more than one outstanding receive on the same UART. */
    else if(port->rx_busy)
    {
        result = RET_ERR_STS_BUSY;
    }
    else
    {
        /*
         * Store the caller-owned buffer and requested capacity.
         *
         * Reset the receive progress counters before marking
         * the asynchronous operation active.
         */
        port->rx_buffer   = buffer;
        port->rx_capacity = length;
        port->rx_received = 0U;
        port->rx_count    = 0U;
        port->rx_busy     = true;
        result            = RET_ERR_STS_OK;
    }

#else

    /* Report unsupported operation when asynchronous RX is disabled. */
    result = RET_ERR_STS_NOT_SUPPORTED;

#endif

    /* Publish the updated RX state by releasing the port mutex. */
    release_port(port);

    return result;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t bspUartCancelRx(bspUartHandle_t* handle)
{
    uart_port_state_t* port;

    /* Validate the handle and synchronize access to pending RX state. */
    ret_err_sts_t result = acquire_port(handle, &port);

    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

    /*
     * Prepare a local completion record to preserve callback details
     * after the internal receive state is reset.
     */
    uart_rx_completion_t done = { 0 };
    bool had_rx               = port->rx_busy;

    /*
     * Complete the pending receive using the data accumulated
     * before cancellation.
     */
    if(had_rx)
    {
        complete_rx_locked(port, &done);
    }

    /*
     * Release the mutex before invoking any application callback.
     * This prevents callback execution while holding the port lock.
     */
    release_port(port);

    /* Notify the application only if a pending receive existed. */
    if(had_rx && (done.callback != NULL))
    {
        done.callback(RET_ERR_STS_CANCELLED, done.buffer, done.length, done.callback_context);
    }

    return RET_ERR_STS_OK;

#else

    /* Release the mutex before reporting unsupported functionality. */
    release_port(port);
    return RET_ERR_STS_NOT_SUPPORTED;

#endif
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

#if (bspCONFIG_UART_ENABLE_ASYNC == 1U)

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
