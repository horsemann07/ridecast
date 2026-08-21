/**
 * @file bsp_err_sts.c
 * @brief BSP error status string conversion utilities.
 *
 * This file implements helper functions for converting BSP error
 * status codes (`bsp_err_sts_t`) into human-readable string names.
 *
 * These utilities are intended for:
 * - Logging and diagnostics
 * - CLI output
 * - Debugging and development tools
 *
 * The conversion functions provide symbolic names corresponding
 * to the enum identifiers (e.g. "BSP_ERR_STS_TIMEOUT") and must
 * not be used for program logic decisions.
 *
 * @par Usage
 * @code
 * bsp_err_sts_t ret = bspUartInit(&uart);
 * if (ret != BSP_ERR_STS_OK)
 * {
 *     BSP_LOG_Error("UART init failed: %s",
 *                   bsp_err_sts_to_str(ret));
 * }
 * @endcode
 *
 * @par Design Notes
 * - The returned strings are statically allocated and read-only.
 * - Unknown or out-of-range values are mapped to a safe default string.
 * - This file must be updated whenever new error codes are added
 *   to the bsp_err_sts_t enumeration.
 *
 * @par Thread Safety
 * - All functions in this file are thread-safe.
 * - No dynamic memory allocation is used.
 *
 * @see bsp_err_sts.h
 */


#include "bsp_err_sts.h"


const char* bsp_err_sts_to_str(bsp_err_sts_t status)
{
    switch(status)
    {
        /* -------------------------------------------------
         * Common / Generic Errors (0–49)
         * ------------------------------------------------- */
        case BSP_ERR_STS_OK:
        {
            return "BSP_ERR_STS_OK";
        }
        case BSP_ERR_STS_FAIL:
        {
            return "BSP_ERR_STS_FAIL";
        }
        case BSP_ERR_STS_UNKNOWN:
        {
            return "BSP_ERR_STS_UNKNOWN";
        }
        case BSP_ERR_STS_INVALID_PARAM:
        {
            return "BSP_ERR_STS_INVALID_PARAM";
        }
        case BSP_ERR_STS_INVALID_STATE:
        {
            return "BSP_ERR_STS_INVALID_STATE";
        }
        case BSP_ERR_STS_INVALID_SIZE:
        {
            return "BSP_ERR_STS_INVALID_SIZE";
        }
        case BSP_ERR_STS_NOT_FOUND:
        {
            return "BSP_ERR_STS_NOT_FOUND";
        }
        case BSP_ERR_STS_NOT_EXIST:
        {
            return "BSP_ERR_STS_NOT_EXIST";
        }
        case BSP_ERR_STS_NOT_INIT:
        {
            return "BSP_ERR_STS_NOT_INIT";
        }
        case BSP_ERR_STS_ALREADY_INIT:
        {
            return "BSP_ERR_STS_ALREADY_INIT";
        }
        case BSP_ERR_STS_BUSY:
        {
            return "BSP_ERR_STS_BUSY";
        }
        case BSP_ERR_STS_TIMEOUT:
        {
            return "BSP_ERR_STS_TIMEOUT";
        }
        case BSP_ERR_STS_NO_MEM:
        {
            return "BSP_ERR_STS_NO_MEM";
        }
        case BSP_ERR_STS_BUFFER_OVERFLOW:
        {
            return "BSP_ERR_STS_BUFFER_OVERFLOW";
        }
        case BSP_ERR_STS_UNSUPPORTED:
        {
            return "BSP_ERR_STS_UNSUPPORTED";
        }
        case BSP_ERR_STS_INTERNAL_ERR:
        {
            return "BSP_ERR_STS_INTERNAL_ERR";
        }
        case BSP_ERR_STS_CANCELLED:
        {
            return "BSP_ERR_STS_CANCELLED";
        }
        case BSP_ERR_STS_IN_PROGRESS:
        {
            return "BSP_ERR_STS_IN_PROGRESS";
        }
        case BSP_ERR_STS_NOT_READY:
        {
            return "BSP_ERR_STS_NOT_READY";
        }
        case BSP_ERR_STS_INVALID_FMT:
        {
            return "BSP_ERR_STS_INVALID_FMT";
        }
        case BSP_ERR_STS_INVALID_RESP:
        {
            return "BSP_ERR_STS_INVALID_RESP";
        }
        case BSP_ERR_STS_OUT_OF_BOUNDS:
        {
            return "BSP_ERR_STS_OUT_OF_BOUNDS";
        }
        case BSP_ERR_STS_CFG_ERR:
        {
            return "BSP_ERR_STS_CFG_ERR";
        }
        case BSP_ERR_STS_INV_VERSION:
        {
            return "BSP_ERR_STS_INV_VERSION";
        }
        case BSP_ERR_STS_NOT_RUNNING:
        {
            return "BSP_ERR_STS_NOT_RUNNING";
        }
        case BSP_ERR_STS_INIT_FAILED:
        {
            return "BSP_ERR_STS_INIT_FAILED";
        }

        /* -------------------------------------------------
         * Command / CLI Errors (50–79)
         * ------------------------------------------------- */
        case BSP_ERR_STS_INVALID_CMD:
        {
            return "BSP_ERR_STS_INVALID_CMD";
        }
        case BSP_ERR_STS_ARG_MISSING:
        {
            return "BSP_ERR_STS_ARG_MISSING";
        }
        case BSP_ERR_STS_ARG_INVALID:
        {
            return "BSP_ERR_STS_ARG_INVALID";
        }
        case BSP_ERR_STS_EXEC_FAIL:
        {
            return "BSP_ERR_STS_EXEC_FAIL";
        }
        case BSP_ERR_STS_DENIED:
        {
            return "BSP_ERR_STS_DENIED";
        }

        /* -------------------------------------------------
         * Wireless / Radio Errors (60–79)
         * ------------------------------------------------- */
        case BSP_ERR_STS_RADIO_NOT_READY:
        {
            return "BSP_ERR_STS_RADIO_NOT_READY";
        }
        case BSP_ERR_STS_RADIO_DISABLED:
        {
            return "BSP_ERR_STS_RADIO_DISABLED";
        }
        case BSP_ERR_STS_NOT_PAIRED:
        {
            return "BSP_ERR_STS_NOT_PAIRED";
        }
        case BSP_ERR_STS_AUTH_FAIL:
        {
            return "BSP_ERR_STS_AUTH_FAIL";
        }
        case BSP_ERR_STS_CONN_LOST:
        {
            return "BSP_ERR_STS_CONN_LOST";
        }
        case BSP_ERR_STS_ADDR_INVALID:
        {
            return "BSP_ERR_STS_ADDR_INVALID";
        }
        case BSP_ERR_STS_NO_SERVICE:
        {
            return "BSP_ERR_STS_NO_SERVICE";
        }
        case BSP_ERR_STS_NO_CHAR:
        {
            return "BSP_ERR_STS_NO_CHAR";
        }
        case BSP_ERR_STS_DATA_REJECT:
        {
            return "BSP_ERR_STS_DATA_REJECT";
        }
        case BSP_ERR_STS_NO_CONN:
        {
            return "BSP_ERR_STS_NO_CONN";
        }
        case BSP_ERR_STS_CONN_FAILED:
        {
            return "BSP_ERR_STS_CONN_FAILED";
        }
        case BSP_ERR_STS_CONN_TIMEOUT:
        {
            return "BSP_ERR_STS_CONN_TIMEOUT";
        }
        case BSP_ERR_STS_UNSUPPORTED_MODE:
        {
            return "BSP_ERR_STS_UNSUPPORTED_MODE";
        }
        case BSP_ERR_STS_HANDSHAKE_FAIL:
        {
            return "BSP_ERR_STS_HANDSHAKE_FAIL";
        }
        case BSP_ERR_STS_INVALID_SOCKET:
        {
            return "BSP_ERR_STS_INVALID_SOCKET";
        }
        case BSP_ERR_STS_INVALID_IP_ADDR:
        {
            return "BSP_ERR_STS_INVALID_IP_ADDR";
        }
        case BSP_ERR_STS_INVALID_PORT:
        {
            return "BSP_ERR_STS_INVALID_PORT";
        }
        case BSP_ERR_STS_SHUTDOWN_FAIL:
        {
            return "BSP_ERR_STS_SHUTDOWN_FAIL";
        }

        /* -------------------------------------------------
         * File / Storage Errors (90–99)
         * ------------------------------------------------- */
        case BSP_ERR_STS_CORRUPT:
        {
            return "BSP_ERR_STS_CORRUPT";
        }
        case BSP_ERR_STS_NO_SPACE:
        {
            return "BSP_ERR_STS_NO_SPACE";
        }
        case BSP_ERR_STS_LOCKED:
        {
            return "BSP_ERR_STS_LOCKED";
        }
        case BSP_ERR_STS_READ_FAIL:
        {
            return "BSP_ERR_STS_READ_FAIL";
        }
        case BSP_ERR_STS_WRITE_FAIL:
        {
            return "BSP_ERR_STS_WRITE_FAIL";
        }
        case BSP_ERR_STS_DELETE_FAIL:
        {
            return "BSP_ERR_STS_DELETE_FAIL";
        }

        /* -------------------------------------------------
         * Crypto / Security Errors (110–119)
         * ------------------------------------------------- */
        case BSP_ERR_STS_KEY_INVALID:
        {
            return "BSP_ERR_STS_KEY_INVALID";
        }
        case BSP_ERR_STS_KEY_EXPIRED:
        {
            return "BSP_ERR_STS_KEY_EXPIRED";
        }
        case BSP_ERR_STS_AUTH_TAG_FAIL:
        {
            return "BSP_ERR_STS_AUTH_TAG_FAIL";
        }
        case BSP_ERR_STS_ENCRYPT_FAIL:
        {
            return "BSP_ERR_STS_ENCRYPT_FAIL";
        }
        case BSP_ERR_STS_DECRYPT_FAIL:
        {
            return "BSP_ERR_STS_DECRYPT_FAIL";
        }
        case BSP_ERR_STS_INV_CRC:
        {
            return "BSP_ERR_STS_INV_CRC";
        }
        case BSP_ERR_STS_INV_HASH:
        {
            return "BSP_ERR_STS_INV_HASH";
        }
        case BSP_ERR_STS_INV_SIGNATURE:
        {
            return "BSP_ERR_STS_INV_SIGNATURE";
        }
        case BSP_ERR_STS_INV_CERT:
        {
            return "BSP_ERR_STS_INV_CERT";
        }

        /* -------------------------------------------------
         * LCD / Display Errors (200–229)
         * ------------------------------------------------- */
        case BSP_ERR_STS_BUS_ERROR:
        {
            return "BSP_ERR_STS_BUS_ERROR";
        }
        case BSP_ERR_STS_NO_ACK:
        {
            return "BSP_ERR_STS_NO_ACK";
        }

        default:
        {
            return "BSP_ERR_STS_UNKNOWN";
        }
    }
}
