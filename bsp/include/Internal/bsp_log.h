/**
 * @file bsp_log.h
 * @brief BSP logging and CLI public interface.
 *
 * This header declares the public BSP-level logging API used to output
 * formatted diagnostic and debug messages over UART and to interact
 * with the Embedded CLI.
 *
 * The API provides:
 * - BSP-scoped log severity levels
 * - Thread-safe logging macros
 * - Initialization and deinitialization functions
 *
 * This interface is intended to be used by application and middleware
 * layers and hides all internal implementation details such as UART
 * callbacks, RTOS synchronization, and CLI integration.
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
 * - BSP UART driver must be initialized before calling bsp_log_init().
 * - CMSIS-RTOS2 kernel must be running.
 *
 * @par Thread Safety
 * - All logging macros and APIs are thread-safe.
 * - Log output is serialized internally using an RTOS mutex.
 *
 * @par Notes
 * - Only one UART instance is supported for BSP logging.
 * - Logging output is best-effort and may be dropped if UART resources
 *   are unavailable.
 *
 * @see bsp_log.c
 * @see bsp_uart.h
 */


#ifndef BSP_LOG_H
#define BSP_LOG_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "bsp_config.h"
#include "bsp_err_sts.h"
#include "bsp_uart.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* -------------------------------------------------- */
    /* BSP LOGGER LEVEL ENUM                              */
    /* -------------------------------------------------- */

    /**
     * @brief BSP logger severity levels.
     *
     * These values are used with bsp_log() and related macros
     * to indicate the severity of a log message.
     */
    typedef enum
    {
        BSP_LOG_NONE  = 0, /*<! Disable logging */
        BSP_LOG_WARN  = 1, /*<! Warning conditions */
        BSP_LOG_ERROR = 2, /*<! Error conditions */
        BSP_LOG_INFO  = 3, /*<! Informational messages */
        BSP_LOG_DEBUG = 4  /*<! Debug / verbose messages */
    } bspLogLevel_t;

    /* -------------------------------------------------- */
    /* PUBLIC API                                        */
    /* -------------------------------------------------- */
    /**
     * @brief Enable or disable BSP logging at runtime.
     *
     * @param[in] enable true = enable logging, false = disable logging.
     *
     * @return BSP_ERR_STS_OK Always succeeds.
     */
    bsp_err_sts_t bsp_log_enable(bool enable);

    /**
     * @brief Get current BSP logging enable state.
     *
     * @return true if logging is enabled, otherwise false.
     */
    bool bsp_log_is_enabled(void);

    /**
     * @brief Set runtime BSP log level.
     *
     * @param[in] level BSP logger level.
     *
     * @return BSP_ERR_STS_OK Always succeeds.
     */
    bsp_err_sts_t bsp_log_set_level(bspLogLevel_t level);

    /**
     * @brief Get current BSP log level.
     *
     * @return Current BSP logger level.
     */
    bspLogLevel_t bsp_log_get_level(void);

    /**
     * @brief Log a formatted message via BSP logger.
     *
     * @param[in] level Log severity level (BSP_LOG_xxx).
     * @param[in] file  Source file name (typically __FILE__).
     * @param[in] line  Source line number (typically __LINE__).
     * @param[in] fmt   printf-style format string.
     * @param[in] ...   Format arguments.
     *
     * @note Thread-safe.
     */
    void bsp_log(bspLogLevel_t level, const char* file, int line, const char* fmt, ...);

    /**
     * @brief Initialize BSP logger and Embedded CLI over UART.
     *
     * @param[in] uartHandle Pointer to initialized BSP UART handle.
     *
     * @return BSP_ERR_STS_OK            Initialization successful.
     * @return BSP_ERR_STS_INVALID_PARAM Invalid UART handle.
     * @return BSP_ERR_STS_NO_MEM        RTOS resource allocation failed.
     * @return BSP_ERR_STS_FAIL          Initialization failed.
     */
    bsp_err_sts_t bsp_log_init(bspUartHandle_t* uartHandle);

    /**
     * @brief Deinitialize BSP logger and CLI.
     *
     * @return BSP_ERR_STS_OK Deinitialization successful.
     */
    bsp_err_sts_t bsp_log_deinit(void);


/* -------------------------------------------------- */
/* CONVENIENCE MACROS                                */
/* -------------------------------------------------- */

/* Macro to get basename of __FILE__ */
#ifndef __FILENAME__
    #define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#endif /* __FILENAME__ */

    /*
     * Runtime-only filtering:
     * - All log macros are always compiled in.
     * - BSP_COMPILED_LOG_LEVEL is used only as the default startup level.
     * - Use bsp_log_set_level(...) at runtime to enable/disable any level.
     */

#define BSP_LOGE(fmt, ...)                                                      \
    do                                                                          \
    {                                                                           \
        if(bsp_log_get_level() >= BSP_LOG_ERROR)                                \
        {                                                                       \
            bsp_log(BSP_LOG_ERROR, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__); \
        }                                                                       \
    } while(0)

#define BSP_LOGW(fmt, ...)                                                     \
    do                                                                         \
    {                                                                          \
        if(bsp_log_get_level() >= BSP_LOG_WARN)                                \
        {                                                                      \
            bsp_log(BSP_LOG_WARN, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__); \
        }                                                                      \
    } while(0)

#define BSP_LOGI(fmt, ...)                                                     \
    do                                                                         \
    {                                                                          \
        if(bsp_log_get_level() >= BSP_LOG_INFO)                                \
        {                                                                      \
            bsp_log(BSP_LOG_INFO, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__); \
        }                                                                      \
    } while(0)

#define BSP_LOGD(fmt, ...)                                                      \
    do                                                                          \
    {                                                                           \
        if(bsp_log_get_level() >= BSP_LOG_DEBUG)                                \
        {                                                                       \
            bsp_log(BSP_LOG_DEBUG, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__); \
        }                                                                       \
    } while(0)
#ifdef __cplusplus
}
#endif

#endif /* BSP_LOG_H */
