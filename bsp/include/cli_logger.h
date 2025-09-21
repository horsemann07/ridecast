/**
 * @file logger.h
 * @brief Custom logging mechanism using Embedded CLI as backend.
 *
 * This module provides a unified logging interface with support
 * for different log levels (INFO, WARN, ERROR, DEBUG).
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "bsp_uart.h"
#include "bsp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Logger buffer size  */
#define LOGGER_TASK_NAME       ("LoggerTask")
#define LOGGER_BUFFER_SIZE     ((uint32_t)256)
#define LOGGER_QUEUE_SIZE      ((uint32_t)32)
#define LOGGER_TASK_STACK_SIZE ((uint32_t)512)
#define LOGGER_TASK_PRIORITY   (osPriorityNormal)

#define CLI_RX_BUFFER_SIZE     128
#define CLI_TX_BUFFER_SIZE     255
#define CLI_BUFFER_SIZE        128
#define CLI_TX_BUFFER_SIZE     255
    /**
     * @brief Log levels
     */
    typedef enum
    {
        LOGGER_LEVEL_ERROR = 0,
        LOGGER_LEVEL_WARN,
        LOGGER_LEVEL_INFO,
        LOGGER_LEVEL_DEBUG
    } LoggerLevel_t;

    /**
     * @brief Initialize the logger system.
     *
     * Typically sets up UART/CLI backend.
     */
    errStatus_t cliLoggerInit(bspUartHandle_t handle);

    /**
     * @brief Initialize the logger system.
     *
     * Typically sets up UART/CLI backend.
     */
    errStatus_t cliLoggerDeInit(bspUartHandle_t handle);

    /**
     * @brief Log a message with a specific log level.
     *
     * @param[in] level Log level (INFO, WARN, ERROR, DEBUG).
     * @param[in] file Source file name (for context).
     * @param[in] line Line number in source file (for context).
     * @param[in] fmt printf-style format string.
     * @param[in] ... Additional arguments for format string.
     */
    void cliLoggerLog(LoggerLevel_t level, const uint8_t* file, int line, const char* fmt, ...);


    /**
     * @brief Optional: flush logs immediately
     */
    errStatus_t cliLoggerFlush(void);

/**
 * @brief Convenience macros for logging.
 */
#if LOGGER_INCLUDE_FILELINE
    // Macro to get basename of __FILE__
    #define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)

    #define BSP_LOGI(fmt, ...) \
        cliLoggerLog(LOGGER_LEVEL_INFO, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)
    #define BSP_LOGE(fmt, ...) \
        cliLoggerLog(LOGGER_LEVEL_ERROR, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)
    #define BSP_LOGW(fmt, ...) \
        cliLoggerLog(LOGGER_LEVEL_WARN, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)
    #define BSP_LOGD(fmt, ...) \
        cliLoggerLog(LOGGER_LEVEL_DEBUG, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)
#else
    #define BSP_LOGI(fmt, ...) \
        cliLoggerLog(LOGGER_LEVEL_INFO, NULL, 0, fmt, ##__VA_ARGS__)
    #define BSP_LOGE(fmt, ...) \
        cliLoggerLog(LOGGER_LEVEL_ERROR, NULL, 0, fmt, ##__VA_ARGS__)
    #define BSP_LOGW(fmt, ...) \
        cliLoggerLog(LOGGER_LEVEL_WARN, NULL, 0, fmt, ##__VA_ARGS__)
    #define BSP_LOGD(fmt, ...) \
        cliLoggerLog(LOGGER_LEVEL_DEBUG, NULL, 0, fmt, ##__VA_ARGS__)
#endif
#ifdef __cplusplus
}
#endif

#endif /* LOGGER_H */
