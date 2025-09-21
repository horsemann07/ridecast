#ifdef __cplusplus
/**
 * @file bsp_uart.h
 * @brief BSP UART driver API definitions.
 *
 * This file contains type definitions and function prototypes for the BSP UART driver,
 * supporting both synchronous and asynchronous operations, configuration, and status reporting.
 */

#ifdef __cplusplus
extern "C"
{
#endif

/* Standard includes. */
#include <stddef.h>
#include <stdint.h>

#include "err_status.h"

    // =========================
    // Public Functions / Types
    // =========================

    /**
     * @enum bspUartOpStatus_t
     * @brief UART read/write operation status values.
     */
    typedef enum
    {
        eBspUartWriteCompleted, /**< UART write operation completed successfully. */
        eBspUartReadCompleted, /**< UART read operation completed successfully. */
        eBspUartLastWriteFailed, /**< UART driver reported error during write operation. */
        eBspUartLastReadFailed /**< UART driver reported error during read operation. */
    } bspUartOpStatus_t;

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
     * @typedef bspUartCallback_t
     * @brief Callback function type for UART operation completion.
     *
     * @param[out] status      UART asynchronous operation status.
     * @param[in]  userContext User-provided context pointer (opaque to the driver).
     */
    typedef void (*bspUartCallback_t) (bspUartOpStatus_t status, void* userContext);

    /**
     * @struct bspUartDescriptor
     * @brief Forward declaration of the UART descriptor type.
     */
    struct bspUartDescriptor;

    /**
     * @typedef bspUartHandle_t
     * @brief Handle type returned by bspUartOpen().
     */
    typedef struct bspUartDescriptor* bspUartHandle_t;

    /**
     * @enum bspUartIoctlRequest_t
     * @brief Control (IOCTL) requests for BSP UART driver.
     */
    typedef enum
    {
        eBspUartSetConfig, /**< Set UART configuration using @ref eBspUartConfig_t. */
        eBspUartGetConfig, /**< Get current UART configuration into @ref eBspUartConfig_t. */
        eBspUartGetTxCount, /**< Get the number of bytes transmitted in last write. */
        eBspUartGetRxCount /**< Get the number of bytes received in last read. */
    } bspUartIoctlRequest_t;

    /**
     * @struct bspUartConfig_t
     * @brief Configuration parameters for BSP UART.
     *
     * @var bspUartConfig_t::baudrate
     * Baud rate to configure (e.g., 9600, 115200).
     * @var bspUartConfig_t::parity
     * Parity mode, see @ref bspUartParity_t.
     * @var bspUartConfig_t::stopBits
     * Stop bits configuration, see @ref bspUartStopBits_t.
     * @var bspUartConfig_t::wordLength
     * Word length (number of data bits, e.g., 8).
     * @var bspUartConfig_t::flowControlEn
     * Flow control: 0 = disabled, 1 = enabled.
     */
    typedef struct
    {
        uint32_t baudrate;
        bspUartParity_t parity;
        bspUartStopBits_t stopBits;
        uint8_t wordLength;
        uint8_t flowControlEn;
    } bspUartConfig_t;

    // =========================
    // BSP UART API
    // =========================

    /**
     * @brief Initialize the UART peripheral.
     *
     * @param[in] handle UART handle.
     * @return Status of the operation.
     */
    errStatus_t bspUartInit (bspUartHandle_t handle);

    /**
     * @brief De-initialize the UART peripheral.
     *
     * @param[in] handle UART handle.
     * @return Status of the operation.
     */
    errStatus_t bspUartDeInit (bspUartHandle_t handle);

    /**
     * @brief Send data synchronously over UART.
     *
     * @param[in] handle UART handle.
     * @param[in] data Pointer to data buffer to send.
     * @param[in] length Number of bytes to send.
     * @return Status of the operation.
     */
    errStatus_t bspUartSendSync (bspUartHandle_t handle, const uint8_t* data, size_t length);

    /**
     * @brief Receive data synchronously over UART.
     *
     * @param[in] handle UART handle.
     * @param[out] data Pointer to buffer to store received data.
     * @param[in] length Number of bytes to receive.
     * @param[in] timeout_ms Timeout in milliseconds.
     * @return Status of the operation.
     */
    errStatus_t
    bspUartReceiveSync (bspUartHandle_t handle, uint8_t* data, size_t length, uint32_t timeout_ms);

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
    errStatus_t bspUartSendAsync (bspUartHandle_t handle,
    const uint8_t* data,
    size_t length,
    bspUartCallback_t callback,
    void* userContext);

    /**
     * @brief Receive data asynchronously over UART.
     *
     * @param[in] handle UART handle.
     * @param[out] data Pointer to buffer to store received data.
     * @param[in] length Number of bytes to receive.
     * @param[in] callback Callback function to be called on completion.
     * @param[in] userContext User-provided context pointer.
     * @return Status of the operation.
     */
    errStatus_t bspUartReceiveAsync (bspUartHandle_t handle,
    uint8_t* data,
    size_t length,
    bspUartCallback_t callback,
    void* userContext);

#ifdef __cplusplus
}
#endif