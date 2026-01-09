
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
 * @note All functions return an @ref errStatus_t indicating the result of the operation.
 *       Refer to @ref bsp_err.h for error code definitions.
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
 * - err_status.h: Defines the errStatus_t type and error codes used by all BSP Wi-Fi API functions.
 */
#include "bsp_config.h"
#include "err_status.h"

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

    // =========================
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
        eWiFiModeNotSupported /**< Unsupported mode. */
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

        /* ---------- Station (STA) Events ---------- */
        eBSPWifiEventSTAConnected,    /**< Station connected to AP */
        eBSPWifiEventSTADisconnected, /**< Station disconnected from AP */
        eBSPWifiEventSTAGotIP,        /**< Station received IP address */

        /* ---------- Access Point (AP) Events ---------- */
        eBSPWifiEventAPStateChanged,        /**< AP started or stopped */
        eBSPWifiEventAPStationConnected,    /**< A station connected to AP */
        eBSPWifiEventAPStationDisconnected, /**< A station disconnected from AP */

        /* ---------- WPS Events ---------- */
        eBSPWifiEventWPSSuccess, /**< WPS succeeded */
        eBSPWifiEventWPSFailed,  /**< WPS failed */
        eBSPWifiEventWPSTimeout, /**< WPS timed out */

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
        eBSPWifiPhy11b = 0, /**< IEEE 802.11b. */
        eBSPWifiPhy11g,     /**< IEEE 802.11g. */
        eBSPWifiPhy11n,     /**< IEEE 802.11n. */
        eBSPWifiPhy11ac,    /**< IEEE 802.11ac. */
        eBSPWifiPhy11ax,    /**< IEEE 802.11ax. */
        eBSPWifiPhyMax      /**< Maximum PHY mode value (end marker). */
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
     * @brief Wi-Fi connection status.
     *
     * Enumerates the possible connection statuses for a Wi-Fi network.
     */
    typedef enum
    {
        eBSPWifiConnected = 0,    /**< Connected to a Wi-Fi network. */
        eBSPWifiDisconnected,     /**< Disconnected from a Wi-Fi network. */
        eBSPWifiConnectionFailed, /**< Connection attempt failed. */
        eBSPWifiMax /**< Maximum connection status value (end marker). */
    } bspWifiConnectionStatus_t;

    /**
     * @brief Wi-Fi IP address types.
     *
     * Enumerates the possible IP address types for Wi-Fi connections.
     */
    typedef enum {
        eBSPWifiIPv4 = 0,
        eBSPWifiIPv6,
        eBSPWifiIPNotSupported
    } bspWifiIPAddressType_t;

    /**
     * @brief Wi-Fi configuration structure for AP or Station mode.
     *
     * This structure holds the configuration parameters required to initialize
     * a Wi-Fi interface in either Access Point (AP) or Station mode.
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< SSID of the network. */
        uint8_t password[BSP_WIFI_PASSWORD_MAX_LEN]; /**< Password of the network. */
        bspWifiSecurity_t security; /**< Security type (see bspWifiSecurity_t). */
        uint8_t channel;            /**< Channel number (1-11 for 2.4GHz). */
        uint8_t ssidHidden;     /**< SSID hidden: 0 = visible, 1 = hidden. */
        uint8_t maxConnections; /**< Maximum connections for AP mode (1-4). */
        uint8_t pmMode;    /**< Power management mode (see bspWifiPmMode_t). */
        uint8_t bandwidth; /**< Channel bandwidth (see bspWifiBandwidth_t). */
        uint8_t phyMode;   /**< PHY mode (see bspWifiPhyMode_t). */
        uint32_t maxIdlePeriodSec; /** Maximum sec wifi keep idle */
    } bspWifiConfig_t;


    /**
     * @brief Wi-Fi WEP key structure.
     *
     * Holds a single WEP key and its length for use in WEP-secured networks.
     */
    typedef struct
    {
        uint8_t cKey[BSP_WIFI_WEP_KEY_LEN_MAX]; /**< WEP key (binary array, not C-string). */
        uint8_t u8Length;                       /**< Length of the WEP key. */
    } bspWifiWepKey_t;

    /**
     * @brief WPA/WPA2 passphrase structure.
     *
     * Stores a WPA/WPA2 passphrase and its length for use in secure Wi-Fi connections.
     */
    typedef struct
    {
        uint8_t cPassphrase[BSP_WIFI_PASSWORD_MAX_LEN]; /**< WPA passphrase (binary array, not C-string). */
        uint8_t u8Length; /**< Length of the passphrase (8-64 uint8_tacters). */
    } bspWifiWpaPassphrase_t;

    /**
     * @brief Wi-Fi network parameters for connection.
     *
     * Contains all parameters required to connect to a Wi-Fi network, including
     * SSID, security type, and credentials (WEP or WPA/WPA2).
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< SSID of the Wi-Fi network (binary array, not C-string). */
        uint8_t ssidLength;         /**< Length of the SSID. */
        bspWifiSecurity_t security; /**< Security type (see bspWifiSecurity_t). */
        union
        {
            bspWifiWepKey_t wep[BSP_WIFI_WEP_KEY_LEN_MAX]; /**< WEP keys (64- and 128-bit keys only). */
            bspWifiWpaPassphrase_t wpa; /**< WPA/WPA2 passphrase. */
        } password;                     /**< Credentials for the network. */
        uint8_t defaultWepKeyIndex; /**< Default WEP key index (0 to wificonfigMAX_WEPKEYS - 1). */
        uint8_t channel; /**< Channel number (1-11 for 2.4GHz). */
    } bspWifiNetworkParams_t;

    /**
     * @brief Wi-Fi scan configuration structure.
     *
     * Used to specify parameters for scanning Wi-Fi networks, such as targeted SSID and channel.
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< SSID for targeted scan (binary array, not C-string). */
        uint8_t ssidLength; /**< SSID length, 0 if broadcast scan. */
        uint8_t channel;    /**< Channel to scan (0 means all channels). */
        uint8_t show_hidden; /**< Whether to show hidden networks (0 = no, 1 = yes). */
    } bspWifiScanConfig_t;

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
    } bspWifiScanResult_t;

    /**
     * @brief Wi-Fi station information structure.
     *
     * Contains the MAC address of a connected Wi-Fi station.
     */
    typedef struct
    {
        uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< MAC address of the station. */
        uint8_t rssi;                           /**< rssi of stations */
    } bspWifiStationInfo_t;

    /**
     * @brief Wi-Fi network profile structure.
     *
     * Stores a saved Wi-Fi network profile, including SSID, BSSID, password, and security type.
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< SSID of the network. */
        uint8_t ssidLength;                  /**< Length of the SSID. */
        uint8_t bssid[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< BSSID (MAC address) of the AP. */
        uint8_t password[BSP_WIFI_PASSWORD_MAX_LEN]; /**< Password for the network. */
        uint8_t passwordLength;     /**< Length of the password. */
        bspWifiSecurity_t security; /**< Security type. */
    } bspWifiNetworkProfile_t;


    /** */
    typedef struct
    {
        bspWifiNetworkParams_t staConfig; /**< Station side config */
        bspWifiConfig_t apConfig;         /**< Access Point config */
    } bspWifiApStaConfig_t;


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
     * @brief Wi-Fi connection information structure.
     *
     * Contains information about the current Wi-Fi connection.
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< SSID of the connected network. */
        uint8_t ssidLength;                  /**< Length of the SSID. */
        uint8_t bssid[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< BSSID (MAC address) of the AP. */
        bspWifiSecurity_t security;               /**< Security type. */
        uint8_t channel;                          /**< Channel number. */
        bspWifiReason_t disconReason;             /**< Disconnected reason */
    } bspWifiConnectionInfo_t;

    /**
     * @brief Wi-Fi statistics information structure.
     *
     * Provides statistics and counters for Wi-Fi transmission and reception.
     */
    typedef struct
    {
        uint32_t txSuccessCount; /**< Number of successful transmissions. */
        uint32_t txRetryCount;   /**< Number of transmission retries. */
        uint32_t txFailCount;    /**< Number of failed transmissions. */
        uint32_t rxSuccessCount; /**< Number of successful receptions. */
        uint32_t rxCRCErrorCount; /**< Number of CRC errors in received frames. */
        uint32_t micErrorCount; /**< Number of MIC (Message Integrity Code) errors. */
        int8_t noise;           /**< Noise level in dBm. */
        uint16_t phyRate;       /**< Physical layer rate. */
        uint16_t txRate;        /**< Transmission rate. */
        uint16_t rxRate;        /**< Reception rate. */
        int8_t rssi;            /**< Received Signal Strength Indicator. */
        uint8_t bandwidth;   /**< Channel bandwidth. */
        uint8_t idleTimePer; /**< Percentage of idle time. */
    } bspWifitisticInfo_t;


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

    typedef struct
    {
        bspWifiEvent_t tEventType;
        union
        {
            bspWifiConnectionInfo_t tConnectionInfo;
            bspWifiScanResult_t tScanResult;
            bspWifiNetworkProfile_t tNetworkProfile;
            bspWifiApStaConfig_t tApStaConfig;
            bspWifiIPConfig_t tIPConfig;
            bspWifiStationInfo_t tStationInfo;
        } tEventData;
    } bspWifiContext_t;


    /**
     * @brief  Wi-Fi event callback function type.
     * @param[in] event The Wi-Fi event type (see bspWifiEvent_t).
     * @param[in] data Pointer to event-specific data (may be NULL).
     * @return None
     */
    typedef void (*bspWifiEventCallback_t)(bspWifiEvent_t event, void* data);


    /**
     * @brief Register a callback for Wi-Fi events.
     * @param[in] event The Wi-Fi event type to register for (see bspWifiEvent_t).
     * @param[in] callback Function pointer to event callback.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL if registration fails.
     * @return ERR_STS_INV_PARAM if callback is invalid.
     */
    errStatus_t bspWifiRegisterEventCallback(bspWifiEvent_t ptEvent, bspWifiEventCallback_t pCallback)


    /**
     * @brief Unregister the Wi-Fi event callback.
     * @param[in] event The Wi-Fi event type to unregister (@see bspWifiEvent_t).
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL if unregistration fails.
     */
    errStatus_t bspWifiUnregisterEventCallback(bspWifiEvent_t ptEvent);

    /**
     * @brief Initialize and start the Wi-Fi interface in the specified mode.
     *
     * This function initializes the Wi-Fi hardware and stack, sets the desired
     * Wi-Fi operation mode, and starts the interface. Depending on the selected
     * mode, it may also start a SoftAP or prepare the station interface for
     * connection.
     *
     * @param[in] mode   The desired Wi-Fi mode. Can be one of:
     *                   - eWiFiModeStation       : Station mode (connects to an AP)
     *                   - eWiFiModeAP            : Access Point mode
     *                   - eWiFiModeP2P           : Peer-to-peer mode
     *                   - eWiFiModeAPStation     : Simultaneous AP + Station mode
     *                   - eWiFiModeNotSupported  : Unsupported mode
     * @param[in] pvParam Optional pointer to configuration parameters depending
     *                    on the mode:
     *                    - For AP mode: pointer to `bspWifiConfig_t`
     *                    - For Station mode: pointer to `bspWifiNetworkParams_t` (optional)
     *                    - For other modes: can be NULL
     *
     * @return ERR_STS_OK                Wi-Fi started successfully.
     * @return ERR_STS_INIT_FAIL         Failed to initialize Wi-Fi hardware or stack.
     * @return ERR_STS_INV_PARAM         Invalid mode or invalid parameter provided.
     * @return ERR_STS_FAIL              Generic failure during start-up.
     *
     * @note The function is blocking and returns only after the Wi-Fi interface
     *       is initialized and ready. For asynchronous connection, use
     *       bspWifiConnectToAp after starting in Station mode.
     */
    errStatus_t bspWifiOn(bspWifiMode_t mode, void* pvParam);


    /**
     * @brief Deinitialize and stop the Wi-Fi interface.
     *
     * This function stops the Wi-Fi interface, disables the Wi-Fi hardware,
     * and releases any resources allocated by the Wi-Fi stack. After calling
     * this function, the Wi-Fi module will no longer be operational until
     * `bspWifiOn()` is called again.
     *
     * @return ERR_STS_OK    Wi-Fi module successfully deinitialized and stopped.
     * @return ERR_STS_FAIL  Failed to stop or deinitialize the Wi-Fi hardware/stack.
     *
     * @note Any ongoing connections or network operations will be terminated.
     */
    errStatus_t bspWifiOff(void);


    /**
     * @brief Connect to a Wi-Fi Access Point.
     * @param[in] params Pointer to network parameters.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_CONN_FAIL if connection fails.
     * @return ERR_STS_AUTH_FAIL if authentication fails.
     * @return ERR_STS_TIMEOUT if connection times out.
     * @return ERR_STS_INV_PARAM if parameters are invalid.
     */
    errStatus_t bspWifiConnectToAp(const bspWifiNetworkParams_t* const params);


    /**
     * @brief Disconnect from the currently connected Wi-Fi Access Point.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_DISCONN_FAIL if disconnection fails.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiDisconnectToAp(void);


    /**
     * @brief Reset the Wi-Fi hardware and stack.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiReset(void);


    /**
     * @brief Set the Wi-Fi operation mode.
     * @param[in] mode Desired Wi-Fi mode.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_INV_PARAM if mode is invalid.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiSetMode(bspWifiMode_t mode);


    /**
     * @brief Get the current Wi-Fi operation mode.
     * @param[out] mode Pointer to store the current mode.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiGetMode(bspWifiMode_t* mode);


    /**
     * @brief Add a Wi-Fi network profile to the saved list.
     * @param[in] ptProfile Pointer to the network profile to add.
     * @param[in] pu16Index Pointer to the new index added.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     * @return ERR_STS_INV_PARAM if profile is invalid or list is full.
     */
    errStatus_t bspWifiAddNetworkProfile(const bspWifiNetworkProfile_t* const ptProfile,
                                         uint16_t* pu16Index);


    /**
     * @brief Remove a Wi-Fi network profile by SSID.
     * @param[in] ssid Pointer to SSID string.
     * @param[in] ssidLength Length of the SSID.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL if profile not found or removal fails.
     * @return ERR_STS_INV_PARAM if parameters are invalid.
     */
    errStatus_t bspWifiRemoveNetworkProfile(const uint8_t* const ssid, uint8_t ssidLength);


    /**
     * @brief Clear all saved Wi-Fi network profiles.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiClearNetworkProfiles(void);


    /**
     * @brief Get a saved Wi-Fi network profile by SSID.
     * @param[in] pu8SSID Pointer to SSID string.
     * @param[in] u8SSIDLen Length of the SSID.
     * @param[out] ptProfile Pointer to store the retrieved profile.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL if profile not found.
     * @return ERR_STS_INV_PARAM if parameters are invalid.
     */
    errStatus_t bspWifiGetNetworkProfiles(const uint8_t* const pu8SSID,
                                          uint8_t u8SSIDLen,
                                          bspWifiNetworkProfile_t* const ptProfile);
    /**
     * @brief Send ICMP ping(s) to a remote IP address.
     * @param[in] pu8IpAddress IP address string.
     * @param[in] u16Count Number of pings to send.
     * @param[in] u16IntervalMs Interval between pings in ms.
     * @param[in] u16TimeoutMs Timeout for each ping in ms.
     * @param[out] pu16Received Pointer to store number of replies received.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_TIMEOUT if ping times out.
     * @return ERR_STS_FAIL for general failure.
     * @return ERR_STS_INV_PARAM if parameters are invalid.
     */
    errStatus_t bspStaWifiPing(const uint8_t* const pu8IpAddress,
                               uint16_t u16Count,
                               uint16_t u16IntervalMs,
                               uint16_t u16TimeoutMs,
                               uint16_t* pu16Received);


    /**
     * @brief Get the MAC address of the Wi-Fi interface.
     * @param[out] mac Pointer to buffer for MAC address.
     * @param[out] length Pointer to store MAC address length.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiGetMacAddress(uint8_t* mac, uint8_t* length);


    /**
     * @brief Resolve a hostname to an IP address.
     * @param[in] hostname Hostname string.
     * @param[out] ipAddress Pointer to buffer for IP address.
     * @param[out] length Pointer to store IP address length.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL if resolution fails.
     * @return ERR_STS_INV_PARAM if parameters are invalid.
     */
    errStatus_t bspWifiGetHostIPAddress(const uint8_t* const pu8Hostname,
                                        uint8_t* pu8IpAddress,
                                        uint8_t* pu8Length);


    /**
     * @brief Start the Wi-Fi Access Point with the given configuration.
     * @param[in] config Pointer to AP configuration structure.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     * @return ERR_STS_INV_PARAM if configuration is invalid.
     */
    errStatus_t bspWifiStartAP(const bspWifiConfig_t* const ptConfig);


    /**
     * @brief Stop the Wi-Fi Access Point.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiStopAp(void);

    /**
     * @brief Check if the device is connected to a Wi-Fi network.
     * @param[in] params Pointer to network parameters to check.
     * @param[out] isConnected Pointer to store connection status (1 = connected, 0 = not connected).
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     * @return ERR_STS_INV_PARAM if parameters are invalid.
     */
    errStatus_t bspWifiIsConnected(const bspWifiNetworkParams_t* const ptNetwork,
                                   uint8_t* u8IsConnected);

    /**
     * @brief Set the Wi-Fi power management mode.
     * @param[in] pmMode Power management mode to set.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_INV_PARAM if mode is invalid.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiSetPMMode(bspWifiPmMode_t pmMode);

    /**
     * @brief Get the current Wi-Fi power management mode.
     * @param[out] pmMode Pointer to store the current mode.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiGetPMMode(bspWifiPmMode_t* pmMode);

    /**
     * @brief Start scanning for Wi-Fi networks with the given configuration.
     * @param[in] config Pointer to scan configuration structure.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     * @return ERR_STS_INV_PARAM if configuration is invalid.
     */
    errStatus_t bspWifiStartScan(const bspWifiScanResult_t* const ptScanResult,
                                 bspWifiScanConfig_t* const ptScanConfig,
                                 uint16_t* pu16NumNetworks, uint16_t pu16MaxNumNetworks);


    /**
     * @brief Get the list of connected stations (clients) when in AP mode.
     *
     * @param[out] ptStations      Pointer to array where station info will be written.
     * @param[in,out] pu8NumStations
     *      - Input: pointer holding the maximum number of stations to fetch (array capacity).
     *      - Output: actual number of stations copied.
     * @param[in]  u8MaxStations   Maximum stations that can be returned in ptStations.
     *
     * @return ERR_STS_OK                  On success
     * @return ERR_STS_INV_PARAM           If any pointer is NULL
     * @return ERR_STS_NOT_READY           If WiFi/AP is not running
     * @return ERR_STS_NO_MEM              If output buffer is too small
     * @return ERR_STS_UNSUPPORTED_FEATURE If AP mode is not supported in BSP
     */
    errStatus_t bspWifiGetConnectedStations(bspWifiStationInfo_t* stations,
                                            uint8_t* numStations,
                                            uint8_t maxStations);


    /**
     * @brief Clear the list of connected stations (AP mode).
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiClearConnectedStations(void);

    // TODO:
    /**
     * @brief Start connecting to a Wi-Fi Access Point asynchronously.
     * @param[in] params Pointer to network parameters.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_CONN_FAIL if connection fails.
     * @return ERR_STS_AUTH_FAIL if authentication fails.
     * @return ERR_STS_TIMEOUT if connection times out.
     * @return ERR_STS_INV_PARAM if parameters are invalid.
     */
    errStatus_t bspWifirtConnectAP(const bspWifiNetworkParams_t* const params);

    // TODO:
    /**
     * @brief Stop the asynchronous connection attempt to a Wi-Fi AP.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiStopConnectAP(void);

    // TODO:
    /**
     * @brief Get information about the current Wi-Fi connection.
     * @param[out] info Pointer to store connection info.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiGetConnectionInfo(bspWifiConnectionInfo_t* info);

    // TODO:
    /**
     * @brief Get the current IP configuration (legacy, use bspWifiGetIPConfig).
     * @param[out] info Pointer to store IP configuration.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiGetIPInfo(bspWifiConfig_t* info);

    // TODO:
    /**
     * @brief Set the static IP configuration for the Wi-Fi interface.
     * @param[in] ipConfig Pointer to IP configuration structure.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_INV_PARAM if configuration is invalid.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiSetIPConfig(const bspWifiIPConfig_t* const ipConfig);

    /**
     * @brief Get the current RSSI (signal strength).
     *
     * Reads the received signal strength of the connected AP.
     * Valid only when STA is connected to an Access Point.
     *
     * @param[out] pi8RSSI Pointer to store RSSI value.
     * @return ERR_STS_OK   Successfully retrieved RSSI.
     * @return ERR_STS_FAIL Wi-Fi not connected or query failed.
     * @return ERR_STS_INV_PARAM If output pointer is NULL.
     */
    errStatus_t bspWifiGetRSSI(int8_t* pi8RSSI);


    /**
     * @brief Get the current Wi-Fi channel.
     * @param[out] channel Pointer to store channel number.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiGetChannel(uint8_t* pu8Channel);


    /**
     * @brief Get the MAC address of the Wi-Fi interface.
     * @param[out] mac Pointer to buffer for MAC address.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiGetMAC(uint8_t* pu8MAC);

    /**
     * @brief Get the current Wi-Fi country code.
     * @param[out] countryCode Pointer to buffer for country code string.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiGetCountryCode(uint8_t* pu8CountryCode);


    /**
     * @brief Set the Wi-Fi country code.
     * @param[in] countryCode Pointer to country code string.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_INV_PARAM if code is invalid.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiSetCountryCode(const uint8_t* pu8CountryCode);


    /**
     * @brief Get Wi-Fi statistics information.
     * @param[out] stats Pointer to statistics info structure.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiGetStatistics(bspWifitisticInfo_t* ptStatisticInfo);


    /**
     * @brief Get Wi-Fi hardware and feature capability information.
     * @param[out] capInfo Pointer to capability info structure.
     * @return ERR_STS_OK on success.
     * @return ERR_STS_FAIL for general failure.
     */
    errStatus_t bspWifiGetCapabilityInfo(bspWifiCapabilityInfo_t* ptCapInfo);


#ifdef __cplusplus
}
#endif

#endif /* __BSP_WIFI_H__ */