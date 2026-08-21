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
extern "C" {
#endif

/*---------------------------------------------------------------------------
 * Includes
 *---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "bsp_config.h"

#include "bsp_console_transport.h"
#include "bsp_err_sts.h"
#include "bsp_mutex.h"

/*---------------------------------------------------------------------------
 * Configuration
 *---------------------------------------------------------------------------*/
#ifndef BSP_CLI_RX_BUF_SIZE
#define BSP_CLI_RX_BUF_SIZE         (64U)
#endif

#ifndef BSP_CLI_TX_BUF_SIZE
#define BSP_CLI_TX_BUF_SIZE         (256U)
#endif

#ifndef BSP_CLI_CMD_BUF_SIZE
#define BSP_CLI_CMD_BUF_SIZE        (256U)
#endif

#ifndef BSP_CLI_RING_SIZE
#define BSP_CLI_RING_SIZE           (256U)
#endif

#ifndef BSP_CLI_MAX_BINDINGS
#define BSP_CLI_MAX_BINDINGS        (16U)
#endif

#ifndef BSP_CLI_MUTEX_TIMEOUT_MS
#define BSP_CLI_MUTEX_TIMEOUT_MS    (100U)
#endif

/*---------------------------------------------------------------------------
 * Forward Declarations
 *---------------------------------------------------------------------------*/
typedef struct bspCliContext bspCliContext_t;
typedef bspCliContext_t* bspCliHandle_t;

/*---------------------------------------------------------------------------
 * Command Handler
 *---------------------------------------------------------------------------*/
typedef void (*bspCliCmdHandler_t)(
    uint32_t argc,
    char* argv[]);

/*---------------------------------------------------------------------------
 * Command Definition
 *---------------------------------------------------------------------------*/
typedef struct
{
    const char* name;
    const char* help;
    bspCliCmdHandler_t handler;

} bspCliCommand_t;

/*---------------------------------------------------------------------------
 * Transport RX Callback
 *---------------------------------------------------------------------------*/
typedef void (*bspCliRxCallback_t)(
    bsp_err_sts_t status,
    uint8_t* data,
    uint16_t len,
    void* userCtx);

/*---------------------------------------------------------------------------
 * Statistics
 *---------------------------------------------------------------------------*/
typedef struct
{
    uint32_t rxBytes;

    uint32_t txBytes;

    uint32_t rxOverflowCnt;

    uint32_t commandCount;

} bspCliStats_t;

/*---------------------------------------------------------------------------
 * CLI Context
 *---------------------------------------------------------------------------*/
struct bspCliContext
{
    /* Internal CLI instance */
    void* cli;

    /* Transport */
    const bspConsoleTransport_t* transport;

    /* Synchronization */
    bspMutexHandle_t mutex;

    /* State */
    bool initialized;

    /* RX Buffer */
    uint8_t rxBuf[BSP_CLI_RX_BUF_SIZE];

    /* TX Buffer */
    uint8_t txBuf[BSP_CLI_TX_BUF_SIZE];

    uint16_t txLen;

    /* RX Ring Buffer */
    uint8_t rxRing[BSP_CLI_RING_SIZE];

    volatile uint16_t rxHead;

    volatile uint16_t rxTail;

    /* Statistics */
    volatile uint32_t rxBytes;

    volatile uint32_t txBytes;

    volatile uint32_t rxOverflowCnt;

    volatile uint32_t commandCount;

    /* EmbeddedCLI Internal Buffer */
    uint32_t cliBuffer[
        (BSP_CLI_CMD_BUF_SIZE + sizeof(uint32_t) - 1U) /
        sizeof(uint32_t)];

    /* Internal Command Storage */
    uint8_t bindingStorage[BSP_CLI_MAX_BINDINGS * sizeof(void*) * 16U];

    uint32_t bindingCount;
};

/*---------------------------------------------------------------------------
 * Public APIs
 *---------------------------------------------------------------------------*/

/**
 * @brief Initialize CLI instance.
 *
 * @param[in,out] handle     CLI context.
 * @param[in]     transport  Transport interface.
 *
 * @retval BSP_ERR_STS_OK
 * @retval BSP_ERR_STS_INVALID_PARAM
 * @retval BSP_ERR_STS_FAIL
 */
bsp_err_sts_t bsp_cli_init(
    bspCliHandle_t handle,
    const bspCliTransport_t* transport);

/**
 * @brief Deinitialize CLI instance.
 *
 * @param[in,out] handle CLI context.
 *
 * @retval BSP_ERR_STS_OK
 */
bsp_err_sts_t bsp_cli_deinit(
    bspCliHandle_t handle);

/**
 * @brief Process CLI input and execute commands.
 *
 * Call periodically from a task or main loop.
 *
 * @param[in,out] handle CLI context.
 */
void bsp_cli_process(
    bspCliHandle_t handle);

/**
 * @brief Register a CLI command.
 *
 * @param[in,out] handle  CLI context.
 * @param[in]     command Command definition.
 *
 * @retval BSP_ERR_STS_OK
 * @retval BSP_ERR_STS_INVALID_PARAM
 * @retval BSP_ERR_STS_NO_MEM
 */
bsp_err_sts_t bsp_cli_register_command(
    bspCliHandle_t handle,
    const bspCliCommand_t* command);

/**
 * @brief Unregister a CLI command.
 *
 * @param[in,out] handle CLI context.
 * @param[in]     name   Command name.
 *
 * @retval BSP_ERR_STS_OK
 * @retval BSP_ERR_STS_NOT_FOUND
 */
bsp_err_sts_t bsp_cli_unregister_command(
    bspCliHandle_t handle,
    const char* name);

/**
 * @brief Print formatted text to CLI transport.
 *
 * @param[in,out] handle CLI context.
 * @param[in]     fmt    Format string.
 */
void bsp_cli_printf(
    bspCliHandle_t handle,
    const char* fmt,
    ...);

/**
 * @brief Get CLI statistics.
 *
 * @param[in]  handle CLI context.
 * @param[out] stats  Statistics structure.
 *
 * @retval BSP_ERR_STS_OK
 * @retval BSP_ERR_STS_INVALID_PARAM
 */
bsp_err_sts_t bsp_cli_get_stats(
    bspCliHandle_t handle,
    bspCliStats_t* stats);

/**
 * @brief Reset CLI statistics.
 *
 * @param[in,out] handle CLI context.
 */
void bsp_cli_reset_stats(
    bspCliHandle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CLI_H */