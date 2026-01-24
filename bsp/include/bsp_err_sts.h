/**
 * @file bsp_err_sts.h
 * @brief Common BSP error status definitions.
 *
 * This header defines a unified error status mechanism used across
 * BSP, middleware, and application layers.
 *
 * The error codes are grouped by functional category to simplify
 * debugging, logging, and cross-module integration.
 *
 * Error codes are designed to be:
 * - Small integer values
 * - Easily comparable
 * - Suitable for logging and CLI output
 *
 * @par Error Code Ranges
 * - 0–49     : General & common errors
 * - 50–79    : Network / wireless / transport errors
 * - 200–249  : Reserved for future modules
 *
 * @par Usage
 * @code
 * bsp_err_sts_t ret = bspUartInit(&uart);
 * if (ret != ERR_STS_OK)
 * {
 *     BSP_LOG_Error("UART init failed: %s",
 *                   bsp_err_sts_to_str(ret));
 * }
 * @endcode
 *
 * @note All BSP APIs should return bsp_err_sts_t unless explicitly documented otherwise.
 *
 * @see bsp_log.h
 */


#ifndef __BSP_ERR_H__
#define __BSP_ERR_H__

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Error Status Mechanism:
     * -----------------------
     * - Each error status is represented by a unique numeric code.
     * - Codes are grouped by functional category for quick identification
     *   during debugging, logging, and CLI diagnostics.
     *
     * Error Code Ranges:
     *   0–49     : Common / Generic BSP errors (reused across all modules)
     *   50–79    : Command / CLI errors
     *   100–129  : Wireless errors (BLE, Wi-Fi, and generic radio states)
     *   200–229  : LCD / Display errors
     *   230+     : Reserved for future modules (Sensor, Storage, etc.)
     *
     * Design Notes:
     * - Generic errors such as FAIL, TIMEOUT, BUSY, NO_MEM, and INVALID_PARAM
     *   must be reused across modules instead of defining module-specific
     *   *_FAIL or *_TIMEOUT errors.
     * - Module-specific error codes are added only when they convey
     *   additional semantic information (e.g., NOT_PAIRED, NO_SERVICE).
     * - Error values alone should be sufficient to identify the failing
     *   subsystem during diagnostics.
     *
     * Extension Guidelines:
     * - Add new module-specific errors only in their designated range.
     * - Do not reuse numeric ranges across categories.
     * - Keep error names short, consistent, and descriptive.
     */

    typedef enum
    {
        /* =========================================================
         * Common / Generic Errors (0–49)
         * ========================================================= */
        BSP_ERR_STS_OK              = 0,  /* Operation successful */
        BSP_ERR_STS_FAIL            = 1,  /* Generic failure */
        BSP_ERR_STS_UNKNOWN         = 2,  /* Unknown error */
        BSP_ERR_STS_INVALID_PARAM   = 3,  /* Invalid parameter */
        BSP_ERR_STS_INVALID_STATE   = 4,  /* Invalid state */
        BSP_ERR_STS_INVALID_SIZE    = 5,  /* Invalid size */
        BSP_ERR_STS_NOT_FOUND       = 6,  /* Resource not found */
        BSP_ERR_STS_NOT_EXIST       = 7,  /* Resource does not exist */
        BSP_ERR_STS_NOT_INIT        = 8,  /* Module not initialized */
        BSP_ERR_STS_ALREADY_INIT    = 9,  /* Module already initialized */
        BSP_ERR_STS_BUSY            = 10, /* Resource busy */
        BSP_ERR_STS_TIMEOUT         = 11, /* Operation timed out */
        BSP_ERR_STS_NO_MEM          = 12, /* Memory allocation failed */
        BSP_ERR_STS_BUFFER_OVERFLOW = 13, /* Buffer overflow */
        BSP_ERR_STS_UNSUPPORTED     = 14, /* Feature not supported */
        BSP_ERR_STS_INTERNAL        = 15, /* Internal error */
        BSP_ERR_STS_CANCELLED       = 16, /* Operation cancelled */
        BSP_ERR_STS_IN_PROGRESS     = 17, /* Operation in progress */
        BSP_ERR_STS_NOT_READY       = 18, /* Resource not ready */
        BSP_ERR_STS_INVALID_FMT     = 19, /* Invalid format */
        BSP_ERR_STS_INVALID_RESP    = 20, /* Invalid response */
        BSP_ERR_STS_OUT_OF_BOUNDS   = 21, /* Out of bounds */
        BSP_ERR_STS_CFG_ERR         = 22, /* Configuration error */
        BSP_ERR_STS_INV_VERSION     = 23, /* Invalid version */
        BSP_ERR_STS_NOT_RUNNING     = 24, /* Not running */
        BSP_ERR_STS_INTERNAL_ERR    = 25, /* Internal error */
        BSP_ERR_STS_INIT_FAILED     = 26, /* Initialization failed */
        BSP_ERR_STS_BUF_TOO_SMALL   = 27, /* Buffer too small */
        BSP_ERR_STS_MEM_NOT_ALLOC   = 28, /* Memory not allocated */


        /* =========================================================
         * Command / CLI Errors (50–79)
         * ========================================================= */
        BSP_ERR_STS_INVALID_CMD = 50, /* Command not found */
        BSP_ERR_STS_ARG_MISSING = 51, /* Missing argument */
        BSP_ERR_STS_ARG_INVALID = 52, /* Invalid argument */
        BSP_ERR_STS_EXEC_FAIL   = 53, /* Command execution failed */
        BSP_ERR_STS_DENIED      = 54, /* Command access denied */

        /* =========================================================
         * Wireless / BLE / Wi-Fi Errors (60-79)
         * ========================================================= */
        BSP_ERR_STS_RADIO_NOT_READY  = 60, /* Radio/stack not ready */
        BSP_ERR_STS_RADIO_DISABLED   = 61, /* Radio disabled */
        BSP_ERR_STS_NOT_PAIRED       = 62, /* Not paired / bonded */
        BSP_ERR_STS_AUTH_FAIL        = 63, /* Authentication failed */
        BSP_ERR_STS_CONN_LOST        = 64, /* Link lost unexpectedly */
        BSP_ERR_STS_ADDR_INVALID     = 65, /* Invalid peer address */
        BSP_ERR_STS_NO_SERVICE       = 66, /* Service not found */
        BSP_ERR_STS_NO_CHAR          = 67, /* Characteristic not found */
        BSP_ERR_STS_DATA_REJECT      = 68, /* Data rejected by peer */
        BSP_ERR_STS_NO_CONN          = 69, /* No active connection */
        BSP_ERR_STS_CONN_FAILED      = 70, /* Connection failed */
        BSP_ERR_STS_CONN_TIMEOUT     = 71, /* Connection timeout */
        BSP_ERR_STS_UNSUPPORTED_MODE = 72, /* Unsupported mode */
        BSP_ERR_STS_HANDSHAKE_FAIL   = 73, /* Handshake failed */
        BSP_ERR_STS_INVALID_SOCKET   = 74, /* Invalid socket */
        BSP_ERR_STS_INVALID_IP_ADDR  = 75, /* Invalid IP address */
        BSP_ERR_STS_INVALID_PORT     = 76, /* Invalid port number */
        BSP_ERR_STS_SHUTDOWN_FAIL    = 77, /* Shutdown failed */

        /* =========================================================
         * File / Storage Errors (90–99)
         * ========================================================= */
        BSP_ERR_STS_CORRUPT     = 90, /* File data corrupted */
        BSP_ERR_STS_NO_SPACE    = 91, /* No storage space left */
        BSP_ERR_STS_LOCKED      = 93, /* File locked or in use */
        BSP_ERR_STS_READ_FAIL   = 94, /* File read operation failed */
        BSP_ERR_STS_WRITE_FAIL  = 95, /* File write operation failed */
        BSP_ERR_STS_DELETE_FAIL = 96, /* File delete operation failed */

        /* NOTE:
         * Use generic errors like:
         *  - BSP_ERR_STS_FAIL
         *
         *  - BSP_ERR_STS_TIMEOUT
         *  - BSP_ERR_STS_BUSY
         * for TX/RX/init failures instead of adding *_FAIL here.
         */


        /* =========================================================
         * Crypto / Security Errors (110–119)
         * ========================================================= */
        BSP_ERR_STS_KEY_INVALID   = 110, /* Invalid or malformed key */
        BSP_ERR_STS_KEY_EXPIRED   = 111, /* Key expired */
        BSP_ERR_STS_AUTH_TAG_FAIL = 112, /* Authentication tag mismatch */
        BSP_ERR_STS_ENCRYPT_FAIL  = 113, /* Encryption failed */
        BSP_ERR_STS_DECRYPT_FAIL  = 114, /* Decryption failed */
        BSP_ERR_STS_INV_CRC       = 115, /* Invalid CRC */
        BSP_ERR_STS_INV_HASH      = 116, /* Invalid hash */
        BSP_ERR_STS_INV_SIGNATURE = 117, /* Invalid signature */
        BSP_ERR_STS_INV_CERT      = 118, /* Invalid certificate */
        /* =========================================================
         * LCD / Display Errors (200–229)
         * ========================================================= */
        BSP_ERR_STS_BUS_ERROR = 203, /* SPI/I2C bus error */
        BSP_ERR_STS_NO_ACK    = 204, /* No response from LCD */

        /* NOTE:
         * Use generic errors like:
         *  - BSP_ERR_STS_FAIL
         *  - BSP_ERR_STS_TIMEOUT
         *  - BSP_ERR_STS_INVALID_PARAM
         * for read/write/draw failures.
         */

        /* =========================================================
         * Reserved for future modules (230+)
         * ========================================================= */
        BSP_ERR_STS_MAX /* Sentinel (not an error code) */

    } bsp_err_sts_t;


    /**
     * @brief Convert an error status code to its string name.
     *
     * Provides a human-readable symbolic name for an `bsp_err_sts_t` code, which can
     * be useful for debugging, logging, and user-facing diagnostics.
     *
     * @param[in]  status   Error code to convert (of type bsp_err_sts_t).
     *
     * @return Pointer to a constant string representing the symbolic name of the
     *         error (e.g., "ERR_STS_TIMEOUT").
     *         Returns "ERR_STS_UNKNOWN" if the code is unrecognized.
     *
     * @note The returned string is statically allocated and must not be freed or modified.
     */
    const char* bsp_err_sts_to_str(bsp_err_sts_t status);

// Macro to check a parameter and exit if it's invalid
#define CHECK_PARAM(param)                    \
    do                                        \
    {                                         \
        if(!(param))                          \
        {                                     \
            return BSP_ERR_STS_INVALID_PARAM; \
        }                                     \
    } while(0)

    // Macro to check a pointer and exit if it's invalid
#define CHECK_POINTER(ptr)                    \
    do                                        \
    {                                         \
        if(!(ptr))                            \
        {                                     \
            return BSP_ERR_STS_MEM_NOT_ALLOC; \
        }                                     \
    } while(0)


#define UNUSED(param)  \
    do                 \
    {                  \
        (void)(param); \
    } while(0)


#ifdef __cplusplus
}

#endif

#endif /* __BSP_ERR_H__ */
