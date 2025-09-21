#ifndef ERR_STATUS_H
#define ERR_STATUS_H

#ifdef __cplusplus
extern "C" {
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

typedef enum {
    // =========================
    // General & Common Errors (0-49)
    // =========================
    ERR_STS_OK                  = 0,    // Operation successful
    ERR_STS_FAIL                = 1,    // General failure
    ERR_STS_UNKN                = 2,    // Unknown error
    ERR_STS_INV_PARAM           = 3,    // Invalid parameter
    ERR_STS_INV_STATE           = 4,    // Invalid state
    ERR_STS_INV_SIZE            = 5,    // Invalid size
    ERR_STS_INV_RESPONSE        = 6,    // Invalid response
    ERR_STS_INV_CRC             = 7,    // CRC check failed
    ERR_STS_INV_VERSION         = 8,    // Unsupported version
    ERR_STS_NOT_FOUND           = 9,    // Item not found
    ERR_STS_NOT_EXIST           = 10,   // Item does not exist
    ERR_STS_CFG_ERR             = 11,   // Configuration error
    ERR_STS_INIT_FAIL           = 12,   // Initialization failed
    ERR_STS_RD_FAIL             = 13,   // Read operation failed
    ERR_STS_WR_FAIL             = 14,   // Write operation failed
    ERR_STS_TIMEOUT             = 15,   // Operation timed out
    ERR_STS_BUSY                = 16,   // Resource busy
    ERR_STS_NO_MEM              = 17,   // Memory allocation failed
    ERR_STS_NOT_RUNNING         = 18,   // Not running
    ERR_STS_FUNC_NOT_SUPPORTED  = 19,   // Function not supported (generic)

    // =========================
    // BLE Errors (50-99)
    // =========================
    ERR_STS_BLE_INIT            = 50,  // BLE initialization failed
    ERR_STS_BLE_CONN            = 51,  // BLE connection failed
    ERR_STS_BLE_DISCONN         = 52,  // BLE disconnection failed
    ERR_STS_BLE_TIMEOUT         = 53,  // BLE operation timed out
    ERR_STS_BLE_AUTH            = 54,  // BLE authentication failed
    ERR_STS_BLE_CFG             = 55,  // BLE configuration error
    ERR_STS_BLE_SEND            = 56,  // BLE send operation failed
    ERR_STS_BLE_RECV            = 57,  // BLE receive operation failed

    // =========================
    // WiFi Errors (100-149)
    // =========================
    ERR_STS_AUTH_FAIL           = 100,  // WiFi initialization failed
    ERR_STS_CONN_FAIL           = 101,  // WiFi connection failed
    ERR_STS_WIFI_DISCONN        = 102,  // WiFi disconnection failed
    ERR_STS_WIFI_TIMEOUT        = 103,  // WiFi operation timed out
    ERR_STS_WIFI_AUTH           = 104,  // WiFi authentication failed
    ERR_STS_WIFI_CFG            = 105,  // WiFi configuration error
    ERR_STS_WIFI_SEND           = 106,  // WiFi send operation failed
    ERR_STS_WIFI_RECV           = 107,  // WiFi receive operation failed

    // =========================
    // Wireless Transport / Generic Wireless Errors (150-199)
    // =========================
    ERR_STS_WIRELESS_INIT        = 150,  // Wireless transport initialization failed
    ERR_STS_WIRELESS_SEND        = 151,  // Wireless send operation failed
    ERR_STS_WIRELESS_RECV        = 152,  // Wireless receive operation failed
    ERR_STS_WIRELESS_TIMEOUT     = 153,  // Wireless operation timed out
    ERR_STS_WIRELESS_CFG         = 154,  // Wireless configuration error
    ERR_STS_WIRELESS_AUTH        = 155,  // Authentication failed
    ERR_STS_WIRELESS_DISCONN     = 156,  // Unexpected disconnection
    ERR_STS_WIRELESS_COLLISION   = 157,  // Transmission collision / channel busy
    ERR_STS_WIRELESS_OVERFLOW    = 158,  // Buffer overflow
    ERR_STS_WIRELESS_UNKNOWN     = 159,  // Unknown wireless transport error

    // =========================
    // Reserved for future modules (200-249)
    // =========================
    // Example: LCD, Sensor, Storage, etc.
    // You can add new module errors here in the 200-249 range

    // =========================
    // Reserved (200+)
    // =========================
    ERR_STS_RSVD_1               = 200,
    ERR_STS_RSVD_2               = 201,
    ERR_STS_RSVD_3               = 202,
    ERR_STS_RSVD_4               = 203,
    ERR_STS_RSVD_5               = 904,

    ERR_STS_MAX                          // Maximum error code value
} errStatus_t;

// Macro to check a parameter and exit if it's invalid
#define CHECK_PARAM(param)                  \
    do {                                    \
        if (!(param)) {                     \
            return ERR_STS_INV_PARAM;       \
        }                                   \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // ERR_STATUS_H
