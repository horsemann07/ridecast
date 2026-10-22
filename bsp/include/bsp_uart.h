
/**
 * @file bsp_uart.h
 * @brief BSP UART driver API definitions.
 *
 * This file contains type definitions and function prototypes for the BSP UART driver,
 * supporting both synchronous and asynchronous operations, configuration, and status reporting.
 */


#ifndef __BSP_UART_H__
#define __BSP_UART_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Standard includes. */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


#include "bsp_config.h"
#include "bsp_err_sts.h"

// =============================================================================
// Public Types / Definitions
// =============================================================================

/**
 * @brief Common UART baud-rate definitions.
 */
#define BSP_UART_BAUD_1200        ((uint32_t)1200U)
#define BSP_UART_BAUD_9600        ((uint32_t)9600U)
#define BSP_UART_BAUD_19200       ((uint32_t)19200U)
#define BSP_UART_BAUD_38400       ((uint32_t)38400U)
#define BSP_UART_BAUD_57600       ((uint32_t)57600U)
#define BSP_UART_BAUD_115200      ((uint32_t)115200U)
#define BSP_UART_BAUD_230400      ((uint32_t)230400U)
#define BSP_UART_BAUD_460800      ((uint32_t)460800U)
#define BSP_UART_BAUD_921600      ((uint32_t)921600U)

/**
 * @brief Maximum/default UART buffer size configured by the BSP.
 */
#define BSP_UART_RXTX_BUFFER_SIZE (bspCONFIG_UART_RXTX_BUFFER_SIZE)

/**
 * @brief Compile-time enable/disable for asynchronous UART support.
 */
#define BSP_UART_ENABLE_ASYNC     (bspCONFIG_UART_ENABLE_ASYNC)

    /**
     * @brief BSP UART owner identifier.
     *
     * The owner identifies the functional module that owns a UART instance.
     * This can be used by the BSP for configuration lookup, access control,
     * or diagnostic purposes.
     */
    typedef uint8_t bspUartOwner_t;


    /**
     * @brief UART baud-rate type.
     */
    typedef uint32_t bspUartBaudrate_t;


    /**
     * @enum bspUartPort_t
     * @brief Identifies the UART peripheral instance.
     *
     * The actual number of available UART instances is platform-dependent.
     * Unsupported instances shall return PLT_ERR_INVALID_PARAM.
     */
    typedef enum
    {
        eBspUartPort0 = 0U,
        eBspUartPort1 = 1U,
        eBspUartPort2 = 2U,
        eBspUartPort3 = 3U,
        eBspUartPort4 = 4U,
        eBspUartPort5 = 5U,
        eBspUartPort6 = 6U,
        eBspUartPort7 = 7U
    } bspUartPort_t;


    /**
     * @enum bspUartParity_t
     * @brief UART parity configuration.
     */
    typedef enum
    {
        eBspUartParityNone = 0U, /**< No parity bit. */
        eBspUartParityOdd  = 1U, /**< Odd parity. */
        eBspUartParityEven = 2U  /**< Even parity. */
    } bspUartParity_t;


    /**
     * @enum bspUartStopBits_t
     * @brief UART stop-bit configuration.
     */
    typedef enum
    {
        eBspUartStopBitsOne = 0U, /**< One stop bit. */
        eBspUartStopBitsTwo = 1U  /**< Two stop bits. */
    } bspUartStopBits_t;


    /**
     * @enum bspUartMode_t
     * @brief UART operating direction.
     */
    typedef enum
    {
        eBspUartModeTx   = 0U, /**< Transmit only. */
        eBspUartModeRx   = 1U, /**< Receive only. */
        eBspUartModeTxRx = 2U  /**< Transmit and receive. */
    } bspUartMode_t;


    /**
     * @enum bspUartWordLength_t
     * @brief UART data-word length.
     *
     * The supported values represent the number of data bits in each UART word.
     */
    typedef enum
    {
        eBspUartWordLength5 = 5U, /**< 5 data bits. */
        eBspUartWordLength6 = 6U, /**< 6 data bits. */
        eBspUartWordLength7 = 7U, /**< 7 data bits. */
        eBspUartWordLength8 = 8U  /**< 8 data bits. */
    } bspUartWordLength_t;


    /**
     * @enum bspUartDmaMode_t
     * @brief UART DMA usage mode.
     *
     * DMA operation is an implementation detail of the BSP. The application
     * shall normally use the standard synchronous/asynchronous UART APIs
     * without directly depending on DMA-specific driver APIs.
     */
    typedef enum
    {
        eBspUartDmaDisabled = 0U, /**< DMA disabled for TX and RX. */
        eBspUartDmaTx       = 1U, /**< DMA enabled for TX only. */
        eBspUartDmaRx       = 2U, /**< DMA enabled for RX only. */
        eBspUartDmaTxRx     = 3U  /**< DMA enabled for both TX and RX. */
    } bspUartDmaMode_t;


    /**
     * @enum bspUartFlowControl_t
     * @brief UART hardware flow-control mode.
     */
    typedef enum
    {
        eBspUartFlowControlNone   = 0U, /**< No hardware flow control. */
        eBspUartFlowControlRtsCts = 1U  /**< RTS/CTS hardware flow control. */
    } bspUartFlowControl_t;


    /**
     * @enum bspUartIoctlRequest_t
     * @brief Optional UART control/extension requests.
     *
     * Normal UART operations such as status queries, cancellation, flushing,
     * and transfer completion shall use dedicated BSP APIs.
     *
     * IOCTL is reserved for optional or future UART controls that do not
     * justify a dedicated public API.
     */
    typedef enum
    {
        eBspUartIoctlReserved = 0U
    } bspUartIoctlRequest_t;


    /**
     * @struct bspUartConfig_t
     * @brief UART initialization and operating configuration.
     *
     * This structure contains only UART configuration parameters. Runtime
     * state, callback state, synchronization objects, and platform-specific
     * driver resources are maintained internally by the BSP.
     */
    typedef struct
    {
        /* =========================
         * Instance / ownership
         * ========================= */

        bspUartPort_t port;
        bspUartOwner_t owner;

        /* =========================
         * UART protocol configuration
         * ========================= */

        bspUartBaudrate_t baudrate;
        bspUartWordLength_t wordLength;
        bspUartParity_t parity;
        bspUartStopBits_t stopBits;
        bspUartMode_t mode;

        /* =========================
         * Pin configuration
         * ========================= */

        uint32_t uartRxPin;
        uint32_t uartTxPin;
        uint32_t uartRtsPin;
        uint32_t uartCtsPin;

        /* =========================
         * FIFO / buffering
         * ========================= */

        uint16_t fifoSize;
        uint16_t rxThreshold;

        /* =========================
         * Optional UART features
         * ========================= */

        bspUartFlowControl_t flowControl;
        bspUartDmaMode_t dmaMode;

        /* =========================
         * Hardware-specific options
         * ========================= */

        uint8_t oversampling;

        bool invertTx;
        bool invertRx;

    } bspUartConfig_t;


    /**
     * @struct bspUartStatus_t
     * @brief Current runtime status of a UART instance.
     *
     * The BSP updates these values internally. Applications shall use the
     * corresponding status APIs instead of modifying this structure directly.
     */
    typedef struct
    {
        bool initialized; /**< True when UART is initialized. */
        bool rxBusy;      /**< True when an RX transfer is active. */
        bool txBusy;      /**< True when a TX transfer is active. */

        size_t rxCount;   /**< Number of bytes received by latest transfer. */
        size_t txCount; /**< Number of bytes transmitted by latest transfer. */

        uint32_t rxErrorCount; /**< Number of detected RX errors. */
        uint32_t txErrorCount; /**< Number of detected TX errors. */

    } bspUartStatus_t;


    /**
     * @typedef bspUartRxCallback_t
     * @brief Callback invoked when an asynchronous RX operation completes.
     *
     * @param[in] status
     *        Result of the RX operation.
     *
     * @param[in] data
     *        Pointer to the receive buffer supplied by the caller.
     *        This pointer may be NULL when the operation fails.
     *
     * @param[in] data_len
     *        Number of valid bytes received.
     *
     * @param[in] userContext
     *        User-defined context supplied during callback registration.
     */
    typedef void (*bspUartRxCallback_t)(bsp_err_sts_t status,
                                        uint8_t* data,
                                        size_t data_len,
                                        void* userContext);


    /**
     * @typedef bspUartTxCallback_t
     * @brief Callback invoked when an asynchronous TX operation completes.
     *
     * @param[in] status
     *        Result of the TX operation.
     *
     * @param[in] data_len
     *        Number of bytes successfully transmitted.
     *
     * @param[in] userContext
     *        User-defined context supplied during callback registration.
     */
    typedef void (*bspUartTxCallback_t)(bsp_err_sts_t status, size_t data_len, void* userContext);


    /**
     * @struct bspUartHandle_t
     * @brief UART BSP handle.
     *
     * The handle represents one logical UART instance and is passed to UART BSP
     * APIs for all operations.
     *
     * The BSP owns and manages the runtime state associated with this handle,
     * including synchronization objects, driver resources, asynchronous
     * transfer state, and callback information.
     *
     * The @ref config member contains the user-configurable UART parameters.
     *
     * @note
     * The application shall not modify BSP-managed runtime data after
     * bspUartInit() has been called.
     */
    typedef struct
    {
        /* =========================
         * UART configuration
         * ========================= */

        bspUartConfig_t config;
    } bspUartHandle_t;


    /**
     * @brief Initialize a UART port.
     *
     * Configures the UART hardware according to the parameters provided
     * in the BSP UART handle. This includes baud rate, word length,
     * parity, stop bits, flow control, and pin mapping.
     *
     * If asynchronous mode is enabled, this function also installs the
     * UART driver, creates the event queue, and starts the UART event
     * handling thread.
     *
     * @param[in] ptHandle Pointer to BSP UART handle containing configuration.
     *
     * @return PLT_ERR_STS_OK            UART initialized successfully.
     * @return PLT_ERR_STS_INVALID_PARAM Invalid handle or UART port number.
     * @return PLT_ERR_STS_FAIL          UART driver or hardware configuration failed.
     * @return PLT_ERR_STS_NO_MEM        Failed to create RTOS resources.
     *
     * @note This function must be called before any UART send/receive APIs.
     * @note In async mode, a single shared event thread is used for all UARTs.
     */
    bsp_err_sts_t bspUartInit(bspUartHandle_t* ptHandle);

    /**
     * @brief De-initialize a UART port.
     *
     * Stops UART operation, uninstalls the ESP-IDF UART driver,
     * deletes associated RTOS resources, and clears the runtime
     * context for the specified UART port.
     *
     * @param[in] ptHandle Pointer to BSP UART handle.
     *
     * @return PLT_ERR_STS_OK            UART de-initialized successfully.
     * @return PLT_ERR_STS_INVALID_PARAM Invalid handle or UART port number.
     * @return PLT_ERR_STS_FAIL          Failed to uninstall UART driver.
     *
     * @note Any ongoing asynchronous RX operation is aborted.
     * @note After de-initialization, the UART must be re-initialized
     *       before reuse.
     */
    bsp_err_sts_t bspUartDeInit(bspUartHandle_t* ptHandle);

    /**
     * @brief Send data synchronously over UART.
     *
     * Writes the specified buffer to the UART transmit FIFO and blocks
     * until all data has been transmitted or a timeout occurs.
     *
     * @param[in] handle     Pointer to BSP UART handle.
     * @param[in] data       Pointer to data buffer to transmit.
     * @param[in] length     Number of bytes to transmit.
     * @param[in] timeout_ms Timeout in milliseconds to wait for transmission.
     *
     * @return PLT_ERR_STS_OK            Data transmitted successfully.
     * @return PLT_ERR_STS_INVALID_PARAM Invalid parameters.
     * @return PLT_ERR_STS_FAIL          UART transmission failed or timed out.
     *
     * @note This API blocks the calling thread.
     * @note Suitable for low-frequency or control-path transmissions.
     */
    bsp_err_sts_t
    bspUartSendSync(bspUartHandle_t* handle, const uint8_t* data, size_t length, uint32_t timeout_ms);

    /**
     * @brief Receive data synchronously from UART.
     *
     * Reads up to the requested number of bytes from the UART receive
     * FIFO. The call blocks until data is received or the timeout expires.
     *
     * @param[in]     handle     Pointer to BSP UART handle.
     * @param[out]    buffer     Buffer to store received data.
     * @param[in]     buffer_len Length of the data buffer.
     * @param[out]    rx_len     On output: actual number of bytes received.
     * @param[in]     timeout_ms Timeout in milliseconds to wait for data.
     *
     * @return PLT_ERR_STS_OK            Data received successfully.
     * @return PLT_ERR_STS_INVALID_PARAM Invalid parameters.
     * @return PLT_ERR_STS_FAIL          UART reception failed.
     *
     * @note This API blocks the calling thread.
     */
    bsp_err_sts_t bspUartReceiveSync(bspUartHandle_t* handle,
                                     uint8_t* buffer,
                                     size_t buffer_len,
                                     size_t* rx_len,
                                     uint32_t timeout_ms);


    /**
     * @brief Start an asynchronous UART receive operation.
     *
     * Initiates a non-blocking RX operation. Incoming data is accumulated
     * into the provided buffer, and the registered callback is invoked
     * when the RX operation completes or is cancelled.
     *
     * @param[in] handle Pointer to BSP UART handle.
     * @param[out] buffer Buffer to store received data.
     * @param[in] length Number of bytes to receive.
     *
     * @return PLT_ERR_STS_OK            RX operation started successfully.
     * @return PLT_ERR_STS_INVALID_PARAM Invalid parameters.
     * @return PLT_ERR_STS_BUSY          RX already in progress.
     * @return PLT_ERR_STS_NOT_EXIST     Callback not registered.
     *
     * @note The buffer must remain valid until the callback is invoked.
     * @note Only one asynchronous RX operation is allowed at a time.
     */

    bsp_err_sts_t bspUartReadAsync(bspUartHandle_t* handle, uint8_t* buffer, size_t length);

    /**
     * @brief Send data asynchronously over UART.
     *
     * Writes data to the UART transmit FIFO without waiting for
     * transmission completion.
     *
     * @param[in] handle Pointer to BSP UART handle.
     * @param[in] buffer Data buffer to transmit.
     * @param[in] length Number of bytes to transmit.
     *
     * @return PLT_ERR_STS_OK            Data queued for transmission.
     * @return PLT_ERR_STS_INVALID_PARAM Invalid parameters.
     * @return PLT_ERR_STS_FAIL          UART write failed.
     *
     * @note This API does not provide completion notification.
     */

    bsp_err_sts_t
    bspUartSendAsync(bspUartHandle_t* handle, const uint8_t* buffer, size_t length);

    /**
     * @brief Perform control operations on a UART port.
     *
     * Provides control and query operations for asynchronous UART RX,
     * such as querying RX status, retrieving received byte count, or
     * cancelling an ongoing RX operation.
     *
     * @param[in] handle Pointer to BSP UART handle.
     * @param[in] req    IOCTL request identifier.
     * @param[in,out] arg Pointer to request-specific argument.
     *
     * @return PLT_ERR_STS_OK                 Operation successful.
     * @return PLT_ERR_STS_INVALID_PARAM      Invalid parameters or request.
     * @return PLT_ERR_STS_UNSUPPORTED Async mode not enabled.
     *
     * @note Some requests require @p arg to be non-NULL and point to a
     *       specific data type (e.g., size_t or bool).
     * @note Cancelling RX triggers the registered callback with failure status.
     */

    bsp_err_sts_t
    bspUartIoctl(bspUartHandle_t* handle, bspUartIoctlRequest_t req, void* arg);

    /*================ Callback ================*/

    /**
     * @brief Register the receive-complete callback for a UART instance.
     *
     * The callback is invoked when an asynchronous receive operation
     * completes, is cancelled, or fails. Passing NULL as @p callback
     * unregisters any previously registered callback.
     *
     * @note The callback may be executed in interrupt context; keep it short
     *       and non-blocking.
     *
     * @param[in] handle       Pointer to the UART handle.
     * @param[in] callback     Function to call on RX events, or NULL to unregister.
     * @param[in] userContext  Opaque pointer passed back to the callback unchanged.
     *
     * @retval BSP_OK                 Callback registered successfully.
     * @retval PLT_ERR_INVALID_PARAM  @p handle is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartSetRxCallback(bspUartHandle_t* handle,
                                       bspUartRxCallback_t callback,
                                       void* userContext);

    /**
     * @brief Register the transmit-complete callback for a UART instance.
     *
     * The callback is invoked when an asynchronous transmit operation
     * completes or fails. Passing NULL as @p callback unregisters any
     * previously registered callback.
     *
     * @note The callback may be executed in interrupt context; keep it short
     *       and non-blocking.
     *
     * @param[in] handle       Pointer to the UART handle.
     * @param[in] callback     Function to call on TX events, or NULL to unregister.
     * @param[in] userContext  Opaque pointer passed back to the callback unchanged.
     *
     * @retval BSP_OK                 Callback registered successfully.
     * @retval PLT_ERR_INVALID_PARAM  @p handle is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartSetTxCallback(bspUartHandle_t* handle,
                                       bspUartTxCallback_t callback,
                                       void* userContext);


    /*================ Control ================*/

    /**
     * @brief Cancel an ongoing asynchronous receive operation.
     *
     * If a receive is in progress it is aborted and the RX callback (if
     * registered) may be notified with a cancelled/aborted status. Data
     * already received remains available via the RX count.
     *
     * @param[in] handle  Pointer to the UART handle.
     *
     * @retval BSP_OK                 Receive cancelled, or no receive was active.
     * @retval PLT_ERR_INVALID_PARAM  @p handle is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartCancelRx(bspUartHandle_t* handle);

    /**
     * @brief Block until all pending transmit data has been sent.
     *
     * Waits until the TX buffer/FIFO and the shift register are empty, or
     * until the timeout expires.
     *
     * @param[in] handle       Pointer to the UART handle.
     * @param[in] timeout_ms   Maximum time to wait in milliseconds.
     *
     * @retval BSP_OK                 All data transmitted.
     * @retval PLT_ERR_TIMEOUT        Timeout expired before transmission finished.
     * @retval PLT_ERR_INVALID_PARAM  @p handle is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartWaitTxDone(bspUartHandle_t* handle, uint32_t timeout_ms);

    /**
     * @brief Discard all data in the receive buffer/FIFO.
     *
     * @param[in] handle  Pointer to the UART handle.
     *
     * @retval BSP_OK                 RX buffer flushed.
     * @retval PLT_ERR_INVALID_PARAM  @p handle is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartFlushRx(bspUartHandle_t* handle);

    /**
     * @brief Discard all pending data in the transmit buffer/FIFO.
     *
     * @note Data already being shifted out on the wire may still complete.
     *
     * @param[in] handle  Pointer to the UART handle.
     *
     * @retval BSP_OK                 TX buffer flushed.
     * @retval PLT_ERR_INVALID_PARAM  @p handle is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartFlushTx(bspUartHandle_t* handle);


    /*================ Status ================*/

    /**
     * @brief Get the current status of the UART instance.
     *
     * @param[in]  handle  Pointer to the UART handle.
     * @param[out] status  Pointer to a structure that receives the status
     *                     (e.g. busy flags, error flags).
     *
     * @retval BSP_OK                 Status returned successfully.
     * @retval PLT_ERR_INVALID_PARAM  @p handle or @p status is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartGetStatus(bspUartHandle_t* handle, bspUartStatus_t* status);

    /**
     * @brief Check whether a receive operation is currently in progress.
     *
     * @param[in]  handle  Pointer to the UART handle.
     * @param[out] busy    Set to true if RX is busy, false otherwise.
     *
     * @retval BSP_OK                 Query successful.
     * @retval PLT_ERR_INVALID_PARAM  @p handle or @p busy is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartIsRxBusy(bspUartHandle_t* handle, bool* busy);

    /**
     * @brief Check whether a transmit operation is currently in progress.
     *
     * @param[in]  handle  Pointer to the UART handle.
     * @param[out] busy    Set to true if TX is busy, false otherwise.
     *
     * @retval BSP_OK                 Query successful.
     * @retval PLT_ERR_INVALID_PARAM  @p handle or @p busy is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartIsTxBusy(bspUartHandle_t* handle, bool* busy);

    /**
     * @brief Get the number of bytes received so far.
     *
     * Returns the number of bytes received in the current (or most recent)
     * receive operation, or the number of bytes available in the RX buffer,
     * depending on the driver implementation.
     *
     * @param[in]  handle    Pointer to the UART handle.
     * @param[out] rx_count  Receives the number of bytes received.
     *
     * @retval BSP_OK                 Count returned successfully.
     * @retval PLT_ERR_INVALID_PARAM  @p handle or @p rx_count is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartGetRxCount(bspUartHandle_t* handle, size_t* rx_count);

    /**
     * @brief Get the number of bytes transmitted so far.
     *
     * Returns the number of bytes sent in the current (or most recent)
     * transmit operation, or the number of bytes still pending in the TX
     * buffer, depending on the driver implementation.
     *
     * @param[in]  handle    Pointer to the UART handle.
     * @param[out] tx_count  Receives the number of bytes transmitted.
     *
     * @retval BSP_OK                 Count returned successfully.
     * @retval PLT_ERR_INVALID_PARAM  @p handle or @p tx_count is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartGetTxCount(bspUartHandle_t* handle, size_t* tx_count);


    /*================ Configuration ================*/

    /**
     * @brief Apply a new configuration to the UART instance.
     *
     * Reconfigures parameters such as baud rate, data bits, parity and
     * stop bits.
     *
     * @warning Do not call while a transmit or receive is in progress; the
     *          result is undefined or PLT_ERR_BUSY may be returned.
     *
     * @param[in] handle  Pointer to the UART handle.
     * @param[in] config  Pointer to the configuration to apply.
     *
     * @retval BSP_OK                 Configuration applied.
     * @retval PLT_ERR_INVALID_PARAM  @p handle or @p config is NULL, or a
     *                                configuration field is invalid.
     * @retval PLT_ERR_BUSY           UART is busy with an active transfer.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartSetConfig(bspUartHandle_t* handle, const bspUartConfig_t* config);

    /**
     * @brief Read back the current UART configuration.
     *
     * @param[in]  handle  Pointer to the UART handle.
     * @param[out] config  Pointer to a structure that receives the
     *                     current configuration.
     *
     * @retval BSP_OK                 Configuration returned successfully.
     * @retval PLT_ERR_INVALID_PARAM  @p handle or @p config is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartGetConfig(bspUartHandle_t* handle, bspUartConfig_t* config);

    /**
     * @brief Change the UART baud rate.
     *
     * Only the baud rate is modified; all other configuration parameters
     * remain unchanged.
     *
     * @warning Do not call while a transfer is in progress.
     *
     * @param[in] handle    Pointer to the UART handle.
     * @param[in] baudrate  New baud rate to apply.
     *
     * @retval BSP_OK                 Baud rate changed.
     * @retval PLT_ERR_INVALID_PARAM  @p handle is NULL or @p baudrate is unsupported.
     * @retval PLT_ERR_BUSY           UART is busy with an active transfer.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartSetBaudrate(bspUartHandle_t* handle, bspUartBaudrate_t baudrate);

    /**
     * @brief Get the current UART baud rate.
     *
     * @param[in]  handle    Pointer to the UART handle.
     * @param[out] baudrate  Receives the current baud rate.
     *
     * @retval BSP_OK                 Baud rate returned successfully.
     * @retval PLT_ERR_INVALID_PARAM  @p handle or @p baudrate is NULL.
     * @retval PLT_ERR_NOT_INIT       UART instance is not initialized.
     */
    bsp_err_sts_t bspUartGetBaudrate(bspUartHandle_t* handle, bspUartBaudrate_t* baudrate);


#ifdef __cplusplus
}
#endif

#endif /* __BSP_UART_H__ */