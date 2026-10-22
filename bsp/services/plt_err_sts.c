/**
 * @file bsp_err_sts.c
 * @brief BSP error status string conversion utilities.
 *
 * This file implements helper functions for converting BSP error
 * status codes (`plt_err_sts_t`) into human-readable string names.
 *
 * These utilities are intended for:
 * - Logging and diagnostics
 * - CLI output
 * - Debugging and development tools
 *
 * The conversion functions provide symbolic names corresponding
 * to the enum identifiers (e.g. "PLT_ERR_STS_TIMEOUT") and must
 * not be used for program logic decisions.
 *
 * @par Usage
 * @code
 * plt_err_sts_t ret = bspUartInit(&uart);
 * if (ret != PLT_ERR_STS_OK)
 * {
 *     BSP_LOG_Error("UART init failed: %s",
 *                   plt_err_sts_to_str(ret));
 * }
 * @endcode
 *
 * @par Design Notes
 * - The returned strings are statically allocated and read-only.
 * - Unknown or out-of-range values are mapped to a safe default string.
 * - This file must be updated whenever new error codes are added
 *   to the plt_err_sts_t enumeration.
 *
 * @par Thread Safety
 * - All functions in this file are thread-safe.
 * - No dynamic memory allocation is used.
 *
 * @see bsp_err_sts.h
 */


#include "bsp_err_sts.h"


const char* plt_err_sts_to_str(plt_err_sts_t status)
{
    switch(status)
    {
        /* -------------------------------------------------
         * Common / Generic Errors (0–49)
         * ------------------------------------------------- */
        case PLT_ERR_STS_OK:
        {
            return "PLT_ERR_STS_OK";
        }
        case PLT_ERR_STS_FAIL:
        {
            return "PLT_ERR_STS_FAIL";
        }
        case PLT_ERR_STS_UNKNOWN:
        {
            return "PLT_ERR_STS_UNKNOWN";
        }
        case PLT_ERR_STS_INVALID_PARAM:
        {
            return "PLT_ERR_STS_INVALID_PARAM";
        }
        case PLT_ERR_STS_INVALID_STATE:
        {
            return "PLT_ERR_STS_INVALID_STATE";
        }
        case PLT_ERR_STS_INVALID_SIZE:
        {
            return "PLT_ERR_STS_INVALID_SIZE";
        }
        case PLT_ERR_STS_NOT_FOUND:
        {
            return "PLT_ERR_STS_NOT_FOUND";
        }
        case PLT_ERR_STS_NOT_EXIST:
        {
            return "PLT_ERR_STS_NOT_EXIST";
        }
        case PLT_ERR_STS_NOT_INIT:
        {
            return "PLT_ERR_STS_NOT_INIT";
        }
        case PLT_ERR_STS_ALREADY_INIT:
        {
            return "PLT_ERR_STS_ALREADY_INIT";
        }
        case PLT_ERR_STS_BUSY:
        {
            return "PLT_ERR_STS_BUSY";
        }
        case PLT_ERR_STS_TIMEOUT:
        {
            return "PLT_ERR_STS_TIMEOUT";
        }
        case PLT_ERR_STS_NO_MEM:
        {
            return "PLT_ERR_STS_NO_MEM";
        }
        case PLT_ERR_STS_BUFFER_OVERFLOW:
        {
            return "PLT_ERR_STS_BUFFER_OVERFLOW";
        }
        case PLT_ERR_STS_UNSUPPORTED:
        {
            return "PLT_ERR_STS_UNSUPPORTED";
        }
        case PLT_ERR_STS_INTERNAL_ERR:
        {
            return "PLT_ERR_STS_INTERNAL_ERR";
        }
        case PLT_ERR_STS_CANCELLED:
        {
            return "PLT_ERR_STS_CANCELLED";
        }
        case PLT_ERR_STS_IN_PROGRESS:
        {
            return "PLT_ERR_STS_IN_PROGRESS";
        }
        case PLT_ERR_STS_NOT_READY:
        {
            return "PLT_ERR_STS_NOT_READY";
        }
        case PLT_ERR_STS_INVALID_FMT:
        {
            return "PLT_ERR_STS_INVALID_FMT";
        }
        case PLT_ERR_STS_INVALID_RESP:
        {
            return "PLT_ERR_STS_INVALID_RESP";
        }
        case PLT_ERR_STS_OUT_OF_BOUNDS:
        {
            return "PLT_ERR_STS_OUT_OF_BOUNDS";
        }
        case PLT_ERR_STS_CFG_ERR:
        {
            return "PLT_ERR_STS_CFG_ERR";
        }
        case PLT_ERR_STS_INV_VERSION:
        {
            return "PLT_ERR_STS_INV_VERSION";
        }
        case PLT_ERR_STS_NOT_RUNNING:
        {
            return "PLT_ERR_STS_NOT_RUNNING";
        }
        case PLT_ERR_STS_INIT_FAILED:
        {
            return "PLT_ERR_STS_INIT_FAILED";
        }

        /* -------------------------------------------------
         * Command / CLI Errors (50–79)
         * ------------------------------------------------- */
        case PLT_ERR_STS_INVALID_CMD:
        {
            return "PLT_ERR_STS_INVALID_CMD";
        }
        case PLT_ERR_STS_ARG_MISSING:
        {
            return "PLT_ERR_STS_ARG_MISSING";
        }
        case PLT_ERR_STS_ARG_INVALID:
        {
            return "PLT_ERR_STS_ARG_INVALID";
        }
        case PLT_ERR_STS_EXEC_FAIL:
        {
            return "PLT_ERR_STS_EXEC_FAIL";
        }
        case PLT_ERR_STS_DENIED:
        {
            return "PLT_ERR_STS_DENIED";
        }

        /* -------------------------------------------------
         * Wireless / Radio Errors (60–79)
         * ------------------------------------------------- */
        case PLT_ERR_STS_RADIO_NOT_READY:
        {
            return "PLT_ERR_STS_RADIO_NOT_READY";
        }
        case PLT_ERR_STS_RADIO_DISABLED:
        {
            return "PLT_ERR_STS_RADIO_DISABLED";
        }
        case PLT_ERR_STS_NOT_PAIRED:
        {
            return "PLT_ERR_STS_NOT_PAIRED";
        }
        case PLT_ERR_STS_AUTH_FAIL:
        {
            return "PLT_ERR_STS_AUTH_FAIL";
        }
        case PLT_ERR_STS_CONN_LOST:
        {
            return "PLT_ERR_STS_CONN_LOST";
        }
        case PLT_ERR_STS_ADDR_INVALID:
        {
            return "PLT_ERR_STS_ADDR_INVALID";
        }
        case PLT_ERR_STS_NO_SERVICE:
        {
            return "PLT_ERR_STS_NO_SERVICE";
        }
        case PLT_ERR_STS_NO_CHAR:
        {
            return "PLT_ERR_STS_NO_CHAR";
        }
        case PLT_ERR_STS_DATA_REJECT:
        {
            return "PLT_ERR_STS_DATA_REJECT";
        }
        case PLT_ERR_STS_NO_CONN:
        {
            return "PLT_ERR_STS_NO_CONN";
        }
        case PLT_ERR_STS_CONN_FAILED:
        {
            return "PLT_ERR_STS_CONN_FAILED";
        }
        case PLT_ERR_STS_CONN_TIMEOUT:
        {
            return "PLT_ERR_STS_CONN_TIMEOUT";
        }
        case PLT_ERR_STS_UNSUPPORTED_MODE:
        {
            return "PLT_ERR_STS_UNSUPPORTED_MODE";
        }
        case PLT_ERR_STS_HANDSHAKE_FAIL:
        {
            return "PLT_ERR_STS_HANDSHAKE_FAIL";
        }
        case PLT_ERR_STS_INVALID_SOCKET:
        {
            return "PLT_ERR_STS_INVALID_SOCKET";
        }
        case PLT_ERR_STS_INVALID_IP_ADDR:
        {
            return "PLT_ERR_STS_INVALID_IP_ADDR";
        }
        case PLT_ERR_STS_INVALID_PORT:
        {
            return "PLT_ERR_STS_INVALID_PORT";
        }
        case PLT_ERR_STS_SHUTDOWN_FAIL:
        {
            return "PLT_ERR_STS_SHUTDOWN_FAIL";
        }

        /* -------------------------------------------------
         * File / Storage Errors (90–99)
         * ------------------------------------------------- */
        case PLT_ERR_STS_CORRUPT:
        {
            return "PLT_ERR_STS_CORRUPT";
        }
        case PLT_ERR_STS_NO_SPACE:
        {
            return "PLT_ERR_STS_NO_SPACE";
        }
        case PLT_ERR_STS_LOCKED:
        {
            return "PLT_ERR_STS_LOCKED";
        }
        case PLT_ERR_STS_READ_FAIL:
        {
            return "PLT_ERR_STS_READ_FAIL";
        }
        case PLT_ERR_STS_WRITE_FAIL:
        {
            return "PLT_ERR_STS_WRITE_FAIL";
        }
        case PLT_ERR_STS_DELETE_FAIL:
        {
            return "PLT_ERR_STS_DELETE_FAIL";
        }

        /* -------------------------------------------------
         * Crypto / Security Errors (110–119)
         * ------------------------------------------------- */
        case PLT_ERR_STS_KEY_INVALID:
        {
            return "PLT_ERR_STS_KEY_INVALID";
        }
        case PLT_ERR_STS_KEY_EXPIRED:
        {
            return "PLT_ERR_STS_KEY_EXPIRED";
        }
        case PLT_ERR_STS_AUTH_TAG_FAIL:
        {
            return "PLT_ERR_STS_AUTH_TAG_FAIL";
        }
        case PLT_ERR_STS_ENCRYPT_FAIL:
        {
            return "PLT_ERR_STS_ENCRYPT_FAIL";
        }
        case PLT_ERR_STS_DECRYPT_FAIL:
        {
            return "PLT_ERR_STS_DECRYPT_FAIL";
        }
        case PLT_ERR_STS_INV_CRC:
        {
            return "PLT_ERR_STS_INV_CRC";
        }
        case PLT_ERR_STS_INV_HASH:
        {
            return "PLT_ERR_STS_INV_HASH";
        }
        case PLT_ERR_STS_INV_SIGNATURE:
        {
            return "PLT_ERR_STS_INV_SIGNATURE";
        }
        case PLT_ERR_STS_INV_CERT:
        {
            return "PLT_ERR_STS_INV_CERT";
        }

        /* -------------------------------------------------
         * LCD / Display Errors (200–229)
         * ------------------------------------------------- */
        case PLT_ERR_STS_BUS_ERROR:
        {
            return "PLT_ERR_STS_BUS_ERROR";
        }
        case PLT_ERR_STS_NO_ACK:
        {
            return "PLT_ERR_STS_NO_ACK";
        }

        default:
        {
            return "PLT_ERR_STS_UNKNOWN";
        }
    }
}
