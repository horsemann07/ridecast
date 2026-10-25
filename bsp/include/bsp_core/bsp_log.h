
/* ============================================================================
 * File        : log.h
 * Description : BSP asynchronous logging public API
 * Author      : Jha Raghav
 * Date        : 2026-10-24
 * ============================================================================
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Description:
 *   Defines the public interface for the reusable BSP logging module,
 *   including:
 *
 *   - Logging severity levels and control APIs
 *   - Task-context logging macros and functions
 *   - ISR-safe logging interfaces and typed values
 *   - Logging message formatting configuration
 *   - UTC, local, date/time, and Unix timestamp modes
 *   - Application-provided clock and timezone callback types
 *   - Configurable logging transport interface
 *   - Application-owned CMSIS-RTOS2 resource configuration
 *   - Logger initialization and deinitialization APIs
 *   - Runtime logging level and format configuration APIs
 *
 * Integration:
 *   This header exposes the platform-independent BSP logger API.
 *   Applications must configure the logger through log_task_config_t
 *   and initialize it using log_init().
 *
 *   The logging worker executes through the application-provided
 *   CMSIS-RTOS2 thread.
 *
 * Requirements:
 *   - C11 compatible compiler.
 *   - CMSIS-RTOS2 API definitions.
 *   - BSP logger configuration definitions.
 *   - Platform-independent error status definitions.
 *   - Application-managed RTOS resources and output transport.
 *
 * Notes:
 *   - Task logging and ISR logging use separate message queues.
 *   - ISR logging must not perform blocking or printf-style operations.
 *   - Calendar timestamps are derived from application-provided
 *     Unix UTC seconds.
 *   - Output encryption is the responsibility of the selected
 *     secure transport and is not provided by this API alone.
 *
 * ============================================================================
 * Change Log:
 *
 * Date          Author          Description
 * ------------  --------------  ----------------------------------------------
 * 2026-10-24    Jha Raghav      Extended timestamp modes for date and Unix time
 * 2026-10-23    Jha Raghav      Updated logging configuration and public APIs
 * ============================================================================
 */

#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

    /*-----------------------------------------------------------------------------
     * INCLUDES
     *----------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bsp_config.h"
#include "cmsis_os2.h"

#include "bsp_err_sts.h"


    /* -------------------------------------------------------------------------- */
    /* Logger Severity Levels */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Supported logging severity levels.
     *
     * @details
     * Determines which log messages are emitted based on the
     * configured runtime severity threshold.
     *
     * A message is emitted when its severity level is less
     * than or equal to the configured logging level.
     *
     * Higher numeric values represent more verbose logging.
     */
    typedef enum
    {
        LOG_LEVEL_NONE  = 0, /*!< Disable all logging. */
        LOG_LEVEL_ERROR = 1, /*!< Error messages. */
        LOG_LEVEL_WARN  = 2, /*!< Warning messages. */
        LOG_LEVEL_INFO  = 3, /*!< Informational messages. */
        LOG_LEVEL_DEBUG = 4  /*!< Debug messages. */

    } log_level_t;

    /* Default logging level for the current translation unit. */
#ifndef configLOG_LOCAL_LEVEL
    #define configLOG_LOCAL_LEVEL LOG_LEVEL_INFO
#endif

    /* -------------------------------------------------------------------------- */
    /* ISR Logging Value Types */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Supported ISR logging value types.
     *
     * @details
     * Identifies the data type associated with an ISR log record.
     * Allows values to be logged without performing printf-style
     * formatting inside interrupt context.
     */
    typedef enum
    {
        LOG_ISR_VALUE_NONE = 0U, /*!< No associated value. */

        LOG_ISR_VALUE_U8,        /*!< Unsigned 8-bit integer. */
        LOG_ISR_VALUE_U16,       /*!< Unsigned 16-bit integer. */
        LOG_ISR_VALUE_U32,       /*!< Unsigned 32-bit integer. */
        LOG_ISR_VALUE_U64,       /*!< Unsigned 64-bit integer. */

        LOG_ISR_VALUE_I8,        /*!< Signed 8-bit integer. */
        LOG_ISR_VALUE_I16,       /*!< Signed 16-bit integer. */
        LOG_ISR_VALUE_I32,       /*!< Signed 32-bit integer. */
        LOG_ISR_VALUE_I64,       /*!< Signed 64-bit integer. */

        LOG_ISR_VALUE_HEX8,      /*!< 8-bit hexadecimal value. */
        LOG_ISR_VALUE_HEX16,     /*!< 16-bit hexadecimal value. */
        LOG_ISR_VALUE_HEX32,     /*!< 32-bit hexadecimal value. */
        LOG_ISR_VALUE_HEX64,     /*!< 64-bit hexadecimal value. */

        LOG_ISR_VALUE_BOOL,      /*!< Boolean value. */
        LOG_ISR_VALUE_CHAR       /*!< Character value. */

    } log_isr_value_type_t;


    /**
     * @brief Container for typed ISR logging values.
     *
     * @details
     * Provides storage for supported integer, Boolean, and character
     * values without requiring variadic formatting in interrupt context.
     *
     * @note
     * The active union member must correspond to the associated
     * log_isr_value_type_t value.
     */
    typedef union
    {
        uint8_t u8;     /*!< Unsigned 8-bit value. */
        uint16_t u16;   /*!< Unsigned 16-bit value. */
        uint32_t u32;   /*!< Unsigned 32-bit value. */
        uint64_t u64;   /*!< Unsigned 64-bit value. */

        int8_t i8;      /*!< Signed 8-bit value. */
        int16_t i16;    /*!< Signed 16-bit value. */
        int32_t i32;    /*!< Signed 32-bit value. */
        int64_t i64;    /*!< Signed 64-bit value. */

        bool boolean;   /*!< Boolean value. */
        char character; /*!< Character value. */

    } log_isr_value_t;

    /* -------------------------------------------------------------------------- */
    /* Task Logging Record */
    /* -------------------------------------------------------------------------- */
    typedef struct
    {
        log_level_t level;
        const char* file;
        uint32_t line;
        const char* function;
        uint16_t length;
        char message[configLOG_TASK_MESSAGE_MAX];
    } log_text_record_t;


    /* -------------------------------------------------------------------------- */
    /* ISR Logging Value Container */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Internal ISR log queue record.
     *
     * @note
     * This structure is shared between the logger implementation
     * and the BSP queue configuration.
     */
    typedef struct
    {
        log_level_t level;
        const char* file;
        uint32_t line;
        const char* function;
        log_isr_value_type_t type;
        log_isr_value_t value;
        char message[configLOG_ISR_MESSAGE_MAX];

    } log_isr_record_t;


    /* -------------------------------------------------------------------------- */
    /* Logger Transport Callback */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Logging transport write callback.
     *
     * @details
     * Writes a formatted log message to the application-defined
     * output interface, such as UART, USB, or another transport.
     *
     * @param[in] data
     * Pointer to the formatted log data.
     *
     * @param[in] length
     * Number of bytes to transmit.
     *
     * @param[in] context
     * Application-defined transport context.
     *
     * @return
     * Transport operation status.
     *
     * @warning
     * The callback must consume or copy the complete data buffer
     * before returning. Starting DMA without copying or retaining
     * the data safely is not sufficient.
     *
     * @note
     * This callback is invoked only from the logging worker thread.
     */
    typedef ret_err_sts_t (*log_transport_write_t)(const uint8_t* data, size_t length, void* context);

    /* -------------------------------------------------------------------------- */
    /* Logger Clock Callbacks */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Callback to retrieve the current UTC timestamp.
     *
     * @details
     * Retrieves the current UTC Unix timestamp in seconds
     * from an application-provided clock source.
     *
     * @param[out] utc_seconds
     * Pointer to the variable receiving the UTC timestamp.
     *
     * @param[in] context
     * Application-defined clock context.
     *
     * @retval true
     * Valid UTC timestamp retrieved successfully.
     *
     * @retval false
     * UTC timestamp is unavailable or retrieval failed.
     *
     * @note
     * This callback is invoked only from the logging worker thread.
     */
    typedef bool (*log_get_utc_seconds_t)(uint64_t* utc_seconds, void* context);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Callback to retrieve the local UTC offset.
     *
     * @details
     * Returns the UTC offset in minutes for the specified
     * timestamp, allowing daylight saving time and
     * location-dependent timezone adjustments.
     *
     * @param[in] utc_seconds
     * UTC Unix timestamp in seconds.
     *
     * @param[in] context
     * Application-defined clock context.
     *
     * @return
     * UTC offset in minutes, east of UTC.
     *
     * @note
     * Positive values indicate timezones east of UTC.
     * For example, India Standard Time is +330 minutes.
     *
     * @note
     * This callback is optional and is invoked only
     * from the logging worker thread.
     */
    typedef int32_t (*log_get_utc_offset_minutes_t)(uint64_t utc_seconds, void* context);

    /* -------------------------------------------------------------------------- */
    /* Logger Timestamp Modes */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Supported logging timestamp modes.
     *
     * @details
     * Selects the timestamp representation used when
     * formatting log messages.
     */
    typedef enum
    {
        LOG_TIME_NONE = 0,

        /* Time only */
        LOG_TIME_UTC,
        LOG_TIME_LOCAL_24H,

        /* Date and time */
        LOG_DATE_TIME_UTC,
        LOG_DATE_TIME_LOCAL_24H,

        /* Unix time: seconds since 1970-01-01 00:00:00 UTC */
        LOG_TIME_UNIX_SECONDS,

        LOG_TIME_MODE_MAX
    } log_time_mode_t;

    /* -------------------------------------------------------------------------- */
    /* Logger Format Configuration */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Logging message formatting configuration.
     *
     * @details
     * Controls the optional metadata fields included
     * in each formatted log message.
     */
    typedef struct
    {
        bool message_number; /*!< Enable sequential message numbering. */
        bool level;     /*!< Enable severity identifier: [E], [W], [I], [D]. */
        bool file_line; /*!< Enable source file and line: main.c:42. */
        bool function;  /*!< Enable source function name. */
        /**
         * @brief Enable ANSI terminal color output.
         *
         * Requires configLOG_COLOR_SUPPORT == 1U.
         * Ignored when compile-time support is unavailable.
         */
        bool color_enabled;

        log_time_mode_t time_mode; /*!< Configure timestamp representation. */

    } log_format_config_t;

    /* -------------------------------------------------------------------------- */
    /* Logger Task Configuration */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Logging module initialization configuration.
     *
     * @details
     * Defines the RTOS resources, logging severity, transport
     * callback, formatting options, clock source, and shutdown
     * timeout required by the logging module.
     *
     * All CMSIS-RTOS2 objects must be created and managed
     * by the application.
     *
     * @note
     * The logging module does not create or delete RTOS objects.
     *
     * @note
     * The worker thread must be created using log_task()
     * as its entry function.
     *
     * @warning
     * The ready semaphore maximum count must accommodate
     * the combined task and ISR queue capacities plus
     * one additional shutdown notification.
     */
    typedef struct
    {
        /* Application-owned CMSIS-RTOS2 resources. */

        osThreadId_t worker_thread;    /*!< Logging worker thread handle. */
        osMessageQueueId_t task_queue; /*!< Task logging message queue. */
        osMessageQueueId_t isr_queue;  /*!< ISR logging message queue. */
        osSemaphoreId_t ready_sem;     /*!< Worker notification semaphore. */
        osEventFlagsId_t control_flags; /*!< Worker shutdown acknowledgement flags. */
        osMutexId_t format_mutex;       /* Application-owned */

        /* Logging severity and transport configuration. */
        log_level_t initial_level;   /*!< Initial logging severity threshold. */
        log_transport_write_t write; /*!< Application transport write callback. */
        void* write_context; /*!< Application-defined transport context. */
        log_format_config_t format; /*!< Logging message formatting options. */

        /* Application-provided clock configuration. */
        log_get_utc_seconds_t get_utc_seconds; /*!< UTC timestamp callback. */
        void* clock_context;        /*!< Application-defined clock context. */
        int32_t utc_offset_minutes; /*!< Fixed local UTC offset in minutes. */
        log_get_utc_offset_minutes_t get_utc_offset_minutes; /*!< Optional dynamic UTC offset callback. */

        /* Logging worker shutdown configuration. */
        uint32_t shutdown_timeout_ticks; /*!< Shutdown timeout; zero selects 1000 ticks. */

    } log_task_config_t;

    /* -------------------------------------------------------------------------- */
    /* Logger Worker Thread API */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Logging worker thread entry function.
     *
     * @details
     * Waits for the logging module initialization signal
     * before entering the main logging processing loop.
     *
     * During normal operation, the worker waits for a semaphore
     * notification and processes pending logging records.
     *
     * Task and ISR queue priority is alternated to provide
     * balanced processing between both logging sources.
     *
     * During shutdown, the worker drains all pending records,
     * signals shutdown acknowledgement, and terminates.
     *
     * @param[in] argument
     * Optional thread argument.
     * Currently unused.
     *
     * @return
     * None.
     *
     * @pre
     * The application must create the logging worker thread
     * before calling log_init().
     *
     * @note
     * This function is intended to be used as a CMSIS-RTOS2
     * thread entry function.
     *
     * @note
     * The worker waits indefinitely for the initialization
     * signal and logging semaphore notifications.
     *
     * @warning
     * All task and ISR log producers must be stopped before
     * the logging module shutdown sequence begins.
     *
     * @par MISRA C Considerations
     * - Unused function argument is explicitly cast to void.
     * - RTOS API return values are validated.
     * - Shared shutdown state is accessed atomically.
     * - Queue processing uses a bounded operation per wakeup.
     * - The worker exits explicitly after shutdown acknowledgement.
     */
    void log_task(void* argument);


    /* -------------------------------------------------------------------------- */
    /* Logger Initialization and Deinitialization */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Initialize the asynchronous CMSIS-RTOS2 logger.
     *
     * @details
     * Validates the application-provided configuration, initializes
     * the internal logger state, and starts the logging worker.
     *
     * All RTOS objects must be created by the application.
     * This API does not allocate or create RTOS resources.
     *
     * @param[in] config
     * Pointer to the logger configuration structure.
     *
     * @retval RET_ERR_STS_OK
     * Logger initialized successfully.
     *
     * @retval RET_ERR_STS_INVALID_PARAM
     * Invalid configuration or unsupported parameter.
     *
     * @retval RET_ERR_STS_ALREADY_INIT
     * Logger is already initialized.
     *
     * @retval RET_ERR_STS_INVALID_STATE
     * Invalid RTOS state or worker startup failure.
     *
     * @retval RET_ERR_STS_INVALID_SIZE
     * Incorrect logging queue element size.
     *
     * @note
     * The worker thread and required RTOS objects must exist
     * before this function is called.
     */
    ret_err_sts_t log_init(const log_task_config_t* config);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Deinitialize the asynchronous logger.
     *
     * @details
     * Disables logging, drains pending task and ISR messages,
     * waits for worker shutdown acknowledgement, and joins
     * the logging worker thread.
     *
     * The application remains responsible for deleting
     * its RTOS objects after successful deinitialization.
     *
     * @retval RET_ERR_STS_OK
     * Logger deinitialized successfully.
     *
     * @retval RET_ERR_STS_NOT_INITIALIZED
     * Logger was not initialized.
     *
     * @retval RET_ERR_STS_INVALID_STATE
     * Invalid kernel state or calling thread context.
     *
     * @retval RET_ERR_STS_TIMEOUT
     * Worker shutdown acknowledgement timed out.
     *
     * @retval RET_ERR_STS_FAIL
     * Semaphore release or worker join failed.
     *
     * @pre
     * All task and ISR log producers must be stopped.
     *
     * @note
     * Must be called from task context.
     *
     * @warning
     * The output transport callback must complete within
     * a bounded time to allow successful shutdown.
     */
    ret_err_sts_t log_deinit(void);

    /* -------------------------------------------------------------------------- */
    /* Logger Runtime Control */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Update the logging output format at runtime.
     *
     * @param[in] format New logging format configuration.
     *
     * @return RET_ERR_STS_OK on success.
     * @return RET_ERR_STS_INVALID_PARAM for invalid configuration.
     * @return RET_ERR_STS_NOT_INITIALIZED if logger is not initialized.
     * @return RET_ERR_STS_BUSY if logger is shutting down.
     *
     * @note Thread-safe. Must not be called from ISR context.
     */
    ret_err_sts_t log_set_format(const log_format_config_t* format);

    /**
     * @brief Get the current logging output format.
     *
     * @param[out] format Receives the current format configuration.
     *
     * @note Thread-safe. Must not be called from ISR context.
     */
    ret_err_sts_t log_get_format(log_format_config_t* format);

    /**
     * @brief Enable or disable logging.
     *
     * @details
     * Updates the logger enable state without modifying
     * the configured severity level.
     *
     * Re-enabling logging restores the previously
     * configured severity threshold.
     *
     * @param[in] enable
     * true to enable logging, false to disable logging.
     *
     * @retval RET_ERR_STS_OK
     * Logging state updated successfully.
     *
     * @retval RET_ERR_STS_NOT_INITIALIZED
     * Logger is not initialized or is stopping.
     *
     * @note
     * Disabling logging does not discard messages
     * already present in the logging queues.
     */
    ret_err_sts_t log_enable(bool enable);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Get the current logging enable state.
     *
     * @retval true
     * Logger is initialized, enabled, and not stopping.
     *
     * @retval false
     * Logger is disabled, uninitialized, or stopping.
     *
     * @note
     * Shared state is accessed atomically.
     */
    bool log_is_enabled(void);

    /* -------------------------------------------------------------------------- */
    /* Logger Severity Configuration */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Set the runtime logging severity level.
     *
     * @details
     * Configures the maximum severity threshold used
     * to filter logging messages.
     *
     * Valid levels range from LOG_LEVEL_NONE
     * through LOG_LEVEL_DEBUG.
     *
     * @param[in] level
     * Requested logging severity level.
     *
     * @retval RET_ERR_STS_OK
     * Logging level updated successfully.
     *
     * @retval RET_ERR_STS_INVALID_PARAM
     * Invalid logging severity level.
     *
     * @retval RET_ERR_STS_NOT_INITIALIZED
     * Logger is not initialized or is stopping.
     */
    ret_err_sts_t log_set_level(log_level_t level);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Get the currently configured logging severity level.
     *
     * @return
     * Current logging severity level.
     *
     * @note
     * The configured level is retrieved atomically.
     */
    log_level_t log_get_level(void);

    /* -------------------------------------------------------------------------- */
    /* Task Context Logging API */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Submit a formatted log message from task context.
     *
     * @details
     * Formats a printf-style message and submits it to the
     * task logging queue for asynchronous processing.
     *
     * The logging worker formats the final output prefix
     * and forwards the message to the configured transport.
     *
     * @param[in] level
     * Logging severity level.
     *
     * @param[in] file
     * Source file name, normally provided by __FILE__.
     *
     * @param[in] line
     * Source line number, normally provided by __LINE__.
     *
     * @param[in] function
     * Source function name, normally provided by __func__.
     *
     * @param[in] fmt
     * Pointer to the printf-style format string.
     *
     * @param[in] ...
     * Variable arguments corresponding to the format string.
     *
     * @return
     * None.
     *
     * @note
     * This API must be called from task context only.
     *
     * @note
     * Messages exceeding the configured buffer capacity
     * are truncated and NUL terminated.
     *
     * @warning
     * Messages may be discarded if the logging queue is full.
     */
    void log_printf(log_level_t level,
                    const char* file,
                    uint32_t line,
                    const char* function,
                    const char* fmt,
                    ...);

    /* -------------------------------------------------------------------------- */
    /* ISR Context Logging APIs */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Submit a text log message from interrupt context.
     *
     * @details
     * Copies a NUL-terminated message into a fixed-size ISR
     * logging record and submits it using nonblocking RTOS APIs.
     *
     * This function does not:
     * - Acquire a mutex.
     * - Block the calling context.
     * - Allocate dynamic memory.
     * - Perform printf-style formatting.
     * - Access the output transport directly.
     *
     * @param[in] level
     * Logging severity level.
     *
     * @param[in] file
     * Source file name with sufficient lifetime.
     *
     * @param[in] line
     * Source line number.
     *
     * @param[in] function
     * Source function name with sufficient lifetime.
     *
     * @param[in] message
     * Pointer to the NUL-terminated message.
     *
     * @retval true
     * ISR logging record submitted successfully.
     *
     * @retval false
     * Invalid message, filtered severity, queue failure,
     * or worker notification failure.
     *
     * @note
     * The message is copied into the ISR record.
     * File and function pointers are retained without copying.
     *
     * @warning
     * The target CMSIS-RTOS2 implementation must support
     * the queue and semaphore operations from the calling ISR.
     */
    bool log_printf_isr(log_level_t level,
                        const char* file,
                        uint32_t line,
                        const char* function,
                        const char* message);

    /* -------------------------------------------------------------------------- */

    /**
     * @brief Submit a typed value log message from interrupt context.
     *
     * @details
     * Copies a text message and associated typed value into
     * an ISR logging record for asynchronous processing.
     *
     * Value formatting is performed by the logging worker,
     * avoiding printf-style operations inside interrupt context.
     *
     * @param[in] level
     * Logging severity level.
     *
     * @param[in] file
     * Source file name with sufficient lifetime.
     *
     * @param[in] line
     * Source line number.
     *
     * @param[in] function
     * Source function name with sufficient lifetime.
     *
     * @param[in] message
     * Pointer to the NUL-terminated message.
     *
     * @param[in] valueType
     * Type identifier of the associated value.
     *
     * @param[in] value
     * Typed value to be included in the log record.
     *
     * @retval true
     * ISR logging record submitted successfully.
     *
     * @retval false
     * Invalid parameters, filtered severity, queue failure,
     * or worker notification failure.
     *
     * @note
     * No printf-style formatting is performed in ISR context.
     *
     * @note
     * The active union member must match valueType.
     *
     * @warning
     * The target CMSIS-RTOS2 implementation must support
     * the queue and semaphore operations from the calling ISR.
     */
    bool log_isr_value(log_level_t level,
                       const char* file,
                       uint32_t line,
                       const char* function,
                       const char* message,
                       log_isr_value_type_t valueType,
                       log_isr_value_t value);

/*=============================================================================
 * CONVENIENCE MACROS
 *============================================================================*/
/**
 * @brief Pass __FILE__ without parsing paths in ISR context.
 *
 * The logging worker converts this to a basename when formatting output.
 */
#ifndef LOG_FILENAME
    #define LOG_FILENAME (__FILE__)
#endif

#define LOGE(...)                                                                              \
    do                                                                                         \
    {                                                                                          \
        if((configLOG_LOCAL_LEVEL >= LOG_LEVEL_ERROR) && (log_get_level() >= LOG_LEVEL_ERROR)) \
        {                                                                                      \
            log_printf(LOG_LEVEL_ERROR, LOG_FILENAME, (uint32_t)__LINE__,                      \
                       __func__, __VA_ARGS__);                                                 \
        }                                                                                      \
    } while(0)

#define LOGW(...)                                                                            \
    do                                                                                       \
    {                                                                                        \
        if((configLOG_LOCAL_LEVEL >= LOG_LEVEL_WARN) && (log_get_level() >= LOG_LEVEL_WARN)) \
        {                                                                                    \
            log_printf(LOG_LEVEL_WARN, LOG_FILENAME, (uint32_t)__LINE__,                     \
                       __func__, __VA_ARGS__);                                               \
        }                                                                                    \
    } while(0)

#define LOGI(...)                                                                            \
    do                                                                                       \
    {                                                                                        \
        if((configLOG_LOCAL_LEVEL >= LOG_LEVEL_INFO) && (log_get_level() >= LOG_LEVEL_INFO)) \
        {                                                                                    \
            log_printf(LOG_LEVEL_INFO, LOG_FILENAME, (uint32_t)__LINE__,                     \
                       __func__, __VA_ARGS__);                                               \
        }                                                                                    \
    } while(0)

#define LOGD(...)                                                                              \
    do                                                                                         \
    {                                                                                          \
        if((configLOG_LOCAL_LEVEL >= LOG_LEVEL_DEBUG) && (log_get_level() >= LOG_LEVEL_DEBUG)) \
        {                                                                                      \
            log_printf(LOG_LEVEL_DEBUG, LOG_FILENAME, (uint32_t)__LINE__,                      \
                       __func__, __VA_ARGS__);                                                 \
        }                                                                                      \
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
#define LOGE_ISR(message)                                                                      \
    do                                                                                         \
    {                                                                                          \
        if((configLOG_LOCAL_LEVEL >= LOG_LEVEL_ERROR) && (log_get_level() >= LOG_LEVEL_ERROR)) \
        {                                                                                      \
            (void)log_printf_isr(LOG_LEVEL_ERROR, LOG_FILENAME,                                \
                                 (uint32_t)__LINE__, __func__, (message));                     \
        }                                                                                      \
    } while(0)

/**
 * @brief Log a warning message from ISR context.
 *
 * @param[in] message Null-terminated message.
 */
#define LOGW_ISR(message)                                                                    \
    do                                                                                       \
    {                                                                                        \
        if((configLOG_LOCAL_LEVEL >= LOG_LEVEL_WARN) && (log_get_level() >= LOG_LEVEL_WARN)) \
        {                                                                                    \
            (void)log_printf_isr(LOG_LEVEL_WARN, LOG_FILENAME,                               \
                                 (uint32_t)__LINE__, __func__, (message));                   \
        }                                                                                    \
    } while(0)

/**
 * @brief Log an informational message from ISR context.
 *
 * @param[in] message Null-terminated message.
 */
#define LOGI_ISR(message)                                                                    \
    do                                                                                       \
    {                                                                                        \
        if((configLOG_LOCAL_LEVEL >= LOG_LEVEL_INFO) && (log_get_level() >= LOG_LEVEL_INFO)) \
        {                                                                                    \
            (void)log_printf_isr(LOG_LEVEL_INFO, LOG_FILENAME,                               \
                                 (uint32_t)__LINE__, __func__, (message));                   \
        }                                                                                    \
    } while(0)

/**
 * @brief Log a debug message from ISR context.
 *
 * @param[in] message Null-terminated message.
 */
#define LOGD_ISR(message)                                                                      \
    do                                                                                         \
    {                                                                                          \
        if((configLOG_LOCAL_LEVEL >= LOG_LEVEL_DEBUG) && (log_get_level() >= LOG_LEVEL_DEBUG)) \
        {                                                                                      \
            (void)log_printf_isr(LOG_LEVEL_DEBUG, LOG_FILENAME,                                \
                                 (uint32_t)__LINE__, __func__, (message));                     \
        }                                                                                      \
    } while(0)

/*=============================================================================
 * ISR TYPE LOGGING
 *============================================================================*/
#define LOG_ISR_VALUE(level_, type_, member_, message_, value_)                  \
    do                                                                           \
    {                                                                            \
        if((configLOG_LOCAL_LEVEL >= (level_)) && (log_get_level() >= (level_))) \
        {                                                                        \
            log_isr_value_t logValue;                                            \
            logValue.member_ = (value_);                                         \
                                                                                 \
            (void)log_isr_value((level_), LOG_FILENAME, (uint32_t)__LINE__,      \
                                __func__, (message_), (type_), logValue);        \
        }                                                                        \
    } while(0)

/*=============================================================================
 * ISR UINSIGNED
 *============================================================================*/
#define LOGE_ISR_U8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_U8, u8, (message), (uint8_t)(value))

#define LOGW_ISR_U8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_U8, u8, (message), (uint8_t)(value))

#define LOGI_ISR_U8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_U8, u8, (message), (uint8_t)(value))

#define LOGD_ISR_U8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_U8, u8, (message), (uint8_t)(value))

#define LOGE_ISR_U16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_U16, u16, (message), (uint16_t)(value))

#define LOGW_ISR_U16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_U16, u16, (message), (uint16_t)(value))

#define LOGI_ISR_U16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_U16, u16, (message), (uint16_t)(value))

#define LOGD_ISR_U16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_U16, u16, (message), (uint16_t)(value))

#define LOGE_ISR_U32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_U32, u32, (message), (uint32_t)(value))

#define LOGW_ISR_U32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_U32, u32, (message), (uint32_t)(value))

#define LOGI_ISR_U32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_U32, u32, (message), (uint32_t)(value))

#define LOGD_ISR_U32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_U32, u32, (message), (uint32_t)(value))

/*=============================================================================
 * ISR SIGNED
 *============================================================================*/
#define LOGE_ISR_I8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_I8, i8, (message), (int8_t)(value))

#define LOGW_ISR_I8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_I8, i8, (message), (int8_t)(value))

#define LOGI_ISR_I8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_I8, i8, (message), (int8_t)(value))

#define LOGD_ISR_I8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_I8, i8, (message), (int8_t)(value))

#define LOGE_ISR_I16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_I16, i16, (message), (int16_t)(value))

#define LOGW_ISR_I16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_I16, i16, (message), (int16_t)(value))

#define LOGI_ISR_I16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_I16, i16, (message), (int16_t)(value))

#define LOGD_ISR_I16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_I16, i16, (message), (int16_t)(value))

#define LOGE_ISR_I32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_I32, i32, (message), (int32_t)(value))

#define LOGW_ISR_I32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_I32, i32, (message), (int32_t)(value))

#define LOGI_ISR_I32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_I32, i32, (message), (int32_t)(value))

#define LOGD_ISR_I32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_I32, i32, (message), (int32_t)(value))

/*=============================================================================
 * ISR HEX
 *============================================================================*/
#define LOGE_ISR_HEX32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_HEX32, u32, (message), (uint32_t)(value))

#define LOGW_ISR_HEX32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_HEX32, u32, (message), (uint32_t)(value))

#define LOGI_ISR_HEX32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_HEX32, u32, (message), (uint32_t)(value))

#define LOGD_ISR_HEX32(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_HEX32, u32, (message), (uint32_t)(value))

/*=============================================================================
 * ISR BOOL
 *============================================================================*/
#define LOGE_ISR_BOOL(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_BOOL, boolean, (message), (bool)(value))

#define LOGW_ISR_BOOL(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_BOOL, boolean, (message), (bool)(value))

#define LOGI_ISR_BOOL(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_BOOL, boolean, (message), (bool)(value))

#define LOGD_ISR_BOOL(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_BOOL, boolean, (message), (bool)(value))

/*=============================================================================
 * ISR CHAR
 *============================================================================*/
#define LOGE_ISR_CHAR(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_CHAR, character, (message), (char)(value))

#define LOGW_ISR_CHAR(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_CHAR, character, (message), (char)(value))

#define LOGI_ISR_CHAR(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_CHAR, character, (message), (char)(value))

#define LOGD_ISR_CHAR(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_CHAR, character, (message), (char)(value))

/* Additional typed ISR macros for 64-bit and HEX8/16/64 values. */
#define LOGE_ISR_U64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_U64, u64, (message), (uint64_t)(value))

#define LOGW_ISR_U64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_U64, u64, (message), (uint64_t)(value))

#define LOGI_ISR_U64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_U64, u64, (message), (uint64_t)(value))

#define LOGD_ISR_U64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_U64, u64, (message), (uint64_t)(value))

#define LOGE_ISR_I64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_I64, i64, (message), (int64_t)(value))

#define LOGW_ISR_I64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_I64, i64, (message), (int64_t)(value))

#define LOGI_ISR_I64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_I64, i64, (message), (int64_t)(value))

#define LOGD_ISR_I64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_I64, i64, (message), (int64_t)(value))

#define LOGE_ISR_HEX8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_HEX8, u8, (message), (uint8_t)(value))

#define LOGW_ISR_HEX8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_HEX8, u8, (message), (uint8_t)(value))

#define LOGI_ISR_HEX8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_HEX8, u8, (message), (uint8_t)(value))

#define LOGD_ISR_HEX8(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_HEX8, u8, (message), (uint8_t)(value))

#define LOGE_ISR_HEX16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_HEX16, u16, (message), (uint16_t)(value))

#define LOGW_ISR_HEX16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_HEX16, u16, (message), (uint16_t)(value))

#define LOGI_ISR_HEX16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_HEX16, u16, (message), (uint16_t)(value))

#define LOGD_ISR_HEX16(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_HEX16, u16, (message), (uint16_t)(value))

#define LOGE_ISR_HEX64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_ERROR, LOG_ISR_VALUE_HEX64, u64, (message), (uint64_t)(value))

#define LOGW_ISR_HEX64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_WARN, LOG_ISR_VALUE_HEX64, u64, (message), (uint64_t)(value))

#define LOGI_ISR_HEX64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_INFO, LOG_ISR_VALUE_HEX64, u64, (message), (uint64_t)(value))

#define LOGD_ISR_HEX64(message, value) \
    LOG_ISR_VALUE(LOG_LEVEL_DEBUG, LOG_ISR_VALUE_HEX64, u64, (message), (uint64_t)(value))

#ifdef __cplusplus
}
#endif

#endif /* LOG_H */