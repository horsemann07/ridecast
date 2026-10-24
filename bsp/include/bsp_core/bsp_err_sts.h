/* ============================================================================
 * File        : ret_err_sts.h
 * Description : BSP platform-independent error status definitions and utilities
 * Author      : Jha Raghav
 * Date        : 2026-10-24
 * ============================================================================
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Description:
 *   Defines reusable platform-wide error status codes and validation
 *   utilities for BSP, middleware, drivers, and application modules,
 *   including:
 *
 *   - Unified platform-independent error status enumeration
 *   - Explicit numeric error codes grouped by functional category
 *   - X-Macro-based error status definitions
 *   - Error status to string conversion
 *   - Common parameter and pointer validation macros
 *   - Buffer length and boundary validation macros
 *   - Range and index validation macros
 *   - Resource state and operation status validation
 *   - Memory alignment and bitmask validation utilities
 *
 * Integration:
 *   This header provides common error status definitions for
 *   reusable BSP components and application modules.
 *
 *   Modules should use ret_err_sts_t for standardized error
 *   reporting where applicable.
 *
 *   The error status definitions are maintained through
 *   RET_ERR_STS_LIST() as a single source of truth.
 *
 * Requirements:
 *   - C11 compatible compiler.
 *   - Standard integer and size type definitions.
 *   - No RTOS or platform-specific dependencies.
 *   - No dynamic memory allocation.
 *
 * Notes:
 *   - Error status numeric values must remain stable.
 *   - New error codes should be added to RET_ERR_STS_LIST().
 *   - RET_ERR_STS_FATAL is reserved for fatal errors.
 *   - Validation macros return from the calling function.
 *   - Validation macros must be used with compatible return types.
 *
 * ============================================================================
 * Change Log:
 *
 * Date          Author          Description
 * ------------  --------------  ----------------------------------------------
 * 2026-10-24    Jha Raghav      Standardized documentation and header structure
 * ============================================================================
 */
// clang-format off
#ifndef BSP_ERR_STS_H
#define BSP_ERR_STS_H

#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* ========================================================================
 * Platform Error Status List
 * ======================================================================== */



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
#define RET_ERR_STS_LIST(X)                                             \
                                                                        \
    /* ================================================================= \
     * Common / Generic Status (0-49)                                  \
     * ================================================================= */ \
    X(RET_ERR_STS_OK,                    0)                             \
    X(RET_ERR_STS_FAIL,                  1)                             \
    X(RET_ERR_STS_UNKNOWN,               2)                             \
    X(RET_ERR_STS_INVALID_PARAM,         3)                             \
    X(RET_ERR_STS_INVALID_STATE,         4)                             \
    X(RET_ERR_STS_INVALID_SIZE,          5)                             \
    X(RET_ERR_STS_INVALID_LENGTH,        6)                             \
    X(RET_ERR_STS_NOT_FOUND,             7)                             \
    X(RET_ERR_STS_NOT_EXIST,             8)                             \
    X(RET_ERR_STS_ALREADY_EXISTS,        9)                             \
    X(RET_ERR_STS_NOT_INITIALIZED,      10)                             \
    X(RET_ERR_STS_ALREADY_INIT,         11)                             \
    X(RET_ERR_STS_NOT_READY,            12)                             \
    X(RET_ERR_STS_BUSY,                 13)                             \
    X(RET_ERR_STS_TIMEOUT,              14)                             \
    X(RET_ERR_STS_CANCELLED,            15)                             \
    X(RET_ERR_STS_IN_PROGRESS,          16)                             \
    X(RET_ERR_STS_NOT_SUPPORTED,        17)                             \
    X(RET_ERR_STS_INTERNAL,             18)                             \
    X(RET_ERR_STS_INIT_FAILED,          19)                             \
    X(RET_ERR_STS_DEINIT_FAILED,        20)                             \
    X(RET_ERR_STS_CONFIG_FAILED,        21)                             \
    X(RET_ERR_STS_OPERATION_FAILED,     22)                             \
    X(RET_ERR_STS_OPERATION_ABORTED,    23)                             \
    X(RET_ERR_STS_RESOURCE_BUSY,        24)                             \
    X(RET_ERR_STS_RESOURCE_UNAVAILABLE, 25)                             \
    X(RET_ERR_STS_RESOURCE_EXHAUSTED,   26)                             \
    X(RET_ERR_STS_NO_MEM,               27)                             \
    X(RET_ERR_STS_BUFFER_OVERFLOW,      28)                             \
    X(RET_ERR_STS_BUFFER_UNDERRUN,      29)                             \
    X(RET_ERR_STS_BUFFER_FULL,          30)                             \
    X(RET_ERR_STS_BUFFER_EMPTY,         31)                             \
    X(RET_ERR_STS_MEM_NOT_ALLOC,        32)                             \
    X(RET_ERR_STS_OUT_OF_BOUNDS,        33)                             \
    X(RET_ERR_STS_INVALID_HANDLE,       34)                             \
    X(RET_ERR_STS_INVALID_ID,           35)                             \
    X(RET_ERR_STS_PERMISSION_DENIED,    36)                             \
    X(RET_ERR_STS_ACCESS_DENIED,        37)                             \
    X(RET_ERR_STS_DATA_INVALID,         38)                             \
    X(RET_ERR_STS_DATA_CORRUPT,         39)                             \
    X(RET_ERR_STS_DATA_REJECTED,        40)                             \
    X(RET_ERR_STS_VERSION_MISMATCH,     41)                             \
    X(RET_ERR_STS_STATE_MISMATCH,       42)                             \
    X(RET_ERR_STS_RETRY_REQUIRED,       43)                             \
    X(RET_ERR_STS_RETRY_LIMIT,          44)                             \
                                                                        \
    /* ================================================================= \
     * Communication / Transfer Status (50-69)                        \
     * Generic for UART / SPI / I2C / I3C / CAN / USB / etc.           \
     * ================================================================= */ \
    X(RET_ERR_STS_TX_FAILED,             50)                            \
    X(RET_ERR_STS_RX_FAILED,             51)                            \
    X(RET_ERR_STS_TX_TIMEOUT,            52)                            \
    X(RET_ERR_STS_RX_TIMEOUT,            53)                            \
    X(RET_ERR_STS_TX_BUSY,               54)                            \
    X(RET_ERR_STS_RX_BUSY,               55)                            \
    X(RET_ERR_STS_NO_DATA,               56)                            \
    X(RET_ERR_STS_NO_RESPONSE,           57)                            \
    X(RET_ERR_STS_FRAME_ERROR,           58)                            \
    X(RET_ERR_STS_PROTOCOL_ERROR,        59)                            \
    X(RET_ERR_STS_CRC_ERROR,             60)                            \
    X(RET_ERR_STS_CHECKSUM_ERROR,        61)                            \
    X(RET_ERR_STS_ACK_FAILED,            62)                            \
    X(RET_ERR_STS_BUS_ERROR,             63)                            \
    X(RET_ERR_STS_LINK_ERROR,            64)                            \
    X(RET_ERR_STS_DISCONNECTED,          65)                            \
    X(RET_ERR_STS_CONNECTION_FAILED,     66)                            \
    X(RET_ERR_STS_DATA_OVERFLOW,         67)                            \
    X(RET_ERR_STS_DATA_UNDERRUN,         68)                            \
    X(RET_ERR_STS_SYNC_FAILED,           69)                            \
                                                                        \
    /* ================================================================= \
     * Connection / Network Status (70-89)                             \
     * Generic for Wireless / Ethernet / Network / Links               \
     * ================================================================= */ \
    X(RET_ERR_STS_ALREADY_CONNECTED,     70)                            \
    X(RET_ERR_STS_ALREADY_DISCONNECTED,  71)                            \
    X(RET_ERR_STS_CONNECTING,            72)                            \
    X(RET_ERR_STS_DISCONNECTING,         73)                            \
    X(RET_ERR_STS_RECONNECTING,          74)                            \
    X(RET_ERR_STS_CONNECTION_LOST,       75)                            \
    X(RET_ERR_STS_CONNECTION_TIMEOUT,    76)                            \
    X(RET_ERR_STS_PEER_UNAVAILABLE,      77)                            \
    X(RET_ERR_STS_NETWORK_UNAVAILABLE,   78)                            \
    X(RET_ERR_STS_NOT_CONNECTED,         79)                            \
    X(RET_ERR_STS_CHANNEL_BUSY,          80)                            \
    X(RET_ERR_STS_LINK_BUSY,             81)                            \
    X(RET_ERR_STS_LINK_NOT_READY,        82)                            \
    X(RET_ERR_STS_LINK_UNSTABLE,         83)                            \
    X(RET_ERR_STS_AUTH_FAILED,           84)                            \
    X(RET_ERR_STS_NOT_PAIRED,            85)                            \
    X(RET_ERR_STS_DISCOVERY_FAILED,      86)                            \
    X(RET_ERR_STS_HANDSHAKE_FAILED,      87)                            \
    X(RET_ERR_STS_ADDRESS_INVALID,       88)                            \
    X(RET_ERR_STS_ENDPOINT_UNAVAILABLE,  89)                            \
                                                                        \
    /* ================================================================= \
     * Storage / Persistent Data Status (90-109)                       \
     * Generic for Flash / EEPROM / NVM / File Systems                 \
     * ================================================================= */ \
    X(RET_ERR_STS_STORAGE_FULL,          90)                            \
    X(RET_ERR_STS_STORAGE_CORRUPT,       91)                            \
    X(RET_ERR_STS_STORAGE_NOT_READY,     92)                            \
    X(RET_ERR_STS_STORAGE_BUSY,          93)                            \
    X(RET_ERR_STS_READ_FAILED,           94)                            \
    X(RET_ERR_STS_WRITE_FAILED,          95)                            \
    X(RET_ERR_STS_DELETE_FAILED,         96)                            \
    X(RET_ERR_STS_OPEN_FAILED,           97)                            \
    X(RET_ERR_STS_CLOSE_FAILED,          98)                            \
    X(RET_ERR_STS_CREATE_FAILED,         99)                            \
    X(RET_ERR_STS_READ_ONLY,            100)                            \
    X(RET_ERR_STS_END_OF_DATA,          101)                            \
    X(RET_ERR_STS_STORAGE_INIT_FAILED,  102)                            \
    X(RET_ERR_STS_STORAGE_MOUNT_FAILED, 103)                            \
    X(RET_ERR_STS_STORAGE_FORMAT_FAILED,104)                            \
    X(RET_ERR_STS_STORAGE_RESET_FAILED, 105)                            \
    X(RET_ERR_STS_DATA_NOT_FOUND,       106)                            \
    X(RET_ERR_STS_DATA_NOT_AVAILABLE,   107)                            \
    X(RET_ERR_STS_DATA_EXPIRED,         108)                            \
    X(RET_ERR_STS_DATA_LOCKED,          109)                            \
                                                                        \
    /* ================================================================= \
     * Security / Crypto Status (110-129)                              \
     * ================================================================= */ \
    X(RET_ERR_STS_KEY_INVALID,          110)                            \
    X(RET_ERR_STS_KEY_EXPIRED,          111)                            \
    X(RET_ERR_STS_AUTH_TAG_FAILED,      112)                            \
    X(RET_ERR_STS_ENCRYPT_FAILED,       113)                            \
    X(RET_ERR_STS_DECRYPT_FAILED,       114)                            \
    X(RET_ERR_STS_HASH_INVALID,         115)                            \
    X(RET_ERR_STS_SIGNATURE_INVALID,    116)                            \
    X(RET_ERR_STS_CERT_INVALID,         117)                            \
    X(RET_ERR_STS_SECURITY_VIOLATION,   118)                            \
    X(RET_ERR_STS_SECURITY_LOCKED,      119)                            \
    X(RET_ERR_STS_SECURE_BOOT_FAILED,   120)                            \
    X(RET_ERR_STS_AUTHORIZATION_FAILED, 121)                            \
    X(RET_ERR_STS_SECURITY_NOT_READY,   122)                            \
    X(RET_ERR_STS_SECURITY_NOT_SUPPORTED, 123)                         \
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
    X(RET_ERR_STS_FATAL,                255)

/* ========================================================================
 * Error Status Enumeration
 * ======================================================================== */

/**
 * @brief Generate an enumeration entry from an error status definition.
 */
#define RET_ERR_STS_ENUM(name, value) name = value,

typedef enum
{
    RET_ERR_STS_LIST(RET_ERR_STS_ENUM)

    /**
     * @brief Number of defined platform error status values.
     *
     * This is an end marker and is not a valid error status.
     */
    RET_ERR_STS_MAX
} ret_err_sts_t;

#undef RET_ERR_STS_ENUM




/* ========================================================================
 * Error Status String Conversion
 * ======================================================================== */
/**
 * @brief Convert an error status to its symbolic name.
 *
 * @param[in] status Error status code.
 *
 * @return Pointer to a read-only status string.
 *
 * @note Thread-safe and header-only.
 * @note No dynamic memory allocation.
 */
static inline const char* ret_err_sts_to_str(ret_err_sts_t status)
{
    const char* result;

    switch(status)
    {
        /* Generate cases from the master status list. */
        #define RET_ERR_STS_STRING_CASE(name, value) \
            case name:                              \
            {                                       \
                result = #name;                     \
                break;                              \
            }

        RET_ERR_STS_LIST(RET_ERR_STS_STRING_CASE)

        #undef RET_ERR_STS_STRING_CASE

        default:
        {
            result = "RET_ERR_STS_UNKNOWN";
            break;
        }
    }

    return result;
}

/* ========================================================= */

/**
 * @brief Validate a parameter condition.
 *
 * @param[in] param
 * Condition to validate.
 *
 * @param[in] ret
 * Error code to return when validation fails.
 */
#define RET_CHECK_PARAM(param, ret) \
    do                              \
    {                               \
        if(!(param))                \
        {                           \
            return (ret);           \
        }                           \
    } while(0)


/**
 * @brief Validate a pointer.
 *
 * @param[in] ptr
 * Pointer to validate.
 *
 * @param[in] ret
 * Error code to return if pointer is NULL.
 */
#define RET_CHECK_POINTER(ptr, ret) \
    do                              \
    {                               \
        if((ptr) == NULL)           \
        {                           \
            return (ret);           \
        }                           \
    } while(0)


/**
 * @brief Validate that a value is not zero.
 *
 * @param[in] value
 * Value to validate.
 *
 * @param[in] ret
 * Error code to return if value is zero.
 */
#define RET_CHECK_NOT_ZERO(value, ret) \
    do                                 \
    {                                  \
        if((value) == 0)               \
        {                              \
            return (ret);              \
        }                              \
    } while(0)


/**
 * @brief Validate a value against a range.
 *
 * @param[in] value
 * Value to validate.
 *
 * @param[in] min
 * Minimum allowed value.
 *
 * @param[in] max
 * Maximum allowed value.
 *
 * @param[in] ret
 * Error code to return if outside range.
 *
 * @note Range is inclusive.
 */
#define RET_CHECK_RANGE(value, min, max, ret)      \
    do                                             \
    {                                              \
        if(((value) < (min)) || ((value) > (max))) \
        {                                          \
            return (ret);                          \
        }                                          \
    } while(0)


/**
 * @brief Validate an array index.
 *
 * @param[in] index
 * Array index to validate.
 *
 * @param[in] count
 * Number of elements in the array.
 *
 * @param[in] ret
 * Error code to return if index is invalid.
 */
#define RET_CHECK_INDEX(index, count, ret)     \
    do                                         \
    {                                          \
        if((size_t)(index) >= (size_t)(count)) \
        {                                      \
            return (ret);                      \
        }                                      \
    } while(0)


/**
 * @brief Validate a buffer and its length.
 *
 * @param[in] buffer
 * Buffer pointer to validate.
 *
 * @param[in] length
 * Buffer length.
 *
 * @param[in] ret
 * Error code to return if buffer is NULL
 * or length is zero.
 */
#define RET_CHECK_BUFFER(buffer, length, ret)      \
    do                                             \
    {                                              \
        if(((buffer) == NULL) || ((length) == 0U)) \
        {                                          \
            return (ret);                          \
        }                                          \
    } while(0)


/**
 * @brief Validate an expected value.
 *
 * @param[in] actual
 * Actual value.
 *
 * @param[in] expected
 * Expected value.
 *
 * @param[in] ret
 * Error code to return if values differ.
 */
#define RET_CHECK_EQUAL(actual, expected, ret) \
    do                                         \
    {                                          \
        if((actual) != (expected))             \
        {                                      \
            return (ret);                      \
        }                                      \
    } while(0)


/**
 * @brief Validate a function execution status.
 *
 * @param[in] expr
 * Function call or status expression.
 *
 * @param[in] expected
 * Expected success status.
 *
 * @param[in] ret
 * Error code to return on failure.
 */
#define RET_CHECK_STATUS(expr, expected, ret) \
    do                                        \
    {                                         \
        if((expr) != (expected))              \
        {                                     \
            return (ret);                     \
        }                                     \
    } while(0)

/**
 * @brief Validate that required bits are set.
 *
 * @param[in] value
 * Register value or bit field.
 *
 * @param[in] mask
 * Required bit mask.
 *
 * @param[in] ret
 * Error code returned if any required bit is clear.
 */
#define RET_CHECK_BITS_SET(value, mask, ret) \
    do                                       \
    {                                        \
        if(((value) & (mask)) != (mask))     \
        {                                    \
            return (ret);                    \
        }                                    \
    } while(0)


/**
 * @brief Validate that specified bits are clear.
 *
 * @param[in] value
 * Register value or bit field.
 *
 * @param[in] mask
 * Bits that must be clear.
 *
 * @param[in] ret
 * Error code returned if any bit is set.
 */
#define RET_CHECK_BITS_CLEAR(value, mask, ret) \
    do                                         \
    {                                          \
        if(((value) & (mask)) != 0U)           \
        {                                      \
            return (ret);                      \
        }                                      \
    } while(0)


/**
 * @brief Validate memory address alignment.
 *
 * @param[in] ptr
 * Pointer to validate.
 *
 * @param[in] alignment
 * Required alignment in bytes.
 *
 * @param[in] ret
 * Error code returned for invalid alignment.
 *
 * @note Alignment must be a nonzero power of two.
 */
#define RET_CHECK_ALIGNMENT(ptr, alignment, ret)                 \
    do                                                           \
    {                                                            \
        uintptr_t align_ = (uintptr_t)(alignment);               \
        if((align_ == 0U) || ((align_ & (align_ - 1U)) != 0U) || \
           (((uintptr_t)(ptr) & (align_ - 1U)) != 0U))           \
        {                                                        \
            return (ret);                                        \
        }                                                        \
    } while(0)


/**
 * @brief Validate system or peripheral state.
 *
 * @param[in] condition
 * Expected state condition.
 *
 * @param[in] ret
 * Error code returned if condition is false.
 */
#define RET_CHECK_STATE(condition, ret) \
    do                                  \
    {                                   \
        if(!(condition))                \
        {                               \
            return (ret);               \
        }                               \
    } while(0)


/**
 * @brief Validate a condition and jump to cleanup.
 *
 * @param[in] condition
 * Condition to validate.
 *
 * @param[in] label
 * Cleanup label.
 *
 * @note Caller must set the appropriate error
 * status before invoking this macro.
 */
#define RET_CHECK_GOTO(condition, label) \
    do                                   \
    {                                    \
        if(!(condition))                 \
        {                                \
            goto label;                  \
        }                                \
    } while(0)


    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BSP_ERR_STS_H */
