/**
 * @file bsp_log.c
 * @brief BSP logging and CLI integration over UART.
 *
 * This module implements a lightweight BSP-level logging facility
 * built on top of the UART BSP and the Embedded CLI library.
 *
 * It provides:
 * - Thread-safe formatted logging APIs
 * - Integration of Embedded CLI over UART
 * - Asynchronous UART RX handling for CLI input
 * - Non-blocking UART TX for log output
 *
 * The logger and CLI share the same UART instance and are protected
 * using an RTOS mutex to ensure safe concurrent access from multiple
 * threads.
 *
 * @par Usage
 * @code
 * bsp_log_init(&uartHandle);
 *
 * BSP_LOG_Info("System started");
 * BSP_LOG_Warn("Low battery");
 * BSP_LOG_Error("Initialization failed");
 *
 * bsp_log_deinit();
 * @endcode
 *
 * @par Prerequisites
 * - BSP UART driver must be initialized prior to calling
 *   bsp_log_init().
 * - CMSIS-RTOS2 kernel must be running.
 * - Embedded CLI library must be available in the project.
 *
 * @par Threading Model
 * - Logging APIs may be called from any thread context.
 * - UART RX callbacks are executed in the UART event thread context.
 * - Logging output is serialized using an RTOS mutex.
 *
 * @par Notes
 * - Only one UART instance is supported for logging and CLI.
 * - Only one asynchronous RX operation is active per UART.
 * - Log output is best-effort and may be dropped if UART is busy.
 *
 * @see bsp_log.h
 * @see bsp_uart.h
 * @see embedded_cli.h
 */


#include "bsp_uart.h"
#include "cmsis_os2.h"
#include "embedded_cli.h"
#include "bsp_log.h"
#include "bsp_err_sts.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------- */
/* STATIC STATE                                       */
/* -------------------------------------------------- */

static bspUartHandle_t* s_cliUart = NULL;
static osMutexId_t      s_cliMutex = NULL;

static EmbeddedCli*       s_cli = NULL;
static EmbeddedCliConfig* s_cliCfg = NULL;

static uint8_t s_cliBuffer[BSP_UART_RXTX_BUFFER_SIZE];

/* -------------------------------------------------- */
/* UART → CLI RX CALLBACK                             */
/* -------------------------------------------------- */

static void bspCliUartRxCb(bsp_err_sts_t status,
                           uint8_t* data,
                           uint16_t len,
                           void* userCtx)
{
    (void)userCtx;

    if(status != BSP_ERR_STS_OK || s_cli == NULL || data == NULL)
        return;

    for(uint16_t i = 0; i < len; i++)
    {
        embeddedCliReceiveChar(s_cli, (char)data[i]);
    }
}

/* -------------------------------------------------- */
/* CLI → UART TX CHAR                                 */
/* -------------------------------------------------- */

static void bspCliWriteChar(EmbeddedCli* cli, char c)
{
    bspUartHandle_t* uart = (bspUartHandle_t*)cli->appContext;
    if(uart != NULL)
    {
        bspUartWriteAsync(uart, (uint8_t*)&c, 1);
    }
}

/* -------------------------------------------------- */
/* PUBLIC API                                         */
/* -------------------------------------------------- */

/**
 * @brief Initialize BSP logger and Embedded CLI on UART.
 */
bsp_err_sts_t bsp_log_init(bspUartHandle_t* uart)
{
    if(uart == NULL)
        return BSP_ERR_STS_INVALID_PARAM;

    s_cliUart = uart;

    if(s_cliMutex == NULL)
    {
        s_cliMutex = osMutexNew(NULL);
        if(s_cliMutex == NULL)
            return BSP_ERR_STS_NO_MEM;
    }

    s_cliCfg = embeddedCliDefaultConfig();
    s_cliCfg->cliBuffer     = (uint32_t*)s_cliBuffer;
    s_cliCfg->cliBufferSize = BSP_UART_RXTX_BUFFER_SIZE;
    s_cliCfg->rxBufferSize  = BSP_UART_RXTX_BUFFER_SIZE;
    s_cliCfg->cmdBufferSize = BSP_UART_RXTX_BUFFER_SIZE;

    s_cli = embeddedCliNew(s_cliCfg);
    if(s_cli == NULL)
    {
        osMutexDelete(s_cliMutex);
        s_cliMutex = NULL;
        return BSP_ERR_STS_FAIL;
    }

    s_cli->writeChar  = bspCliWriteChar;
    s_cli->appContext = uart;

    bspUartSetCallback(uart, bspCliUartRxCb, NULL);

    return BSP_ERR_STS_OK;
}

/**
 * @brief Deinitialize BSP logger and CLI.
 */
bsp_err_sts_t bsp_log_deinit(void)
{
    if(s_cli != NULL)
    {
        embeddedCliFree(s_cli);
        s_cli = NULL;
    }

    if(s_cliMutex != NULL)
    {
        osMutexDelete(s_cliMutex);
        s_cliMutex = NULL;
    }

    s_cliUart = NULL;
    return BSP_ERR_STS_OK;
}

/**
 * @brief Log formatted message via Embedded CLI.
 */
void bsp_log(bspLogLevel_t level,
                    const char* file,
                    int line,
                    const char* fmt,
                    ...)
{
    if(s_cli == NULL || s_cliMutex == NULL)
        return;

    char buffer[BSP_UART_RXTX_BUFFER_SIZE];
    size_t len = 0;

    const char* lvl = "I";
    if(level == BSP_LOG_WARN)  lvl = "W";
    if(level == BSP_LOG_ERROR) lvl = "E";
    if(level == BSP_LOG_DEBUG) lvl = "D";

#if LOGGER_INCLUDE_FILELINE
    len = snprintf(buffer, BSP_UART_RXTX_BUFFER_SIZE,
                   "[%s] (%s:%d) ", lvl, file, line);
#else
    len = snprintf(buffer, BSP_UART_RXTX_BUFFER_SIZE,
                   "[%s] ", lvl);
#endif

    va_list args;
    va_start(args, fmt);
    len += vsnprintf(buffer + len,
                     BSP_UART_RXTX_BUFFER_SIZE - len,
                     fmt, args);
    va_end(args);

    osMutexAcquire(s_cliMutex, osWaitForever);
    embeddedCliPrint(s_cli, buffer);
    osMutexRelease(s_cliMutex);
}
