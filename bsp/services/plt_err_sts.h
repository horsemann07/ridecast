/**
 * @file    plt_err_sts.h
 * @brief   Platform error status definitions and utilities.
 *
 * This file defines the platform-wide error status codes used by
 * HAL, BSP, middleware, drivers, RTOS abstraction, and application
 * layers.
 *
 * The error status list is maintained using an X-Macro so that the
 * enumeration values and their string representations are generated
 * from a single source of truth.
 *
 * @par Design Notes
 * - PLT_ERR_STS_LIST() is the single source of truth for error codes.
 * - Each error code has an explicitly assigned numeric value.
 * - Numeric ranges are reserved for different generic platform
 *   categories.
 * - PLT_ERR_STS_MAX is not part of the error status list.
 * - PLT_ERR_STS_FATAL is intentionally the last defined error status.
 *
 * @par Thread Safety
 * This header does not maintain any shared mutable state.
 *
 * @note
 * When adding a new error status, update only PLT_ERR_STS_LIST().
 */

#ifndef PLT_ERR_STS_H
#define PLT_ERR_STS_H

#include <stdint.h>

/* ========================================================================
 * Platform Error Status List
 * ======================================================================== */

// clang-format off

/**
 * @brief Master list of platform error status codes.
 *
 * Each entry follows the format:
 *
 *     X(status_name, numeric_value)
 *
 * This list is the single source of truth for:
 * - Error enumeration values.
 * - Error status string conversion.
 *
 * @note
 * Add new platform-wide error statuses only to this list.
 */
#define PLT_ERR_STS_LIST(X)                                             \
                                                                        \
    /* ================================================================= \
     * Common / Generic Status (0-49)                                  \
     * ================================================================= */ \
    X(PLT_ERR_STS_OK,                    0)                             \
    X(PLT_ERR_STS_FAIL,                  1)                             \
    X(PLT_ERR_STS_UNKNOWN,               2)                             \
    X(PLT_ERR_STS_INVALID_PARAM,         3)                             \
    X(PLT_ERR_STS_INVALID_STATE,         4)                             \
    X(PLT_ERR_STS_INVALID_SIZE,          5)                             \
    X(PLT_ERR_STS_INVALID_LENGTH,        6)                             \
    X(PLT_ERR_STS_NOT_FOUND,             7)                             \
    X(PLT_ERR_STS_NOT_EXIST,             8)                             \
    X(PLT_ERR_STS_ALREADY_EXISTS,        9)                             \
    X(PLT_ERR_STS_NOT_INITIALIZED,      10)                             \
    X(PLT_ERR_STS_ALREADY_INIT,         11)                             \
    X(PLT_ERR_STS_NOT_READY,            12)                             \
    X(PLT_ERR_STS_BUSY,                 13)                             \
    X(PLT_ERR_STS_TIMEOUT,              14)                             \
    X(PLT_ERR_STS_CANCELLED,            15)                             \
    X(PLT_ERR_STS_IN_PROGRESS,          16)                             \
    X(PLT_ERR_STS_NOT_SUPPORTED,        17)                             \
    X(PLT_ERR_STS_INTERNAL,             18)                             \
    X(PLT_ERR_STS_INIT_FAILED,          19)                             \
    X(PLT_ERR_STS_DEINIT_FAILED,        20)                             \
    X(PLT_ERR_STS_CONFIG_FAILED,        21)                             \
    X(PLT_ERR_STS_OPERATION_FAILED,     22)                             \
    X(PLT_ERR_STS_OPERATION_ABORTED,    23)                             \
    X(PLT_ERR_STS_RESOURCE_BUSY,        24)                             \
    X(PLT_ERR_STS_RESOURCE_UNAVAILABLE, 25)                             \
    X(PLT_ERR_STS_RESOURCE_EXHAUSTED,   26)                             \
    X(PLT_ERR_STS_NO_MEM,               27)                             \
    X(PLT_ERR_STS_BUFFER_OVERFLOW,      28)                             \
    X(PLT_ERR_STS_BUFFER_UNDERRUN,      29)                             \
    X(PLT_ERR_STS_BUFFER_FULL,          30)                             \
    X(PLT_ERR_STS_BUFFER_EMPTY,         31)                             \
    X(PLT_ERR_STS_MEM_NOT_ALLOC,        32)                             \
    X(PLT_ERR_STS_OUT_OF_BOUNDS,        33)                             \
    X(PLT_ERR_STS_INVALID_HANDLE,       34)                             \
    X(PLT_ERR_STS_INVALID_ID,           35)                             \
    X(PLT_ERR_STS_PERMISSION_DENIED,    36)                             \
    X(PLT_ERR_STS_ACCESS_DENIED,        37)                             \
    X(PLT_ERR_STS_DATA_INVALID,         38)                             \
    X(PLT_ERR_STS_DATA_CORRUPT,         39)                             \
    X(PLT_ERR_STS_DATA_REJECTED,        40)                             \
    X(PLT_ERR_STS_VERSION_MISMATCH,     41)                             \
    X(PLT_ERR_STS_STATE_MISMATCH,       42)                             \
    X(PLT_ERR_STS_RETRY_REQUIRED,       43)                             \
    X(PLT_ERR_STS_RETRY_LIMIT,          44)                             \
                                                                        \
    /* ================================================================= \
     * Communication / Transfer Status (50-69)                        \
     * Generic for UART / SPI / I2C / I3C / CAN / USB / etc.           \
     * ================================================================= */ \
    X(PLT_ERR_STS_TX_FAILED,             50)                            \
    X(PLT_ERR_STS_RX_FAILED,             51)                            \
    X(PLT_ERR_STS_TX_TIMEOUT,            52)                            \
    X(PLT_ERR_STS_RX_TIMEOUT,            53)                            \
    X(PLT_ERR_STS_TX_BUSY,               54)                            \
    X(PLT_ERR_STS_RX_BUSY,               55)                            \
    X(PLT_ERR_STS_NO_DATA,               56)                            \
    X(PLT_ERR_STS_NO_RESPONSE,           57)                            \
    X(PLT_ERR_STS_FRAME_ERROR,           58)                            \
    X(PLT_ERR_STS_PROTOCOL_ERROR,        59)                            \
    X(PLT_ERR_STS_CRC_ERROR,             60)                            \
    X(PLT_ERR_STS_CHECKSUM_ERROR,        61)                            \
    X(PLT_ERR_STS_ACK_FAILED,            62)                            \
    X(PLT_ERR_STS_BUS_ERROR,             63)                            \
    X(PLT_ERR_STS_LINK_ERROR,            64)                            \
    X(PLT_ERR_STS_DISCONNECTED,          65)                            \
    X(PLT_ERR_STS_CONNECTION_FAILED,     66)                            \
    X(PLT_ERR_STS_DATA_OVERFLOW,         67)                            \
    X(PLT_ERR_STS_DATA_UNDERRUN,         68)                            \
    X(PLT_ERR_STS_SYNC_FAILED,           69)                            \
                                                                        \
    /* ================================================================= \
     * Connection / Network Status (70-89)                             \
     * Generic for Wireless / Ethernet / Network / Links               \
     * ================================================================= */ \
    X(PLT_ERR_STS_ALREADY_CONNECTED,     70)                            \
    X(PLT_ERR_STS_ALREADY_DISCONNECTED,  71)                            \
    X(PLT_ERR_STS_CONNECTING,            72)                            \
    X(PLT_ERR_STS_DISCONNECTING,         73)                            \
    X(PLT_ERR_STS_RECONNECTING,          74)                            \
    X(PLT_ERR_STS_CONNECTION_LOST,       75)                            \
    X(PLT_ERR_STS_CONNECTION_TIMEOUT,    76)                            \
    X(PLT_ERR_STS_PEER_UNAVAILABLE,      77)                            \
    X(PLT_ERR_STS_NETWORK_UNAVAILABLE,   78)                            \
    X(PLT_ERR_STS_NOT_CONNECTED,         79)                            \
    X(PLT_ERR_STS_CHANNEL_BUSY,          80)                            \
    X(PLT_ERR_STS_LINK_BUSY,             81)                            \
    X(PLT_ERR_STS_LINK_NOT_READY,        82)                            \
    X(PLT_ERR_STS_LINK_UNSTABLE,         83)                            \
    X(PLT_ERR_STS_AUTH_FAILED,           84)                            \
    X(PLT_ERR_STS_NOT_PAIRED,            85)                            \
    X(PLT_ERR_STS_DISCOVERY_FAILED,      86)                            \
    X(PLT_ERR_STS_HANDSHAKE_FAILED,      87)                            \
    X(PLT_ERR_STS_ADDRESS_INVALID,       88)                            \
    X(PLT_ERR_STS_ENDPOINT_UNAVAILABLE,  89)                            \
                                                                        \
    /* ================================================================= \
     * Storage / Persistent Data Status (90-109)                       \
     * Generic for Flash / EEPROM / NVM / File Systems                 \
     * ================================================================= */ \
    X(PLT_ERR_STS_STORAGE_FULL,          90)                            \
    X(PLT_ERR_STS_STORAGE_CORRUPT,       91)                            \
    X(PLT_ERR_STS_STORAGE_NOT_READY,     92)                            \
    X(PLT_ERR_STS_STORAGE_BUSY,          93)                            \
    X(PLT_ERR_STS_READ_FAILED,           94)                            \
    X(PLT_ERR_STS_WRITE_FAILED,          95)                            \
    X(PLT_ERR_STS_DELETE_FAILED,         96)                            \
    X(PLT_ERR_STS_OPEN_FAILED,           97)                            \
    X(PLT_ERR_STS_CLOSE_FAILED,          98)                            \
    X(PLT_ERR_STS_CREATE_FAILED,         99)                            \
    X(PLT_ERR_STS_READ_ONLY,            100)                            \
    X(PLT_ERR_STS_END_OF_DATA,          101)                            \
    X(PLT_ERR_STS_STORAGE_INIT_FAILED,  102)                            \
    X(PLT_ERR_STS_STORAGE_MOUNT_FAILED, 103)                            \
    X(PLT_ERR_STS_STORAGE_FORMAT_FAILED,104)                            \
    X(PLT_ERR_STS_STORAGE_RESET_FAILED, 105)                            \
    X(PLT_ERR_STS_DATA_NOT_FOUND,       106)                            \
    X(PLT_ERR_STS_DATA_NOT_AVAILABLE,   107)                            \
    X(PLT_ERR_STS_DATA_EXPIRED,         108)                            \
    X(PLT_ERR_STS_DATA_LOCKED,          109)                            \
                                                                        \
    /* ================================================================= \
     * Security / Crypto Status (110-129)                              \
     * ================================================================= */ \
    X(PLT_ERR_STS_KEY_INVALID,          110)                            \
    X(PLT_ERR_STS_KEY_EXPIRED,          111)                            \
    X(PLT_ERR_STS_AUTH_TAG_FAILED,      112)                            \
    X(PLT_ERR_STS_ENCRYPT_FAILED,       113)                            \
    X(PLT_ERR_STS_DECRYPT_FAILED,       114)                            \
    X(PLT_ERR_STS_HASH_INVALID,         115)                            \
    X(PLT_ERR_STS_SIGNATURE_INVALID,    116)                            \
    X(PLT_ERR_STS_CERT_INVALID,         117)                            \
    X(PLT_ERR_STS_SECURITY_VIOLATION,   118)                            \
    X(PLT_ERR_STS_SECURITY_LOCKED,      119)                            \
    X(PLT_ERR_STS_SECURE_BOOT_FAILED,   120)                            \
    X(PLT_ERR_STS_AUTHORIZATION_FAILED, 121)                            \
    X(PLT_ERR_STS_SECURITY_NOT_READY,   122)                            \
    X(PLT_ERR_STS_SECURITY_NOT_SUPPORTED, 123)                         \
                                                                        \
    /* ================================================================= \
     * Reserved for future platform-wide categories (130-229)          \
     * ================================================================= */ \
                                                                        \
    /* ================================================================= \
     * Application / System Specific Status (230-254)                  \
     * ================================================================= */ \
                                                                        \
    /* ================================================================= \
     * Fatal Status                                                     \
     * ================================================================= */ \
    X(PLT_ERR_STS_FATAL,                255)

/* ========================================================================
 * Error Status Enumeration
 * ======================================================================== */

/**
 * @brief Generate an enumeration entry from an error status definition.
 */
#define PLT_ERR_STS_ENUM(name, value) name = value,

typedef enum
{
    PLT_ERR_STS_LIST(PLT_ERR_STS_ENUM)

    /**
     * @brief Number of defined platform error status values.
     *
     * This is an end marker and is not a valid error status.
     */
    PLT_ERR_STS_MAX
} plt_err_sts_t;

#undef PLT_ERR_STS_ENUM


// clang-format on

/* ========================================================================
 * Error Status String Conversion
 * ======================================================================== */

/**
 * @brief Convert an error status code to its string representation.
 *
 * @param[in] status
 * Platform error status code to convert.
 *
 * @return Pointer to a constant string containing the symbolic
 *         name of the supplied error status.
 *
 * @retval "PLT_ERR_STS_UNKNOWN"
 * Returned when the supplied status code is not recognized.
 *
 * @note
 * The returned string is statically allocated and must not be
 * modified or freed by the caller.
 */
const char* plt_err_sts_to_str(plt_err_sts_t status);

/* ========================================================================
 * Parameter Validation Macros
 * ======================================================================== */

/**
 * @brief Validate a parameter condition.
 *
 * @param[in] param
 * Condition to validate.
 *
 * @return
 * Returns PLT_ERR_STS_INVALID_PARAM from the calling function when
 * the condition evaluates to false.
 */
#define PLT_CHECK_PARAM(param)                \
    do                                        \
    {                                         \
        if(!(param))                          \
        {                                     \
            return PLT_ERR_STS_INVALID_PARAM; \
        }                                     \
    } while(0)

/**
 * @brief Validate a pointer.
 *
 * @param[in] ptr
 * Pointer to validate.
 *
 * @return
 * Returns PLT_ERR_STS_MEM_NOT_ALLOC from the calling function when
 * the pointer is NULL.
 */
#define PLT_CHECK_POINTER(ptr)                \
    do                                        \
    {                                         \
        if((ptr) == NULL)                     \
        {                                     \
            return PLT_ERR_STS_MEM_NOT_ALLOC; \
        }                                     \
    } while(0)

#endif /* PLT_ERR_STS_H */