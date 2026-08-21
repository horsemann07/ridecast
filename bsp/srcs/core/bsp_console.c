/**
 * @file bsp_console.c
 * @brief BSP console common initialization implementation.
 *
 * This module does not implement logger or CLI functionality.
 *
 * It only provides a common initialization/deinitialization sequence
 * for:
 *
 *     bsp_log
 *     bsp_cli
 *
 * Each component continues to own its own implementation and state.
 */


/*=============================================================================
 * PROJECT INCLUDES
 *============================================================================*/
#include "bsp_cfg_map.h"
#include "bsp_console.h"

/*=============================================================================
 * PRIVATE DATA
 *============================================================================*/


/*=============================================================================
 * PUBLIC API
 *============================================================================*/
static bsp_err_sts_t bspConsoleUartWrite(
    void* context,
    const uint8_t* data,
    uint16_t len);

static bsp_err_sts_t bspConsoleUartReadStart(
    void* context,
    uint8_t* buffer,
    uint16_t len);

static bsp_err_sts_t bspConsoleUartSetCallback(
    void* context,
    bspConsoleRxCallback_t callback,
    void* userCtx);
/*=============================================================================
 * PRIVATE DATA
 *============================================================================*/

/**
 * @brief UART configuration used by the console transport.
 *
 * This configuration can alternatively be obtained from bsp_config.c
 * if the project already has a central BSP configuration table.
 */
static bspUartHandle_t *s_consoleUart = &g_bspUartCfg[BSP_UART_OWNER_CONSOLE];

/*=============================================================================
 * CONSOLE TRANSPORT OBJECT
 *============================================================================*/

/**
 * @brief Common console transport interface.
 */
static const bspConsoleTransport_t s_consoleTransport =
{
    .write =
        bspConsoleUartWrite,

    .read_start =
        bspConsoleUartReadStart,

    .set_callback = bspConsoleUartSetCallback,

    .context = s_consoleUart
};


/*=============================================================================
 * TRANSPORT CALLBACKS
 *============================================================================*/

/**
 * @brief Write data through UART.
 */
static bsp_err_sts_t bspConsoleUartWrite(
    void* context,
    const uint8_t* data,
    uint16_t len)
{
    bspUartHandle_t* uart;

    if((context == NULL) ||
       (data == NULL) ||
       (len == 0U))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    uart = (bspUartHandle_t*)context;

    return bspUartWriteAsync(
        uart,
        data,
        (size_t)len);
}


/**
 * @brief Start UART asynchronous receive.
 */
static bsp_err_sts_t bspConsoleUartReadStart(
    void* context,
    uint8_t* buffer,
    uint16_t len)
{
    bspUartHandle_t* uart;

    if((context == NULL) ||
       (buffer == NULL) ||
       (len == 0U))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    uart = (bspUartHandle_t*)context;

    return bspUartReadAsync(
        uart,
        buffer,
        (size_t)len);
}


/**
 * @brief Register UART RX callback.
 */
static bsp_err_sts_t bspConsoleUartSetCallback(
    void* context,
    bspConsoleRxCallback_t callback,
    void* userCtx)
{
    bspUartHandle_t* uart;

    if(context == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    uart = (bspUartHandle_t*)context;

    return bspUartSetCallback(
        uart,
        callback,
        userCtx);
}

/*----------------------------------------------------------------------------*/
bsp_err_sts_t bsp_console_init(
    const bspConsoleConfig_t* config)
{
    bsp_err_sts_t status;


    /*---------------------------------------------------------------------
     * Validate configuration.
     *---------------------------------------------------------------------*/

    if(config == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(config->logTransport == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

#if (BSP_CLI_ENABLE == 1U)

    if(config->cliTransport == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }
#endif

    /*---------------------------------------------------------------------
     * Initialize logger.
     *---------------------------------------------------------------------*/
    status = bsp_log_init(config->logTransport);

    if(status != BSP_ERR_STS_OK)
    {
        return status;
    }


#if (BSP_CLI_ENABLE == 1U)

    /*---------------------------------------------------------------------
     * Initialize CLI.
     *---------------------------------------------------------------------*/

    status = bsp_cli_init(config->cliTransport);

    if(status != BSP_ERR_STS_OK)
    {
        /*
         * Logger was initialized successfully but CLI failed.
         *
         * Roll back logger initialization so the console subsystem
         * does not remain partially initialized.
         */
        (void)bsp_log_deinit();

        return status;
    }

#endif
    s_consoleInitialized = true;

    return BSP_ERR_STS_OK;
}


/*----------------------------------------------------------------------------*/

bsp_err_sts_t bsp_console_deinit(void)
{
    bsp_err_sts_t status;
    bsp_err_sts_t logStatus;

    /*---------------------------------------------------------------------
     * Check initialization state.
     *---------------------------------------------------------------------*/
    if(s_consoleInitialized == false)
    {
        return BSP_ERR_STS_NOT_INIT;
    }

    /*
     * Mark the common console layer as deinitialized first.
     */
    s_consoleInitialized = false;

    /*---------------------------------------------------------------------
     * Deinitialize CLI first.
     *---------------------------------------------------------------------*/
#if (BSP_CLI_ENABLE == 1U)

    status = bsp_cli_deinit();

    if(status != BSP_ERR_STS_OK)
    {
        /*
         * Continue with logger deinitialization even if CLI
         * deinitialization reports an error.
         */
    }
#else
    status = BSP_ERR_STS_OK;
#endif

    /*---------------------------------------------------------------------
     * Deinitialize logger.
     *---------------------------------------------------------------------*/
    logStatus = bsp_log_deinit();

    /*---------------------------------------------------------------------
     * Return the most relevant failure status.
     *---------------------------------------------------------------------*/

#if (BSP_CLI_ENABLE == 1U)
    if(status != BSP_ERR_STS_OK)
    {
        return status;
    }
#endif

    return logStatus;
}
/*---------------------------------------------------------------------*/