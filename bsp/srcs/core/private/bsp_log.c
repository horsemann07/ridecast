/**
 * @file bsp_log.c
 * @brief BSP logging implementation.
 *
 * This module implements the BSP logging subsystem.
 *
 * The logger is intentionally independent of the BSP CLI and EmbeddedCLI.
 * It provides:
 *
 * - Runtime log enable/disable.
 * - Runtime log-level filtering.
 * - Formatted log messages.
 * - Source file and line information.
 * - Thread-safe output serialization.
 * - Static log formatting storage.
 *
 * The actual output transport is abstracted through
 * bsp_log_transport_write().
 *
 * @par Architecture
 *
 * @code
 * Application
 *      |
 *      v
 *  BSP_LOGI/E/W/D
 *      |
 *      v
 *   bsp_log()
 *      |
 *      +---- level filtering
 *      +---- formatting
 *      +---- mutex
 *      |
 *      v
 * bsp_log_transport_write()
 *      |
 *      +---- UART
 *      +---- USB CDC
 *      +---- RTT
 *      +---- BLE
 *      +---- Telnet
 * @endcode
 *
 * @note EmbeddedCLI is not used by this module.
 *
 * @see bsp_log.h
 */

#include "bsp_log.h"

#include "bsp_config.h"
#include "bsp_osal.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*=============================================================================
 * COMPILE-TIME VALIDATION
 *============================================================================*/

#if ((BSP_COMPILED_LOG_LEVEL < 0) || \
     (BSP_COMPILED_LOG_LEVEL > 4))
#error "Invalid BSP_COMPILED_LOG_LEVEL"
#endif


/*=============================================================================
 * PRIVATE TYPES
 *============================================================================*/

 
 typedef struct
{
    uint32_t queued;
    uint32_t dropped;
    uint32_t written;
    uint32_t transportErrors;
} bspLogStats_t;


/**
 * @brief Internal logger state.
 */
typedef struct
{
    bool initialized;
    bool enabled;
    bspLogLevel_t level;

    /**
     * @brief Logger synchronization object.
     */
    bspMutexHandle_t mutex;
    bspQueueHandle_t isrQueue;
    bspLogStats_t stats;
    /**
     * @brief Console transport used by the logger.
     *
     * The transport is owned by the BSP/application.
     * The logger only stores the reference.
     */
    const bspConsoleTransport_t* transport;

} bspLogContext_t;


/**
 * @brief ISR logger queue record.
 */
typedef struct
{
    bspLogLevel_t level;
    const char* file;
    uint32_t line;
    const char* function;
    char message[BSP_LOG_ISR_MESSAGE_MAX_LEN];
    bspLogIsrValueType_t valueType;
    bspLogIsrValue_t value;
} bspLogIsrRecord_t;

/*=============================================================================
 * STATIC DATA
 *============================================================================*/
/**
 * @brief BSP logger context.
 */
static bspLogContext_t s_logContext =
{
    .initialized = false,
    .enabled     = false,
    .level       = (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL,
    .mutex       = NULL,
    .isrQueue    = NULL,
    .transport   = NULL
};

/**
 * @brief Static log formatting buffer.
 *
 * Access is protected by s_logContext.mutex.
 */
static char s_logLineBuf[BSP_LOG_LINE_MAX_LEN];

/*=============================================================================
 * PRIVATE FUNCTIONS
 *============================================================================*/
/**
 * @brief Validate a console transport interface.
 *
 * The logger requires only the write() operation.
 *
 * @param[in] transport
 *        Console transport interface.
 *
 * @return true if the transport is valid.
 * @return false otherwise.
 */
static bool bspLogIsValidTransport(
    const bspConsoleTransport_t* transport)
{
    if(transport == NULL)
    {
        return false;
    }

    if(transport->write == NULL)
    {
        return false;
    }

    return true;
}

/**
 * @brief Convert log level to printable character.
 *
 * @param[in] level BSP log level.
 *
 * @return Character representing the log level.
 */
static char bspLogLevelToChar(
    bspLogLevel_t level)
{
    char levelChar;

    switch(level)
    {
        case BSP_LOG_ERROR:
        {
            levelChar = 'E';
            break;
        }

        case BSP_LOG_WARN:
        {
            levelChar = 'W';
            break;
        }

        case BSP_LOG_INFO:
        {
            levelChar = 'I';
            break;
        }

        case BSP_LOG_DEBUG:
        {
            levelChar = 'D';
            break;
        }

        case BSP_LOG_NONE:
        default:
        {
            levelChar = '?';
            break;
        }
    }

    return levelChar;
}


/**
 * @brief Check whether a log level is valid.
 *
 * @param[in] level BSP log level.
 *
 * @return true if valid, otherwise false.
 */
static bool bspLogIsValidLevel(
    bspLogLevel_t level)
{
    return ((level >= BSP_LOG_NONE) &&
            (level <= BSP_LOG_DEBUG));
}


/**
 * @brief Check whether a message should be emitted.
 *
 * @param[in] level Message level.
 *
 * @return true if the message should be emitted.
 */
static bool bspLogShouldOutput(
    bspLogLevel_t level)
{
    if(s_logContext.initialized == false)
    {
        return false;
    }

    if(s_logContext.enabled == false)
    {
        return false;
    }

    if(level == BSP_LOG_NONE)
    {
        return false;
    }

    if(level > s_logContext.level)
    {
        return false;
    }

    return true;
}

static void bspLogCopyString(
    char* destination,
    size_t destinationSize,
    const char* source)
{
    size_t index;

    if((destination == NULL) ||
       (source == NULL) ||
       (destinationSize == 0U))
    {
        return;
    }

    index = 0U;

    while((index < (destinationSize - 1U)) &&
          (source[index] != '\0'))
    {
        destination[index] = source[index];
        index++;
    }

    destination[index] = '\0';
}



static void bspLogProcessIsrRecord(
    const bspLogIsrRecord_t* record)
{
    int ret;

    if(record == NULL)
    {
        return;
    }

    switch(record->valueType)
    {
        case BSP_LOG_ISR_VALUE_NONE:
        {
            ret = snprintf(
                s_logLineBuf,
                sizeof(s_logLineBuf),
                "[%c] (%s:%lu:%s) %s\r\n",
                bspLogLevelToChar(record->level),
                (record->file != NULL) ? record->file : "?",
                (unsigned long)record->line,
                (record->function != NULL) ?
                    record->function : "?",
                record->message);

            break;
        }

        case BSP_LOG_ISR_VALUE_U8:
        {
            ret = snprintf(
                s_logLineBuf,
                sizeof(s_logLineBuf),
                "[%c] (%s:%lu:%s) %s: %u\r\n",
                bspLogLevelToChar(record->level),
                (record->file != NULL) ? record->file : "?",
                (unsigned long)record->line,
                (record->function != NULL) ?
                    record->function : "?",
                record->message,
                (unsigned int)record->value.u8);

            break;
        }

        case BSP_LOG_ISR_VALUE_U16:
        {
            ret = snprintf(
                s_logLineBuf,
                sizeof(s_logLineBuf),
                "[%c] (%s:%lu:%s) %s: %u\r\n",
                bspLogLevelToChar(record->level),
                (record->file != NULL) ? record->file : "?",
                (unsigned long)record->line,
                (record->function != NULL) ?
                    record->function : "?",
                record->message,
                (unsigned int)record->value.u16);

            break;
        }

        case BSP_LOG_ISR_VALUE_U32:
        {
            ret = snprintf(
                s_logLineBuf,
                sizeof(s_logLineBuf),
                "[%c] (%s:%lu:%s) %s: %lu\r\n",
                bspLogLevelToChar(record->level),
                (record->file != NULL) ? record->file : "?",
                (unsigned long)record->line,
                (record->function != NULL) ?
                    record->function : "?",
                record->message,
                (unsigned long)record->value.u32);

            break;
        }

        case BSP_LOG_ISR_VALUE_HEX32:
        {
            ret = snprintf(
                s_logLineBuf,
                sizeof(s_logLineBuf),
                "[%c] (%s:%lu:%s) %s: 0x%08lX\r\n",
                bspLogLevelToChar(record->level),
                (record->file != NULL) ? record->file : "?",
                (unsigned long)record->line,
                (record->function != NULL) ?
                    record->function : "?",
                record->message,
                (unsigned long)record->value.u32);

            break;
        }

        case BSP_LOG_ISR_VALUE_I32:
        {
            ret = snprintf(
                s_logLineBuf,
                sizeof(s_logLineBuf),
                "[%c] (%s:%lu:%s) %s: %ld\r\n",
                bspLogLevelToChar(record->level),
                (record->file != NULL) ? record->file : "?",
                (unsigned long)record->line,
                (record->function != NULL) ?
                    record->function : "?",
                record->message,
                (long)record->value.i32);

            break;
        }

        case BSP_LOG_ISR_VALUE_BOOL:
        {
            ret = snprintf(
                s_logLineBuf,
                sizeof(s_logLineBuf),
                "[%c] (%s:%lu:%s) %s: %s\r\n",
                bspLogLevelToChar(record->level),
                (record->file != NULL) ? record->file : "?",
                (unsigned long)record->line,
                (record->function != NULL) ?
                    record->function : "?",
                record->message,
                (record->value.boolean == true) ?
                    "true" : "false");

            break;
        }

        case BSP_LOG_ISR_VALUE_CHAR:
        {
            ret = snprintf(
                s_logLineBuf,
                sizeof(s_logLineBuf),
                "[%c] (%s:%lu:%s) %s: %c\r\n",
                bspLogLevelToChar(record->level),
                (record->file != NULL) ? record->file : "?",
                (unsigned long)record->line,
                (record->function != NULL) ?
                    record->function : "?",
                record->message,
                record->value.character);

            break;
        }

        default:
        {
            return;
        }
    }

    if(ret <= 0)
    {
        return;
    }

    if(s_logContext.transport != NULL)
    {
        if(s_logContext.transport->write != NULL)
        {
            (void)s_logContext.transport->write(
                s_logContext.transport->context,
                (const uint8_t*)s_logLineBuf,
                (uint16_t)ret);

            s_logContext.stats.written++;
        }
    }
}

/*=============================================================================
 * PUBLIC API
 *============================================================================*/

/**
 * @brief Initialize the BSP logger.
 *
 * @param[in] transport
 *        Console transport interface used by the logger.
 *
 * @return BSP_ERR_STS_OK
 *         Logger initialized successfully.
 *
 * @return BSP_ERR_STS_ALREADY_INIT
 *         Logger was already initialized.
 *
 * @return BSP_ERR_STS_INVALID_PARAM
 *         Invalid transport interface.
 *
 * @return BSP_ERR_STS_NO_MEM
 *         Logger synchronization resource could not be created.
 */
bsp_err_sts_t bsp_log_init(
    const bspConsoleTransport_t* transport)
{
    if(s_logContext.initialized == true)
    {
        return BSP_ERR_STS_ALREADY_INIT;
    }

    /*
     * The logger requires a valid write() operation.
     *
     * read_start() and set_callback() are not required by the logger.
     */
    if(bspLogIsValidTransport(transport) == false)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /*
     * Create logger synchronization object.
     */
    s_logContext.mutex = bspMutexCreate();

    if(s_logContext.mutex == NULL)
    {
        return BSP_ERR_STS_NO_MEM;
    }

    /*
     * Store the transport reference.
     *
     * The transport object is owned by the caller.
     * The logger does not allocate, modify, or free it.
     */
    s_logContext.transport = transport;

    /*
     * Configure logger state.
     */
    s_logContext.level =
        (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL;

    s_logContext.enabled =
        (BSP_COMPILED_LOG_LEVEL != BSP_LOG_NONE);

    /*
     * Logger is now ready.
     */
    s_logContext.initialized = true;

    return BSP_ERR_STS_OK;
}


/*----------------------------------------------------------------------------*/

/**
 * @brief Deinitialize the BSP logger.
 *
 * The console transport is owned by the BSP/application and is therefore
 * not deinitialized or freed by this function.
 *
 * @return BSP_ERR_STS_OK
 *         Logger deinitialized successfully.
 *
 * @return BSP_ERR_STS_NOT_INIT
 *         Logger was not initialized.
 */
bsp_err_sts_t bsp_log_deinit(void)
{
    if(s_logContext.initialized == false)
    {
        return BSP_ERR_STS_OK;
    }

    /*
     * Prevent new logging operations from entering the logger.
     */
    s_logContext.initialized = false;
    s_logContext.enabled     = false;

    /*
     * Release logger synchronization object.
     */
    if(s_logContext.mutex != NULL)
    {
        (void)bspMutexDelete(s_logContext.mutex);

        s_logContext.mutex = NULL;
    }

    /*
     * Release the reference to the transport.
     *
     * The logger does NOT destroy or deinitialize the transport.
     */
    s_logContext.transport = NULL;

    /*
     * Restore default runtime state.
     */
    s_logContext.level =
        (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL;

    /*
     * Clear formatting buffer.
     */
    (void)memset(
        s_logLineBuf,
        0,
        sizeof(s_logLineBuf));

    return BSP_ERR_STS_OK;
}


/*----------------------------------------------------------------------------*/

bsp_err_sts_t bsp_log_enable(
    bool enable)
{
    if(s_logContext.initialized == false)
    {
        return BSP_ERR_STS_NOT_INIT;
    }

    s_logContext.enabled = enable;

    return BSP_ERR_STS_OK;
}


/*----------------------------------------------------------------------------*/

bool bsp_log_is_enabled(void)
{
    if(s_logContext.initialized == false)
    {
        return false;
    }

    return s_logContext.enabled;
}


/*----------------------------------------------------------------------------*/

bsp_err_sts_t bsp_log_set_level(
    bspLogLevel_t level)
{
    if(bspLogIsValidLevel(level) == false)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(s_logContext.initialized == false)
    {
        return BSP_ERR_STS_NOT_INIT;
    }

    /*
     * Do not allow runtime configuration to exceed the compile-time
     * logging capability.
     */
    if(level > (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL)
    {
        s_logContext.level =
            (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL;
    }
    else
    {
        s_logContext.level = level;
    }

    /*
     * BSP_LOG_NONE is equivalent to disabling logging.
     *
     * However, keep the enabled state independent so that:
     *
     * bsp_log_enable(false);
     *
     * and:
     *
     * bsp_log_set_level(BSP_LOG_NONE);
     *
     * remain conceptually different operations.
     */
    return BSP_ERR_STS_OK;
}


/*----------------------------------------------------------------------------*/

bspLogLevel_t bsp_log_get_level(void)
{
    return s_logContext.level;
}


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

void bsp_log(
    bspLogLevel_t level,
    const char* file,
    uint32_t line,
    const char* function,
    const char* fmt,
    ...)
{
    char levelChar;
    size_t prefixLen;
    size_t totalLen;
    int ret;
    va_list args;
    const bspConsoleTransport_t* transport;

    /*
     * Validate message level first.
     */
    if(bspLogIsValidLevel(level) == false)
    {
        return;
    }

    /*
     * Fast path filtering before taking the mutex.
     */
    if(bspLogShouldOutput(level) == false)
    {
        return;
    }

    if(fmt == NULL)
    {
        return;
    }

    /*
     * Serialize access to the static formatting buffer and
     * console transport.
     */
    if(bspMutexLock(
           s_logContext.mutex,
           BSP_LOG_MUTEX_TIMEOUT_MS) != osOK)
    {
        /*
         * Logging is best-effort.
         *
         * Never block indefinitely waiting for log output.
         */
        return;
    }

    /*
     * Re-check logger state after acquiring the mutex.
     *
     * This protects against logger state changes while waiting
     * for the mutex.
     */
    if(bspLogShouldOutput(level) == false)
    {
        (void)bspMutexUnlock(s_logContext.mutex);

        return;
    }

    /*
     * Get the configured console transport.
     */
    transport = s_logContext.transport;

    if((transport == NULL) ||
       (transport->write == NULL))
    {
        (void)bspMutexUnlock(s_logContext.mutex);

        return;
    }

    levelChar = bspLogLevelToChar(level);

    /*
     * Build log prefix.
     *
     * Example:
     *
     * [I] (main.c:125:system_init) System started
     */
    if(file != NULL)
    {
        ret = snprintf(
            s_logLineBuf,
            sizeof(s_logLineBuf),
            "[%c] (%s:%lu:%s) ",
            levelChar,
            file,
            (unsigned long)line,
            (function != NULL) ? function : "?");
    }
    else
    {
        ret = snprintf(
            s_logLineBuf,
            sizeof(s_logLineBuf),
            "[%c] ",
            levelChar);
    }

    if(ret < 0)
    {
        (void)bspMutexUnlock(s_logContext.mutex);

        return;
    }

    prefixLen = (size_t)ret;

    /*
     * Protect against a prefix consuming the complete buffer.
     */
    if(prefixLen >= (sizeof(s_logLineBuf) - 3U))
    {
        prefixLen = sizeof(s_logLineBuf) - 4U;

        s_logLineBuf[prefixLen] = '\0';
    }

    /*
     * Append formatted user message.
     */
    va_start(args, fmt);

    ret = vsnprintf(
        &s_logLineBuf[prefixLen],
        sizeof(s_logLineBuf) - prefixLen,
        fmt,
        args);

    va_end(args);

    if(ret < 0)
    {
        (void)bspMutexUnlock(s_logContext.mutex);

        return;
    }

    /*
     * Calculate final length.
     *
     * Reserve:
     *
     * +2 for CR/LF
     * +1 for '\0'
     */
    totalLen = prefixLen + (size_t)ret;

    if(totalLen >= (sizeof(s_logLineBuf) - 3U))
    {
        totalLen = sizeof(s_logLineBuf) - 3U;
    }

    /*
     * Append CR/LF.
     */
    s_logLineBuf[totalLen]      = '\r';
    s_logLineBuf[totalLen + 1U] = '\n';
    s_logLineBuf[totalLen + 2U] = '\0';

    totalLen += 2U;

    /*
     * Send the formatted message through the configured
     * BSP console transport.
     *
     * The transport implementation is responsible for
     * UART / USB CDC / RTT / BLE / Telnet, etc.
     */
    (void)transport->write(
        transport->context,
        (const uint8_t*)s_logLineBuf,
        (uint16_t)totalLen);

    /*
     * Release logger lock.
     */
    (void)bspMutexUnlock(s_logContext.mutex);
}

void bsp_log_isr(
    bspLogLevel_t level,
    const char* file,
    uint32_t line,
    const char* function,
    const char* message)
{
    bspLogIsrRecord_t record;

    if(bspLogIsValidLevel(level) == false)
    {
        return;
    }

    if(level == BSP_LOG_NONE)
    {
        return;
    }

    if(s_logContext.initialized == false)
    {
        return;
    }

    if(s_logContext.enabled == false)
    {
        return;
    }

    if(level > s_logContext.level)
    {
        return;
    }

    if(message == NULL)
    {
        return;
    }

    record.level    = level;
    record.file     = file;
    record.line     = line;
    record.function = function;

    bspLogCopyString(
        record.message,
        sizeof(record.message),
        message);

    record.valueType = BSP_LOG_ISR_VALUE_NONE;

    if(bspQueueSendFromISR(
           s_logContext.isrQueue,
           &record) != BSP_ERR_STS_OK)
    {
        s_logContext.stats.dropped++;

        return;
    }

    s_logContext.stats.queued++;

    return;
}


bool bsp_log_isr_value(
    bspLogLevel_t level,
    const char* file,
    uint32_t line,
    const char* function,
    const char* message,
    bspLogIsrValueType_t valueType,
    bspLogIsrValue_t value)
{
    bspLogIsrRecord_t record;

    if(bspLogIsValidLevel(level) == false)
    {
        return false;
    }

    if(level == BSP_LOG_NONE)
    {
        return false;
    }

    if(s_logContext.initialized == false)
    {
        return false;
    }

    if(s_logContext.enabled == false)
    {
        return false;
    }

    if(level > s_logContext.level)
    {
        return false;
    }

    if(message == NULL)
    {
        return false;
    }

    if(valueType == BSP_LOG_ISR_VALUE_NONE)
    {
        return false;
    }

    record.level    = level;
    record.file     = file;
    record.line     = line;
    record.function = function;

    bspLogCopyString(
        record.message,
        sizeof(record.message),
        message);

    record.valueType = valueType;
    record.value     = value;

    if(bspQueueSendFromISR(
           s_logContext.isrQueue,
           &record) != osOK)
    {
        s_logContext.stats.dropped++;

        return false;
    }

    s_logContext.stats.queued++;

    return true;
}
