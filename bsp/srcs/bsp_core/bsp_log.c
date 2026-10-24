/* ============================================================================
 * File        : log.c
 * Description : BSP asynchronous logging implementation
 * Author      : Jha Raghav
 * Date        : 2026-10-24
 * ============================================================================
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Description:
 *   Implements the reusable, platform-independent BSP logging module,
 *   including:
 *
 *   - Thread-safe task-context logging
 *   - ISR-safe logging with dedicated message queues
 *   - Application-owned CMSIS-RTOS2 resources
 *   - Asynchronous logging worker and message processing
 *   - Configurable logging severity levels
 *   - Configurable log prefixes and ANSI color formatting
 *   - UTC and local 24-hour time formatting
 *   - UTC and local calendar date/time formatting
 *   - Unix UTC timestamp formatting in seconds
 *   - Application-provided clock and timezone callbacks
 *   - Runtime logging format configuration
 *   - Configurable output transport callback
 *   - Controlled worker initialization and shutdown
 *
 * Integration:
 *   This module is part of the reusable BSP logger.
 *   The application must create and supply the required CMSIS-RTOS2
 *   resources through log_task_config_t before calling log_init().
 *
 * Requirements:
 *   - C11 compiler with required lock-free atomic operations.
 *   - CMSIS-RTOS2 compatible kernel implementation.
 *   - Application-owned logging worker, queues, and synchronization.
 *   - Application-provided output transport callback.
 *   - Application-provided UTC clock callback when timestamps are enabled.
 *   - BSP logger configuration and error status definitions.
 *
 * ============================================================================
 * Change Log:
 *
 * Date          Author          Description
 * ------------  --------------  ----------------------------------------------
 * 2026-10-24    Jha Raghav      Added date/time and Unix timestamp support
 * 2026-10-24    Jha Raghav      Corrected logger initialization state handling
 * 2026-10-23    Jha Raghav      Updated asynchronous logging and CMSIS support
 * ============================================================================
 */
#include "bsp_log.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Verify lock-free atomic operation support.
 *
 * @details
 * Ensures that atomic Boolean and integer operations are
 * always lock-free on the target platform.
 *
 * Lock-free atomic operations are required because the logging
 * module accesses shared control variables from both task
 * and interrupt execution contexts.
 *
 * @note
 * ATOMIC_BOOL_LOCK_FREE and ATOMIC_INT_LOCK_FREE must
 * both evaluate to 2, indicating always lock-free support.
 *
 * @warning
 * Compilation is terminated if the target platform
 * does not provide the required atomic guarantees.
 *
 * @par MISRA C Considerations
 * - Compile-time validation prevents unsupported configurations.
 * - No runtime overhead is introduced.
 * - Preprocessor conditions use defined constant expressions.
 */
#if (ATOMIC_BOOL_LOCK_FREE != 2) || (ATOMIC_INT_LOCK_FREE != 2)
    #error "ISR logger requires lock-free atomic bool and int"
#endif

/* ============================================================================
 * LOGGER COLOR CONFIGURATION
 * ============================================================================ */

/**
 * @brief Enable or disable ANSI logging color support at compile time.
 *
 * 0U : Exclude ANSI color implementation.
 * 1U : Include ANSI color implementation.
 *
 * Runtime configuration is available only when enabled.
 */
#ifndef bspCONFIG_LOG_COLOR_SUPPORT
    #define bspCONFIG_LOG_COLOR_SUPPORT (0U)
#endif

#if (bspCONFIG_LOG_COLOR_SUPPORT != 0U) && (bspCONFIG_LOG_COLOR_SUPPORT != 1U)
    #error "Invalid bspCONFIG_LOG_COLOR_SUPPORT configuration"
#endif

/* -------------------------------------------------------------------------- */
/* Logger Buffer Configuration                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Maximum task log message size in bytes.
 *
 * @details
 * Defines the maximum capacity of a task-context log message
 * buffer, including the terminating NUL character.
 *
 * The application may override this value at compile time.
 */
#ifndef configLOG_TASK_MESSAGE_MAX
    #define configLOG_TASK_MESSAGE_MAX 160U
#endif

/* -------------------------------------------------------------------------- */

/**
 * @brief Maximum ISR log message size in bytes.
 *
 * @details
 * Defines the maximum capacity of an interrupt-context
 * log message buffer, including the terminating NUL character.
 *
 * The application may override this value at compile time.
 */
#ifndef configLOG_ISR_MESSAGE_MAX
    #define configLOG_ISR_MESSAGE_MAX 96U
#endif

/* -------------------------------------------------------------------------- */

/**
 * @brief Maximum formatted log output size in bytes.
 *
 * @details
 * Defines the maximum capacity of the final formatted
 * log output buffer.
 *
 * Includes the configured log prefix, message text,
 * CRLF characters, and terminating NUL character.
 *
 * The application may override this value at compile time.
 */
#ifndef configLOG_OUTPUT_MAX
    #define configLOG_OUTPUT_MAX 320U
#endif

/* -------------------------------------------------------------------------- */
/* Logger Compile-Time Configuration Validation                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Validate the maximum task log message length.
 *
 * @details
 * Ensures that the configured task message buffer capacity
 * does not exceed the maximum value representable by
 * the uint16_t type.
 *
 * @warning
 * Compilation is terminated if configLOG_TASK_MESSAGE_MAX
 * exceeds the supported 16-bit length range.
 *
 * @par MISRA C Considerations
 * - Prevents potential integer truncation during length storage.
 * - Ensures compatibility with the message length field.
 * - Performs validation without runtime overhead.
 */
#if (configLOG_TASK_MESSAGE_MAX > UINT16_MAX)
    #error "Log message length exceeds uint16_t"
#endif

/* -------------------------------------------------------------------------- */

/**
 * @brief Validate minimum logging buffer capacities.
 *
 * @details
 * Ensures that the configured task message buffer,
 * ISR message buffer, and output buffer satisfy
 * the minimum size requirements.
 *
 * Minimum supported capacities:
 * - LOG_TASK_MESSAGE_MAX : 8 bytes.
 * - LOG_ISR_MESSAGE_MAX  : 2 bytes.
 * - LOG_OUTPUT_MAX       : 48 bytes.
 *
 * @note
 * The minimum capacities provide basic space for
 * message storage and formatting.
 *
 * @warning
 * Compilation is terminated if any configured buffer
 * capacity is below its required minimum.
 *
 * @par MISRA C Considerations
 * - Buffer configuration is validated at compile time.
 * - Invalid configurations are rejected before execution.
 * - Unsigned integer constants are used for size comparisons.
 */
#if (configLOG_TASK_MESSAGE_MAX < 8U) || (configLOG_ISR_MESSAGE_MAX < 2U) || \
(configLOG_OUTPUT_MAX < 48U)
    #error "Logger record or output buffer configuration is too small"
#endif
/* -------------------------------------------------------------------------- */
/* Logger Worker Control Flags                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Thread flag used to start the logging worker.
 *
 * @note This flag bit is reserved exclusively for the logger.
 */
#define LOG_WORKER_START_FLAG    (1UL << 0)

/**
 * @brief Event flag used to acknowledge logging worker shutdown.
 *
 * @note This flag bit is reserved exclusively for the logger.
 */
#define LOG_WORKER_STOP_ACK_FLAG (1UL << 0)


/* ============================================================================
 * LOGGER ANSI COLOR CONFIGURATION
 * ============================================================================ */

#if (bspCONFIG_LOG_COLOR_SUPPORT == 1U)

    #define LOG_COLOR_ERROR         "\x1B[31m"
    #define LOG_COLOR_WARN          "\x1B[33m"
    #define LOG_COLOR_INFO          "\x1B[32m"
    #define LOG_COLOR_DEBUG         "\x1B[36m"
    #define LOG_COLOR_RESET         "\x1B[0m"

    /* Five-byte prefix and four-byte reset sequence. */
    #define LOG_COLOR_PREFIX_LENGTH (5U)
    #define LOG_COLOR_RESET_LENGTH  (4U)
    #define LOG_COLOR_EXTRA_LENGTH  (9U)

#endif // bspCONFIG_LOG_COLOR_SUPPORT

/* -------------------------------------------------------------------------- */
/* Task Logging Record                                                        */
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
/* ISR Logging Record                                                         */
/* -------------------------------------------------------------------------- */

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
/**
 * @brief Internal logging module configuration.
 *
 * @details
 * Stores the validated logging configuration, including
 * RTOS objects, output callbacks, and formatting options.
 *
 * @note
 * Configuration is initialized by log_init() and cleared
 * after successful log_deinit().
 */
static log_task_config_t s_config;

/* -------------------------------------------------------------------------- */
/**
 * @brief Logging module initialization state.
 *
 * @details
 * Indicates whether the logging module has been initialized.
 * Atomic access ensures safe visibility across execution contexts.
 *
 * @note
 * true  : Logging module is initialized.
 * false : Logging module is not initialized.
 */
static _Atomic bool s_initialized = false;

/* Serializes log_init(); this is not the initialized state. */
static _Atomic bool s_initializing = false;

/* -------------------------------------------------------------------------- */

/**
 * @brief Logging module shutdown state.
 *
 * @details
 * Indicates whether the logging module is currently
 * executing its shutdown sequence.
 *
 * @note
 * true  : Shutdown is in progress.
 * false : Normal operation or uninitialized state.
 */
static _Atomic bool s_stopping = false;

/* -------------------------------------------------------------------------- */

/**
 * @brief Logging module enable state.
 *
 * @details
 * Controls whether logging requests are accepted
 * for processing.
 *
 * @note
 * true  : Logging is enabled.
 * false : Logging is disabled.
 */
static _Atomic bool s_enabled = false;

/* -------------------------------------------------------------------------- */

/**
 * @brief Currently configured logging severity threshold.
 *
 * @details
 * Stores the maximum logging severity level permitted
 * for processing.
 *
 * @note
 * The value is updated atomically by log_set_level()
 * and accessed by should_log().
 */
static _Atomic int s_level = LOG_LEVEL_NONE;

/* -------------------------------------------------------------------------- */

/**
 * @brief Sequential logging message counter.
 *
 * @details
 * Maintains the sequence number of emitted log records.
 * The counter is incremented when message numbering
 * is enabled in the logging format configuration.
 *
 * @note
 * Only the logging worker thread modifies this counter
 * during normal operation.
 *
 * @note
 * The counter is reset during log_init().
 *
 * @warning
 * Initialization and reset must not occur concurrently
 * with logging worker execution.
 */
static uint64_t s_message_number = 0U;

/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/* PRIVATE FUNCTION DECLARATIONS                                              */
/* -------------------------------------------------------------------------- */

#if (bspCONFIG_LOG_COLOR_SUPPORT == 1U)
/**
 * @brief Return the ANSI color for a logging severity.
 *
 * @param[in] level Logging severity.
 *
 * @return Read-only ANSI color sequence.
 *
 * @note Called only from the logging worker.
 */
static const char* log_level_color(log_level_t level);
#endif // bspCONFIG_LOG_COLOR_SUPPORT

/**
 * @brief Validate the specified logging severity level.
 *
 * @details
 * Checks whether the supplied logging level is within
 * the supported enumeration range.
 *
 * @param[in] level
 * Logging severity level to validate.
 *
 * @retval true
 * Logging level is within the supported range.
 *
 * @retval false
 * Logging level is outside the supported range.
 *
 * @par MISRA C Considerations
 * - Enumeration boundaries are explicitly checked.
 * - The function has no side effects.
 * - A Boolean result is returned.
 */
static bool level_valid(log_level_t level);

/**
 * @brief Determine whether a log message should be processed.
 *
 * @details
 * Checks the logging module initialization, shutdown,
 * enable state, and configured severity threshold.
 *
 * A log message is permitted only when:
 * - The logging module is initialized.
 * - The logging module is not stopping.
 * - Logging is enabled.
 * - The requested level is greater than LOG_LEVEL_NONE.
 * - The requested level does not exceed the configured level.
 *
 * @param[in] level
 * Severity level of the requested log message.
 *
 * @retval true
 * The log message is permitted for processing.
 *
 * @retval false
 * The log message is filtered or the logging module
 * is not in an operational state.
 *
 * @par MISRA C Considerations
 * - Shared control variables are accessed atomically.
 * - Logical operators are used for Boolean conditions.
 * - Explicit conversion is used for the configured level.
 * - No shared state is modified.
 */
static bool should_log(log_level_t level);


/**
 * @brief Get the abbreviated name of a logging severity level.
 *
 * @details
 * Converts a logging severity enumeration into its
 * corresponding single-character string representation.
 *
 * Supported representations:
 * - LOG_LEVEL_ERROR : "E"
 * - LOG_LEVEL_WARN  : "W"
 * - LOG_LEVEL_INFO  : "I"
 * - LOG_LEVEL_DEBUG : "D"
 *
 * Unsupported levels are represented by "?".
 *
 * @param[in] level
 * Logging severity level to convert.
 *
 * @return
 * Pointer to a constant NUL-terminated string containing
 * the abbreviated logging severity identifier.
 *
 * @note
 * The returned string has static storage duration
 * and must not be modified by the caller.
 *
 * @par MISRA C Considerations
 * - All supported logging levels are explicitly handled.
 * - The switch statement includes a default branch.
 * - Constant string literals are returned.
 * - No dynamic memory allocation is performed.
 */
static const char* level_name(log_level_t level);


/**
 * @brief Append formatted text to a bounded output buffer.
 *
 * @details
 * Formats a variable argument string and appends it to the
 * existing output buffer without exceeding its capacity.
 *
 * The function reserves sufficient space for the final
 * carriage return, line feed, and NUL terminator.
 *
 * @param[in,out] out
 * Pointer to the destination output buffer.
 *
 * @param[in] capacity
 * Total capacity of the destination buffer in bytes.
 *
 * @param[in,out] used
 * Pointer to the number of bytes currently used in the buffer.
 *
 * @param[in] format
 * Pointer to the printf-style format string.
 *
 * @param[in] ...
 * Variable arguments corresponding to the format string.
 *
 * @return
 * None.
 *
 * @pre
 * The output buffer, used pointer, and format string must be valid.
 *
 * @note
 * This function is called only from the logging worker thread.
 *
 * @note
 * Two bytes are reserved for CRLF, with an additional byte
 * reserved for the NUL terminator.
 *
 * @par MISRA C Considerations
 * - Buffer capacity is checked before formatting.
 * - All buffer offsets are bounded by the available capacity.
 * - Variable argument processing is properly terminated.
 * - Explicit conversion is used for formatted string length.
 */
static void append_fmt(char* out, size_t capacity, size_t* used, const char* format, ...);


/**
 * @brief Append a formatted timestamp to the logging buffer.
 *
 * @details
 * Retrieves the current UTC time using the application-provided
 * clock callback and formats it according to the configured
 * timestamp mode.
 *
 * Supported timestamp modes include:
 * - No timestamp.
 * - UTC 24-hour timestamp.
 * - Local 24-hour timestamp with UTC offset.
 *
 * If the clock callback fails or provides an invalid timestamp,
 * an unavailable-time indicator is appended instead.
 *
 * @param[in,out] out
 * Pointer to the destination output buffer.
 *
 * @param[in] capacity
 * Total capacity of the destination buffer in bytes.
 *
 * @param[in,out] used
 * Pointer to the number of bytes currently used in the buffer.
 *
 * @param[in] format
 * Pointer to the logging format configuration.
 *
 * @return
 * None.
 */
static void
append_timestamp(char* out, size_t capacity, size_t* used, const log_format_config_t* format);

/**
 * @brief Append configured logging metadata to the output buffer.
 *
 * @details
 * Constructs the logging prefix according to the enabled
 * formatting options.
 *
 * The prefix may contain:
 * - Timestamp.
 * - Sequential message number.
 * - Logging severity level.
 * - Source file name and line number.
 * - Source function name.
 *
 * @param[in,out] out
 * Pointer to the destination output buffer.
 *
 * @param[in] capacity
 * Total capacity of the destination buffer in bytes.
 *
 * @param[in,out] used
 * Pointer to the number of bytes currently used in the buffer.
 *
 * @param[in] level
 * Severity level associated with the log record.
 *
 * @param[in] file
 * Source file name associated with the log record.
 *
 * @param[in] line
 * Source line number associated with the log record.
 *
 * @param[in] function
 * Source function name associated with the log record.
 *
 * @param[in] format
 * Pointer to the logging format configuration.
 *
 * @return
 * None.
 *
 */
static void append_prefix(char* out,
                          size_t capacity,
                          size_t* used,
                          log_level_t level,
                          const char* file,
                          uint32_t line,
                          const char* function,
                          const log_format_config_t* format);


/**
 * @brief Append a typed ISR value to the logging buffer.
 *
 * @details
 * Formats and appends the value stored in an ISR logging
 * record according to its configured value type.
 *
 * Supported value formats include:
 * - Unsigned integers (8, 16, 32, and 64 bits).
 * - Signed integers (8, 16, 32, and 64 bits).
 * - Hexadecimal integers (8, 16, 32, and 64 bits).
 * - Boolean values.
 * - Character values.
 *
 * @param[in,out] out
 * Pointer to the destination output buffer.
 *
 * @param[in] capacity
 * Total capacity of the destination buffer in bytes.
 *
 * @param[in,out] used
 * Pointer to the number of bytes currently used in the buffer.
 *
 * @param[in] rec
 * Pointer to the ISR logging record containing the
 * value type and associated value.
 *
 * @return
 * None.
 */
static void
append_isr_value(char* out, size_t capacity, size_t* used, const log_isr_record_t* rec);


/**
 * @brief Format and transmit a logging record.
 *
 * @details
 * Constructs a complete log message by appending the configured
 * log prefix, message text, and optional ISR value.
 *
 * The formatted message is terminated with CRLF and passed
 * to the configured output transport callback.
 *
 * @param[in] level
 * Severity level of the log record.
 *
 * @param[in] file
 * Source file name associated with the log record.
 *
 * @param[in] line
 * Source line number associated with the log record.
 *
 * @param[in] function
 * Function name associated with the log record.
 *
 * @param[in] message
 * Pointer to the NUL-terminated log message.
 *
 * @param[in] isr
 * Pointer to an optional ISR log record.
 * NULL indicates that no ISR value is appended.
 *
 * @return
 * None.
 */
static void emit_record(log_level_t level,
                        const char* file,
                        uint32_t line,
                        const char* function,
                        const char* message,
                        const log_isr_record_t* isr);


/**
 * @brief Consume and process one pending logging record.
 *
 * @details
 * Attempts to retrieve a single logging record from either
 * the ISR queue or task queue based on the requested priority.
 *
 * When ISR priority is enabled, the ISR queue is checked first.
 * Otherwise, the task queue is checked first.
 *
 * Retrieved records are formatted and transmitted using
 * the internal emit_record() function.
 *
 * @param[in] isr_first
 * Queue processing priority.
 * - true  : Check ISR queue before task queue.
 * - false : Check task queue before ISR queue.
 *
 * @retval true
 * A logging record was successfully retrieved from a queue.
 *
 * @retval false
 * No logging record was available in the checked queues.
 */
static bool consume_one(bool isr_first);


/**
 * @brief Copy a text message into an ISR logging buffer.
 *
 * @details
 * Copies a NUL-terminated message into the destination buffer
 * without exceeding the specified buffer capacity.
 *
 * The function performs a bounded character-by-character copy
 * and returns failure if the source message cannot fit
 * completely within the destination buffer.
 *
 * @param[out] dst
 * Pointer to the destination message buffer.
 *
 * @param[in] capacity
 * Total capacity of the destination buffer in bytes,
 * including space for the NUL terminator.
 *
 * @param[in] message
 * Pointer to the source NUL-terminated message.
 *
 * @retval true
 * Message copied successfully, including its NUL terminator.
 *
 * @retval false
 * Invalid pointer or insufficient destination capacity.
 *
 */
static bool copy_isr_message(char* dst, size_t capacity, const char* message);

/**
 * @brief Submit a logging record from interrupt context.
 *
 * @details
 * Validates the logging level and attempts to insert
 * the supplied ISR record into the ISR message queue.
 *
 * After successful queue insertion, the logging worker
 * semaphore is released to notify the worker that a
 * new record is available.
 *
 * @param[in] rec
 * Pointer to the ISR logging record to submit.
 *
 * @retval true
 * ISR record queued and worker semaphore released successfully.
 *
 * @retval false
 * Logging level is filtered, queue insertion failed,
 * or worker semaphore release failed.
 *
 */
static bool submit_isr(const log_isr_record_t* rec);

/**
 * @brief Validate the specified logging timestamp mode.
 *
 * @details
 * Checks whether the supplied timestamp mode is within
 * the supported enumeration range.
 *
 * @param[in] mode
 * Logging timestamp mode to validate.
 *
 * @retval true
 * Timestamp mode is within the supported range.
 *
 * @retval false
 * Timestamp mode is outside the supported range.
 */
static bool logger_time_mode_is_local(log_time_mode_t mode);

/**
 * @brief Validate the specified logging timestamp mode.
 *
 * @details
 * Checks whether the supplied timestamp mode is within
 * the supported enumeration range.
 *
 * @param[in] mode
 * Logging timestamp mode to validate.
 *
 * @retval true
 * Timestamp mode is within the supported range.
 *
 * @retval false
 * Timestamp mode is outside the supported range.
 */
static bool logger_time_mode_is_local(log_time_mode_t mode);

/**
 * @brief Validate the specified logging timestamp mode.
 *
 * @details
 * Checks whether the supplied timestamp mode is within
 * the supported enumeration range.
 *
 * @param[in] mode
 * Logging timestamp mode to validate.
 *
 * @retval true
 * Timestamp mode is within the supported range.
 *
 * @retval false
 * Timestamp mode is outside the supported range.
 */
static bool logger_time_mode_valid(log_time_mode_t mode);
/* -------------------------------------------------------------------------- */
/* PUBLIC API                                                                 */
/* -------------------------------------------------------------------------- */

void log_task(void* argument)
{
    bool isr_first = true;
    uint32_t flags;

    /* Mark the unused thread argument explicitly. */
    (void)argument;

    /*
     * Allow the application to create the worker thread
     * before initializing the logging module.
     *
     * CMSIS-RTOS2 retains thread flags even when the
     * receiving thread has not started waiting.
     */

    /* Wait for the logging worker initialization signal. */
    flags = osThreadFlagsWait(LOG_WORKER_START_FLAG, osFlagsWaitAny, osWaitForever);

    /* Terminate the worker if the initial wait fails. */
    if((flags & osFlagsError) != 0U)
    {
        return;
    }

    /* Execute the logging worker processing loop. */
    for(;;)
    {
        /* Wait for a pending log record or shutdown notification. */
        if(osSemaphoreAcquire(s_config.ready_sem, osWaitForever) != osOK)
        {
            continue;
        }

        /* Check whether the logging module is stopping. */
        if(atomic_load(&s_stopping))
        {
            /*
             * Shutdown precondition:
             * No task or ISR producer may submit new
             * logging records during queue draining.
             */

            /* Drain all remaining task and ISR log records. */
            while(consume_one(isr_first))
            {
                /* Alternate queue priority between records. */
                isr_first = !isr_first;
            }

            /* Notify the controller that shutdown is complete. */
            (void)osEventFlagsSet(s_config.control_flags, LOG_WORKER_STOP_ACK_FLAG);

            /* Terminate the logging worker thread. */
            return;
        }

        /* Consume one pending logging record. */
        (void)consume_one(isr_first);

        /* Alternate priority for the next queue operation. */
        isr_first = !isr_first;
    }
}

/* -------------------------------------------------------------------------- */
ret_err_sts_t log_init(const log_task_config_t* config)
{
    uint64_t wake_capacity;
    osKernelState_t kernel;
    uint32_t result;
    bool expected        = false;
    ret_err_sts_t status = RET_ERR_STS_OK;

    /* Validate the configuration pointer. */
    if(config == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Validate application-owned RTOS objects. */
    if((config->worker_thread == NULL) || (config->task_queue == NULL) ||
       (config->isr_queue == NULL) || (config->ready_sem == NULL) ||
       (config->control_flags == NULL) || (config->write == NULL) ||
       (config->format_mutex == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Validate the configured logging level. */
    if(!level_valid(config->initial_level))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Validate all supported time-only, date-time, and Unix modes. */
    if(!logger_time_mode_valid(config->format.time_mode))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* All modes except NONE obtain Unix UTC seconds from the application. */
    if((config->format.time_mode != LOG_TIME_NONE) && (config->get_utc_seconds == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Only local time modes use the UTC offset. */
    if(logger_time_mode_is_local(config->format.time_mode) &&
       ((config->utc_offset_minutes < -840) || (config->utc_offset_minutes > 840)))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

/* Reject color requests when color support is excluded. */
#if (bspCONFIG_LOG_COLOR_SUPPORT == 0U)
    if(config->format.color_enabled)
    {
        return RET_ERR_STS_NOT_SUPPORTED;
    }
#endif // bspCONFIG_LOG_COLOR_SUPPORT

    /* Reserve initialization independently from the initialized flag. */
    if(!atomic_compare_exchange_strong(&s_initializing, &expected, true))
    {
        return RET_ERR_STS_BUSY;
    }

    if(atomic_load(&s_initialized))
    {
        status = RET_ERR_STS_ALREADY_INIT;
        goto exit;
    }

    /* Validate CMSIS kernel state. */
    kernel = osKernelGetState();

    if((kernel != osKernelReady) && (kernel != osKernelRunning))
    {
        status = RET_ERR_STS_INVALID_STATE;
        goto exit;
    }

    /* Validate queue element sizes. */
    if((osMessageQueueGetMsgSize(config->task_queue) != sizeof(log_text_record_t)) ||
       (osMessageQueueGetMsgSize(config->isr_queue) != sizeof(log_isr_record_t)))
    {
        status = RET_ERR_STS_INVALID_SIZE;
        goto exit;
    }

    /* Validate queue capacities. */
    uint32_t task_capacity = osMessageQueueGetCapacity(config->task_queue);
    uint32_t isr_capacity  = osMessageQueueGetCapacity(config->isr_queue);

    if((task_capacity == 0U) || (isr_capacity == 0U))
    {
        status = RET_ERR_STS_INVALID_SIZE;
        goto exit;
    }

    /* Both queues and the wake semaphore must start empty. */
    if((osMessageQueueGetCount(config->task_queue) != 0U) ||
       (osMessageQueueGetCount(config->isr_queue) != 0U) ||
       (osSemaphoreGetCount(config->ready_sem) != 0U))
    {
        status = RET_ERR_STS_INVALID_STATE;
        goto exit;
    }

    /*
     * Calculate required semaphore capacity.
     *
     * CMSIS does not expose the semaphore's maximum count.
     * The application must create it with at least this capacity.
     */
    wake_capacity = (uint64_t)task_capacity + (uint64_t)isr_capacity + 1ULL;

    if(wake_capacity > UINT32_MAX)
    {
        status = RET_ERR_STS_INVALID_PARAM;
        goto exit;
    }

    /* Clear any previous worker stop acknowledgement. */
    result = osEventFlagsClear(config->control_flags, LOG_WORKER_STOP_ACK_FLAG);

    if((result & osFlagsError) != 0U)
    {
        status = RET_ERR_STS_INVALID_STATE;
        goto exit;
    }

    /* Store application configuration. */
    memcpy(&s_config, config, sizeof(s_config));

    /* Reset message numbering. */
    s_message_number = 0U;

    /* Initialize logger state. */
    atomic_store(&s_level, (int)config->initial_level);
    atomic_store(&s_enabled, true);
    atomic_store(&s_stopping, false);

    /*
     * Mark initialized before starting the worker.
     * The worker may run immediately when the kernel is running.
     */
    atomic_store(&s_initialized, true);

    /* Start the application-created logging worker. */
    result = osThreadFlagsSet(config->worker_thread, LOG_WORKER_START_FLAG);

    if((result & osFlagsError) != 0U)
    {
        /* Roll back initialization. */
        atomic_store(&s_initialized, false);
        atomic_store(&s_enabled, false);
        atomic_store(&s_stopping, false);

        memset(&s_config, 0, sizeof(s_config));
        s_message_number = 0U;

        status = RET_ERR_STS_INVALID_STATE;
        goto exit;
    }

exit:
    /* Preserve s_initialized on successful initialization. */
    atomic_store(&s_initializing, false);
    return status;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t log_deinit(void)
{
    uint32_t ack;
    uint32_t timeout;

    /* Verify that the logging module is initialized. */
    if(!atomic_load(&s_initialized))
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /* Validate kernel state and calling thread context. */
    if((osKernelGetState() != osKernelRunning) || (osThreadGetId() == s_config.worker_thread))
    {
        return RET_ERR_STS_INVALID_STATE;
    }

    /*
     * All task and ISR log producers must be quiesced
     * before initiating the shutdown sequence.
     */

    /* Atomically transition the logging module to stopping state. */
    if(!atomic_exchange(&s_stopping, true))
    {
        /* Disable further log submissions. */
        atomic_store(&s_enabled, false);

        /* Wake the logging worker to process the shutdown request. */
        if(osSemaphoreRelease(s_config.ready_sem) != osOK)
        {
            return RET_ERR_STS_FAIL;
        }
    }

    /* Select the configured or default shutdown timeout. */
    timeout = (s_config.shutdown_timeout_ticks != 0U) ? s_config.shutdown_timeout_ticks : 1000U;

    /* Wait for the worker shutdown acknowledgement. */
    ack = osEventFlagsWait(s_config.control_flags, LOG_WORKER_STOP_ACK_FLAG,
                           osFlagsWaitAny, timeout);

    /* Report failure if the acknowledgement wait fails. */
    if((ack & osFlagsError) != 0U)
    {
        return RET_ERR_STS_TIMEOUT;
    }

    /*
     * The application must create the worker thread with
     * osThreadJoinable to guarantee complete termination
     * before releasing associated RTOS resources.
     */

    /* Wait until the logging worker thread has terminated. */
    if(osThreadJoin(s_config.worker_thread) != osOK)
    {
        return RET_ERR_STS_FAIL;
    }

    /* Reset shared logging state after worker termination. */
    atomic_store(&s_initialized, false);
    atomic_store(&s_level, LOG_LEVEL_NONE);

    /* Clear the internal logging configuration. */
    (void)memset(&s_config, 0, sizeof(s_config));

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t log_enable(bool enable)
{
    /* Validate initialization and shutdown states. */
    if((!atomic_load(&s_initialized)) || (atomic_load(&s_stopping)))
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /* Atomically update the logging enable state. */
    atomic_store(&s_enabled, enable);

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

bool log_is_enabled(void)
{
    /* Return the effective logging enable state. */
    return (atomic_load(&s_initialized) && !atomic_load(&s_stopping) &&
            atomic_load(&s_enabled));
}

/* -------------------------------------------------------------------------- */

ret_err_sts_t log_set_format(const log_format_config_t* format)
{
    if(format == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(!logger_time_mode_valid(format->time_mode))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(!atomic_load(&s_initialized))
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(atomic_load(&s_stopping))
    {
        return RET_ERR_STS_BUSY;
    }

    if((format->time_mode != LOG_TIME_NONE) && (s_config.get_utc_seconds == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(logger_time_mode_is_local(format->time_mode) &&
       ((s_config.utc_offset_minutes < -840) || (s_config.utc_offset_minutes > 840)))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Validate compile-time color availability. */
#if (bspCONFIG_LOG_COLOR_SUPPORT == 0U)
    if(format->color_enabled)
    {
        return RET_ERR_STS_NOT_SUPPORTED;
    }
#endif // bspCONFIG_LOG_COLOR_SUPPORT

    if(osMutexAcquire(s_config.format_mutex, osWaitForever) != osOK)
    {
        return RET_ERR_STS_FAIL;
    }

    memcpy(&s_config.format, format, sizeof(s_config.format));

    (void)osMutexRelease(s_config.format_mutex);

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */
ret_err_sts_t log_get_format(log_format_config_t* format)
{
    if(format == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(!atomic_load(&s_initialized))
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(osMutexAcquire(s_config.format_mutex, osWaitForever) != osOK)
    {
        return RET_ERR_STS_FAIL;
    }

    memcpy(format, &s_config.format, sizeof(s_config.format));

    (void)osMutexRelease(s_config.format_mutex);

    return RET_ERR_STS_OK;
}
/* -------------------------------------------------------------------------- */

ret_err_sts_t log_set_level(log_level_t level)
{
    /* Validate the requested logging level. */
    if(!level_valid(level))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Verify that the module is initialized and operational. */
    if((!atomic_load(&s_initialized)) || (atomic_load(&s_stopping)))
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /* Atomically update the configured logging level. */
    atomic_store(&s_level, (int)level);

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

log_level_t log_get_level(void)
{
    return (log_level_t)atomic_load(&s_level);
}

/* -------------------------------------------------------------------------- */

void log_printf(log_level_t level, const char* file, uint32_t line, const char* function, const char* fmt, ...)
{
    log_text_record_t rec;
    int written;
    va_list args;

    /* Validate format string and configured logging level. */
    if((fmt == NULL) || (!should_log(level)))
    {
        return;
    }

    /* Initialize the complete log record. */
    (void)memset(&rec, 0, sizeof(rec));

    /* Populate log metadata. */
    rec.level    = level;
    rec.file     = file;
    rec.line     = line;
    rec.function = function;

    /* Format the variable argument message. */
    va_start(args, fmt);

    written = vsnprintf(rec.message, sizeof(rec.message), fmt, args);

    va_end(args);

    /* Reject messages if formatting fails. */
    if(written < 0)
    {
        return;
    }

    /*
     * Calculate the valid message length.
     * Truncated messages remain NUL terminated.
     */
    rec.length = (uint16_t)(((size_t)written < sizeof(rec.message)) ?
                            (size_t)written :
                            (sizeof(rec.message) - 1U));

    /* Submit the formatted record to the logging queue. */
    if(osMessageQueuePut(s_config.task_queue, &rec, 0U, 0U) == osOK)
    {
        /* Notify the logging task that data is available. */
        (void)osSemaphoreRelease(s_config.ready_sem);
    }
}

/* -------------------------------------------------------------------------- */

bool log_printf_isr(log_level_t level, const char* file, uint32_t line, const char* function, const char* message)
{
    log_isr_record_t rec = { 0 };

    /* Validate message pointer and configured logging level. */
    if((message == NULL) || (!should_log(level)))
    {
        return false;
    }

    /* Populate ISR log metadata. */
    rec.level    = level;
    rec.file     = file;
    rec.line     = line;
    rec.function = function;
    rec.type     = LOG_ISR_VALUE_NONE;

    /* Copy the message into the ISR log record. */
    if(!copy_isr_message(rec.message, sizeof(rec.message), message))
    {
        return false;
    }

    /* Submit the record through the ISR-safe logging path. */
    return submit_isr(&rec);
}

/* -------------------------------------------------------------------------- */
bool log_isr_value(log_level_t level,
                   const char* file,
                   uint32_t line,
                   const char* function,
                   const char* message,
                   log_isr_value_type_t type,
                   log_isr_value_t value)
{
    log_isr_record_t rec = { 0 };

    /* Validate message, logging level, and value type. */
    if((message == NULL) || (!should_log(level)) ||
       (type <= LOG_ISR_VALUE_NONE) || (type > LOG_ISR_VALUE_CHAR))
    {
        return false;
    }

    /* Populate ISR log metadata. */
    rec.level    = level;
    rec.file     = file;
    rec.line     = line;
    rec.function = function;
    rec.type     = type;
    rec.value    = value;

    /* Copy the message into the ISR log record. */
    if(!copy_isr_message(rec.message, sizeof(rec.message), message))
    {
        return false;
    }

    /* Submit the record through the ISR-safe logging path. */
    return submit_isr(&rec);
}


/* -------------------------------------------------------------------------- */
/* PRIVATE FUNCTIONS                                                          */
/* -------------------------------------------------------------------------- */

#if (bspCONFIG_LOG_COLOR_SUPPORT == 1U)

/**
 * @brief Return the ANSI color for a logging severity.
 *
 * @param[in] level Logging severity.
 *
 * @return Read-only ANSI color sequence.
 *
 * @note Called only from the logging worker.
 */
static const char* log_level_color(log_level_t level)
{
    const char* color;

    switch(level)
    {
        case LOG_LEVEL_ERROR:
        {
            color = LOG_COLOR_ERROR;
            break;
        }

        case LOG_LEVEL_WARN:
        {
            color = LOG_COLOR_WARN;
            break;
        }

        case LOG_LEVEL_INFO:
        {
            color = LOG_COLOR_INFO;
            break;
        }

        case LOG_LEVEL_DEBUG:
        {
            color = LOG_COLOR_DEBUG;
            break;
        }

        default:
        {
            color = "";
            break;
        }
    }

    return color;
}

#endif // bspCONFIG_LOG_COLOR_SUPPORT

/* -------------------------------------------------------------------------- */
static bool logger_time_mode_valid(log_time_mode_t mode)
{
    switch(mode)
    {
        case LOG_TIME_NONE:
        case LOG_TIME_UTC:
        case LOG_TIME_LOCAL_24H:
        case LOG_DATE_TIME_UTC:
        case LOG_DATE_TIME_LOCAL_24H:
        case LOG_TIME_UNIX_SECONDS:
            return true;

        default:
            return false;
    }
}

/* -------------------------------------------------------------------------- */
static bool logger_time_mode_is_local(log_time_mode_t mode)
{
    return ((mode == LOG_TIME_LOCAL_24H) || (mode == LOG_DATE_TIME_LOCAL_24H));
}

/* -------------------------------------------------------------------------- */

static const char* file_name(const char* path)
{
    const char* name;
    const char* current;

    /* Handle an unavailable source file path. */
    if(path == NULL)
    {
        return "?";
    }

    /* Assume the complete input represents a filename. */
    name    = path;
    current = path;

    /* Locate the final path separator. */
    while(*current != '\0')
    {
        if((*current == '/') || (*current == '\\'))
        {
            /* The filename starts after the separator. */
            name = current + 1;
        }

        ++current;
    }

    return name;
}

/* -------------------------------------------------------------------------- */

static bool copy_isr_message(char* dst, size_t capacity, const char* message)
{
    size_t i;

    /* Validate source and destination buffer pointers. */
    if((dst == NULL) || (message == NULL))
    {
        return false;
    }

    /* Copy characters without exceeding destination capacity. */
    for(i = 0U; i < capacity; ++i)
    {
        /* Check whether the source message has terminated. */
        if(message[i] == '\0')
        {
            /* Terminate the destination message. */
            dst[i] = '\0';

            return true;
        }

        /* Reject messages that cannot fit with a terminator. */
        if(i == (capacity - 1U))
        {
            return false;
        }

        /* Copy the current source character. */
        dst[i] = message[i];
    }

    /* Return failure if no valid terminator was copied. */
    return false;
}

/* -------------------------------------------------------------------------- */

static bool submit_isr(const log_isr_record_t* rec)
{
    /* Verify that the logging level is currently enabled. */
    if(!should_log(rec->level))
    {
        return false;
    }

    /* Submit the ISR record using a nonblocking queue operation. */
    if(osMessageQueuePut(s_config.isr_queue, rec, 0U, 0U) != osOK)
    {
        return false;
    }

    /*
     * The ready semaphore maximum count must support:
     * - Task queue capacity.
     * - ISR queue capacity.
     * - One additional shutdown notification.
     */

    /* Notify the logging worker of the pending ISR record. */
    return (osSemaphoreRelease(s_config.ready_sem) == osOK);
}

/* -------------------------------------------------------------------------- */


static bool level_valid(log_level_t level)
{
    /* Validate the logging level against supported boundaries. */
    return ((level >= LOG_LEVEL_NONE) && (level <= LOG_LEVEL_DEBUG));
}

/* -------------------------------------------------------------------------- */

static bool should_log(log_level_t level)
{
    /* Validate module state and the configured logging threshold. */
    return (atomic_load(&s_initialized) && !atomic_load(&s_stopping) &&
            atomic_load(&s_enabled) && (level > LOG_LEVEL_NONE) &&
            (level <= (log_level_t)atomic_load(&s_level)));
}

/* -------------------------------------------------------------------------- */

static const char* level_name(log_level_t level)
{
    /* Convert the logging level to its abbreviated identifier. */
    switch(level)
    {
        case LOG_LEVEL_ERROR:
        {
            return "E";
        }

        case LOG_LEVEL_WARN:
        {
            return "W";
        }

        case LOG_LEVEL_INFO:
        {
            return "I";
        }

        case LOG_LEVEL_DEBUG:
        {
            return "D";
        }

        default:
        {
            return "?";
        }
    }
}

/* -------------------------------------------------------------------------- */

static void append_fmt(char* out, size_t capacity, size_t* used, const char* format, ...)
{
    int result;
    size_t available;
    va_list args;

    /* Verify sufficient capacity for formatted data and CRLF. */
    if((*used >= capacity) || ((capacity - *used) <= 3U))
    {
        return;
    }

    /* Reserve space for the final CRLF characters. */
    available = capacity - *used - 2U;

    /* Initialize the variable argument list. */
    va_start(args, format);

    /* Append formatted text within the available buffer space. */
    result = vsnprintf(out + *used, available, format, args);

    /* Release the variable argument list. */
    va_end(args);

    /* Reject the formatting result if an error occurred. */
    if(result < 0)
    {
        return;
    }

    /* Update the used length without exceeding buffer capacity. */
    *used += ((size_t)result < available) ? (size_t)result : (available - 1U);
}

/* -------------------------------------------------------------------------- */

/* Convert days since 1970-01-01 to a proleptic Gregorian date.
 * Uses integer arithmetic only; no platform time library is required.
 * Validated output range: years 0001..9999.
 */
static bool logger_calendar_from_days(int64_t days, unsigned* year, unsigned* month, unsigned* day)
{
    int64_t z;
    int64_t era;
    uint32_t doe;
    uint32_t yoe;
    int64_t y;
    uint32_t doy;
    uint32_t mp;
    int32_t m;

    z = days + INT64_C(719468);
    era = (z >= 0) ? (z / INT64_C(146097)) : ((z - INT64_C(146096)) / INT64_C(146097));
    doe = (uint32_t)(z - (era * INT64_C(146097)));
    yoe = (doe - (doe / 1460U) + (doe / 36524U) - (doe / 146096U)) / 365U;
    y   = (int64_t)yoe + (era * INT64_C(400));
    doy = doe - ((365U * yoe) + (yoe / 4U) - (yoe / 100U));
    mp  = ((5U * doy) + 2U) / 153U;
    m   = (int32_t)mp + ((mp < 10U) ? 3 : -9);
    y += (m <= 2) ? INT64_C(1) : INT64_C(0);

    if((y < INT64_C(1)) || (y > INT64_C(9999)))
    {
        return false;
    }

    *year  = (unsigned)y;
    *month = (unsigned)m;
    *day   = (unsigned)(doy - (((153U * mp) + 2U) / 5U) + 1U);
    return true;
}

static void append_timestamp(char* out, size_t capacity, size_t* used, const log_format_config_t* format)
{
    uint64_t utc_seconds = UINT64_C(0);
    int32_t offset       = 0;
    int64_t seconds_in_day;
    int64_t days;
    unsigned hours;
    unsigned minutes;
    unsigned seconds;
    unsigned year;
    unsigned month;
    unsigned day;
    unsigned off_h;
    unsigned off_m;
    int64_t abs_offset;
    char sign;

    if((out == NULL) || (used == NULL) || (format == NULL) ||
       (capacity == 0U) || (*used >= capacity))
    {
        return;
    }

    if(format->time_mode == LOG_TIME_NONE)
    {
        return;
    }

    /* The application provides Unix UTC seconds, never a local epoch. */
    if((s_config.get_utc_seconds == NULL) ||
       (!s_config.get_utc_seconds(&utc_seconds, s_config.clock_context)))
    {
        append_fmt(out, capacity, used, "[TIME?] ");
        return;
    }

    if(format->time_mode == LOG_TIME_UNIX_SECONDS)
    {
        append_fmt(out, capacity, used, "[%" PRIu64 "] ", utc_seconds);
        return;
    }

    if(logger_time_mode_is_local(format->time_mode))
    {
        offset = (s_config.get_utc_offset_minutes != NULL) ?
                 s_config.get_utc_offset_minutes(utc_seconds, s_config.clock_context) :
                 s_config.utc_offset_minutes;

        if((offset < -840) || (offset > 840))
        {
            append_fmt(out, capacity, used, "[TIME?] ");
            return;
        }
    }

    /* Time of day is sufficient for time-only modes. */
    seconds_in_day =
    (int64_t)(utc_seconds % UINT64_C(86400)) + ((int64_t)offset * INT64_C(60));

    if(seconds_in_day < 0)
    {
        seconds_in_day += INT64_C(86400);
    }
    else if(seconds_in_day >= INT64_C(86400))
    {
        seconds_in_day -= INT64_C(86400);
    }

    hours   = (unsigned)(seconds_in_day / INT64_C(3600));
    minutes = (unsigned)((seconds_in_day / INT64_C(60)) % INT64_C(60));
    seconds = (unsigned)(seconds_in_day % INT64_C(60));

    if((format->time_mode == LOG_DATE_TIME_UTC) || (format->time_mode == LOG_DATE_TIME_LOCAL_24H))
    {
        /* Bound calendar conversion to printable YYYY values. */
        if(utc_seconds > UINT64_C(253402300799))
        {
            append_fmt(out, capacity, used, "[TIME?] ");
            return;
        }

        days = (int64_t)(utc_seconds / UINT64_C(86400));

        /* Local offset can cross into the previous or next date. */
        if(logger_time_mode_is_local(format->time_mode))
        {
            int64_t local_s = (int64_t)(utc_seconds % UINT64_C(86400)) +
                              ((int64_t)offset * INT64_C(60));
            if(local_s < 0)
            {
                --days;
            }
            else if(local_s >= INT64_C(86400))
            {
                ++days;
            }
        }

        if(!logger_calendar_from_days(days, &year, &month, &day))
        {
            append_fmt(out, capacity, used, "[TIME?] ");
            return;
        }
    }

    if(logger_time_mode_is_local(format->time_mode))
    {
        sign       = (offset < 0) ? '-' : '+';
        abs_offset = (offset < 0) ? -(int64_t)offset : (int64_t)offset;
        off_h      = (unsigned)(abs_offset / INT64_C(60));
        off_m      = (unsigned)(abs_offset % INT64_C(60));
    }
    else
    {
        sign  = '+';
        off_h = 0U;
        off_m = 0U;
    }

    switch(format->time_mode)
    {
        case LOG_TIME_UTC:
            append_fmt(out, capacity, used, "[%02u:%02u:%02u UTC] ", hours, minutes, seconds);
            break;

        case LOG_TIME_LOCAL_24H:
            append_fmt(out, capacity, used, "[%02u:%02u:%02u %c%02u:%02u] ",
                       hours, minutes, seconds, sign, off_h, off_m);
            break;

        case LOG_DATE_TIME_UTC:
            append_fmt(out, capacity, used, "[%04u-%02u-%02u %02u:%02u:%02u UTC] ",
                       year, month, day, hours, minutes, seconds);
            break;

        case LOG_DATE_TIME_LOCAL_24H:
            append_fmt(out, capacity, used, "[%04u-%02u-%02u %02u:%02u:%02u %c%02u:%02u] ",
                       year, month, day, hours, minutes, seconds, sign, off_h, off_m);
            break;

        default:
            /* NONE and UNIX_SECONDS returned above. */
            break;
    }
}
/* -------------------------------------------------------------------------- */

static void append_prefix(char* out,
                          size_t capacity,
                          size_t* used,
                          log_level_t level,
                          const char* file,
                          uint32_t line,
                          const char* function,
                          const log_format_config_t* format)
{
    /* Append the configured timestamp prefix. */
    append_timestamp(out, capacity, used, format);

    /* Append the sequential message number when enabled. */
    if(format->message_number)
    {
        /* Increment the logging message sequence counter. */
        ++s_message_number;

        /* Append the zero-padded message sequence number. */
        append_fmt(out, capacity, used, "*%06" PRIu64 " ", s_message_number);
    }

    /* Append the logging severity level when enabled. */
    if(format->level)
    {
        append_fmt(out, capacity, used, "[%s] ", level_name(level));
    }

    /* Append the source file name and line number when enabled. */
    if(format->file_line)
    {
        append_fmt(out, capacity, used, "%s:%" PRIu32 " ", file_name(file), line);
    }

    /* Append the source function name when enabled. */
    if(format->function)
    {
        append_fmt(out, capacity, used, "%s() ", (function != NULL) ? function : "?");
    }
}

/* -------------------------------------------------------------------------- */

static void append_isr_value(char* out, size_t capacity, size_t* used, const log_isr_record_t* rec)
{
    /* Select the formatting operation based on the ISR value type. */
    switch(rec->type)
    {
        case LOG_ISR_VALUE_NONE:
        {
            /* No associated value requires formatting. */
            break;
        }

        case LOG_ISR_VALUE_U8:
        {
            /* Append an unsigned 8-bit integer value. */
            append_fmt(out, capacity, used, ": %u", (unsigned)rec->value.u8);
            break;
        }

        case LOG_ISR_VALUE_U16:
        {
            /* Append an unsigned 16-bit integer value. */
            append_fmt(out, capacity, used, ": %u", (unsigned)rec->value.u16);
            break;
        }

        case LOG_ISR_VALUE_U32:
        {
            /* Append an unsigned 32-bit integer value. */
            append_fmt(out, capacity, used, ": %" PRIu32, rec->value.u32);
            break;
        }

        case LOG_ISR_VALUE_U64:
        {
            /* Append an unsigned 64-bit integer value. */
            append_fmt(out, capacity, used, ": %" PRIu64, rec->value.u64);
            break;
        }

        case LOG_ISR_VALUE_I8:
        {
            /* Append a signed 8-bit integer value. */
            append_fmt(out, capacity, used, ": %d", (int)rec->value.i8);
            break;
        }

        case LOG_ISR_VALUE_I16:
        {
            /* Append a signed 16-bit integer value. */
            append_fmt(out, capacity, used, ": %d", (int)rec->value.i16);
            break;
        }

        case LOG_ISR_VALUE_I32:
        {
            /* Append a signed 32-bit integer value. */
            append_fmt(out, capacity, used, ": %" PRId32, rec->value.i32);
            break;
        }

        case LOG_ISR_VALUE_I64:
        {
            /* Append a signed 64-bit integer value. */
            append_fmt(out, capacity, used, ": %" PRId64, rec->value.i64);
            break;
        }

        case LOG_ISR_VALUE_HEX8:
        {
            /* Append an 8-bit hexadecimal value. */
            append_fmt(out, capacity, used, ": 0x%02X", (unsigned)rec->value.u8);
            break;
        }

        case LOG_ISR_VALUE_HEX16:
        {
            /* Append a 16-bit hexadecimal value. */
            append_fmt(out, capacity, used, ": 0x%04X", (unsigned)rec->value.u16);
            break;
        }

        case LOG_ISR_VALUE_HEX32:
        {
            /* Append a 32-bit hexadecimal value. */
            append_fmt(out, capacity, used, ": 0x%08" PRIX32, rec->value.u32);
            break;
        }

        case LOG_ISR_VALUE_HEX64:
        {
            /* Append a 64-bit hexadecimal value. */
            append_fmt(out, capacity, used, ": 0x%016" PRIX64, rec->value.u64);
            break;
        }

        case LOG_ISR_VALUE_BOOL:
        {
            /* Append the Boolean value as readable text. */
            append_fmt(out, capacity, used, ": %s", rec->value.boolean ? "true" : "false");
            break;
        }

        case LOG_ISR_VALUE_CHAR:
        {
            /* Append the associated character value. */
            append_fmt(out, capacity, used, ": %c", rec->value.character);
            break;
        }

        default:
        {
            /* Ignore unsupported ISR value types. */
            break;
        }
    }
}

/* -------------------------------------------------------------------------- */

static void emit_record(log_level_t level,
                        const char* file,
                        uint32_t line,
                        const char* function,
                        const char* message,
                        const log_isr_record_t* isr)
{
    size_t used = 0U;
    size_t format_capacity;
    log_format_config_t format;

#if (bspCONFIG_LOG_COLOR_SUPPORT == 1U)
    char output[configLOG_OUTPUT_MAX + LOG_COLOR_EXTRA_LENGTH];
    bool use_color;
    const char* color;
#else
    char output[configLOG_OUTPUT_MAX];
#endif // bspCONFIG_LOG_COLOR_SUPPORT

    /* Obtain a consistent runtime format snapshot. */
    if(osMutexAcquire(s_config.format_mutex, osWaitForever) != osOK)
    {
        return;
    }

    format = s_config.format;

    if(osMutexRelease(s_config.format_mutex) != osOK)
    {
        return;
    }

    /* Initialize the output buffer as an empty string. */
    output[0] = '\0';

    format_capacity = sizeof(output);

#if (bspCONFIG_LOG_COLOR_SUPPORT == 1U)

    use_color = format.color_enabled;
    color     = log_level_color(level);

    if(use_color && (color[0] != '\0'))
    {
        /*
         * Reserve four bytes for ANSI color reset.
         * append_fmt() already reserves CRLF and NUL.
         */
        format_capacity -= LOG_COLOR_RESET_LENGTH;

        /* Append the five-byte ANSI color prefix. */
        (void)memcpy(&output[used], color, LOG_COLOR_PREFIX_LENGTH);

        used += LOG_COLOR_PREFIX_LENGTH;
        output[used] = '\0';
    }
    else
    {
        use_color = false;
    }

#endif // bspCONFIG_LOG_COLOR_SUPPORT

    /* Append configured metadata. */
    append_prefix(output, format_capacity, &used, level, file, line, function, &format);

    /* Append the log message. */
    append_fmt(output, format_capacity, &used, "%s", message);

    /* Append ISR value when present. */
    if(isr != NULL)
    {
        append_isr_value(output, format_capacity, &used, isr);
    }

#if (bspCONFIG_LOG_COLOR_SUPPORT == 1U)

    /* Restore the terminal color before the newline. */
    if(use_color)
    {
        (void)memcpy(&output[used], LOG_COLOR_RESET, LOG_COLOR_RESET_LENGTH);
        used += LOG_COLOR_RESET_LENGTH;
    }

#endif // bspCONFIG_LOG_COLOR_SUPPORT

    /* Append carriage return and line feed characters. */
    output[used++] = '\r';
    output[used++] = '\n';

    /* Ensure the final output is NUL terminated. */
    output[used] = '\0';

    /*
     * The transport callback must consume or copy the
     * output buffer before this function returns.
     */

    /* Transmit the formatted logging record. */
    (void)s_config.write((const uint8_t*)output, used, s_config.write_context);
}

/* -------------------------------------------------------------------------- */

static bool consume_one(bool isr_first)
{
    log_text_record_t task;
    log_isr_record_t isr;

    /* Attempt to process an ISR record first when requested. */
    if(isr_first && (osMessageQueueGet(s_config.isr_queue, &isr, NULL, 0U) == osOK))
    {
        /* Format and transmit the retrieved ISR record. */
        emit_record(isr.level, isr.file, isr.line, isr.function, isr.message, &isr);

        return true;
    }

    /* Attempt to retrieve a pending task logging record. */
    if(osMessageQueueGet(s_config.task_queue, &task, NULL, 0U) == osOK)
    {
        /* Validate the task message length and NUL terminator. */
        if((task.length < configLOG_TASK_MESSAGE_MAX) && (task.message[task.length] == '\0'))
        {
            /* Format and transmit the retrieved task record. */
            emit_record(task.level, task.file, task.line, task.function, task.message, NULL);
        }

        return true;
    }

    /* Attempt to process an ISR record after the task queue. */
    if((!isr_first) && (osMessageQueueGet(s_config.isr_queue, &isr, NULL, 0U) == osOK))
    {
        /* Format and transmit the retrieved ISR record. */
        emit_record(isr.level, isr.file, isr.line, isr.function, isr.message, &isr);

        return true;
    }

    /* No pending logging record was retrieved. */
    return false;
}

/* -------------------------------------------------------------------------- */
