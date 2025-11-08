#ifndef ERR_STATUS_H
#define ERR_STATUS_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Error Status Mechanism:
     * -----------------------
     * - Each error is assigned a unique code.
     * - Codes are grouped by category for easier identification.
     *
     * Categories:
     *   0-49       : General & Common Errors
     *   50-99      : BLE Errors
     *   100-149    : WiFi Errors
     *   150-199    : Wireless Transport / Generic Wireless Errors (Zigbee, LoRa, etc.)
     *   200-500    : Reserved for future modules (LCD, Sensor, etc.)
     *
     * Notes:
     *   - ERR_STS_FUNC_NOT_SUPPORTED belongs to the Generic category.
     *   - For new modules (LCD, Sensor, etc.) you can add errors in the 200-249 range.
     */

    typedef enum
    {
        // =========================
        // General & Common Errors (0-49)
        // =========================
        ERR_STS_OK                 = 0,  // Operation successful
        ERR_STS_FAIL               = 1,  // General failure
        ERR_STS_UNKN               = 2,  // Unknown error
        ERR_STS_INVALID_PARAM      = 3,  // Invalid parameter
        ERR_STS_INV_STATE          = 4,  // Invalid state
        ERR_STS_INV_SIZE           = 5,  // Invalid size
        ERR_STS_INV_RESPONSE       = 6,  // Invalid response
        ERR_STS_INV_CRC            = 7,  // CRC check failed
        ERR_STS_INV_VERSION        = 8,  // Unsupported version
        ERR_STS_NOT_FOUND          = 9,  // Item not found
        ERR_STS_NOT_EXIST          = 10, // Item does not exist
        ERR_STS_CFG_ERR            = 11, // Configuration error
        ERR_STS_INIT_FAIL          = 12, // Initialization failed
        ERR_STS_RD_FAIL            = 13, // Read operation failed
        ERR_STS_WR_FAIL            = 14, // Write operation failed
        ERR_STS_TIMEOUT            = 15, // Operation timed out
        ERR_STS_BUSY               = 16, // Resource busy
        ERR_STS_NO_MEM             = 17, // Memory allocation failed
        ERR_STS_NOT_RUNNING        = 18, // Not running
        ERR_STS_FUNC_NOT_SUPPORTED = 19, // Function not supported (generic)
        ERR_STS_NOT_INIT           = 20, // Not initialized
        ERR_STS_INTERNAL_ERROR     = 21, // Internal error

        // =========================
        // Network / Wireless / Transport Errors (50–79)
        // =========================
        ERR_STS_TRANSPORT_INIT_FAIL = 50, // Transport or stack initialization failed
        ERR_STS_CONNECT_FAILED = 51, // Failed to connect to remote host or access point
        ERR_STS_DISCONNECTED = 52, // Connection closed or lost unexpectedly
        ERR_STS_SEND_FAILED  = 53, // Data send operation failed
        ERR_STS_RECV_FAILED  = 54, // Data receive operation failed
        ERR_STS_TIMEOUT      = 55, // Operation timed out
        ERR_STS_AUTH_FAILED = 56, // Authentication or credential verification failed
        ERR_STS_DNS_FAILED      = 57, // DNS lookup or resolution failed
        ERR_STS_INVALID_ADDR    = 58, // Invalid IP/MAC address or endpoint
        ERR_STS_RESOURCE_BUSY   = 59, // Resource (socket, buffer, mutex) busy
        ERR_STS_BUFFER_OVERFLOW = 60, // Data buffer overflow occurred
        ERR_STS_UNSUPPORTED_MODE = 61, // Unsupported transport mode (e.g., TLS disabled)
        ERR_STS_COLLISION     = 62, // Channel busy or transmission collision
        ERR_STS_NO_ACK        = 63, // No acknowledgment received
        ERR_STS_PKT_TOO_LARGE = 64, // Packet size exceeds maximum limit
        ERR_STS_INVALID_PARAM = 65, // Invalid argument or configuration parameter
        ERR_STS_SECURITY_FAIL  = 66, // Generic security/encryption failure
        ERR_STS_HANDSHAKE_FAIL = 67, // TLS or protocol handshake failed
        ERR_STS_CERT_INVALID = 68, // Invalid, expired, or untrusted certificate
        ERR_STS_SHUTDOWN_FAIL  = 69, // Graceful socket/TLS shutdown failed
        ERR_STS_INVALID_SOCKET = 70, // Invalid socket descriptor
        ERR_STS_UNKNOWN        = 71, // Unknown or unspecified transport error


        // =========================
        // Reserved for future modules (200-249)
        // =========================
        // Example: LCD, Sensor, Storage, etc.
        // You can add new module errors here in the 200-249 range

        // =========================
        // Reserved (200+)
        // =========================
        ERR_STS_RSVD_5 = 904,

        ERR_STS_MAX // Maximum error code value
    } errStatus_t;

    /**
     * @brief Convert an error status code to its string name.
     *
     * Provides a human-readable symbolic name for an `errStatus_t` code, which can
     * be useful for debugging, logging, and user-facing diagnostics.
     *
     * @param[in]  status   Error code to convert (of type errStatus_t).
     *
     * @return Pointer to a constant string representing the symbolic name of the
     *         error (e.g., "ERR_STS_TIMEOUT").
     *         Returns "ERR_STS_UNKNOWN" if the code is unrecognized.
     *
     * @note The returned string is statically allocated and must not be freed or modified.
     */
    const char* err_status_err_to_name(errStatus_t status);

// Macro to check a parameter and exit if it's invalid
#define CHECK_PARAM(param)                \
    do                                    \
    {                                     \
        if(!(param))                      \
        {                                 \
            return ERR_STS_INVALID_PARAM; \
        }                                 \
    } while(0)


#define UNUSED(param)  \
    do                 \
    {                  \
        (void)(param); \
    } while(0)


#ifdef __cplusplus
}

#endif

#endif // ERR_STATUS_H
