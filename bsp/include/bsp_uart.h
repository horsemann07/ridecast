
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

#include "bsp_err_sts.h"

#include "bsp_config.h"

    // =========================
    // Public Functions / Types
    // =========================

#define BSP_UART_BAUD_1200            ((uint32_t)1200U)
#define BSP_UART_BAUD_9600            ((uint32_t)9600U)
#define BSP_UART_BAUD_19200           ((uint32_t)19200U)
#define BSP_UART_BAUD_38400           ((uint32_t)38400U)
#define BSP_UART_BAUD_57600           ((uint32_t)57600U)
#define BSP_UART_BAUD_115200          ((uint32_t)115200U)
#define BSP_UART_BAUD_230400          ((uint32_t)230400U)
#define BSP_UART_BAUD_460800          ((uint32_t)460800U)
#define BSP_UART_BAUD_921600          ((uint32_t)921600U)

#define BSP_UART_RXTX_BUFFER_SIZE     (bspCONFIG_UART_RXTX_BUFFER_SIZE)
#define BSP_UART_ASYC_TASK_PRIORITY   (bspCONFIG_UART_ASYC_TASK_PRIORITY)
#define BSP_UART_ASYC_TASK_STACK_SIZE (bspCONFIG_UART_ASYC_TASK_STACK_SIZE)
#define BSP_UART_ASYC_EVNT_QUEUE_LEN  (bspCONFIG_UART_ASYC_EVNT_QUEUE_LEN)


    typedef uint32_t bspUartBaudrate_t;
    typedef uint8_t bspUartOwner_t;

    /**
     * @enum bspUartParity_t
     * @brief UART parity modes.
     */
    typedef enum
    {
        eBspUartParityNone, /**< No parity bit. */
        eBspUartParityOdd,  /**< Odd parity. */
        eBspUartParityEven  /**< Even parity. */
    } bspUartParity_t;

    /**
     * @enum bspUartStopBits_t
     * @brief UART stop bit configuration.
     */
    typedef enum
    {
        eBspUartStopBitsOne, /**< One stop bit. */
        eBspUartStopBitsTwo  /**< Two stop bits. */
    } bspUartStopBits_t;


    /**
     * @enum bspUartIoctlRequest_t
     * @brief Control (IOCTL) requests for BSP UART driver.
     */
    typedef enum
    {
        eBspUartGetRxCount, /**< Get the number of bytes received in the current or last async read. */
        eBspUartIsRxBusy, /**< Check whether an asynchronous receive operation is in progress. */
        eBspUartCancelRx, /**< Cancel the currently active asynchronous receive operation. */

        eBspUartGetTxCount, /**< Get the number of bytes written in the last asynchronous transmit. */
        eBspUartWaitTxDone /**< Block until all transmit data is sent or the specified timeout expires. */
    } bspUartIoctlRequest_t;


    /**
     * @enum bspUartMode_t
     * @brief UART operation mode: transmit, receive, or both.
     */
    typedef enum
    {
        eBspUartModeTx,  /**< Transmit only. */
        eBspUartModeRx,  /**< Receive only. */
        eBspUartModeTxRx /**< Transmit and receive. */
    } bspUartMode_t;

    typedef enum
    {
        eBspUartWordLength5 = 5,
        eBspUartWordLength6 = 6,
        eBspUartWordLength7 = 7,
        eBspUartWordLength8 = 8
    } bspUartWordLength_t;


    /**
     * @struct bspUartHandle_t
     * @brief UART handle/config structure for BSP driver (optimized for memory usage).
     *
     * This structure holds all configuration and runtime parameters for a UART instance.
     * Members are ordered for optimal packing and minimal memory usage on most MCUs.
     *
     * - Place 32-bit fields first, then 16-bit, then 8-bit fields.
     * - Use only the required width for each field.
     *
     * @var bspUartHandle_t::baudrate      UART baud rate (e.g., 9600, 115200).
     * @var bspUartHandle_t::uartRxPin     RX pin number (platform-specific).
     * @var bspUartHandle_t::uartTxPin     TX pin number (platform-specific).
     * @var bspUartHandle_t::fifoSize      FIFO/buffer size (if supported).
     * @var bspUartHandle_t::portNum       UART instance/port number (e.g., 1 for USART1).
     * @var bspUartHandle_t::wordLength    Number of data bits (e.g., 8).
     * @var bspUartHandle_t::parity        UART parity mode (see @ref bspUartParity_t).
     * @var bspUartHandle_t::stopBits      UART stop bits (see @ref bspUartStopBits_t).
     * @var bspUartHandle_t::hwFlowControl Hardware flow control: 0 = disabled, 1 = enabled.
     * @var bspUartHandle_t::mode          UART mode: transmit, receive, or both (see @ref bspUartMode_t).
     * @var bspUartHandle_t::oversampling  Oversampling value (e.g., 8 or 16).
     * @var bspUartHandle_t::invertTx      TX line inversion: 0 = normal, 1 = inverted.
     * @var bspUartHandle_t::invertRx      RX line inversion: 0 = normal, 1 = inverted.
     * @var bspUartHandle_t::dmaEnable     DMA enable: 0 = disabled, 1 = enabled.
     * @var bspUartHandle_t::rxThreshold   RX FIFO threshold (if supported).
     *
     *  @code
     * // Example: Initialize UART handle and use with ESP-IDF
     * bspUartHandle_t uartHandle = {
     *     .portNum = 1, // UART1
     *     .baudrate = BSP_UART_BAUD_115200,
     *     .parity = eBspUartParityNone,
     *     .stopBits = eBspUartStopBitsOne,
     *     .wordLength = 8,
     *     .hwFlowControl = 0,
     *     .mode = eBspUartModeTxRx,
     *     .oversampling = 16,
     *     .invertTx = 0,
     *     .invertRx = 0,
     *     .fifoSize = 128,
     *     .dmaEnable = 0
     * };
     *
     * // Initialize UART peripheral
     * bspUartInit(uartHandle);
     *
     * // Send data
     * const uint8_t msg[] = "Hello UART";
     * bspUartSendSync(uartHandle, msg, sizeof(msg));
     *
     * // Receive data
     * uint8_t rxBuf[16];
     * bspUartReceiveSync(uartHandle, rxBuf, sizeof(rxBuf), 100);
     * @endcode
     */
    typedef struct
    {
        /* =========================
         * Protocol configuration
         * ========================= */
        bspUartBaudrate_t baudrate;     /**< UART baud rate */
        bspUartWordLength_t wordLength; /**< Number of data bits */
        bspUartParity_t parity;         /**< UART parity mode */
        bspUartStopBits_t stopBits;     /**< UART stop bits */
        bspUartMode_t mode; /**< UART mode: transmit, receive, or both. */

        /* =========================
         * Pin configuration
         * ========================= */
        uint32_t uartRxPin;  /**< RX pin number */
        uint32_t uartTxPin;  /**< TX pin number */
        uint32_t uartRtsPin; /**< RTS pin number */
        uint32_t uartCtsPin; /**< CTS pin number */

        /* =========================
         * FIFO and buffering
         * ========================= */
        uint16_t fifoSize; /**< FIFO/buffer size */

        /* =========================
         * Instance identification
         * ========================= */
        uint8_t portNum; /**< UART instance/port number */
        bspUartOwner_t uartOwner; /**< Functional ownership id used for UART configuration lookup by index. */

        /* =========================
         * Low-level options
         * ========================= */
        uint8_t oversampling; /**< Oversampling value (e.g., 8 or 16). */
        uint8_t rxThreshold;  /**< RX FIFO threshold */
        uint8_t invertTx;  /**< TX line inversion: 0 = normal, 1 = inverted. */
        uint8_t invertRx;  /**< RX line inversion: 0 = normal, 1 = inverted. */
        uint8_t dmaEnable; /**< DMA enable: 0 = disabled, 1 = enabled.  */

        bool hwFlowControlEn; /**< Hardware flow control: 0 = disabled, 1 = enabled. */
    } bspUartHandle_t;


    typedef struct
    {
        uint16_t data_len;
        uint8_t data[BSP_UART_RXTX_BUFFER_SIZE];
        uint8_t uart_port;
    } bspUartASyncCtx_t;

    // =========================
    // BSP UART API
    // =========================

   /**
     * @typedef bspUartCallback_t
     * @brief UART asynchronous receive callback function.
     *
     * This callback is invoked by the UART driver from the UART event
     * handling thread context to notify the user about the completion,
     * cancellation, or failure of an asynchronous receive operation.
     *
     * @param[in] status
     *            Status of the asynchronous UART receive operation.
     *            - BSP_ERR_STS_OK       : Receive completed successfully
     *            - BSP_ERR_STS_FAIL     : Receive failed or was cancelled
     *            - Other values     : Implementation-specific error codes
     *
     * @param[in] data
     *            Pointer to the receive buffer containing the received data.
     *            This pointer is the same buffer provided by the user when
     *            starting the asynchronous receive operation.
     *            May be NULL if the receive operation failed or was cancelled.
     *
     * @param[in] data_len
     *            Number of valid bytes available in @p data.
     *            Set to zero if no data was received.
     *
     * @param[in] userContext
     *            User-provided context pointer supplied during callback
     *            registration. The driver does not interpret this value.
     *
     * @note The callback is executed in the context of the UART event thread,
     *       not in interrupt context.
     * @note The receive buffer must remain valid until this callback is invoked.
     * @note Only one asynchronous receive operation is supported per UART port
     *       at a time.
     */
    typedef void (*bspUartCallback_t)(bsp_err_sts_t status,
                                    uint8_t* data,
                                    uint16_t data_len,
                                    void* userContext);


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
     * @return BSP_ERR_STS_OK            UART initialized successfully.
     * @return BSP_ERR_STS_INVALID_PARAM Invalid handle or UART port number.
     * @return BSP_ERR_STS_FAIL          UART driver or hardware configuration failed.
     * @return BSP_ERR_STS_NO_MEM        Failed to create RTOS resources.
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
     * @return BSP_ERR_STS_OK            UART de-initialized successfully.
     * @return BSP_ERR_STS_INVALID_PARAM Invalid handle or UART port number.
     * @return BSP_ERR_STS_FAIL          Failed to uninstall UART driver.
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
     * @return BSP_ERR_STS_OK            Data transmitted successfully.
     * @return BSP_ERR_STS_INVALID_PARAM Invalid parameters.
     * @return BSP_ERR_STS_FAIL          UART transmission failed or timed out.
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
     * @param[out]    data       Buffer to store received data.
     * @param[in]     data_len   Length of the data buffer.
     * @param[out]    rx_len     On output: actual number of bytes received.
     * @param[in]     timeout_ms Timeout in milliseconds to wait for data.
     *
     * @return BSP_ERR_STS_OK            Data received successfully.
     * @return BSP_ERR_STS_INVALID_PARAM Invalid parameters.
     * @return BSP_ERR_STS_FAIL          UART reception failed.
     *
     * @note This API blocks the calling thread.
     */
    bsp_err_sts_t
    bspUartReceiveSync(bspUartHandle_t* handle, uint8_t* data, size_t data_len, size_t* rx_len, uint32_t timeout_ms);

    /**
     * @brief Register an asynchronous UART callback.
     *
     * Registers a user callback function that will be invoked on
     * asynchronous RX completion, cancellation, or error.
     *
     * @param[in] handle        Pointer to BSP UART handle.
     * @param[in] callback      Callback function to register.
     * @param[in] userContext   User-defined context passed to the callback.
     *
     * @return BSP_ERR_STS_OK            Callback registered successfully.
     * @return BSP_ERR_STS_INVALID_PARAM Invalid parameters.
     *
     * @note The callback is invoked from the UART event thread context.
     * @note Only one callback can be registered per UART port.
     */
    bsp_err_sts_t bspUartSetCallback(bspUartHandle_t* handle,
                                   bspUartCallback_t callback,
                                   void* userContext);

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
     * @return BSP_ERR_STS_OK            RX operation started successfully.
     * @return BSP_ERR_STS_INVALID_PARAM Invalid parameters.
     * @return BSP_ERR_STS_BUSY          RX already in progress.
     * @return BSP_ERR_STS_NOT_EXIST     Callback not registered.
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
     * @return BSP_ERR_STS_OK            Data queued for transmission.
     * @return BSP_ERR_STS_INVALID_PARAM Invalid parameters.
     * @return BSP_ERR_STS_FAIL          UART write failed.
     *
     * @note This API does not provide completion notification.
     */

    bsp_err_sts_t bspUartWriteAsync(bspUartHandle_t* handle, const uint8_t* buffer, size_t length);

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
     * @return BSP_ERR_STS_OK                 Operation successful.
     * @return BSP_ERR_STS_INVALID_PARAM      Invalid parameters or request.
     * @return BSP_ERR_STS_UNSUPPORTED Async mode not enabled.
     *
     * @note Some requests require @p arg to be non-NULL and point to a
     *       specific data type (e.g., size_t or bool).
     * @note Cancelling RX triggers the registered callback with failure status.
     */

    bsp_err_sts_t bspUartIoctl(bspUartHandle_t* handle, bspUartIoctlRequest_t req, void* arg);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_UART_H__ */