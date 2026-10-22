/**
 * @file bsp_cli.h
 * @brief BSP Command Line Interface (CLI) API.
 *
 * Portable BSP CLI wrapper around EmbeddedCLI.
 * Supports:
 *  - Multiple CLI instances
 *  - Static allocation
 *  - UART/USB CDC/RTT/Telnet transports
 *  - RTOS and bare-metal systems
 */

#ifndef BSP_CLI_H
#define BSP_CLI_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------------------------------------------------------
 * Includes
 *---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "bsp_config.h"

#include "bsp_err_sts.h"
#include "core/bsp_console_transport.h"
#include "core/bsp_osal.h"


    /*---------------------------------------------------------------------------
     * Forward Declarations
     *---------------------------------------------------------------------------*/

    /**
     * @brief Opaque CLI instance context.
     *
     * The implementation owns the internal EmbeddedCLI object, transport state,
     * RX ring buffer, command bindings, and runtime statistics.
     */
    typedef struct bspCliContext bspCliContext_t;

    /**
     * @brief Handle to one BSP CLI instance.
     *
     * Pass this handle to all BSP CLI public APIs after initialization.
     */
    typedef bspCliContext_t* bspCliHandle_t;

    /*---------------------------------------------------------------------------
     * Command Handler
     *---------------------------------------------------------------------------*/

    /**
     * @brief Application callback executed for a registered CLI command.
     *
     * The command name is not included in @p argv. For example, for:
     *
     * @code
     * set_mode normal
     * @endcode
     *
     * the callback receives:
     *
     * @code
     * argc    = 2U;
     * argv[0] = "set_mode";
     * argv[1] = "normal";
     * @endcode
     *
     * @param[in] argc Number of command tokens in @p argv.
     * @param[in] argv Array of writable, null-terminated command tokens.
     */
    typedef void (*bspCliCmdHandler_t)(uint32_t argc, char* argv[]);

    /*---------------------------------------------------------------------------
     * Command Definition
     *---------------------------------------------------------------------------*/

    /**
     * @brief Application command definition.
     *
     * The command definition and its strings must remain valid while the
     * command is registered with the CLI.
     */
    typedef struct
    {
        /**
         * @brief Command entered by the user.
         *
         * Example: @c "reset".
         */
        const char* name;

        /**
         * @brief Human-readable command description.
         *
         * Displayed by the EmbeddedCLI built-in @c help command.
         * May be @c NULL when no description is required.
         */
        const char* description;

        /**
         * @brief Function called when the command is executed.
         *
         * Must not be @c NULL for application-registered commands.
         */
        bspCliCmdHandler_t handler;
    } bspCliCommand_t;

    /*---------------------------------------------------------------------------
     * Transport RX Callback
     *---------------------------------------------------------------------------*/

    /**
     * @brief CLI receive callback function type.
     *
     * Called by the console transport when new input bytes are available.
     * The BSP CLI implementation stores bytes in its internal RX ring buffer;
     * it does not execute commands from this callback.
     *
     * This callback can run in interrupt context, depending on the transport.
     * Therefore it must not block, allocate memory, or call CLI command handlers.
     *
     * @param[in] status  Receive-operation status.
     * @param[in] data    Received byte buffer.
     * @param[in] len     Number of received bytes.
     * @param[in] userCtx CLI instance context supplied during callback registration.
     */
    typedef void (*bspCliRxCallback_t)(bsp_err_sts_t status, uint8_t* data, uint16_t len, void* userCtx);


    /*---------------------------------------------------------------------------
     * Public APIs
     *---------------------------------------------------------------------------*/
    /**
     * @brief Initialize a CLI instance.
     *
     * The transport must provide a valid write() function. If supported by the
     * transport, the CLI registers an RX callback and starts asynchronous receive.
     *
     * @param[in,out] handle    CLI instance context.
     * @param[in]     transport Console transport interface.
     *
     * @return PLT_ERR_STS_OK on success.
     * @return PLT_ERR_STS_INVALID_PARAM for invalid arguments.
     * @return PLT_ERR_STS_NO_MEM if required static resources are unavailable.
     * @return PLT_ERR_STS_FAIL if EmbeddedCLI initialization fails.
     */
    bsp_err_sts_t bsp_cli_init(bspCliHandle_t handle, const bspConsoleTransport_t* transport);

    /**
     * @brief Deinitialize a CLI instance.
     *
     * Stops transport callbacks, flushes pending TX data, and releases OS objects.
     *
     * @param[in,out] handle CLI instance context.
     *
     * @return PLT_ERR_STS_OK on success.
     * @return PLT_ERR_STS_INVALID_PARAM if handle is NULL.
     */
    bsp_err_sts_t bsp_cli_deinit(bspCliHandle_t handle);

    /**
     * @brief Process received CLI input.
     *
     * Call periodically from the CLI task or bare-metal main loop.
     *
     * This function passes received bytes to EmbeddedCLI, processes commands,
     * flushes pending CLI output, and appends firmware/version/statistics after
     * the built-in EmbeddedCLI @c help command.
     *
     * @param[in,out] handle CLI instance context.
     */
    void bsp_cli_process(bspCliHandle_t handle);

    /**
     * @brief Register an application CLI command.
     *
     * @param[in,out] handle  Initialized CLI instance.
     * @param[in]     command Command name, help text, and handler.
     *
     * @return PLT_ERR_STS_OK on success.
     * @return PLT_ERR_STS_NOT_INIT if CLI is not initialized.
     * @return PLT_ERR_STS_INVALID_PARAM for invalid arguments.
     * @return PLT_ERR_STS_ALREADY_EXISTS if the command name is already used.
     * @return PLT_ERR_STS_NO_MEM if the command-binding limit is reached.
     */
    bsp_err_sts_t bsp_cli_register_command(bspCliHandle_t handle,
                                           const bspCliCommand_t* command);

    /**
     * @brief Unregister an application CLI command.
     *
     * The EmbeddedCLI instance is rebuilt internally after successful removal.
     *
     * @param[in,out] handle Initialized CLI instance.
     * @param[in]     name   Name of the command to remove.
     *
     * @return PLT_ERR_STS_OK on success.
     * @return PLT_ERR_STS_INVALID_PARAM for invalid arguments.
     * @return PLT_ERR_STS_NOT_FOUND if no matching command exists.
     * @return PLT_ERR_STS_FAIL if rebuilding EmbeddedCLI fails.
     */
    bsp_err_sts_t bsp_cli_unregister_command(bspCliHandle_t handle, const char* name);

    /**
     * @brief Print formatted output to the CLI transport.
     *
     * Output is limited to @c BSP_CLI_PRINTF_BUF_SIZE - 1 bytes.
     *
     * @param[in,out] handle Initialized CLI instance.
     * @param[in]     fmt    printf-style format string.
     */
    void bsp_cli_printf(bspCliHandle_t handle, const char* fmt, ...);

    /**
     * @brief Read current CLI statistics.
     *
     * @param[in]  handle Initialized CLI instance.
     * @param[out] stats  Destination statistics structure.
     *
     * @return PLT_ERR_STS_OK on success.
     * @return PLT_ERR_STS_INVALID_PARAM if an argument is NULL.
     */
    bsp_err_sts_t bsp_cli_get_stats(bspCliHandle_t handle, bspCliStats_t* stats);

    /**
     * @brief Reset all CLI statistics counters to zero.
     *
     * @param[in,out] handle CLI instance context.
     */
    void bsp_cli_reset_stats(bspCliHandle_t handle);


    /**
     * @brief Get the CLI instance reserved for bsp_console.
     *
     * The returned instance is statically allocated inside bsp_cli.c.
     */
    bspCliHandle_t bsp_cli_get_console_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CLI_H */