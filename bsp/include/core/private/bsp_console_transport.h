```c
/**
 * @file bsp_console_transport.h
 * @brief BSP console transport abstraction.
 *
 * This header defines the common transport interface used by the BSP CLI
 * and BSP logger.
 *
 * The transport abstraction separates console functionality from the
 * underlying physical communication interface.
 *
 * Supported transport implementations may include:
 *
 * - UART
 * - USB CDC
 * - SEGGER RTT
 * - BLE UART
 * - TCP Telnet
 *
 * @par Architecture
 * @code
 *
 *                    BSP APPLICATION
 *                           |
 *                 +---------+---------+
 *                 |                   |
 *                 v                   v
 *             BSP CLI             BSP LOG
 *                 |                   |
 *                 +---------+---------+
 *                           |
 *                           v
 *              bsp_console_transport.h
 *                           |
 *             +-------------+-------------+
 *             |             |             |
 *             v             v             v
 *           UART         USB CDC         RTT
 *
 * @endcode
 *
 * @par Ownership
 * The BSP/application provides the concrete transport implementation.
 *
 * The CLI and logger only consume this interface and do not access the
 * underlying UART, USB, RTT, BLE, or Telnet implementation directly.
 *
 * @par RX Usage
 * Receive functionality is intended primarily for the CLI.
 *
 * The logger normally uses only the write() operation.
 *
 * @par Thread Safety
 * The transport implementation must ensure that write() is safe when
 * called by multiple execution contexts.
 *
 * In particular, CLI and logging may concurrently request TX operations.
 * The selected transport implementation must serialize access to the
 * underlying physical TX resource.
 *
 * @par ISR Safety
 * The transport callbacks must document whether they are ISR-safe.
 *
 * The CLI RX callback may be invoked from an interrupt context depending
 * on the underlying transport implementation. The callback must therefore
 * avoid blocking operations and must not execute CLI commands directly.
 *
 * @note This interface does not contain any EmbeddedCLI dependency.
 *
 * @see bsp_cli.h
 * @see bsp_log.h
 */

#ifndef BSP_CONSOLE_TRANSPORT_H
#define BSP_CONSOLE_TRANSPORT_H


/*-----------------------------------------------------------------------------
 * SYSTEM LEVEL INCLUDES
 *----------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>


/*-----------------------------------------------------------------------------
 * PROJECT LEVEL INCLUDES
 *----------------------------------------------------------------------------*/

#include "bsp_err_sts.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*=============================================================================
 * TYPES
 *============================================================================*/

/**
 * @brief Console transport RX callback.
 *
 * The transport invokes this callback when receive data is available.
 *
 * The callback must be lightweight. It must not parse commands, execute
 * application commands, or perform potentially blocking operations.
 *
 * The normal BSP CLI implementation stores received data into an RX
 * ring buffer and processes it later from the CLI task/thread context.
 *
 * @param[in] status
 *        Status of the receive operation.
 *
 * @param[in] data
 *        Pointer to received data.
 *
 * @param[in] len
 *        Number of received bytes.
 *
 * @param[in] userCtx
 *        User-provided callback context.
 */
/**
 * @brief Console RX callback.
 */
typedef void (*bspConsoleRxCallback_t)(
    uint8_t* data,
    size_t length,
    bsp_err_sts_t status,
    void* userContext);



/**
 * @brief BSP console transport interface.
 *
 * This structure defines the operations required by the BSP CLI and
 * BSP logger to communicate with a console transport.
 *
 * Only write() is required by the logger.
 *
 * The CLI may use write(), read_start(), and set_callback().
 */
typedef struct bspConsoleTransport
{
    /**
     * @brief Write data to the console transport.
     *
     * This function is used by both BSP CLI and BSP LOG.
     *
     * The implementation must ensure that concurrent writes do not
     * corrupt the output stream.
     *
     * @param[in] context
     *        Transport-specific context.
     *
     * @param[in] data
     *        Pointer to data to transmit.
     *
     * @param[in] len
     *        Number of bytes to transmit.
     *
     * @return BSP_ERR_STS_OK
     *         Data accepted successfully.
     *
     * @return BSP_ERR_STS_INVALID_PARAM
     *         Invalid parameter.
     *
     * @return BSP_ERR_STS_NOT_INIT
     *         Transport is not initialized.
     *
     * @return BSP_ERR_STS_BUSY
     *         Transport cannot currently accept the data.
     *
     * @return BSP_ERR_STS_FAIL
     *         Transport transmission failed.
     */
    bsp_err_sts_t (*write)(
        void* context,
        const uint8_t* data,
        uint16_t len);


    /**
     * @brief Start an asynchronous receive operation.
     *
     * This operation is normally used by BSP CLI.
     *
     * The logger does not require receive functionality.
     *
     * @param[in] context
     *        Transport-specific context.
     *
     * @param[out] buffer
     *        Buffer supplied by the CLI for received data.
     *
     * @param[in] len
     *        Size of the receive buffer.
     *
     * @return BSP_ERR_STS_OK
     *         Receive operation started successfully.
     *
     * @return BSP_ERR_STS_INVALID_PARAM
     *         Invalid parameter.
     *
     * @return BSP_ERR_STS_NOT_INIT
     *         Transport is not initialized.
     *
     * @return BSP_ERR_STS_BUSY
     *         Receive operation is already active.
     *
     * @return BSP_ERR_STS_FAIL
     *         Receive operation failed.
     */
    bsp_err_sts_t (*read_start)(
        void* context,
        uint8_t* buffer,
        uint16_t len);


    /**
     * @brief Register the console RX callback.
     *
     * This operation is normally used by BSP CLI.
     *
     * The logger does not require an RX callback.
     *
     * @param[in] context
     *        Transport-specific context.
     *
     * @param[in] callback
     *        RX callback function.
     *
     * @param[in] userCtx
     *        User context passed to the callback.
     *
     * @return BSP_ERR_STS_OK
     *         Callback registered successfully.
     *
     * @return BSP_ERR_STS_INVALID_PARAM
     *         Invalid parameter.
     *
     * @return BSP_ERR_STS_NOT_INIT
     *         Transport is not initialized.
     */
    bsp_err_sts_t (*set_callback)(
        void* context,
        bspConsoleRxCallback_t callback,
        void* userCtx);


    /**
     * @brief Transport-specific context.
     *
     * This pointer is passed to every transport operation.
     *
     * The BSP/application owns the object referenced by this pointer.
     */
    void* context;

} bspConsoleTransport_t;


/*=============================================================================
 * HELPER MACROS
 *============================================================================*/

/**
 * @brief Check whether a console transport is valid for TX.
 *
 * @param[in] transport
 *        Pointer to console transport.
 *
 * @return true if the transport and write function are valid.
 */
#define BSP_CONSOLE_TRANSPORT_IS_TX_VALID(transport) \
    (((transport) != NULL) &&                         \
     ((transport)->write != NULL))


/**
 * @brief Check whether a console transport is valid for RX.
 *
 * @param[in] transport
 *        Pointer to console transport.
 *
 * @return true if the transport provides RX functionality.
 */
#define BSP_CONSOLE_TRANSPORT_IS_RX_VALID(transport) \
    (((transport) != NULL) &&                         \
     ((transport)->read_start != NULL) &&             \
     ((transport)->set_callback != NULL))



/*=============================================================================
 * PUBLIC API
 *============================================================================*/
/**
 * @brief Initialize the BSP console transport.
 *
 * Initializes the configured console transport backend.
 *
 * The actual transport implementation is selected internally by
 * the BSP console transport layer.
 *
 * @return BSP_ERR_STS_OK
 *         Transport initialized successfully.
 *
 * @return BSP_ERR_STS_ALREADY_INIT
 *         Transport is already initialized.
 *
 * @return BSP_ERR_STS_INVALID_PARAM
 *         Invalid transport configuration.
 *
 * @return BSP_ERR_STS_FAIL
 *         Transport initialization failed.
 */
bsp_err_sts_t bsp_console_transport_init(void);


/**
 * @brief Deinitialize the BSP console transport.
 *
 * Releases resources owned by the console transport.
 *
 * @return BSP_ERR_STS_OK
 *         Transport deinitialized successfully.
 *
 * @return BSP_ERR_STS_NOT_INIT
 *         Transport was not initialized.
 */
bsp_err_sts_t bsp_console_transport_deinit(void);


/**
 * @brief Get the initialized console transport interface.
 *
 * This interface can be passed to BSP logger and BSP CLI.
 *
 * @return Pointer to the console transport interface.
 *
 * @return NULL if the transport is not initialized.
 */
const bspConsoleTransport_t*
bsp_console_transport_get(void);


#ifdef __cplusplus
}
#endif


#endif /* BSP_CONSOLE_TRANSPORT_H */
