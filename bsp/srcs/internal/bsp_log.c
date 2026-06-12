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
 * @par Threading Model
 * - Logging APIs may be called from any thread context.
 * - UART RX callbacks are executed in the UART event thread context.
 * - Logging output is serialized using an RTOS mutex.
 * - bsp_log_process() must be called periodically from a task.
 *
 * @see bsp_log.h
 * @see bsp_uart.h
 * @see embedded_cli.h
 */

#include "bsp_log.h"
#include "bsp_uart.h"
#include "bsp_err_sts.h"
#include "cmsis_os2.h"
#include "embedded_cli.h"

#include "bsp_config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------- */
/* CONFIGURATION                                      */
/* -------------------------------------------------- */

/** @brief Maximum log line length including prefix and newline. */
#define BSP_LOG_LINE_MAX_LEN (256U)

/** @brief RX buffer size for async UART reception. */
#define BSP_LOG_RX_BUF_SIZE (64U)

/** @brief TX buffer size for batched CLI output. */
#define BSP_LOG_TX_BUF_SIZE (256U)

/** @brief CLI internal buffer size. */
#define BSP_LOG_CLI_BUF_SIZE (256U)

/** @brief Maximum number of CLI command bindings. */
#define BSP_LOG_CLI_MAX_BINDINGS (16U)

/** @brief Mutex timeout in milliseconds. */
#define BSP_LOG_MUTEX_TIMEOUT_MS (100U)

/** @brief Number of 32-bit words required for CLI internal buffer. */
#define BSP_LOG_CLI_BUF_WORDS \
    ((BSP_LOG_CLI_BUF_SIZE + (sizeof(uint32_t) - 1U)) / sizeof(uint32_t))

/* -------------------------------------------------- */
/* STATIC STATE                                       */
/* -------------------------------------------------- */

static bspUartHandle_t* s_cliUart  = NULL;
static osMutexId_t s_cliMutex      = NULL;
static EmbeddedCli* s_cli          = NULL;
static volatile bool s_initialized = false;

/** @brief CLI library internal buffer (32-bit aligned as required by EmbeddedCliConfig). */
static uint32_t s_cliInternalBuf[BSP_LOG_CLI_BUF_WORDS];

/** @brief UART async RX buffer. */
static uint8_t s_rxBuf[BSP_LOG_RX_BUF_SIZE];

/** @brief Batched TX buffer for CLI write-char output. */
static uint8_t s_txBuf[BSP_LOG_TX_BUF_SIZE];

/** @brief Current fill level of TX buffer. */
static volatile uint16_t s_txBufLen = 0U;

/** @brief Static log line formatting buffer (mutex-protected). */
static char s_logLineBuf[BSP_LOG_LINE_MAX_LEN];

/**< Current BSP log level. */
#if ((BSP_COMPILED_LOG_LEVEL < 0) || (BSP_COMPILED_LOG_LEVEL > 4))
    #error "Invalid BSP_COMPILED_LOG_LEVEL"
#endif
static bspLogLevel_t g_currentBspLogLevel = (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL;

/* -------------------------------------------------- */
/* FORWARD DECLARATIONS                               */
/* -------------------------------------------------- */

static void bspCliUartRxCb(bsp_err_sts_t status, uint8_t* data, uint16_t len, void* userCtx);
static void bspCliWriteChar(EmbeddedCli* cli, char c);
static void bspLogFlushTx(void);

/* -------------------------------------------------- */
/* UART → CLI RX CALLBACK                             */
/* -------------------------------------------------- */
/* ------------------------------------------------------------------------------------ */
/**
 * @brief UART RX complete callback. Feeds received chars to CLI.
 *
 * @note Executed in UART event thread context (not ISR on most CMSIS drivers).
 *       Uses mutex with timeout to protect CLI internal state.
 *       If mutex cannot be acquired, received data is dropped (best-effort).
 *
 * @param[in] status   Result of the async UART read operation.
 * @param[in] data     Pointer to received data buffer.
 * @param[in] len      Number of bytes received.
 * @param[in] userCtx  User context pointer (unused).
 */
static void bspCliUartRxCb(bsp_err_sts_t status, uint8_t* data, uint16_t len, void* userCtx)
{
    (void)userCtx;

    /* Guard: module must be initialized */
    if(s_initialized == false)
    {
        return;
    }

    /* On error or empty data, re-arm and return */
    if((status != BSP_ERR_STS_OK) || (data == NULL) || (len == 0U))
    {
        (void)bspUartReadAsync(s_cliUart, s_rxBuf, (uint16_t)sizeof(s_rxBuf));
        return;
    }

    /* Feed received characters to CLI under mutex protection */
    if(osMutexAcquire(s_cliMutex, BSP_LOG_MUTEX_TIMEOUT_MS) == osOK)
    {
        for(uint16_t i = 0U; i < len; i++)
        {
            embeddedCliReceiveChar(s_cli, (char)data[i]);
        }
        (void)osMutexRelease(s_cliMutex);
    }
    else
    {
        /* Mutex timeout - drop received data (best-effort) */
    }

    /* Re-arm UART RX for continuous reception */
    (void)bspUartReadAsync(s_cliUart, s_rxBuf, (uint16_t)sizeof(s_rxBuf));
}

/* -------------------------------------------------- */
/* CLI → UART TX (BATCHED)                            */
/* -------------------------------------------------- */
/* ------------------------------------------------------------------------------------ */
/**
 * @brief Write character callback for Embedded CLI library.
 *
 * Buffers characters into s_txBuf and flushes when buffer is full
 * or a newline character is received. This batching avoids per-character
 * DMA/interrupt overhead on the UART driver.
 *
 * @note Called from within embeddedCliProcess() context, which is
 *       always mutex-protected in this implementation.
 *
 * @param[in] cli  Pointer to CLI instance (unused).
 * @param[in] c    Character to write.
 */
static void bspCliWriteChar(EmbeddedCli* cli, char c)
{
    (void)cli;

    if(s_txBufLen < (uint16_t)BSP_LOG_TX_BUF_SIZE)
    {
        s_txBuf[s_txBufLen] = (uint8_t)c;
        s_txBufLen++;
    }

    /* Flush on newline or when buffer is full */
    if((c == '\n') || (s_txBufLen >= (uint16_t)BSP_LOG_TX_BUF_SIZE))
    {
        bspLogFlushTx();
    }
}
/* ------------------------------------------------------------------------------------ */
/**
 * @brief Flush accumulated TX buffer over UART.
 *
 * Sends all buffered characters via async UART write and resets
 * the buffer fill level.
 *
 * @note Caller must hold s_cliMutex or ensure exclusive access.
 */
static void bspLogFlushTx(void)
{
    if((s_cliUart != NULL) && (s_txBufLen > 0U))
    {
        (void)bspUartWriteAsync(s_cliUart, s_txBuf, s_txBufLen);
        s_txBufLen = 0U;
    }
}

/* -------------------------------------------------- */
/* PUBLIC API                                         */
/* -------------------------------------------------- */

bsp_err_sts_t bsp_log_set_level(bspLogLevel_t level)
{
    if((level < BSP_LOG_NONE) || (level > BSP_LOG_DEBUG))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(level > (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL)
    {
        g_currentBspLogLevel = (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL;
    }
    else
    {
        g_currentBspLogLevel = level;
    }

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------------------ */
bspLogLevel_t bsp_log_get_level(void)
{
    return g_currentBspLogLevel;
}

/* ------------------------------------------------------------------------------------ */
bsp_err_sts_t bsp_log_enable(bool enable)
{
    if(enable == false)
    {
        g_currentBspLogLevel = BSP_LOG_NONE;
    }
    else if(g_currentBspLogLevel == BSP_LOG_NONE)
    {
        g_currentBspLogLevel = (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL;
    }

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------------------ */
bool bsp_log_is_enabled(void)
{
    return (g_currentBspLogLevel != BSP_LOG_NONE);
}

/* ------------------------------------------------------------------------------------ */
void bsp_log_process(void)
{
    if(s_initialized == false)
    {
        return;
    }

    if(osMutexAcquire(s_cliMutex, BSP_LOG_MUTEX_TIMEOUT_MS) == osOK)
    {
        embeddedCliProcess(s_cli);

        /* Flush any remaining TX data generated by CLI processing */
        bspLogFlushTx();

        (void)osMutexRelease(s_cliMutex);
    }
}

/* ------------------------------------------------------------------------------------ */
void bsp_log(bspLogLevel_t level, const char* file, int line, const char* fmt, ...)
{
    if(s_initialized == false)
    {
        return;
    }

    if(fmt == NULL)
    {
        return;
    }

    if(g_currentBspLogLevel == BSP_LOG_NONE)
    {
        return;
    }

    const char* lvlStr;
    size_t prefixLen = 0U;
    int bodyLen      = 0;
    size_t totalLen  = 0U;
    va_list args;

    /* Map level to prefix string */
    switch(level)
    {
        case BSP_LOG_INFO:
            lvlStr = "I";
            break;
        case BSP_LOG_WARN:
            lvlStr = "W";
            break;
        case BSP_LOG_ERROR:
            lvlStr = "E";
            break;
        case BSP_LOG_DEBUG:
            lvlStr = "D";
            break;
        default:
            lvlStr = "?";
            break;
    }

    /* Acquire mutex - s_logLineBuf is static shared resource */
    if(osMutexAcquire(s_cliMutex, BSP_LOG_MUTEX_TIMEOUT_MS) != osOK)
    {
        /* Cannot acquire mutex - drop log message (best-effort) */
        return;
    }

    /* Build prefix: [LEVEL] (file:line) */
    if(file != NULL)
    {
        int ret = snprintf(s_logLineBuf, BSP_LOG_LINE_MAX_LEN, "[%s] (%s:%d) ",
                           lvlStr, file, line);
        prefixLen = (ret > 0) ? (size_t)ret : 0U;
    }
    else
    {
        int ret = snprintf(s_logLineBuf, BSP_LOG_LINE_MAX_LEN, "[%s] ", lvlStr);
        prefixLen = (ret > 0) ? (size_t)ret : 0U;
    }

    /* Clamp prefix if it somehow exceeds buffer */
    if(prefixLen >= (BSP_LOG_LINE_MAX_LEN - 3U))
    {
        prefixLen = BSP_LOG_LINE_MAX_LEN - 4U;
    }

    /* Format user message body */
    va_start(args, fmt);
    bodyLen =
    vsnprintf(&s_logLineBuf[prefixLen], BSP_LOG_LINE_MAX_LEN - prefixLen, fmt, args);
    va_end(args);

    if(bodyLen < 0)
    {
        /* Encoding error - release and return */
        (void)osMutexRelease(s_cliMutex);
        return;
    }

    /* Calculate total and clamp to leave room for \r\n\0 */
    totalLen = prefixLen + (size_t)bodyLen;
    if(totalLen >= (BSP_LOG_LINE_MAX_LEN - 2U))
    {
        totalLen = BSP_LOG_LINE_MAX_LEN - 3U;
    }

    /* Append \r\n terminator */
    s_logLineBuf[totalLen]      = '\r';
    s_logLineBuf[totalLen + 1U] = '\n';
    s_logLineBuf[totalLen + 2U] = '\0';
    totalLen += 2U;

    /* Transmit over UART */
    (void)bspUartWriteAsync(s_cliUart, (uint8_t*)s_logLineBuf, (uint16_t)totalLen);

    (void)osMutexRelease(s_cliMutex);
}
/* ------------------------------------------------------------------------------------ */

bsp_err_sts_t bsp_log_init(bspUartHandle_t* uartHandle)
{
    if(uartHandle == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* Prevent double initialization */
    if(s_initialized == true)
    {
        return BSP_ERR_STS_OK;
    }

    s_cliUart  = uartHandle;
    s_txBufLen = 0U;

    s_cliMutex = osMutexNew(&g_bspMutexCfg[BSP_MUTEX_OWNER_LOG].attr);
    if(s_cliMutex == NULL)
    {
        s_cliUart = NULL;
        return BSP_ERR_STS_NO_MEM;
    }

    /* Configure Embedded CLI with static buffer */
    EmbeddedCliConfig cliCfg = { .cliBuffer = s_cliInternalBuf,
                                 .cliBufferSize = (uint16_t)sizeof(s_cliInternalBuf),
                                 .rxBufferSize = (uint16_t)BSP_LOG_RX_BUF_SIZE,
                                 .cmdBufferSize = (uint16_t)BSP_LOG_CLI_BUF_SIZE,
                                 .maxBindingCount = (uint16_t)BSP_LOG_CLI_MAX_BINDINGS };

    s_cli = embeddedCliNew(&cliCfg);
    if(s_cli == NULL)
    {
        (void)osMutexDelete(s_cliMutex);
        s_cliMutex = NULL;
        s_cliUart  = NULL;
        return BSP_ERR_STS_FAIL;
    }

    /* Set CLI write callback */
    s_cli->writeChar = bspCliWriteChar;

    /* Register UART RX callback for CLI input */
    (void)bspUartSetCallback(uartHandle, bspCliUartRxCb, NULL);

    /* Mark as initialized before arming RX */
    s_initialized = true;

    /* Arm first async UART receive */
    (void)bspUartReadAsync(s_cliUart, s_rxBuf, (uint16_t)sizeof(s_rxBuf));

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------------------ */
bsp_err_sts_t bsp_log_deinit(void)
{
    /* Mark uninitialized first to stop RX callback from processing */
    s_initialized = false;

    if(s_cliMutex != NULL)
    {
        (void)osMutexAcquire(s_cliMutex, BSP_LOG_MUTEX_TIMEOUT_MS);

        /* Flush any pending TX data */
        bspLogFlushTx();

        /* Free CLI instance */
        if(s_cli != NULL)
        {
            embeddedCliFree(s_cli);
            s_cli = NULL;
        }

        (void)osMutexRelease(s_cliMutex);
        (void)osMutexDelete(s_cliMutex);
        s_cliMutex = NULL;
    }

    /* Clear UART callback */
    if(s_cliUart != NULL)
    {
        (void)bspUartSetCallback(s_cliUart, NULL, NULL);
        s_cliUart = NULL;
    }

    return BSP_ERR_STS_OK;
}
/* ------------------------------------------------------------------------------------ */