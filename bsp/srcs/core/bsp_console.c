

/*
void bsp_console_task(void* argument)
{
    (void)argument;

    for(;;)
    {
        bsp_console_process();
        osDelay(1U);
    }
}

// UART1 for both CLI and logger.
const bspConsoleConfig_t consoleConfig =
{
    .logTransport = &uart1Transport,
    .logEnabled = true,
    .logLevel = BSP_LOG_INFO,
    .cliEnabled = true,
    .cliTransport = &uart1Transport
};

// Logging only; CLI remains uninitialized.
const bspConsoleConfig_t consoleConfig =
{
    .logTransport = &uart2Transport,
    .logEnabled = false,
    .logLevel = BSP_LOG_INFO,
    .cliEnabled = false,
    .cliTransport = NULL
};
*/


#include "bsp_console.h"

static bool s_consoleInitialized    = false;
static bool s_consoleCliInitialized = false;

#if (BSP_CLI_ENABLE == 1U)
static bspCliHandle_t s_consoleCliHandle = NULL;
#endif

bsp_err_sts_t bsp_console_init(const bspConsoleConfig_t* config)
{
    bsp_err_sts_t status;

    if(config == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(s_consoleInitialized == true)
    {
        return PLT_ERR_STS_ALREADY_INIT;
    }

    /*
     * Keep logger initialized even when logEnabled is false. This permits:
     *
     * bsp_log_enable(true);
     *
     * later at runtime without reinitializing the console.
     */
    if(config->logTransport == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    status = bsp_log_init(config->logTransport);

    if(status != PLT_ERR_STS_OK)
    {
        return status;
    }

    /* Set default log level */
    status = bsp_log_set_level(BSP_LOG_INFO);

    if(status != PLT_ERR_STS_OK)
    {
        (void)bsp_log_deinit();
        return status;
    }

    /* Default: Enable logging */
    status = bsp_log_enable(true);

    if(status != PLT_ERR_STS_OK)
    {
        (void)bsp_log_deinit();
        return status;
    }

#if (BSP_CLI_ENABLE == 1U)

    /*
     * CLI uses the configured transport only when explicitly enabled.
     * logTransport and cliTransport may point to the same transport.
     */
    if(config->cliTransport == NULL)
    {
        (void)bsp_log_deinit();
        return PLT_ERR_STS_INVALID_PARAM;
    }

    s_consoleCliHandle = bsp_cli_get_console_handle();

    status = bsp_cli_init(s_consoleCliHandle, config->cliTransport);

    if(status != PLT_ERR_STS_OK)
    {
        s_consoleCliHandle = NULL;
        (void)bsp_log_deinit();
        return status;
    }

    s_consoleCliInitialized = true;
#endif /* #if (BSP_CLI_ENABLE == 1U) */

    s_consoleInitialized = true;

    return PLT_ERR_STS_OK;
}
/**
 * @brief Process deferred CLI and logger work.
 *
 * Call this function repeatedly from one dedicated console task or from the
 * bare-metal main loop.
 *
 * This function must not be called from ISR context.
 *
 * Processing order:
 *
 * 1. If runtime CLI is enabled, process received CLI bytes.
 *    The transport RX callback only stores bytes in the CLI RX ring buffer.
 *    Command parsing and command handlers run here in normal task context.
 *
 * 2. Process queued ISR log records.
 *    bsp_log_isr() does not use printf, mutexes, or transport writes in ISR.
 *    It only queues a small log record. bsp_log_process() formats and sends
 *    that record here in normal task context.
 *
 * @note When BSP_CLI_ENABLE is 0U, all CLI processing code is removed at
 *       compile time.
 */
void bsp_console_process(void)
{
    /* Console must be initialized before processing CLI or log work. */
    if(s_consoleInitialized == false)
    {
        return;
    }

#if (BSP_CLI_ENABLE == 1U)

    /*
     * CLI is optional at runtime. When cliEnabled was false during
     * bsp_console_init(), no CLI transport callback or RX operation was
     * started, so bsp_cli_process() is not called.
     */
    if((s_consoleCliInitialized == true) && (s_consoleCliHandle != NULL))
    {
        /*
         * Processes RX-ring bytes, lets EmbeddedCLI parse commands, executes
         * completed application handlers, and flushes pending CLI output.
         */
        bsp_cli_process(s_consoleCliHandle);
    }

#endif

    /*
     * Flush deferred ISR log records.
     *
     * This is safe here because bsp_console_process() runs in task/main-loop
     * context, where formatting, mutex locking, and transport writes are
     * permitted.
     */
    bsp_log_process_isr();
}

/*----------------------------------------------------------------------------*/

/**
 * @brief Deinitialize BSP console services.
 *
 * The CLI is stopped first because it may still receive transport input.
 * Logging is stopped afterwards.
 *
 * The function supports all valid configurations:
 *
 * - Logger only
 * - Logger plus CLI on the same transport
 * - Logger and CLI on separate UART/USB/RTT transports
 * - Compile-time-disabled CLI
 *
 * @return PLT_ERR_STS_OK on successful shutdown.
 * @return PLT_ERR_STS_NOT_INIT if bsp_console_init() was not successful.
 * @return CLI deinitialization failure, if one occurs.
 * @return Logger deinitialization status otherwise.
 */
bsp_err_sts_t bsp_console_deinit(void)
{
    bsp_err_sts_t cliStatus = PLT_ERR_STS_OK;
    bsp_err_sts_t logStatus;

    /* Reject deinitialization when the console was never initialized. */
    if(s_consoleInitialized == false)
    {
        return PLT_ERR_STS_NOT_INIT;
    }

    /*
     * Stop future bsp_console_process() calls from processing CLI/log work
     * while components are being deinitialized.
     */
    s_consoleInitialized = false;

#if (BSP_CLI_ENABLE == 1U)

    /*
     * Deinitialize CLI only when it was enabled at runtime and initialized
     * successfully. This unregisters RX callbacks and releases CLI resources.
     */
    if((s_consoleCliInitialized == true) && (s_consoleCliHandle != NULL))
    {
        cliStatus = bsp_cli_deinit(s_consoleCliHandle);
    }

    /* Clear runtime CLI state for a future bsp_console_init() call. */
    s_consoleCliHandle      = NULL;
    s_consoleCliInitialized = false;

#endif

    /*
     * Deinitialize logger after CLI. This releases the ISR log queue,
     * logger mutex, and transport reference. It does not deinitialize the
     * actual UART/USB/RTT hardware because the application owns it.
     */
    logStatus = bsp_log_deinit();

    /*
     * CLI failure has priority because it is deinitialized first and may
     * indicate that its transport callback was not stopped correctly.
     */
    if(cliStatus != PLT_ERR_STS_OK)
    {
        return cliStatus;
    }

    return logStatus;
}