


#include "bsp_uart.h"
#include "cmsis_os2.h"
#include "embedded_cli.h"
#include "bsp_err.h"
#include "cli_logger.h"

static bspUartHandle_t cliUartHandle = NULL;
static osMutexId_t cliLogMutex       = NULL;

static uint8_t cliBuffer[CLI_BUFFER_SIZE];

EmbeddedCli* cliLog = NULL;


static void cliLogUartWriteChar(EmbeddedCli* cli, uint8_t c)
{
    bspUartHandle_t handle = (bspUartHandle_t)cli->appContext;
    if(handle != NULL)
    {
        // Async, non-blocking send (implementation depends on your BSP)
        bspUartAsyncWrite(handle, &c, 1);
    }
}

static void cliLogUartReadChar(uint8_t c)
{
    if(cliLog != NULL)
    {
        embeddedCliReceiveChar(cliLog, c);
    }
}
/**
 * @brief Initialize both Logger and CLI on the same UART.
 *
 * @param handle UART handle to be shared.
 * @return errStatus_t
 */
errStatus_t cliLoggerInit(bspUartHandle_t handle)
{
    if(handle == NULL)
    {
        return ERR_STS_INV_PARAM;
    }

    cliUartHandle = handle;

    // Create mutex if not created
    if(cliLogMutex == NULL)
    {
        cliLogMutex = osMutexNew(NULL);
        if(cliLogMutex == NULL)
        {
            return ERR_STS_FAIL;
        }
    }

    // Create CLI config
    EmbeddedCliConfig* config = embeddedCliDefaultConfig();
    config->cliBuffer         = cliBuffer;
    config->cliBufferSize     = CLI_BUFFER_SIZE;
    config->rxBufferSize      = CLI_RX_BUFFER_SIZE;
    config->cmdBufferSize     = CLI_TX_BUFFER_SIZE;

    // Create CLI instance
    EmbeddedCli* cliLog = embeddedCliNew(config);
    if(cliLog == NULL)
    {
        osMutexDelete(cliLogMutex);
        cliLogMutex   = NULL;
        cliUartHandle = NULL;
        return ERR_STS_FAIL;
    }

    // Bind write function + context
    cliLog->writeChar  = cliLogWriteChar;      //  UART send wrapper
    cliLog->appContext = (void*)cliUartHandle; //  UART handle

    bspUartRegisterRxCallback(cliUartHandle, cliLogUartReadChar);
    return ERR_STS_OK;
}

/**
 * @brief Deinitialize both Logger and CLI.
 *
 * @param handle UART handle to deinitialize.
 * @return errStatus_t
 */
errStatus_t cliLoggerDeInit(void)
{
    // If CLI instance was created, free it
    if(cliLog != NULL)
    {
        embeddedCliFree(cliLog); // provided by Embedded CLI lib
        cliLog = NULL;
    }

    // If mutex exists, delete it
    if(cliLogMutex != NULL)
    {
        osMutexDelete(cliLogMutex);
        cliLogMutex = NULL;
    }

    // Reset UART handle
    cliUartHandle = NULL;

    return ERR_STS_OK;
}


void cliLoggerLog(LoggerLevel_t level, const uint8_t* file, int line, const char* fmt, ...);
{
    if(log_queue == NULL)
    {
        return; // Logger not initialized
    }
    // Buffer for log messages (not currently used in Logger_Task)
    uint8_t logBuffer[LOGGER_BUFFER_SIZE] = { 0 };

    // Length of data in logBuffer (not currently used in Logger_Task)
    volatile size_t logBufferLen = 0;

#ifdef LOGGER_LEVEL_STRING
    const char* levelStr = "INFO";
    switch(level)
    {
    case LOGGER_LEVEL_INFO: levelStr = "INFO"; break;
    case LOGGER_LEVEL_WARN: levelStr = "WARN"; break;
    case LOGGER_LEVEL_ERROR: levelStr = "ERROR"; break;
    case LOGGER_LEVEL_DEBUG: levelStr = "DEBUG"; break;
    default: break;
    }
#if LOGGER_INCLUDE_FILELINE
    logBufferLen =
    snprintf(logBuffer, LOGGER_BUFFER_SIZE, "[%s] (%s:%d) ", levelStr, file, line);
#else
    logBufferLen = snprintf(logBuffer, LOGGER_BUFFER_SIZE, "[%s] ", levelStr);
#endif // LOGGER_INCLUDE_FILELINE
#else
    char levelC = 'I';
    switch(level)
    {
    case LOGGER_LEVEL_INFO: levelC = 'I'; break;
    case LOGGER_LEVEL_WARN: levelC = 'W'; break;
    case LOGGER_LEVEL_ERROR: levelC = 'E'; break;
    case LOGGER_LEVEL_DEBUG: levelC = 'D'; break;
    default: break;
    }
#if LOGGER_INCLUDE_FILELINE
    logBufferLen =
    snprintf(logBuffer, LOGGER_BUFFER_SIZE, "[%c] (%s:%d) ", levelC, file, line);
#else
    logBufferLen = snprintf(logBuffer, LOGGER_BUFFER_SIZE, "[%c] ", levelC);
#endif // LOGGER_INCLUDE_FILELINE
#endif // LOGGER_LEVEL_STRING

    // Append user message
    va_list args;
    va_start(args, fmt);
    logBufferLen +=
    vsnprintf(logBuffer + logBufferLen, LOGGER_BUFFER_SIZE - logBufferLen, fmt, args);
    va_end(args);

    // Protect UART & CLI with mutex
    osMutexAcquire(cliLogMutex, osWaitForever);

    // Send message via CLI
    embeddedCliPrint(cliLog, msg.buffer);
    osMutexRelease(cliLogMutex);
    return ERR_STS_OK;
}
