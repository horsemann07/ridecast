/* ============================================================================
 * File        : bsp_uart.h
 * Description : Platform-independent BSP UART public interface
 * Author      : Jha Raghav
 * Date        : 2026-10-24
 * ============================================================================
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Description:
 *   Defines a portable UART interface shared by ESP32 and STM32H7 ports.
 *   The public API contains no ESP-IDF, STM32 HAL, FreeRTOS, or CMSIS types.
 *
 * Integration:
 *   Include bsp_uart.h from application and middleware code. Link exactly
 *   one target-specific implementation (ESP32 or STM32H7).
 *
 * Requirements:
 *   C11, stdint.h, stddef.h, stdbool.h, and BSP status definitions.
 *
 * Change Log (newest first):
 * Date          Author          Description
 * ------------  --------------  ----------------------------------------------
 * 2026-10-24    Jha Raghav      Aligned portable API with ESP-IDF implementation
 * 2026-10-23    Jha Raghav      UART API draft (original date to be confirmed)
 * ============================================================================ */

#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bsp_config.h"
#include "bsp_err_sts.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* -------------------------------------------------------------------------- */
/* UART Configuration Macros                                                  */
/* -------------------------------------------------------------------------- */

/** Configure the maximum number of bytes processed in one UART RX operation. */
#ifndef configUART_RX_CHUNK_BYTES
    #define configUART_RX_CHUNK_BYTES (256U)
#endif

/** Configure the UART RX timeout in symbol periods. */
#ifndef configUART_RX_TIMEOUT_SYMBOLS
    #define configUART_RX_TIMEOUT_SYMBOLS (3U)
#endif

/* -------------------------------------------------------------------------- */
/* UART Pin Configuration Constants                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Sentinel value used to preserve an existing UART pin mapping.
 *
 * @note Board initialization must configure valid TX and RX pins before
 * the UART peripheral is used.
 */
#define BSP_UART_PIN_NO_CHANGE UINT32_MAX

/* -------------------------------------------------------------------------- */
/* UART Compatibility Configuration Macros                                    */
/* -------------------------------------------------------------------------- */

/** Configure the native UART TX and RX buffer size in bytes. */
#ifndef configUART_RXTX_BUFFER_SIZE
    #define configUART_RXTX_BUFFER_SIZE (1024U)
#endif

/** Enable or disable asynchronous UART operations. */
#ifndef configUART_ENABLE_ASYNC
    #define configUART_ENABLE_ASYNC (1U)
#endif

/** Compatibility alias for the native UART TX and RX buffer size. */
#define BSP_UART_RXTX_BUFFER_SIZE (configUART_RXTX_BUFFER_SIZE)

/** Compatibility alias for asynchronous UART enablement. */
#define BSP_UART_ENABLE_ASYNC     (configUART_ENABLE_ASYNC)

/* -------------------------------------------------------------------------- */
/* UART Baud Rate Constants                                                   */
/* -------------------------------------------------------------------------- */
#define BSP_UART_BAUD_1200        (1200UL)
#define BSP_UART_BAUD_9600        (9600UL)
#define BSP_UART_BAUD_19200       (19200UL)
#define BSP_UART_BAUD_38400       (38400UL)
#define BSP_UART_BAUD_57600       (57600UL)
#define BSP_UART_BAUD_115200      (115200UL)
#define BSP_UART_BAUD_230400      (230400UL)
#define BSP_UART_BAUD_460800      (460800UL)
#define BSP_UART_BAUD_921600      (921600UL)

    /* -------------------------------------------------------------------------- */
    /* UART Public Enumeration Types */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Logical identifiers for hardware UART ports.
     *
     * @note The target-specific UART implementation validates the supported
     * hardware port count at runtime.
     */
    typedef enum
    {
        eBspUartPort0 = 0U, /**< Logical UART port 0. */
        eBspUartPort1,      /**< Logical UART port 1. */
        eBspUartPort2,      /**< Logical UART port 2. */
        eBspUartPort3,      /**< Logical UART port 3. */
        eBspUartPort4,      /**< Logical UART port 4. */
        eBspUartPort5,      /**< Logical UART port 5. */
        eBspUartPort6,      /**< Logical UART port 6. */
        eBspUartPort7       /**< Logical UART port 7. */
    } bspUartPort_t;

    /* -------------------------------------------------------------------------- */
    /* UART Public Scalar Types */
    /* -------------------------------------------------------------------------- */

    /** Application-defined identifier used to track ownership of a UART port. */
    typedef uint8_t bspUartOwner_t;

    /** UART communication speed expressed in bits per second. */
    typedef uint32_t bspUartBaudrate_t;

    /**
     * @brief Target-specific UART pin identifier.
     *
     * @note Represents a native GPIO number on ESP32 or a board pin mapping
     * identifier on STM32H7.
     *
     * @note This type does not expose a target-specific HAL pointer.
     */
    typedef uint32_t bspUartPin_t;

    /* -------------------------------------------------------------------------- */
    /* UART Communication Configuration Types */
    /* -------------------------------------------------------------------------- */

    /** UART parity configuration options. */
    typedef enum
    {
        eBspUartParityNone = 0U, /**< Disable parity checking. */
        eBspUartParityOdd,       /**< Enable odd parity. */
        eBspUartParityEven       /**< Enable even parity. */
    } bspUartParity_t;

    /** UART stop-bit configuration options. */
    typedef enum
    {
        eBspUartStopBitsOne = 0U, /**< Configure one stop bit. */
        eBspUartStopBitsTwo       /**< Configure two stop bits. */
    } bspUartStopBits_t;

    /** UART transmit and receive operating modes. */
    typedef enum
    {
        eBspUartModeTx = 0U, /**< Enable transmit operation only. */
        eBspUartModeRx,      /**< Enable receive operation only. */
        eBspUartModeTxRx     /**< Enable transmit and receive operations. */
    } bspUartMode_t;

    /** UART data word length configuration options. */
    typedef enum
    {
        eBspUartWordLength5 = 5U, /**< Configure 5-bit data words. */
        eBspUartWordLength6,      /**< Configure 6-bit data words. */
        eBspUartWordLength7,      /**< Configure 7-bit data words. */
        eBspUartWordLength8       /**< Configure 8-bit data words. */
    } bspUartWordLength_t;

    /** UART DMA transfer configuration options. */
    typedef enum
    {
        eBspUartDmaDisabled = 0U, /**< Disable DMA transfers. */
        eBspUartDmaTx,            /**< Enable transmit DMA. */
        eBspUartDmaRx,            /**< Enable receive DMA. */
        eBspUartDmaTxRx           /**< Enable transmit and receive DMA. */
    } bspUartDmaMode_t;

    /** UART hardware flow-control configuration options. */
    typedef enum
    {
        eBspUartFlowControlNone = 0U, /**< Disable hardware flow control. */
        eBspUartFlowControlRtsCts     /**< Enable RTS/CTS flow control. */
    } bspUartFlowControl_t;

    /* -------------------------------------------------------------------------- */
    /* UART IOCTL Request Types */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Reserved UART IOCTL request identifiers.
     *
     * @note Reserved for future optional operations. Dedicated BSP APIs
     * should be preferred for supported UART operations.
     */
    typedef enum
    {
        eBspUartIoctlReserved = 0U /**< Reserved request identifier. */
    } bspUartIoctlRequest_t;

    /* -------------------------------------------------------------------------- */
    /* UART Configuration Structure */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Configuration parameters for a BSP UART instance.
     *
     * @details Contains the logical UART port, owner identifier, communication
     * settings, pin mappings, and optional hardware-specific parameters.
     *
     * @note BSP_UART_PIN_NO_CHANGE preserves the existing pin mapping.
     *
     * @note fifoSize and oversampling are optional target-dependent parameters.
     * Unsupported nonzero values return RET_ERR_STS_NOT_SUPPORTED.
     *
     * @note DMA support is target-dependent and may be unavailable.
     *
     * @note A zero rxThreshold selects the target driver default.
     */
    typedef struct
    {
        bspUartPort_t port;         /**< Logical hardware UART port. */
        bspUartOwner_t owner;       /**< Application-defined UART owner. */
        bspUartBaudrate_t baudrate; /**< UART baud rate in bits per second. */
        bspUartWordLength_t wordLength; /**< UART data word length. */
        bspUartParity_t parity;         /**< UART parity configuration. */
        bspUartStopBits_t stopBits;     /**< UART stop-bit configuration. */
        bspUartMode_t mode;             /**< UART transmit and receive mode. */
        bspUartPin_t uartRxPin;         /**< UART receive pin identifier. */
        bspUartPin_t uartTxPin;         /**< UART transmit pin identifier. */
        bspUartPin_t uartRtsPin;        /**< UART request-to-send pin. */
        bspUartPin_t uartCtsPin;        /**< UART clear-to-send pin. */
        uint16_t fifoSize;              /**< Optional hardware FIFO size. */
        uint16_t rxThreshold; /**< RX threshold; zero selects default. */
        bspUartFlowControl_t flowControl; /**< UART hardware flow-control mode. */
        bspUartDmaMode_t dmaMode; /**< UART DMA transfer configuration. */
        uint8_t oversampling; /**< Optional target-dependent oversampling. */
        bool invertTx;        /**< Enable transmit signal inversion. */
        bool invertRx;        /**< Enable receive signal inversion. */
    } bspUartConfig_t;

    /* -------------------------------------------------------------------------- */
    /* UART Runtime Status Structure */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Runtime status and transfer statistics for a UART instance.
     *
     * @details Reports UART initialization state, current transfer activity,
     * accepted transmit bytes, received bytes, and transfer error counts.
     *
     * @note txCount represents bytes accepted for transmission and does not
     * guarantee physical transmission completion.
     *
     * @note rxCount represents bytes received by the UART driver.
     */
    typedef struct
    {
        bool initialized;      /**< UART initialization status. */
        bool rxBusy;           /**< Receive operation activity flag. */
        bool txBusy;           /**< Transmit operation activity flag. */
        size_t rxCount;        /**< Total received bytes. */
        size_t txCount;        /**< Total accepted transmit bytes. */
        uint32_t rxErrorCount; /**< Number of receive errors. */
        uint32_t txErrorCount; /**< Number of transmit errors. */
    } bspUartStatus_t;

    /* -------------------------------------------------------------------------- */
    /* UART Callback Function Types */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Callback invoked when an asynchronous UART receive operation ends.
     *
     * @param[in] status Receive completion or cancellation status.
     * @param[in,out] data Pointer to the caller-owned receive buffer.
     * @param[in] data_len Number of bytes received, including partial data.
     * @param[in] userContext Application-defined callback context.
     *
     * @return None.
     *
     * @note Normal completion executes in UART worker task context, not ISR
     * context.
     *
     * @note Cancellation may invoke the callback synchronously in the
     * cancelling caller's task context.
     *
     * @note The UART state mutex is unlocked before callback invocation.
     *
     * @warning The caller retains ownership of the receive buffer and must
     * keep it valid until the receive operation completes or is cancelled.
     */
    typedef void (*bspUartRxCallback_t)(ret_err_sts_t status,
                                        uint8_t* data,
                                        size_t data_len,
                                        void* userContext);

    /**
     * @brief Reserved callback for asynchronous UART transmit completion.
     *
     * @param[in] status Transmit operation status.
     * @param[in] data_len Number of bytes associated with the transmit operation.
     * @param[in] userContext Application-defined callback context.
     *
     * @return None.
     *
     * @note The ESP-IDF implementation currently returns
     * RET_ERR_STS_NOT_SUPPORTED when registering this callback.
     *
     * @note Copying bytes into the UART driver buffer does not guarantee
     * physical transmission completion.
     */
    typedef void (*bspUartTxCallback_t)(ret_err_sts_t status, size_t data_len, void* userContext);

    /* -------------------------------------------------------------------------- */
    /* UART Handle Structure */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Application-owned handle for a BSP UART instance.
     *
     * @details Stores the configuration associated with a UART instance.
     *
     * @pre Zero-initialize the handle and populate its configuration before
     * calling the UART initialization API.
     *
     * @note The handle must remain at a stable memory address until
     * bspUartDeInit() completes successfully.
     *
     * @note Configuration must not be modified between initialization and
     * deinitialization except through supported BSP APIs.
     *
     * @warning Only one UART handle may own a hardware UART port at a time.
     */
    typedef struct
    {
        bspUartConfig_t config; /**< UART instance configuration. */
    } bspUartHandle_t;

    /* -------------------------------------------------------------------------- */


    /* -------------------------------------------------------------------------- */
    /* UART Initialization and Deinitialization APIs */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Initialize a UART instance using the supplied configuration.
     *
     * @param[in,out] handle Pointer to the UART handle containing the configuration.
     *
     * @return BSP status indicating initialization success or failure.
     *
     * @pre The handle must be valid and its configuration initialized.
     *
     * @note Only one handle may own a hardware UART port at a time.
     * @note Initialization must be performed from task context.
     *
     * @warning Do not access the same handle concurrently during initialization.
     */
    ret_err_sts_t bspUartInit(bspUartHandle_t* handle);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Deinitialize a previously initialized UART instance.
     *
     * @param[in,out] handle Pointer to the initialized UART handle.
     *
     * @return BSP status indicating deinitialization success or failure.
     *
     * @pre Any task executing bspUartProcessRx() for this handle must have exited.
     *
     * @note Deinitialization releases resources associated with the UART instance.
     * @note Deinitialization must be performed from task context.
     *
     * @warning Do not invoke other UART operations concurrently with deinitialization.
     */
    ret_err_sts_t bspUartDeInit(bspUartHandle_t* handle);

    /* -------------------------------------------------------------------------- */
    /* UART Synchronous Communication APIs */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Transmit data synchronously through an initialized UART.
     *
     * @param[in] handle Pointer to an initialized UART handle with TX enabled.
     * @param[in] data Pointer to the data buffer to transmit.
     * @param[in] length Number of bytes to transmit; must be nonzero.
     * @param[in] timeout_ms Maximum wait for transmission completion, in milliseconds.
     *
     * @return BSP status indicating transmission success, timeout, or failure.
     *
     * @pre The data buffer must remain valid for the duration of the operation.
     *
     * @note The implementation may block while accepting transmit data.
     * @note Transmission completion is subject to the specified timeout and
     * the guarantees provided by the target UART driver.
     */
    ret_err_sts_t
    bspUartSendSync(bspUartHandle_t* handle, const uint8_t* data, size_t length, uint32_t timeout_ms);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Receive UART data synchronously into a caller-provided buffer.
     *
     * @param[in] handle Pointer to an initialized UART handle with RX enabled.
     * @param[out] buffer Pointer to the receive buffer.
     * @param[in] buffer_len Capacity of the receive buffer in bytes; must be nonzero.
     * @param[out] rx_len Pointer to the actual number of bytes received.
     * @param[in] timeout_ms Maximum receive wait in milliseconds.
     *
     * @return BSP status indicating receive success or failure.
     *
     * @note Partial reception is permitted.
     * @note A timeout without received data returns RET_ERR_STS_OK with
     * rx_len set to zero.
     *
     * @warning Do not use synchronous and asynchronous receive processing
     * concurrently on the same UART instance.
     */
    ret_err_sts_t bspUartReceiveSync(bspUartHandle_t* handle,
                                     uint8_t* buffer,
                                     size_t buffer_len,
                                     size_t* rx_len,
                                     uint32_t timeout_ms);

    /* -------------------------------------------------------------------------- */
    /* UART Asynchronous Communication APIs */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Request asynchronous reception into a caller-provided buffer.
     *
     * @param[in,out] handle Pointer to an initialized UART handle with RX enabled.
     * @param[out] buffer Pointer to the caller-owned receive buffer.
     * @param[in] length Requested receive length in bytes; must be nonzero.
     *
     * @return BSP status indicating request acceptance or failure.
     *
     * @note Availability of this operation depends on the target implementation.
     * @note Unsupported implementations return RET_ERR_STS_NOT_SUPPORTED.
     *
     * @warning The caller must preserve the buffer lifetime while an
     * asynchronous reception is pending.
     */
    ret_err_sts_t bspUartReadAsync(bspUartHandle_t* handle, uint8_t* buffer, size_t length);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Submit data for asynchronous UART transmission.
     *
     * @param[in] handle Pointer to an initialized UART handle with TX enabled.
     * @param[in] buffer Pointer to the data buffer to transmit.
     * @param[in] length Number of bytes to transmit; must be nonzero.
     *
     * @return BSP status indicating whether the transmit request was accepted.
     *
     * @note The operation may block while transmit buffering is unavailable.
     * @note Successful submission does not guarantee that all bytes have
     * completed physical transmission.
     */
    ret_err_sts_t bspUartSendAsync(bspUartHandle_t* handle, const uint8_t* buffer, size_t length);

    /* -------------------------------------------------------------------------- */
    /* UART Callback Registration APIs */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Register or unregister the UART receive callback.
     *
     * @param[in,out] handle Pointer to an initialized UART handle.
     * @param[in] callback Receive callback function, or NULL to unregister.
     * @param[in] userContext Application-defined context passed to the callback.
     *
     * @return BSP status indicating registration success or failure.
     *
     * @note Callback execution context is defined by the BSP UART callback
     * contract and must not be assumed to be interrupt context.
     *
     * @note The callback is invoked without the UART state mutex held.
     */
    ret_err_sts_t bspUartSetRxCallback(bspUartHandle_t* handle,
                                       bspUartRxCallback_t callback,
                                       void* userContext);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Register or unregister the UART transmit completion callback.
     *
     * @param[in,out] handle Pointer to an initialized UART handle.
     * @param[in] callback Transmit callback function, or NULL to unregister.
     * @param[in] userContext Application-defined context passed to the callback.
     *
     * @return BSP status indicating registration success or failure.
     *
     * @note Callback registration is optional and may not be implemented
     * by all supported UART targets.
     *
     * @note Unsupported implementations return RET_ERR_STS_NOT_SUPPORTED.
     *
     * @note Successful data buffering must not be interpreted as physical
     * transmission completion.
     */
    ret_err_sts_t bspUartSetTxCallback(bspUartHandle_t* handle,
                                       bspUartTxCallback_t callback,
                                       void* userContext);

    /* -------------------------------------------------------------------------- */
    /* UART Receive Processing and Cancellation APIs */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Process pending UART receive events and deliver available data.
     *
     * @param[in,out] handle Pointer to an initialized UART handle with RX enabled.
     * @param[in] timeout_ms Maximum wait for receive activity in milliseconds.
     *
     * @return BSP status indicating processing success or failure.
     *
     * @note Received data is delivered through the registered RX callback.
     * @note A timeout without available data is not treated as a failure.
     * @note Application logic is responsible for protocol framing and
     * assembling complete messages from received data.
     *
     * @pre Only one task may process RX events for a UART instance at a time.
     *
     * @warning Stop receive processing before deinitializing the UART.
     */
    ret_err_sts_t bspUartProcessRx(bspUartHandle_t* handle, uint32_t timeout_ms);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Cancel an outstanding asynchronous UART receive operation.
     *
     * @param[in,out] handle Pointer to an initialized UART handle.
     *
     * @return BSP status indicating cancellation success or failure.
     *
     * @note Cancellation behavior depends on the active receive mechanism.
     * @note Implementations without a pending asynchronous receive may
     * complete this operation without modifying UART state.
     *
     * @note Receive callbacks may be invoked during cancellation according
     * to the BSP callback contract.
     */
    ret_err_sts_t bspUartCancelRx(bspUartHandle_t* handle);

    /* -------------------------------------------------------------------------- */
    /* UART Transmission Completion API */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Wait for pending UART transmission to complete.
     *
     * @param[in] handle Pointer to an initialized UART handle with TX enabled.
     * @param[in] timeout_ms Maximum wait for transmission completion in milliseconds.
     *
     * @return BSP status indicating success, timeout, or another failure.
     *
     * @note The timeout applies to waiting for pending transmission activity.
     */
    ret_err_sts_t bspUartWaitTxDone(bspUartHandle_t* handle, uint32_t timeout_ms);

    /* -------------------------------------------------------------------------- */
    /* UART Buffer Management APIs */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Discard buffered UART receive data.
     *
     * @param[in,out] handle Pointer to an initialized UART handle.
     *
     * @return BSP status indicating flush success or failure.
     *
     * @note The operation returns RET_ERR_STS_BUSY when an asynchronous
     * receive operation prevents safe buffer flushing.
     */
    ret_err_sts_t bspUartFlushRx(bspUartHandle_t* handle);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Discard pending UART transmit data when supported.
     *
     * @param[in,out] handle Pointer to an initialized UART handle.
     *
     * @return BSP status indicating flush success or failure.
     *
     * @note Transmission buffer flushing is target-dependent.
     * @note Unsupported implementations return RET_ERR_STS_NOT_SUPPORTED.
     *
     * @warning Pending transmit data may be discarded without being
     * physically transmitted.
     */
    ret_err_sts_t bspUartFlushTx(bspUartHandle_t* handle);

    /* -------------------------------------------------------------------------- */
    /* UART Runtime Status APIs */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Retrieve the current UART runtime status.
     *
     * @param[in] handle Pointer to an initialized UART handle.
     * @param[out] status Pointer to the structure receiving the UART status.
     *
     * @return BSP status indicating query success or failure.
     *
     * @note The returned structure represents a snapshot of UART state,
     * transfer counters, and error counters.
     */
    ret_err_sts_t bspUartGetStatus(bspUartHandle_t* handle, bspUartStatus_t* status);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Query whether UART reception is currently active.
     *
     * @param[in] handle Pointer to an initialized UART handle.
     * @param[out] busy Pointer to the variable receiving the RX busy state.
     *
     * @return BSP status indicating query success or failure.
     *
     * @note A true busy state indicates that receive processing is active.
     */
    ret_err_sts_t bspUartIsRxBusy(bspUartHandle_t* handle, bool* busy);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Query whether UART transmission is currently active.
     *
     * @param[in] handle Pointer to an initialized UART handle.
     * @param[out] busy Pointer to the variable receiving the TX busy state.
     *
     * @return BSP status indicating query success or failure.
     *
     * @note The returned value represents the transmit activity state
     * at the time of the query.
     */
    ret_err_sts_t bspUartIsTxBusy(bspUartHandle_t* handle, bool* busy);

    /* -------------------------------------------------------------------------- */
    /* UART Transfer Counter APIs */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Retrieve the UART receive byte count.
     *
     * @param[in] handle Pointer to an initialized UART handle.
     * @param[out] rx_count Pointer to the variable receiving the RX byte count.
     *
     * @return BSP status indicating query success or failure.
     *
     * @note The count represents bytes recorded by the UART implementation.
     */
    ret_err_sts_t bspUartGetRxCount(bspUartHandle_t* handle, size_t* rx_count);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Retrieve the UART transmit byte count.
     *
     * @param[in] handle Pointer to an initialized UART handle.
     * @param[out] tx_count Pointer to the variable receiving the TX byte count.
     *
     * @return BSP status indicating query success or failure.
     *
     * @note The count represents bytes accepted for transmission.
     * @note Accepted bytes are not necessarily fully transmitted on the wire.
     */
    ret_err_sts_t bspUartGetTxCount(bspUartHandle_t* handle, size_t* tx_count);

    /* -------------------------------------------------------------------------- */
    /* UART Configuration Management APIs */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Apply a new configuration to an initialized UART instance.
     *
     * @param[in,out] handle Pointer to an initialized UART handle.
     * @param[in] config Pointer to the UART configuration to apply.
     *
     * @return BSP status indicating configuration success or failure.
     *
     * @pre The port and owner in the supplied configuration must match
     * the existing UART handle.
     *
     * @note Reconfiguration may be rejected while RX or TX is active.
     */
    ret_err_sts_t bspUartSetConfig(bspUartHandle_t* handle, const bspUartConfig_t* config);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Retrieve the active configuration of a UART instance.
     *
     * @param[in] handle Pointer to an initialized UART handle.
     * @param[out] config Pointer to the structure receiving the configuration.
     *
     * @return BSP status indicating query success or failure.
     *
     * @note The returned configuration is a copy of the active UART settings.
     */
    ret_err_sts_t bspUartGetConfig(bspUartHandle_t* handle, bspUartConfig_t* config);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Update the UART communication baud rate.
     *
     * @param[in,out] handle Pointer to an initialized UART handle.
     * @param[in] baudrate Requested baud rate in bits per second; must be nonzero.
     *
     * @return BSP status indicating update success or failure.
     *
     * @note Baud rate changes may be rejected while RX or TX is active.
     * @note Other UART configuration parameters remain unchanged.
     */
    ret_err_sts_t bspUartSetBaudrate(bspUartHandle_t* handle, bspUartBaudrate_t baudrate);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Retrieve the configured UART communication baud rate.
     *
     * @param[in] handle Pointer to an initialized UART handle.
     * @param[out] baudrate Pointer to the variable receiving the baud rate.
     *
     * @return BSP status indicating query success or failure.
     *
     * @note The returned baud rate is expressed in bits per second.
     */
    ret_err_sts_t bspUartGetBaudrate(bspUartHandle_t* handle, bspUartBaudrate_t* baudrate);

    /* -------------------------------------------------------------------------- */
    /* UART Extended Control API */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Execute an optional UART control request.
     *
     * @param[in,out] handle Pointer to an initialized UART handle.
     * @param[in] req UART control request identifier.
     * @param[in,out] arg Pointer to request-specific arguments, if required.
     *
     * @return BSP status indicating request success or failure.
     *
     * @note Available control requests depend on the UART BSP implementation.
     * @note Unsupported requests return RET_ERR_STS_NOT_SUPPORTED.
     *
     * @warning The caller must ensure that the argument pointer and data
     * layout are valid for the specified control request.
     */
    ret_err_sts_t
    bspUartIoctl(bspUartHandle_t* handle, bspUartIoctlRequest_t req, void* arg);

    /* -------------------------------------------------------------------------- */


#ifdef __cplusplus
}
#endif
#endif /* BSP_UART_H */
