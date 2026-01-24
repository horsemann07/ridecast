
/**
 * @file bsp_wifi.h
 * @brief Board Support Package (BSP) Wi-Fi API.
 *
 * This header defines the data structures, enumerations, and function prototypes
 * for managing Wi-Fi connectivity on the target hardware platform. It provides
 * APIs for Wi-Fi initialization, configuration, connection management, scanning,
 * power management, event handling, and statistics retrieval.
 *
 * The API supports multiple Wi-Fi operation modes (Station, Access Point, P2P, etc.),
 * various security protocols (Open, WEP, WPA, WPA2, WPA3), and both IPv4 and IPv6 addressing.
 * It also includes support for saving and managing multiple Wi-Fi network profiles,
 * scanning for available networks, and retrieving detailed connection and hardware
 * capability information.
 *
 * @note All functions return an @ref bsp_err_sts_t indicating the result of the operation.
 *       Refer to @ref bsp_err_sts.h for error code definitions.
 *
 * @version 1.0
 */

#ifndef __BSP_WIFI_H__
#define __BSP_WIFI_H__


#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * @brief Required configuration and error code headers for BSP Wi-Fi API.
 *
 * - bsp_config.h: Contains board and Wi-Fi configuration macros (SSID length, password length, etc).
 * - bsp_err_sts.h: Defines the bsp_err_sts_t type and error codes used by all BSP Wi-Fi API functions.
 */
#include "bsp_config.h"
#include "bsp_err_sts.h"

#include "cmsis_os2.h"

#define BSP_WIFI_SSID_MAX_LEN       bspCONFIG_WIFI_SSID_MAX_LEN
#define BSP_WIFI_PASSWORD_MAX_LEN   bspCONFIG_WIFI_PASSWORD_MAX_LEN
#define BSP_WIFI_IP_ADDR_MAX_LEN    bspCONFIG_WIFI_IP_ADDR_MAX_LEN
#define BSP_WIFI_MAC_ADDR_MAX_LEN   bspCONFIG_WIFI_MAC_ADDR_MAX_LEN
#define BSP_WIFI_MAX_SAVED_NETWORKS bspCONFIG_WIFI_MAX_SAVED_NETWORKS

/**< Max time to wait for Wi-Fi semaphore in ms. */
#define BSP_WIFI_MAX_SEMAPHORE_WAIT_TIME_MS (5000)

/**< Maximum length of a WEP key (128-bit key). */
#define BSP_WIFI_WEP_KEY_LEN_MAX (13)

#define BSP_WIFI_IP4_ADDR_SIZE   (4U)
#define BSP_WIFI_IP6_ADDR_SIZE   (16U)
#define BSP_WIFI_MAC_ADDR_SIZE   (6U)

    /*
     * *************************************************************************************************
     *        WIFI ENUMERATIONS
     * *************************************************************************************************
     */

    /**
     * @brief Wi-Fi operation mode.
     *
     * Enumerates the possible Wi-Fi operation modes for the device.
     */
    typedef enum
    {
        eWiFiModeStation = 0, /**< Station mode (client). */
        eWiFiModeAP,          /**< Access point mode. */
        eWiFiModeP2P,         /**< Peer-to-peer mode. */
        eWiFiModeAPStation, /**< Simultaneous AP and Station (repeater) mode. */
        eWiFiModeMax        /**< Unsupported mode. */
    } bspWifiMode_t;

    /**
     * @brief Unified Wi-Fi event types for BSP.
     *
     * Covers general Wi-Fi lifecycle, STA, AP, and WPS related events.
     */
    typedef enum
    {
        /* ---------- General Wi-Fi Events ---------- */
        eBSPWifiEventReady = 0,    /**< Wi-Fi subsystem initialized and ready */
        eBSPWifiEventScanDone,     /**< Wi-Fi scan completed */

        eBSPWifiEventConnected,    /**< Connected to a Wi-Fi network */
        eBSPWifiEventDisconnected, /**< Disconnected from Wi-Fi network */
        eBSPWifiEventConnectionFailed, /**< Connection attempt failed */
        eBSPWifiEventIPReady,          /**< IP address acquired */
        eBSPWifiEventIPFailed,         /**< IP address acquisition failed */

        /* ---------- Station (STA) Events ---------- */
        eBSPWifiEventSTAStarted,      /**< Wi-Fi station started */
        eBSPWifiEventSTAStopped,      /**< Wi-Fi station stopped */
        eBSPWifiEventSTAConnected,    /**< Station connected to AP */
        eBSPWifiEventSTADisconnected, /**< Station disconnected from AP */
        eBSPWifiEventSTAGotIP,        /**< Station received IP address */

        /* ---------- Access Point (AP) Events ---------- */
        eBSPWifiEventAPStarted,             /**< AP started */
        eBSPWifiEventAPStopped,             /**< AP stopped */
        eBSPWifiEventAPStateChanged,        /**< AP started or stopped */
        eBSPWifiEventAPStationConnected,    /**< A station connected to AP */
        eBSPWifiEventAPStationDisconnected, /**< A station disconnected from AP */

        /* ---------- WPS Events ---------- */
        eBSPWifiEventWPSSuccess, /**< WPS succeeded */
        eBSPWifiEventWPSFailed,  /**< WPS failed */
        eBSPWifiEventWPSTimeout, /**< WPS timed out */

        eBSPWifiEventUnknown,    /**< Unknown event */
        /* ---------- End Marker ---------- */
        eBSPWifiEventMax /**< Number of Wi-Fi events (must be last) */

    } bspWifiEvent_t;


    /**
     * @brief Wi-Fi security types.
     *
     * Enumerates the supported Wi-Fi security protocols.
     */
    typedef enum
    {
        eWiFiSecurityOpen = 0,    /**< Open - No security. */
        eWiFiSecurityWEP,         /**< WEP security. */
        eWiFiSecurityWPA,         /**< WPA security. */
        eWiFiSecurityWPA2,        /**< WPA2 security. */
        eWiFiSecurityWPA2_ent,    /**< WPA2 Enterprise security. */
        eWiFiSecurityWPA3,        /**< WPA3 security. */
        eWiFiSecurityNotSupported /**< Unknown or unsupported security. */
    } bspWifiSecurity_t;

    /**
     * @brief Wi-Fi power management modes.
     *
     * Enumerates the supported power management modes for Wi-Fi operation.
     */
    typedef enum
    {
        eBSPWifiPmNormal = 0,  /**< Normal mode. */
        eBSPWifiPmLowPower,    /**< Low power mode. */
        eBSPWifiPmAlwaysOn,    /**< Always on mode. */
        eBSPWifiPmNotSupported /**< Unsupported power management mode. */
    } bspWifiPmMode_t;

    /**
     * @brief Wi-Fi frequency bands.
     *
     * Enumerates the supported Wi-Fi frequency bands.
     */
    typedef enum
    {
        eBSPWifiBand2G = 0, /**< 2.4 GHz band. */
        eBSPWifiBand5G,     /**< 5 GHz band. */
        eBSPWifiBandDual,   /**< Dual band (2.4 GHz and 5 GHz). */
        eBSPWifiBandMax     /**< Maximum band value (end marker). */
    } bspWifiBand_t;

    /**
     * @brief Wi-Fi PHY (physical layer) modes.
     *
     * Enumerates the supported Wi-Fi PHY standards.
     */
    typedef enum
    {
        eBSPWifiPhyNone = 0,
        eBSPWifiPhy11b  = 1, /**< IEEE 802.11b. */
        eBSPWifiPhy11g  = 2, /**< IEEE 802.11g. */
        eBSPWifiPhy11n  = 3, /**< IEEE 802.11n. */
        eBSPWifiPhy11ac = 4, /**< IEEE 802.11ac. */
        eBSPWifiPhy11ax = 5, /**< IEEE 802.11ax. */
        eBSPWifiPhyLR   = 6, /**< IEEE 802.11ax. */
    } bspWifiPhyMode_t;

    /**
     * @brief Wi-Fi channel bandwidths.
     *
     * Enumerates the supported Wi-Fi channel bandwidths.
     */
    typedef enum
    {
        eBSPWifiBW20 = 0, /**< 20 MHz bandwidth. */
        eBSPWifiBW40,     /**< 40 MHz bandwidth. */
        eBSPWifiBW80,     /**< 80 MHz bandwidth. */
        eBSPWifiBW160,    /**< 160 MHz bandwidth. */
        eBSPWifiBWMax     /**< Maximum bandwidth value (end marker). */
    } bspWifiBandwidth_t;

    /**
     * @brief Wi-Fi disconnection and failure reasons.
     *
     * Enumerates possible reasons for Wi-Fi disconnection or failure events.
     */
    typedef enum
    {
        eBSPWifiReasonUnspecified = 0, /**< Unspecified reason. */
        eBSPWifiReasonAPNotFound,      /**< Access point not found. */
        eBSPWifiReasonAuthExpired,     /**< Authentication expired. */
        eBSPWifiReasonAuthLeaveBSS,    /**< Left BSS during authentication. */
        eBSPWifiReasonAuthFailed,      /**< Authentication failed. */
        eBSPWifiReasonAssocExpired,    /**< Association expired. */
        eBSPWifiReasonAssocTooMany,    /**< Too many associations. */
        eBSPWifiReasonAssocPowerCapBad, /**< Association failed: power capability. */
        eBSPWifiReasonAssocSupChanBad, /**< Association failed: supported channel. */
        eBSPWifiReasonAssocFailed, /**< Association failed. */
        eBSPWifiReasonIEInvalid,   /**< Invalid information element. */
        eBSPWifiReasonMICFailure,  /**< MIC (Message Integrity Code) failure. */
        eBSPWifiReason4WayTimeout, /**< 4-way handshake timeout. */
        eBSPWifiReason4WayIEDiffer, /**< 4-way handshake IE difference. */
        eBSPWifiReason4WayFailed,   /**< 4-way handshake failed. */
        eBSPWifiReasonAKMPInvalid, /**< Invalid AKMP (Authentication and Key Management Protocol). */
        eBSPWifiReasonPairwiseCipherInvalid, /**< Invalid pairwise cipher. */
        eBSPWifiReasonGroupCipherInvalid,    /**< Invalid group cipher. */
        eBSPWifiReasonRSNVersionInvalid,     /**< Invalid RSN version. */
        eBSPWifiReasonRSNCapInvalid,         /**< Invalid RSN capabilities. */
        eBSPWifiReasonGroupKeyUpdateTimeout, /**< Group key update timeout. */
        eBSPWifiReasonCipherSuiteRejected,   /**< Cipher suite rejected. */
        eBSPWifiReason8021XAuthFailed, /**< 802.1X authentication failed. */
        eBSPWifiReasonBeaconTimeout,   /**< Beacon timeout. */
        eBSPWifiReasonLinkFailed,      /**< Link failed. */
        eBSPWifiReasonDHCPExpired,     /**< DHCP lease expired. */
        eBSPWifiReasonDHCPFailed,      /**< DHCP failed. */
        eBSPWifiReasonMax /**< Maximum reason value (end marker). */
    } bspWifiReason_t;

    /**
     * @brief Wi-Fi IP address types.
     *
     * Enumerates the possible IP address types for Wi-Fi connections.
     */
    typedef enum
    {
        eBSPWifiIPv4 = 0,
        eBSPWifiIPv6,
        eBSPWifiIPNotSupported
    } bspWifiIPAddressType_t;

    /*
     * *************************************************************************************************
     *        WIFI STRUCTURES
     * *************************************************************************************************
     */

    /**
     * @brief Wi-Fi IP address structure.
     *
     * Represents an IPv4 or IPv6 address for Wi-Fi configuration.
     */
    typedef struct
    {
        bspWifiIPAddressType_t type; /**< Address type (IPv4 or IPv6). */
        uint32_t address[4]; /**< IP address (array for IPv6 support). */
    } bspWifiIPAddress_t;

    /**
     * @brief Wi-Fi IP configuration structure.
     *
     * Holds the IP configuration for a Wi-Fi interface, including IP address, netmask, gateway, and DNS servers.
     */
    typedef struct
    {
        bspWifiIPAddress_t ipAddress; /**< IP address. */
        bspWifiIPAddress_t netMask;   /**< Network mask. */
        bspWifiIPAddress_t gateway;   /**< Gateway address. */
        bspWifiIPAddress_t dns1;      /**< Primary DNS server. */
        bspWifiIPAddress_t dns2;      /**< Secondary DNS server. */
    } bspWifiIPConfig_t;

    /**
     * @brief Wi-Fi station information structure.
     *
     * Contains the MAC address of a connected Wi-Fi station.
     */
    typedef struct
    {
        uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< MAC address of the station. */
        uint8_t rssi;                           /**< rssi of stations */
        bspWifiPhyMode_t phyMode;               /**< PHY mode of the station */
    } bspWifiStationInfo_t;


    /**
     * @brief Wi-Fi capability information structure.
     *
     * Describes the capabilities of the Wi-Fi hardware, including supported bands, PHY modes, and features.
     */
    typedef struct
    {
        bspWifiBand_t band;           /**< Supported Wi-Fi band. */
        bspWifiPhyMode_t phyMode;     /**< Supported PHY mode. */
        bspWifiBandwidth_t bandwidth; /**< Supported channel bandwidth. */
        uint32_t maxAggr;             /**< Maximum aggregation size. */
        uint16_t supportedFeatures;   /**< Bitmask of supported features. */
    } bspWifiCapabilityInfo_t;

    /**
     * @brief Wi-Fi statistics (generic, cross-platform).
     *
     * Values may be partially populated depending on platform capability.
     */
    typedef struct
    {
        /* ---------- Packet counters ---------- */
        uint32_t txPackets; /**< Total transmitted packets */
        uint32_t rxPackets; /**< Total received packets */
        uint32_t txErrors;  /**< Transmission errors */
        uint32_t rxErrors;  /**< Reception errors */

        /* ---------- Link quality ---------- */
        int8_t rssi;  /**< RSSI in dBm (STA or average AP) */
        int8_t noise; /**< Noise floor in dBm (if supported) */

        /* ---------- Rates ---------- */
        uint16_t txRateKbps; /**< Current TX rate (kbps) */
        uint16_t rxRateKbps; /**< Current RX rate (kbps) */

        /* ---------- AP specific ---------- */
        uint8_t connectedStations; /**< Number of connected stations (AP mode) */

        /* ---------- Validity ---------- */
        uint32_t validMask; /**< Bitmask indicating which fields are valid */

    } bspWifiStatistics_t;


    /**
     * @brief Wi-Fi network configuration / profile.
     *
     * This structure is used for:
     *  - Connecting to a Wi-Fi network (STA mode)
     *  - Storing network profiles in non-volatile memory
     *
     * The same structure is intentionally reused to avoid
     * duplication and inconsistency between "connect" and "store" paths.
     */
    typedef struct
    {
        /* -------------------------------------------------
         * Network identity
         * ------------------------------------------------- */
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< SSID (binary, not null-terminated) */
        uint8_t ssidLength;                  /**< Length of SSID */

        uint8_t bssid[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< Optional BSSID (0 = ignore) */

        /* -------------------------------------------------
         * Security configuration
         * ------------------------------------------------- */
        bspWifiSecurity_t security; /**< Security type */

        union
        {
            struct
            {
                uint8_t key[BSP_WIFI_PASSWORD_MAX_LEN];
                uint8_t keyLength;
                uint8_t keyIndex; /**< Default WEP key index */
            } wep;

            struct
            {
                uint8_t passphrase[BSP_WIFI_PASSWORD_MAX_LEN];
                uint8_t length;
            } wpa;

        } credential;

        /* -------------------------------------------------
         * Optional connection hints
         * ------------------------------------------------- */
        uint8_t channel; /**< Preferred channel (0 = auto) */

    } bspWifiNetworkConfig_t;


    /**
     * @brief Wi-Fi scan result structure.
     *
     * Holds information about a discovered Wi-Fi network during a scan.
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< SSID of the discovered network. */
        uint8_t ssidLength;                  /**< Length of the SSID. */
        uint8_t bssid[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< BSSID (MAC address) of the AP. */
        bspWifiSecurity_t security; /**< Security type of the network. */
        int8_t rssi;                /**< Signal strength (RSSI). */
        uint8_t channel;            /**< Channel number. */
        uint8_t num_result;         /**< Number of results. */
    } bspWifiApScanResult_t;


    /**
     * @brief Wi-Fi configuration structure for AP
     *
     * This structure holds the configuration parameters required to initialize
     * a Wi-Fi interface in either Access Point (AP)
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< SSID of the network. */
        uint8_t ssidLength;                  /**< Length of the SSID. */
        uint8_t password[BSP_WIFI_PASSWORD_MAX_LEN]; /**< Password of the network. */
        uint8_t passwordLength;     /**< Length of the password. */
        bspWifiSecurity_t security; /**< Security type (see bspWifiSecurity_t). */
        uint8_t channel;            /**< Channel number (1-11 for 2.4GHz). */
        uint8_t ssidHidden;     /**< SSID hidden: 0 = visible, 1 = hidden. */
        uint8_t maxConnections; /**< Maximum connections for AP mode (1-4). */
        uint8_t pmMode;    /**< Power management mode (see bspWifiPmMode_t). */
        uint8_t bandwidth; /**< Channel bandwidth (see bspWifiBandwidth_t). */
        uint8_t phyMode;   /**< PHY mode (see bspWifiPhyMode_t). */
        uint32_t maxIdlePeriodSec; /** Maximum sec wifi keep idle */
    } bspWifiApConfig_t;

    /**
     * @brief Wi-Fi Station (STA) configuration.
     *
     * Contains parameters required to connect to an external Access Point.
     * Platform-specific tuning (PM, PHY, bandwidth) is handled internally.
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN];         /**< Target AP SSID */
        uint8_t ssidLength;                          /**< Length of SSID */

        uint8_t password[BSP_WIFI_PASSWORD_MAX_LEN]; /**< WPA/WPA2/WPA3 passphrase */
        uint8_t passwordLength;                      /**< Password length */

        bspWifiSecurity_t security;                  /**< Security type */

        uint8_t channel; /**< Optional: fixed channel (0 = auto) */

    } bspWifiStaConfig_t;


    /** */
    typedef struct
    {
        bspWifiStaConfig_t staConfig; /**< Station side config */
        bspWifiApConfig_t apConfig;   /**< Access Point config */
    } bspWifiApStaConfig_t;


    /**
     * @brief Unified Wi-Fi connection information.
     *
     * Provides runtime status, link quality, IP configuration,
     * and peer information for the active Wi-Fi connection.
     *
     * This structure is valid only when Wi-Fi is started.
     * Some fields may be unsupported on certain platforms.
     */
    typedef struct
    {
        /* -------------------------------------------------
         * Link identity
         * ------------------------------------------------- */
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN];      /**< Connected SSID */
        uint8_t ssidLength;                       /**< SSID length */
        uint8_t bssid[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< AP BSSID */

        bspWifiSecurity_t security;               /**< Security type */
        uint8_t channel;                          /**< Operating channel */

        /* -------------------------------------------------
         * Connection state
         * ------------------------------------------------- */
        uint8_t staConnected;                 /**< 1 = STA connected */
        uint8_t apStarted;                    /**< 1 = AP running */
        bspWifiReason_t lastDisconnectReason; /**< Last disconnect reason */

        /* -------------------------------------------------
         * Signal / link quality
         * ------------------------------------------------- */
        bspWifiBand_t band;       /**< Current band */
        bspWifiPhyMode_t phyMode; /**< Current PHY mode */
        int8_t rssi;              /**< RSSI in dBm */
        int8_t noise;             /**< Noise floor (if available) */

        /* -------------------------------------------------
         * Traffic statistics (best-effort)
         * ------------------------------------------------- */
        uint32_t txPackets; /**< Transmitted packets */
        uint32_t txErrors;  /**< TX failures */
        uint32_t rxPackets; /**< Received packets */
        uint32_t rxErrors;  /**< RX failures */

        /* -------------------------------------------------
         * IP configuration
         * ------------------------------------------------- */
        bspWifiIPConfig_t ip; /**< IP configuration */

        /* -------------------------------------------------
         * AP mode info (optional)
         * ------------------------------------------------- */
        uint8_t connectedStations; /**< Number of connected stations (AP mode) */

    } bspWifiConnectionInfo_t;


    /**
     * @brief Wi-Fi event context delivered to application.
     *
     * Carries exactly one event and its associated data.
     * Works for STA, AP, and AP+STA modes.
     */
    typedef struct
    {
        bspWifiEvent_t tEventType;

        union
        {
            bspWifiConnectionInfo_t tConnectionInfo; /**< STA connect / disconnect / IP */
            bspWifiApScanResult_t tScanResult; /**< Scan result */
            bspWifiStaConfig_t tStationCfg;    /**< STA started / configured */
            bspWifiApConfig_t tApCfg;          /**< AP started / configured */
        } tEventData;

    } bspWifiContext_t;


    /**
     * @brief Generic Wi-Fi handle (platform independent).
     *
     * This structure owns Wi-Fi state and IPC objects.
     * Platform-specific data is stored as opaque pointer.
     */
    typedef struct
    {
        /* ---------- Platform private context ---------- */
        void* platform_ctx; /**< Platform-specific Wi-Fi context */

        /* ---------- RTOS primitives (CMSIS only) ---------- */
        osMutexId_t lock;         /**< Wi-Fi lock */
        osMessageQueueId_t evt_q; /**< Event queue to application */

        /* ---------- Runtime state ---------- */
        uint8_t wifi_started;
        uint8_t sta_connected;
        uint8_t ap_started;
        uint8_t auth_failed;
        uint8_t scan_in_progress;

        /* ---------- Connection info ---------- */
        bspWifiContext_t wifi_context;

    } bspWifiHandle_t;

    /*
     **********************************************************************
     *     WIFI COMMON PUBLIC API
     **********************************************************************
     */
    /**
     * @brief Initialize the BSP Wi-Fi subsystem.
     *
     * This function initializes the generic BSP Wi-Fi handle, creates required
     * CMSIS-RTOS synchronization primitives (mutex and event queue), and initializes
     * the underlying platform Wi-Fi layer.
     *
     * This API is idempotent and must be called before any other Wi-Fi operation.
     *
     * @param[in,out] handle Pointer to BSP Wi-Fi handle.
     *
     * @return BSP_ERR_STS_OK           Initialization successful.
     * @return BSP_ERR_STS_INVALID_PARAM Handle pointer is NULL.
     * @return BSP_ERR_STS_NO_MEM       Failed to allocate required RTOS resources.
     */
    bsp_err_sts_t bspWifiInit(bspWifiHandle_t* handle);


    /**
     * @brief Deinitialize and shut down the BSP Wi-Fi subsystem.
     *
     * Stops any running Wi-Fi interfaces, unregisters platform event handlers,
     * deinitializes the Wi-Fi driver, destroys created network interfaces, and
     * releases internal RTOS resources.
     *
     * After this call, the handle becomes invalid until bspWifiInit() is called again.
     *
     * @param[in,out] handle Pointer to BSP Wi-Fi handle.
     *
     * @return BSP_ERR_STS_OK            Deinitialization successful.
     * @return BSP_ERR_STS_INVALID_PARAM Handle or platform context is NULL.
     */
    bsp_err_sts_t bspWifiDeInit(bspWifiHandle_t* handle);

    /**
     * @brief Start Wi-Fi operation in the specified mode.
     *
     * Initializes Wi-Fi if not already initialized, configures the requested
     * Wi-Fi mode, and starts the appropriate interfaces (STA, AP, or both).
     *
     * @param[in,out] handle BSP Wi-Fi handle.
     * @param[in]     mode   Wi-Fi operation mode.
     * @param[in]     pvParam Mode-specific configuration:
     *                        - STA      → bspWifiStaConfig_t
     *                        - AP       → bspWifiApConfig_t
     *                        - AP+STA   → bspWifiApStaConfig_t
     *
     * @return BSP_ERR_STS_OK             Wi-Fi started successfully.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid parameters.
     * @return BSP_ERR_STS_UNSUPPORTED    Mode not supported.
     * @return BSP_ERR_STS_FAIL           Platform failure.
     */
    bsp_err_sts_t bspWifiOn(bspWifiHandle_t* handle, bspWifiMode_t mode, void* pvParam);


    /**
     * @brief Stop and deinitialize Wi-Fi.
     *
     * Gracefully shuts down all Wi-Fi interfaces and releases resources.
     *
     * @param[in,out] handle BSP Wi-Fi handle.
     *
     * @return BSP_ERR_STS_OK            Wi-Fi stopped successfully.
     * @return BSP_ERR_STS_INVALID_PARAM Handle is NULL.
     */
    bsp_err_sts_t bspWifiOff(bspWifiHandle_t* handle);


    /*
     **********************************************************************
     *     WIFI STATION PUBLIC API
     **********************************************************************
     */


    /**
     * @brief Connect STA interface to an Access Point.
     *
     * Initiates a station connection using the configuration stored in the
     * handle. Connection progress and result are signaled via internal event
     * flags and application event queue.
     *
     * @param[in,out] handle BSP Wi-Fi handle.
     *
     * @return BSP_ERR_STS_OK             Connection initiated successfully.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid handle.
     * @return BSP_ERR_STS_INVALID_STATE  Wi-Fi not initialized.
     * @return BSP_ERR_STS_FAIL           Platform error.
     */
    bsp_err_sts_t bspWifiConnectToAp(bspWifiHandle_t* handle);

    /**
     * @brief Disconnect STA interface from the connected Access Point.
     *
     * Requests the platform to disconnect the STA interface and waits for
     * confirmation via internal event flags.
     *
     * @param[in,out] handle BSP Wi-Fi handle.
     *
     * @return BSP_ERR_STS_OK             Successfully disconnected.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid handle.
     * @return BSP_ERR_STS_TIMEOUT        Disconnect confirmation timeout.
     */
    bsp_err_sts_t bspWifiDisconnectToAp(bspWifiHandle_t* handle);


    /**
     * @brief Get list of stations connected to the Access Point.
     *
     * Retrieves information about stations currently associated with the AP.
     *
     * @param[in,out] handle       BSP Wi-Fi handle.
     * @param[out]    stations     Output array for station info.
     * @param[out]    numStations  Number of stations returned.
     * @param[in]     maxStations  Maximum number of stations to return.
     *
     * @return BSP_ERR_STS_OK             Success.
     * @return BSP_ERR_STS_NOT_READY      AP not running.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid arguments.
     */
    bsp_err_sts_t bspWifiGetConnStations(bspWifiHandle_t* handle,
                                         bspWifiStationInfo_t* stations,
                                         uint8_t* numStations,
                                         uint8_t maxStations);


    /**
     * @brief Disconnect a specific station from the Access Point.
     *
     * Forces deauthentication of a connected station identified by MAC address.
     *
     * @param[in,out] handle BSP Wi-Fi handle.
     * @param[in]     mac    MAC address of the station.
     *
     * @return BSP_ERR_STS_OK             Station disconnected.
     * @return BSP_ERR_STS_INVALID_STATE  AP not running.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid parameters.
     */
    bsp_err_sts_t bspWifiDisconnectStation(bspWifiHandle_t* handle,
                                           const uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN]);


    /**
     * @brief Store STA profile persistently.
     *
     * Saves station configuration to non-volatile storage using
     * platform-specific mechanisms.
     *
     * @param[in,out] handle  BSP Wi-Fi handle.
     * @param[in]     station STA configuration to store.
     *
     * @return BSP_ERR_STS_OK             Profile stored successfully.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid arguments.
     * @return BSP_ERR_STS_FAIL           Storage failure.
     */
    bsp_err_sts_t bspWifiStoreStationProfile(bspWifiHandle_t* handle,
                                             bspWifiStaConfig_t* station);


    /**
     * @brief Retrieve stored STA profile by index.
     *
     * @param[in,out] handle  BSP Wi-Fi handle.
     * @param[in]     index   Profile index.
     * @param[out]    sta_cfg Output STA configuration.
     *
     * @return BSP_ERR_STS_OK             Profile retrieved.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid arguments.
     * @return BSP_ERR_STS_FAIL           Profile not found.
     */
    bsp_err_sts_t bspWifiGetStationProfile(bspWifiHandle_t* handle,
                                           uint8_t index,
                                           bspWifiStaConfig_t* sta_cfg);

    /**
     * @brief Remove stored STA profile by SSID.
     *
     * @param[in,out] handle   BSP Wi-Fi handle.
     * @param[in]     ssid     SSID of profile to remove.
     * @param[in]     ssid_len Length of SSID.
     *
     * @return BSP_ERR_STS_OK             Profile removed.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid arguments.
     * @return BSP_ERR_STS_FAIL           Removal failed.
     */
    bsp_err_sts_t bspWifiRemoveStationProfile(bspWifiHandle_t* handle,
                                              const uint8_t* ssid,
                                              uint8_t ssid_len);


    /*
     **********************************************************************
     *     WIFI AP PUBLIC API
     **********************************************************************
     */
    /**
     * @brief Start Wi-Fi Access Point.
     *
     * Configures and starts the AP interface using the supplied configuration.
     *
     * @param[in,out] handle BSP Wi-Fi handle.
     * @param[in]     apCfg  Access Point configuration.
     *
     * @return BSP_ERR_STS_OK             AP started successfully.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid parameters.
     * @return BSP_ERR_STS_FAIL           Platform failure.
     */
    bsp_err_sts_t bspWifiStartAP(bspWifiHandle_t* handle, const bspWifiApConfig_t* apCfg);

    /**
     * @brief Stop Wi-Fi Access Point.
     *
     * Stops the running AP interface.
     *
     * @param[in,out] handle BSP Wi-Fi handle.
     *
     * @return BSP_ERR_STS_OK             AP stopped successfully.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid handle.
     */
    bsp_err_sts_t bspWifiStopAP(bspWifiHandle_t* handle);


    /**
     * @brief Get current Access Point configuration.
     *
     * @param[in,out] handle BSP Wi-Fi handle.
     * @param[out]    apCfg  Output AP configuration.
     *
     * @return BSP_ERR_STS_OK             Success.
     * @return BSP_ERR_STS_NOT_RUNNING    AP not active.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid arguments.
     */
    bsp_err_sts_t bspWifiGetAPConfig(bspWifiHandle_t* handle, bspWifiApConfig_t* apCfg);


    /**
     * @brief Get Access Point runtime statistics.
     *
     * Retrieves traffic and radio statistics for the running AP.
     *
     * @param[in,out] handle BSP Wi-Fi handle.
     * @param[out]    stats  Output statistics structure.
     *
     * @return BSP_ERR_STS_OK             Success.
     * @return BSP_ERR_STS_NOT_RUNNING    AP not active.
     * @return BSP_ERR_STS_INVALID_PARAM  Invalid arguments.
     */
    bsp_err_sts_t bspWifiGetAPStatistics(bspWifiHandle_t* handle, bspWifiStatistics_t* stats);


    /*
     **********************************************************************
     *     WIFI SCAN PUBLIC API
     **********************************************************************
     */
    /**
     * @brief Start Wi-Fi scan (STA mode).
     *
     * Scan results are delivered asynchronously to the application
     * via the Wi-Fi event queue.
     *
     * @param[in,out] handle BSP Wi-Fi handle.
     * @param[out]    results  Output scan results.
     * @param[in]     max_results  Maximum number of results to return.
     *
     * @return BSP_ERR_STS_OK             Scan started.
     * @return BSP_ERR_STS_BUSY           Scan already in progress.
     * @return BSP_ERR_STS_INVALID_STATE  Wi-Fi not started.
     */
    bsp_err_sts_t bspWifiStartScan(bspWifiHandle_t* handle,
                                   bspWifiApScanResult_t* results,
                                   uint8_t max_results);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_WIFI_H__ */