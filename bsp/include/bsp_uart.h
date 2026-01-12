
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

#include "err_status.h"

#include "bsp_config.h"

    // =========================
    // Public Functions / Types
    // =========================

#define BSP_UART_BAUD_1200        ((uint32_t)1200U)
#define BSP_UART_BAUD_9600        ((uint32_t)9600U)
#define BSP_UART_BAUD_19200       ((uint32_t)19200U)
#define BSP_UART_BAUD_38400       ((uint32_t)38400U)
#define BSP_UART_BAUD_57600       ((uint32_t)57600U)
#define BSP_UART_BAUD_115200      ((uint32_t)115200U)
#define BSP_UART_BAUD_230400      ((uint32_t)230400U)
#define BSP_UART_BAUD_460800      ((uint32_t)460800U)
#define BSP_UART_BAUD_921600      ((uint32_t)921600U)

#define BSP_UART_RXTX_BUFFER_SIZE (bspCONFIG_UART_RXTX_BUFFER_SIZE)
#define BSP_UART_POLLING_DELAY_MS (5000U)

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
        eBspUartGetRxCount,  /**< Get the number of bytes received in the current or last async read. */
        eBspUartIsRxBusy,   /**< Check whether an asynchronous receive operation is in progress. */
        eBspUartCancelRx,   /**< Cancel the currently active asynchronous receive operation. */

        eBspUartGetTxCount, /**< Get the number of bytes written in the last asynchronous transmit. */
        eBspUartWaitTxDone  /**< Block until all transmit data is sent or the specified timeout expires. */
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
     * This callback is invoked by the UART driver when asynchronous receive
     * data is available or when a receive operation completes.
     *
     * @param[in] status       Status of the UART asynchronous operation.
     * @param[in] rxCtx        Pointer to UART asynchronous receive context
     *                         containing received data, data length, and
     *                         UART port information.
     * @param[in] userContext  User-provided context pointer (opaque to the driver).
     */
    typedef void (*bspUartCallback_t)(errStatus_t status,
                                      bspUartASyncCtx_t* rxtxCtx,
                                      void* userContext);

    /**
     * @brief Initialize the UART peripheral.
     *
     * @param[in] handle UART handle.
     * @return Status of the operation.
     */
    errStatus_t bspUartInit(bspUartHandle_t* ptHandle);

    /**
     * @brief De-initialize the UART peripheral.
     *
     * @param[in] handle UART handle.
     * @return Status of the operation.
     */
    errStatus_t bspUartDeInit(bspUartHandle_t* ptHandle);

    /**
     * @brief Send data synchronously over UART.
     *
     * @param[in] handle UART handle.
     * @param[in] data Pointer to data buffer to send.
     * @param[in] length Number of bytes to send.
     * @return Status of the operation.
     */
    errStatus_t
    bspUartSendSync(bspUartHandle_t* handle, const uint8_t* data, size_t length, uint32_t timeout_ms);

    /**
     * @brief Receive data synchronously over UART.
     *
     * @param[in] handle UART handle.
     * @param[out] data Pointer to buffer to store received data.
     * @param[in, out] length Number of bytes to receive.
     * @param[in] timeout_ms Timeout in milliseconds.
     * @return Status of the operation.
     */
    errStatus_t
    bspUartReceiveSync(bspUartHandle_t* handle, uint8_t* data, size_t* length, uint32_t timeout_ms);

    /**
     * @brief Send data asynchronously over UART.
     *
     * @param[in] handle UART handle.
     * @param[in] data Pointer to data buffer to send.
     * @param[in] length Number of bytes to send.
     * @param[in] callback Callback function to be called on completion.
     * @param[in] userContext User-provided context pointer.
     * @return Status of the operation.
     */
    errStatus_t bspUartSendAsync(bspUartHandle_t* handle,
                                 const uint8_t* data,
                                 size_t length,
                                 bspUartCallback_t callback,
                                 void* userContext);

    /**
     * @brief Receive data asynchronously over UART.
     *
     * @param[in] handle UART handle.
     * @param[out] data Pointer to buffer to store received data.
     * @param[in, out] length Number of bytes to receive.
     * @param[in] callback Callback function to be called on completion.
     * @param[in] userContext User-provided context pointer.
     * @return Status of the operation.
     */
    errStatus_t bspUartRecvAsyncRegisterCb(bspUartHandle_t* handle,
                                           uint8_t* data,
                                           size_t* length,
                                           bspUartCallback_t callback,
                                           void* userContext);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_UART_H__ */