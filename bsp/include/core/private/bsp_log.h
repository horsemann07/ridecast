/**
 * @file bsp_log.h
 * @brief BSP logging public interface.
 *
 * This header provides the public BSP logging API used by application,
 * middleware, and BSP components to generate diagnostic messages.
 *
 * The logging interface is intentionally independent of the BSP CLI.
 * Applications must not include or depend on EmbeddedCLI through this
 * interface.
 *
 * The BSP logger provides:
 * - Log severity levels.
 * - Runtime enable/disable control.
 * - Runtime log-level filtering.
 * - Formatted logging with source file and line information.
 * - Thread-safe logging.
 * - Transport-independent output through the BSP logging implementation.
 *
 * @par Architecture
 * @code
 * Application / Middleware
 *          |
 *          v
 *      bsp_log.h
 *          |
 *          v
 *      bsp_log.c
 *          |
 *          v
 *     Log Transport
 *          |
 *          +---- UART
 *          +---- USB CDC
 *          +---- RTT
 *          +---- BLE
 *          +---- Telnet
 * @endcode
 *
 * @par Usage
 * @code
 * bsp_log_init();
 *
 * BSP_LOGI("System started");
 * BSP_LOGW("Low battery: %u%%", batteryLevel);
 * BSP_LOGE("Initialization failed: %d", errorCode);
 * BSP_LOGD("State = %u", state);
 *
 * bsp_log_set_level(BSP_LOG_DEBUG);
 *
 * bsp_log_deinit();
 * @endcode
 *
 * @par Thread Safety
 * All public logging APIs are thread-safe.
 *
 * @par ISR Safety
 * Logging APIs must not be called from ISR context unless the selected
 * BSP logging backend explicitly documents ISR support.
 *
 * @par Notes
 * The logger does not expose any EmbeddedCLI types or APIs.
 *
 * @see bsp_log.c
 * @see bsp_cli.h

                     APPLICATION
                         |
             +-----------+-----------+
             |                       |
             v                       v
        bsp_log.h                bsp_cli.h
             |                       |
             v                       v
        bsp_log.c                bsp_cli.c
             |                       |
             v                       v
     Log Transport             CLI Transport
             |                       |
       UART/USB/RTT/...       UART/USB/RTT/...
 */

#ifndef BSP_LOG_H
#define BSP_LOG_H

/*-----------------------------------------------------------------------------
 * SYSTEM LEVEL INCLUDES
 *----------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>


/*-----------------------------------------------------------------------------
 * PROJECT LEVEL INCLUDES
 *----------------------------------------------------------------------------*/
#include "bsp_config.h"
#include "bsp_err_sts.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*=============================================================================
 * CONFIGURATION
 *============================================================================*/

/**
 * @brief Maximum formatted log message length.
 *
 * Includes:
 * - Log prefix.
 * - User message.
 * - CR/LF.
 * - Null terminator.
 */
#ifndef BSP_LOG_LINE_MAX_LEN
#define BSP_LOG_LINE_MAX_LEN    (256U)
#endif


/**
 * @brief Mutex timeout used by the logger.
 */
#ifndef BSP_LOG_MUTEX_TIMEOUT_MS
#define BSP_LOG_MUTEX_TIMEOUT_MS    (100U)
#endif


/**
 * @brief Maximum message length generated from ISR context.
 */
#ifndef BSP_LOG_ISR_MESSAGE_MAX_LEN
#define BSP_LOG_ISR_MESSAGE_MAX_LEN    (96U)
#endif


/**
 * @brief Maximum number of pending ISR log messages.
 */
#ifndef BSP_LOG_ISR_QUEUE_LENGTH
#define BSP_LOG_ISR_QUEUE_LENGTH       (32U)
#endif



/*=============================================================================
 * BSP LOGGER LEVEL
 *============================================================================*/

/**
 * @brief BSP logger severity levels.
 *
 * The configured runtime level determines which messages are emitted.
 *
 * A message is emitted when:
 *
 * @code
 * messageLevel <= currentLogLevel
 * @endcode
 *
 * Higher numeric values represent more verbose logging.
 */
typedef enum
{
    BSP_LOG_NONE  = 0, /*!< Disable all logging. */
    BSP_LOG_WARN  = 1, /*!< Warning conditions. */
    BSP_LOG_ERROR = 2, /*!< Error conditions. */
    BSP_LOG_INFO  = 3, /*!< Informational messages. */
    BSP_LOG_DEBUG = 4  /*!< Debug and verbose messages. */

} bspLogLevel_t;

/*=============================================================================
 * ISR LOGGING TYPES
 *============================================================================*/

/**
 * @brief ISR log value type.
 *
 * This enumeration identifies the type of value associated with
 * an ISR log message.
 */
typedef enum
{
    BSP_LOG_ISR_VALUE_NONE = 0U,

    BSP_LOG_ISR_VALUE_U8,
    BSP_LOG_ISR_VALUE_U16,
    BSP_LOG_ISR_VALUE_U32,
    BSP_LOG_ISR_VALUE_U64,

    BSP_LOG_ISR_VALUE_I8,
    BSP_LOG_ISR_VALUE_I16,
    BSP_LOG_ISR_VALUE_I32,
    BSP_LOG_ISR_VALUE_I64,

    BSP_LOG_ISR_VALUE_HEX8,
    BSP_LOG_ISR_VALUE_HEX16,
    BSP_LOG_ISR_VALUE_HEX32,
    BSP_LOG_ISR_VALUE_HEX64,

    BSP_LOG_ISR_VALUE_BOOL,
    BSP_LOG_ISR_VALUE_CHAR

} bspLogIsrValueType_t;


/**
 * @brief ISR log value container.
 *
 * The union allows the ISR logging API to carry different
 * fundamental data types without printf/variadic formatting.
 */
typedef union
{
    uint8_t  u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;

    int8_t   i8;
    int16_t  i16;
    int32_t  i32;
    int64_t  i64;

    bool     boolean;
    char     character;

} bspLogIsrValue_t;
/*=============================================================================
 * PUBLIC API
 *============================================================================*/

/**
 * @brief Initialize the BSP logger.
 *
 * Initializes internal logger resources and prepares the configured
 * logging output backend.
 *
 * This function must be called before using BSP logging APIs.
 *
 * @param[in] transport
 *        Console transport configuration used by the logger.
 *
 * @return BSP_ERR_STS_OK
 *         Logger initialized successfully.
 *
 * @return BSP_ERR_STS_ALREADY_INIT
 *         Logger was already initialized.
 *
 * @return BSP_ERR_STS_NO_MEM
 *         Required synchronization/resource allocation failed.
 *
 * @return BSP_ERR_STS_FAIL
 *         Logger initialization failed.
 */
bsp_err_sts_t bsp_log_init(
    const bspConsoleTransport_t* transport);


/**
 * @brief Deinitialize the BSP logger.
 *
 * Releases all resources owned by the BSP logger.
 *
 * After this function returns, logging APIs will not generate output
 * until bsp_log_init() is called again.
 *
 * @return BSP_ERR_STS_OK
 *         Logger deinitialized successfully.
 *
 * @return BSP_ERR_STS_NOT_INIT
 *         Logger was not initialized.
 */
bsp_err_sts_t bsp_log_deinit(void);


/**
 * @brief Enable or disable BSP logging.
 *
 * Disabling logging does not change the configured log level.
 * Re-enabling logging restores the previously configured level.
 *
 * @param[in] enable
 *        true to enable logging.
 *        false to disable logging.
 *
 * @return BSP_ERR_STS_OK
 *         Operation completed successfully.
 */
bsp_err_sts_t bsp_log_enable(bool enable);


/**
 * @brief Get BSP logging enable state.
 *
 * @return true
 *         Logging is enabled.
 *
 * @return false
 *         Logging is disabled.
 */
bool bsp_log_is_enabled(void);


/**
 * @brief Set the runtime BSP log level.
 *
 * The runtime level cannot exceed the compile-time maximum configured
 * by BSP_COMPILED_LOG_LEVEL.
 *
 * @param[in] level
 *        Requested BSP log level.
 *
 * @return BSP_ERR_STS_OK
 *         Log level configured successfully.
 *
 * @return BSP_ERR_STS_INVALID_PARAM
 *         Invalid log level.
 */
bsp_err_sts_t bsp_log_set_level(bspLogLevel_t level);


/**
 * @brief Get the current BSP log level.
 *
 * @return Current BSP log level.
 */
bspLogLevel_t bsp_log_get_level(void);


/**
 * @brief Write a formatted BSP log message.
 *
 * @param[in] level
 *        BSP log severity level.
 *
 * @param[in] file
 *        Source file name. Normally supplied using __FILE__.
 *
 * @param[in] line
 *        Source line number. Normally supplied using __LINE__.
 *
 * @param[in] function
 *        Source function name. Normally supplied using __func__.
 *
 * @param[in] fmt
 *        printf-style format string.
 *
 * @param[in] ...
 *        Format arguments.
 *
 * @note Thread-safe.
 *
 * @note This API must not normally be called from ISR context.
 */
void bsp_log(
    bspLogLevel_t level,
    const char* file,
    uint32_t line,
    const char* function,
    const char* fmt,
    ...);

/**
 * @brief Log a message from ISR context.
 *
 * This function is strictly ISR-safe.
 *
 * The function:
 * - does not acquire a mutex
 * - does not block
 * - does not allocate memory
 * - does not call the console transport
 * - does not perform printf formatting
 * - copies the message into a fixed-size queue record
 *
 * @param[in] level
 *        BSP log level.
 *
 * @param[in] file
 *        Source file name.
 *
 * @param[in] line
 *        Source line number.
 *
 * @param[in] function
 *        Function name.
 *
 * @param[in] message
 *        Null-terminated message.
 *
 * @return true
 *         Message queued successfully.
 *
 * @return false
 *         Message was rejected or queue was full.
 */
bool bsp_log_isr(
    bspLogLevel_t level,
    const char* file,
    uint32_t line,
    const char* function,
    const char* message);


/**
 * @brief Log a typed value from ISR context.
 *
 * This function is strictly ISR-safe and does not perform
 * printf-style formatting.
 *
 * @param[in] level
 *        BSP log level.
 *
 * @param[in] file
 *        Source file name.
 *
 * @param[in] line
 *        Source line number.
 *
 * @param[in] function
 *        Function name.
 *
 * @param[in] message
 *        Message describing the value.
 *
 * @param[in] valueType
 *        Type of value.
 *
 * @param[in] value
 *        Value to be logged.
 *
 * @return true
 *         Message queued successfully.
 *
 * @return false
 *         Message was rejected or queue was full.
 */
bool bsp_log_isr_value(
    bspLogLevel_t level,
    const char* file,
    uint32_t line,
    const char* function,
    const char* message,
    bspLogIsrValueType_t valueType,
    bspLogIsrValue_t value);

/*=============================================================================
 * CONVENIENCE MACROS
 *============================================================================*/
/**
 * @brief Return the basename portion of __FILE__.
 *
 * This avoids printing the complete source path in log output.
 */
#ifndef BSP_LOG_FILENAME

    #define BSP_LOG_FILENAME \
        (strrchr("/" __FILE__, '/') + 1)

#endif


/**
 * @brief Log an error message.
 *
 * @param[in] fmt printf-style format string.
 * @param[in] ... Format arguments.
 */
#define BSP_LOGE(fmt, ...)                                             \
    do                                                                 \
    {                                                                  \
        if(bsp_log_get_level() >= BSP_LOG_ERROR)                       \
        {                                                              \
            bsp_log(                                                   \
                BSP_LOG_ERROR,                                         \
                BSP_LOG_FILENAME,                                      \
                (uint32_t)__LINE__,                                    \
                __func__,                                              \
                (fmt),                                                 \
                ##__VA_ARGS__);                                        \
        }                                                              \
    } while(0)


/**
 * @brief Log a warning message.
 *
 * @param[in] fmt printf-style format string.
 * @param[in] ... Format arguments.
 */
#define BSP_LOGW(fmt, ...)                                             \
    do                                                                 \
    {                                                                  \
        if(bsp_log_get_level() >= BSP_LOG_WARN)                        \
        {                                                              \
            bsp_log(                                                   \
                BSP_LOG_WARN,                                          \
                BSP_LOG_FILENAME,                                      \
                (uint32_t)__LINE__,                                    \
                __func__,                                              \
                (fmt),                                                 \
                ##__VA_ARGS__);                                        \
        }                                                              \
    } while(0)


/**
 * @brief Log an informational message.
 *
 * @param[in] fmt printf-style format string.
 * @param[in] ... Format arguments.
 */
#define BSP_LOGI(fmt, ...)                                             \
    do                                                                 \
    {                                                                  \
        if(bsp_log_get_level() >= BSP_LOG_INFO)                        \
        {                                                              \
            bsp_log(                                                   \
                BSP_LOG_INFO,                                          \
                BSP_LOG_FILENAME,                                      \
                (uint32_t)__LINE__,                                    \
                __func__,                                              \
                (fmt),                                                 \
                ##__VA_ARGS__);                                        \
        }                                                              \
    } while(0)


/**
 * @brief Log a debug message.
 *
 * @param[in] fmt printf-style format string.
 * @param[in] ... Format arguments.
 */
#define BSP_LOGD(fmt, ...)                                             \
    do                                                                 \
    {                                                                  \
        if(bsp_log_get_level() >= BSP_LOG_DEBUG)                       \
        {                                                              \
            bsp_log(                                                   \
                BSP_LOG_DEBUG,                                         \
                BSP_LOG_FILENAME,                                      \
                (uint32_t)__LINE__,                                    \
                __func__,                                              \
                (fmt),                                                 \
                ##__VA_ARGS__);                                        \
        }                                                              \
    } while(0)

/*=============================================================================
 * ISR MESSAGE LOGGING
 *  ISR
 *  │
 *  ├─ no mutex
 *  ├─ no malloc
 *  ├─ no printf
 *  ├─ no snprintf
 *  ├─ no transport write
 *  ├─ no blocking
 *  │
 *  ▼
 * bspQueueSendFromISR()
 *  │
 *  ▼
 * osMessageQueuePut(..., timeout = 0)
 *  │
 *  ▼
 * ISR queue
 *  │
 *  ▼
 * Logger task
 *  │
 *  ├─ snprintf()
 *  ├─ mutex
 *  └─ transport->write()
 *  *============================================================================*/

/**
 * @brief Log an error message from ISR context.
 *
 * @param[in] message Null-terminated message.
 */
#define BSP_LOGE_ISR(message)                                          \
    do                                                                 \
    {                                                                  \
        if(bsp_log_get_level() >= BSP_LOG_ERROR)                       \
        {                                                              \
            (void)bsp_log_isr(                                         \
                BSP_LOG_ERROR,                                         \
                BSP_LOG_FILENAME,                                      \
                (uint32_t)__LINE__,                                    \
                __func__,                                              \
                (message));                                            \
        }                                                              \
    } while(0)


/**
 * @brief Log a warning message from ISR context.
 *
 * @param[in] message Null-terminated message.
 */
#define BSP_LOGW_ISR(message)                                          \
    do                                                                 \
    {                                                                  \
        if(bsp_log_get_level() >= BSP_LOG_WARN)                        \
        {                                                              \
            (void)bsp_log_isr(                                         \
                BSP_LOG_WARN,                                          \
                BSP_LOG_FILENAME,                                      \
                (uint32_t)__LINE__,                                    \
                __func__,                                              \
                (message));                                            \
        }                                                              \
    } while(0)


/**
 * @brief Log an informational message from ISR context.
 *
 * @param[in] message Null-terminated message.
 */
#define BSP_LOGI_ISR(message)                                          \
    do                                                                 \
    {                                                                  \
        if(bsp_log_get_level() >= BSP_LOG_INFO)                        \
        {                                                              \
            (void)bsp_log_isr(                                         \
                BSP_LOG_INFO,                                          \
                BSP_LOG_FILENAME,                                      \
                (uint32_t)__LINE__,                                    \
                __func__,                                              \
                (message));                                            \
        }                                                              \
    } while(0)


/**
 * @brief Log a debug message from ISR context.
 *
 * @param[in] message Null-terminated message.
 */
#define BSP_LOGD_ISR(message)                                          \
    do                                                                 \
    {                                                                  \
        if(bsp_log_get_level() >= BSP_LOG_DEBUG)                       \
        {                                                              \
            (void)bsp_log_isr(                                         \
                BSP_LOG_DEBUG,                                         \
                BSP_LOG_FILENAME,                                      \
                (uint32_t)__LINE__,                                    \
                __func__,                                              \
                (message));                                            \
        }                                                              \
    } while(0)


/*=============================================================================
 * ISR TYPE LOGGING
 *============================================================================*/
#define BSP_LOG_ISR_VALUE(level_, type_, member_, message_, value_)    \
    do                                                                 \
    {                                                                  \
        if(bsp_log_get_level() >= (level_))                            \
        {                                                              \
            bspLogIsrValue_t logValue;                                 \
            logValue.member_ = (value_);                               \
                                                                       \
            (void)bsp_log_isr_value(                                   \
                (level_),                                              \
                BSP_LOG_FILENAME,                                      \
                (uint32_t)__LINE__,                                    \
                __func__,                                              \
                (message_),                                            \
                (type_),                                               \
                logValue);                                             \
        }                                                              \
    } while(0)


/*=============================================================================
 * ISR UINSIGNED
 *============================================================================*/
#define BSP_LOGE_ISR_U8(message, value)                               \
    BSP_LOG_ISR_VALUE(                                                 \
        BSP_LOG_ERROR,                                                 \
        BSP_LOG_ISR_VALUE_U8,                                          \
        u8,                                                             \
        (message),                                                     \
        (uint8_t)(value))


#define BSP_LOGW_ISR_U8(message, value)                               \
    BSP_LOG_ISR_VALUE(                                                 \
        BSP_LOG_WARN,                                                  \
        BSP_LOG_ISR_VALUE_U8,                                          \
        u8,                                                             \
        (message),                                                     \
        (uint8_t)(value))


#define BSP_LOGI_ISR_U8(message, value)                               \
    BSP_LOG_ISR_VALUE(                                                 \
        BSP_LOG_INFO,                                                  \
        BSP_LOG_ISR_VALUE_U8,                                          \
        u8,                                                             \
        (message),                                                     \
        (uint8_t)(value))


#define BSP_LOGD_ISR_U8(message, value)                               \
    BSP_LOG_ISR_VALUE(                                                 \
        BSP_LOG_DEBUG,                                                 \
        BSP_LOG_ISR_VALUE_U8,                                          \
        u8,                                                             \
        (message),                                                     \
        (uint8_t)(value))


#define BSP_LOGE_ISR_U16(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_U16, u16, \
                      (message), (uint16_t)(value))

#define BSP_LOGW_ISR_U16(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_U16, u16, \
                      (message), (uint16_t)(value))

#define BSP_LOGI_ISR_U16(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_U16, u16, \
                      (message), (uint16_t)(value))

#define BSP_LOGD_ISR_U16(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_U16, u16, \
                      (message), (uint16_t)(value))

#define BSP_LOGE_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGW_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGI_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGD_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGE_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGW_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGI_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGD_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGE_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGW_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGI_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGD_ISR_U32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_U32, u32, \
                      (message), (uint32_t)(value))

/*=============================================================================
 * ISR SIGNED
 *============================================================================*/
#define BSP_LOGE_ISR_I8(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_I8, i8, \
                      (message), (int8_t)(value))

#define BSP_LOGW_ISR_I8(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_I8, i8, \
                      (message), (int8_t)(value))

#define BSP_LOGI_ISR_I8(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_I8, i8, \
                      (message), (int8_t)(value))

#define BSP_LOGD_ISR_I8(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_I8, i8, \
                      (message), (int8_t)(value))

#define BSP_LOGE_ISR_I16(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_I16, i16, \
                      (message), (int16_t)(value))

#define BSP_LOGW_ISR_I16(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_I16, i16, \
                      (message), (int16_t)(value))

#define BSP_LOGI_ISR_I16(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_I16, i16, \
                      (message), (int16_t)(value))

#define BSP_LOGD_ISR_I16(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_I16, i16, \
                      (message), (int16_t)(value))

#define BSP_LOGE_ISR_I32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_I32, i32, \
                      (message), (int32_t)(value))

#define BSP_LOGW_ISR_I32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_I32, i32, \
                      (message), (int32_t)(value))

#define BSP_LOGI_ISR_I32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_I32, i32, \
                      (message), (int32_t)(value))

#define BSP_LOGD_ISR_I32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_I32, i32, \
                      (message), (int32_t)(value))

#define BSP_LOGE_ISR_I32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_I32, i32, \
                      (message), (int32_t)(value))

#define BSP_LOGW_ISR_I32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_I32, i32, \
                      (message), (int32_t)(value))

#define BSP_LOGI_ISR_I32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_I32, i32, \
                      (message), (int32_t)(value))

#define BSP_LOGD_ISR_I32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_I32, i32, \
                      (message), (int32_t)(value))

/*=============================================================================
 * ISR HEX
 *============================================================================*/
#define BSP_LOGE_ISR_HEX32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_HEX32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGW_ISR_HEX32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_HEX32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGI_ISR_HEX32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_HEX32, u32, \
                      (message), (uint32_t)(value))

#define BSP_LOGD_ISR_HEX32(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_HEX32, u32, \
                      (message), (uint32_t)(value))

/*=============================================================================
 * ISR BOOL
 *============================================================================*/
#define BSP_LOGE_ISR_BOOL(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_BOOL, boolean, \
                      (message), (bool)(value))

#define BSP_LOGW_ISR_BOOL(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_BOOL, boolean, \
                      (message), (bool)(value))

#define BSP_LOGI_ISR_BOOL(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_BOOL, boolean, \
                      (message), (bool)(value))

#define BSP_LOGD_ISR_BOOL(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_BOOL, boolean, \
                      (message), (bool)(value))

/*=============================================================================
 * ISR CHAR
 *============================================================================*/
#define BSP_LOGE_ISR_CHAR(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_ERROR, BSP_LOG_ISR_VALUE_CHAR, character, \
                      (message), (char)(value))

#define BSP_LOGW_ISR_CHAR(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_WARN, BSP_LOG_ISR_VALUE_CHAR, character, \
                      (message), (char)(value))

#define BSP_LOGI_ISR_CHAR(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_INFO, BSP_LOG_ISR_VALUE_CHAR, character, \
                      (message), (char)(value))

#define BSP_LOGD_ISR_CHAR(message, value) \
    BSP_LOG_ISR_VALUE(BSP_LOG_DEBUG, BSP_LOG_ISR_VALUE_CHAR, character, \
                      (message), (char)(value))
                      
                      
#ifdef __cplusplus
}
#endif


#endif /* BSP_LOG_H */