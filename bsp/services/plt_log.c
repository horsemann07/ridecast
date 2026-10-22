
#include "bsp_log.h"
#include "bsp_osal.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/**
 * @brief Maximum formatted log message length.
 *
 * Includes:
 * - Log prefix.
 * - User message.
 * - CR/LF.
 * - Null terminator.
 */

#define BSP_LOG_LINE_MAX_LEN        (bspCONFIG_LOG_LINE_MAX_LEN)


/**
 * @brief Mutex timeout used by the logger.
 */

#define BSP_LOG_MUTEX_TIMEOUT_MS    (bspCONFIG_LOG_MUTEX_TIMEOUT_MS)


/**
 * @brief Maximum message length generated from ISR context.
 */

#define BSP_LOG_ISR_MESSAGE_MAX_LEN (bspCONFIG_LOG_ISR_MESSAGE_MAX_LEN)


/**
 * @brief Maximum number of pending ISR log messages.
 */
#define BSP_LOG_ISR_QUEUE_LENGTH    (bspCONFIG_LOG_ISR_QUEUE_LENGTH)


/* Correct level order.
BSP_LOG_NONE  = 0,
BSP_LOG_ERROR = 1,
BSP_LOG_WARN  = 2,
BSP_LOG_INFO  = 3,
BSP_LOG_DEBUG = 4,
*/
#if ((BSP_COMPILED_LOG_LEVEL < 0) || (BSP_COMPILED_LOG_LEVEL > 4))
    #error "Invalid BSP_COMPILED_LOG_LEVEL"
#endif

#if (BSP_LOG_LINE_MAX_LEN < 16U)
    #error "BSP_LOG_LINE_MAX_LEN must be at least 16"
#endif

#if ((BSP_LOG_ISR_MESSAGE_MAX_LEN < 2U) || (BSP_LOG_ISR_QUEUE_LENGTH < 1U))
    #error "Invalid ISR logging configuration"
#endif


/*----------------------------------------------------------------------------
 * Private data types
 *---------------------------------------------------------------------------*/
typedef struct bspLogIsrRecord
{
    bspLogLevel_t level;
    const char* file;
    uint32_t line;
    const char* function;
    bspLogIsrValueType_t valueType;
    bspLogIsrValue_t value;
    char message[BSP_LOG_ISR_MESSAGE_MAX_LEN];
};


typedef struct
{
    uint32_t queued;
    uint32_t dropped;
    uint32_t written;
    uint32_t transportErrors;
} bspLogStats_t;

typedef struct
{
    volatile bool initialized;
    volatile bool enabled;
    volatile bspLogLevel_t level;

    bspMutexHandle_t mutex;
    bspQueueHandle_t isrQueue;
    const bspConsoleTransport_t* transport;
} bspLogContext_t;


/* One shared formatting buffer; always protected by mutex in task context. */
static char s_logBuffer[BSP_LOG_LINE_MAX_LEN];


/*----------------------------------------------------------------------------
 * Static Variables
 *---------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
 * Forward Declarations
 *---------------------------------------------------------------------------*/

/* ISR API must return bool. */
static bool bsp_log_isr(bspLogLevel_t level,
                        const char* file,
                        uint32_t line,
                        const char* function,
                        const char* message);


/* Call periodically from task/main loop. */
void bsp_log_process(void);


// clang-format off
static bspLogContext_t s_logContext =
{
    .initialized = false,
    .enabled = false,
    .level = (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL,
    .mutex = NULL,
    .isrQueue = NULL,
    .transport = NULL
};
// clang-format on

/* One shared formatting buffer; always protected by mutex in task context. */
static char s_logBuffer[BSP_LOG_LINE_MAX_LEN];

static bool bspLogLevelIsValid(bspLogLevel_t level)
{
    return ((level >= BSP_LOG_NONE) && (level <= BSP_LOG_DEBUG));
}

/*----------------------------------------------------------------------------
 * Private Functions
 *---------------------------------------------------------------------------*/

static bool bspLogShouldPrint(bspLogLevel_t level)
{
    return ((s_logContext.initialized == true) && (s_logContext.enabled == true) &&
            (level != BSP_LOG_NONE) && (level <= s_logContext.level));
}

static char bspLogLevelChar(bspLogLevel_t level)
{
    switch(level)
    {
        case BSP_LOG_ERROR:
            return 'E';
        case BSP_LOG_WARN:
            return 'W';
        case BSP_LOG_INFO:
            return 'I';
        case BSP_LOG_DEBUG:
            return 'D';
        default:
            return '?';
    }
}

static void bspLogCopyString(char* destination, size_t destinationSize, const char* source)
{
    size_t i = 0U;

    if((destination == NULL) || (destinationSize == 0U))
    {
        return;
    }

    if(source != NULL)
    {
        while((i < (destinationSize - 1U)) && (source[i] != '\0'))
        {
            destination[i] = source[i];
            i++;
        }
    }

    destination[i] = '\0';
}
/*
 * Must be called with s_logContext.mutex held.
 * The transport must consume/copy data before write() returns.
 */
static void bspLogWriteLine(bspLogLevel_t level,
                            const char* file,
                            uint32_t line,
                            const char* function,
                            const char* message)
{
    const bspConsoleTransport_t* transport;
    size_t length;
    int result;

    if((message == NULL) || (s_logContext.transport == NULL))
    {
        return;
    }

    transport = s_logContext.transport;

    if(transport->write == NULL)
    {
        return;
    }

    result = snprintf(s_logBuffer, sizeof(s_logBuffer), "[%c] (%s:%" PRIu32 ":%s) %s\r\n",
                      bspLogLevelChar(level), (file != NULL) ? file : "?", line,
                      (function != NULL) ? function : "?", message);

    if(result <= 0)
    {
        return;
    }

    length = (size_t)result;

    /* snprintf returns required length when output was truncated. */
    if(length >= sizeof(s_logBuffer))
    {
        length = sizeof(s_logBuffer) - 1U;
    }

    (void)transport->write(transport->context, (const uint8_t*)s_logBuffer, (uint16_t)length);
}

static void bspLogFormatIsrValue(const bspLogIsrRecord_t* record, char* value, size_t valueSize)
{
    if((record == NULL) || (value == NULL) || (valueSize == 0U))
    {
        return;
    }

    switch(record->valueType)
    {
        case BSP_LOG_ISR_VALUE_U8:
            (void)snprintf(value, valueSize, "%u", (unsigned int)record->value.u8);
            break;

        case BSP_LOG_ISR_VALUE_U16:
            (void)snprintf(value, valueSize, "%u", (unsigned int)record->value.u16);
            break;

        case BSP_LOG_ISR_VALUE_U32:
            (void)snprintf(value, valueSize, "%" PRIu32, record->value.u32);
            break;

        case BSP_LOG_ISR_VALUE_U64:
            (void)snprintf(value, valueSize, "%" PRIu64, record->value.u64);
            break;

        case BSP_LOG_ISR_VALUE_I8:
            (void)snprintf(value, valueSize, "%d", (int)record->value.i8);
            break;

        case BSP_LOG_ISR_VALUE_I16:
            (void)snprintf(value, valueSize, "%d", (int)record->value.i16);
            break;

        case BSP_LOG_ISR_VALUE_I32:
            (void)snprintf(value, valueSize, "%" PRId32, record->value.i32);
            break;

        case BSP_LOG_ISR_VALUE_I64:
            (void)snprintf(value, valueSize, "%" PRId64, record->value.i64);
            break;

        case BSP_LOG_ISR_VALUE_HEX8:
            (void)snprintf(value, valueSize, "0x%02X",
                           (unsigned int)record->value.u8);
            break;

        case BSP_LOG_ISR_VALUE_HEX16:
            (void)snprintf(value, valueSize, "0x%04X",
                           (unsigned int)record->value.u16);
            break;

        case BSP_LOG_ISR_VALUE_HEX32:
            (void)snprintf(value, valueSize, "0x%08" PRIX32, record->value.u32);
            break;

        case BSP_LOG_ISR_VALUE_HEX64:
            (void)snprintf(value, valueSize, "0x%016" PRIX64, record->value.u64);
            break;

        case BSP_LOG_ISR_VALUE_BOOL:
            (void)snprintf(value, valueSize, "%s", record->value.boolean ? "true" : "false");
            break;

        case BSP_LOG_ISR_VALUE_CHAR:
            (void)snprintf(value, valueSize, "%c", record->value.character);
            break;

        default:
            value[0] = '\0';
            break;
    }
}

/* Must be called from task context with logger mutex held. */
static void bspLogPrintIsrRecord(const bspLogIsrRecord_t* record)
{
    char message[BSP_LOG_ISR_MESSAGE_MAX_LEN + 28U];
    char value[24U];

    if((record == NULL) || (bspLogShouldPrint(record->level) == false))
    {
        return;
    }

    if(record->valueType == BSP_LOG_ISR_VALUE_NONE)
    {
        bspLogWriteLine(record->level, record->file, record->line,
                        record->function, record->message);
        return;
    }

    bspLogFormatIsrValue(record, value, sizeof(value));

    (void)snprintf(message, sizeof(message), "%s: %s", record->message, value);

    message[sizeof(message) - 1U] = '\0';

    bspLogWriteLine(record->level, record->file, record->line, record->function, message);
}


static void bspConsoleRxCallback(uint8_t* data, size_t length, bsp_err_sts_t status, void* userCtx)
{
    bspCliContext_t* ctx = (bspCliContext_t*)userCtx;

    if((ctx != NULL) && (status == PLT_ERR_STS_OK) && (data != NULL))
    {
        for(size_t i = 0U; i < length; i++)
        {
            (void)bspCliRingPush(ctx, data[i]);
        }

        (void)osThreadFlagsSet(g_consoleTaskHandle, BSP_CONSOLE_EVENT_RX);
    }

    /* Rearm next asynchronous receive here. */
}

bsp_err_sts_t bsp_log_init(const bspConsoleTransport_t* transport)
{
    if(s_logContext.initialized == true)
    {
        return PLT_ERR_STS_ALREADY_INIT;
    }

    if((transport == NULL) || (transport->write == NULL))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    s_logContext.mutex = bspMutexCreateForLog();

    if(s_logContext.mutex == NULL)
    {
        return PLT_ERR_STS_NO_MEM;
    }

    /*
     * Queue memory is managed by the OSAL/RTOS. ISR calls only enqueue;
     * they never lock, format text, allocate, or access the transport.
     */
    s_logContext.isrQueue = bspQueueCreateForLog(sizeof(bspLogIsrRecord_t));

    if(s_logContext.isrQueue == NULL)
    {
        bspMutexDelete(s_logContext.mutex);
        s_logContext.mutex = NULL;
        return PLT_ERR_STS_NO_MEM;
    }

    s_logContext.transport   = transport;
    s_logContext.level       = (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL;
    s_logContext.enabled     = (BSP_COMPILED_LOG_LEVEL != BSP_LOG_NONE);
    s_logContext.initialized = true;

    return PLT_ERR_STS_OK;
}

bsp_err_sts_t bsp_log_deinit(void)
{
    if(s_logContext.initialized == false)
    {
        return PLT_ERR_STS_OK;
    }

    s_logContext.initialized = false;
    s_logContext.enabled     = false;

    if(s_logContext.isrQueue != NULL)
    {
        (void)bspQueueDelete(s_logContext.isrQueue);
        s_logContext.isrQueue = NULL;
    }

    if(s_logContext.mutex != NULL)
    {
        bspMutexDelete(s_logContext.mutex);
        s_logContext.mutex = NULL;
    }

    s_logContext.transport = NULL;
    s_logContext.level     = (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL;

    return PLT_ERR_STS_OK;
}

bsp_err_sts_t bsp_log_enable(bool enable)
{
    if(s_logContext.initialized == false)
    {
        return PLT_ERR_STS_NOT_INIT;
    }

    s_logContext.enabled = enable;
    return PLT_ERR_STS_OK;
}

bool bsp_log_is_enabled(void)
{
    return ((s_logContext.initialized == true) && (s_logContext.enabled == true));
}

bsp_err_sts_t bsp_log_set_level(bspLogLevel_t level)
{
    if(bspLogLevelIsValid(level) == false)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(s_logContext.initialized == false)
    {
        return PLT_ERR_STS_NOT_INIT;
    }

    if(level > (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL)
    {
        level = (bspLogLevel_t)BSP_COMPILED_LOG_LEVEL;
    }

    s_logContext.level = level;
    return PLT_ERR_STS_OK;
}

bspLogLevel_t bsp_log_get_level(void)
{
    return s_logContext.level;
}

void bsp_log(bspLogLevel_t level, const char* file, uint32_t line, const char* function, const char* fmt, ...)
{
    size_t prefixLength;
    size_t messageLength;
    int result;
    va_list arguments;

    if((fmt == NULL) || (bspLogLevelIsValid(level) == false) ||
       (bspLogShouldPrint(level) == false) || (s_logContext.mutex == NULL))
    {
        return;
    }

    if(bspMutexLock(s_logContext.mutex, BSP_LOG_MUTEX_TIMEOUT_MS) != osOK)
    {
        return;
    }

    if(bspLogShouldPrint(level) == false)
    {
        (void)bspMutexUnlock(s_logContext.mutex);
        return;
    }

    result = snprintf(s_logBuffer, sizeof(s_logBuffer), "[%c] (%s:%" PRIu32 ":%s) ",
                      bspLogLevelChar(level), (file != NULL) ? file : "?", line,
                      (function != NULL) ? function : "?");

    if(result < 0)
    {
        (void)bspMutexUnlock(s_logContext.mutex);
        return;
    }

    prefixLength = (size_t)result;

    if(prefixLength > (sizeof(s_logBuffer) - 3U))
    {
        prefixLength = sizeof(s_logBuffer) - 3U;
    }

    va_start(arguments, fmt);

    result = vsnprintf(&s_logBuffer[prefixLength],
                       sizeof(s_logBuffer) - prefixLength, fmt, arguments);

    va_end(arguments);

    if(result < 0)
    {
        (void)bspMutexUnlock(s_logContext.mutex);
        return;
    }

    messageLength = (size_t)result;

    if(messageLength > ((sizeof(s_logBuffer) - 3U) - prefixLength))
    {
        messageLength = (sizeof(s_logBuffer) - 3U) - prefixLength;
    }

    messageLength += prefixLength;
    s_logBuffer[messageLength++] = '\r';
    s_logBuffer[messageLength++] = '\n';
    s_logBuffer[messageLength]   = '\0';

    if((s_logContext.transport != NULL) && (s_logContext.transport->write != NULL))
    {
        (void)s_logContext.transport->write(s_logContext.transport->context,
                                            (const uint8_t*)s_logBuffer,
                                            (uint16_t)messageLength);
    }

    (void)bspMutexUnlock(s_logContext.mutex);
}

bool bsp_log_isr(bspLogLevel_t level, const char* file, uint32_t line, const char* function, const char* message)
{
    bspLogIsrRecord_t record;

    if((message == NULL) || (s_logContext.isrQueue == NULL) ||
       (bspLogLevelIsValid(level) == false) || (bspLogShouldPrint(level) == false))
    {
        return false;
    }

    record.level     = level;
    record.file      = file;
    record.line      = line;
    record.function  = function;
    record.valueType = BSP_LOG_ISR_VALUE_NONE;

    bspLogCopyString(record.message, sizeof(record.message), message);

    return (bspQueueSendFromISR(s_logContext.isrQueue, &record) == osOK);
}

bool bsp_log_isr_value(bspLogLevel_t level,
                       const char* file,
                       uint32_t line,
                       const char* function,
                       const char* message,
                       bspLogIsrValueType_t valueType,
                       bspLogIsrValue_t value)
{
    bspLogIsrRecord_t record;

    if((message == NULL) || (valueType == BSP_LOG_ISR_VALUE_NONE) ||
       (s_logContext.isrQueue == NULL) || (bspLogLevelIsValid(level) == false) ||
       (bspLogShouldPrint(level) == false))
    {
        return false;
    }

    record.level     = level;
    record.file      = file;
    record.line      = line;
    record.function  = function;
    record.valueType = valueType;
    record.value     = value;

    bspLogCopyString(record.message, sizeof(record.message), message);

    return (bspQueueSendFromISR(s_logContext.isrQueue, &record) == osOK);
}

/**
 * @brief Flush all queued ISR log records.
 *
 * Call this periodically from one non-ISR task or from the main loop:
 *
 * @code
 * for(;;)
 * {
 *     void bsp_log_process_isr(void)
 * };
 * @endcode
 */
void bsp_log_process_isr(void)
{
    bspLogIsrRecord_t record;

    if((s_logContext.initialized == false) || (s_logContext.isrQueue == NULL) ||
       (s_logContext.mutex == NULL))
    {
        return;
    }

    if(bspMutexLock(s_logContext.mutex, BSP_LOG_MUTEX_TIMEOUT_MS) != osOK)
    {
        return;
    }

    while(bspQueueReceive(s_logContext.isrQueue, &record, 0U) == osOK)
    {
        bspLogPrintIsrRecord(&record);
    }

    (void)bspMutexUnlock(s_logContext.mutex);
}