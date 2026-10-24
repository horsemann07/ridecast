/* ============================================================================
 * File        : bsp_wifi.h
 * Description : Portable BSP Wi-Fi public interface
 * Author      : Jha Raghav
 * Date        : 2026-10-24
 * ============================================================================
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Description:
 *   Defines board-independent Wi-Fi configuration, events and operations.
 *   No ESP-IDF or RTOS header is exposed by this API.
 *
 * Integration:
 *   The ESP-IDF implementation resides in bsp/srcs/port/esp32x/bsp_wifi.c.
 *   Other boards can implement the same interface without importing ESP types.
 *
 * Requirements:
 *   - C11 compatible compiler
 *   - BSP configuration and shared ret_err_sts_t header
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-24    Jha Raghav    Application profile callbacks, no direct persistence
 * 2026-10-24    Jha Raghav    Platform-neutral handle, shared status codes
 * 2026-10-23    Jha Raghav    Initial Wi-Fi API (original supplied version)
 * ============================================================================ */
#ifndef BSP_WIFI_H
#define BSP_WIFI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "bsp_config.h"
#include "bsp_err_sts.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Maximum SSID length supported by the BSP configuration. */
#define BSP_WIFI_SSID_MAX_LEN         bspCONFIG_WIFI_SSID_MAX_LEN
/* Maximum password or credential length supported by the BSP configuration. */
#define BSP_WIFI_PASSWORD_MAX_LEN     bspCONFIG_WIFI_PASSWORD_MAX_LEN
/* Maximum textual IP address length supported by the BSP configuration. */
#define BSP_WIFI_IP_ADDR_MAX_LEN      bspCONFIG_WIFI_IP_ADDR_MAX_LEN
/* Maximum MAC address length supported by the BSP configuration. */
#define BSP_WIFI_MAC_ADDR_MAX_LEN     bspCONFIG_WIFI_MAC_ADDR_MAX_LEN
/* Maximum number of saved network profiles supported by the BSP configuration. */
#define BSP_WIFI_MAX_SAVED_NETWORKS   bspCONFIG_WIFI_MAX_SAVED_NETWORKS

/* Size of an IPv4 address in bytes. */
#define BSP_WIFI_IP4_ADDR_SIZE        (4U)
/* Size of an IPv6 address in bytes. */
#define BSP_WIFI_IP6_ADDR_SIZE        (16U)
/* Size of a MAC address in bytes. */
#define BSP_WIFI_MAC_ADDR_SIZE        (6U)
/* Maximum WEP key length in bytes. */
#define BSP_WIFI_WEP_KEY_LEN_MAX      (13U)

/* Bit in bspWifiStatistics_t.validMask indicating connectedStations is valid. */
#define BSP_WIFI_STATS_VALID_STATIONS (1UL << 0U)

#if (BSP_WIFI_SSID_MAX_LEN < 32U)
    #error "bspCONFIG_WIFI_SSID_MAX_LEN must be >= 32"
#endif
#if (BSP_WIFI_PASSWORD_MAX_LEN < 64U)
    #error "bspCONFIG_WIFI_PASSWORD_MAX_LEN must be >= 64"
#endif
#if (BSP_WIFI_MAC_ADDR_MAX_LEN != 6U)
    #error "bspCONFIG_WIFI_MAC_ADDR_MAX_LEN must equal 6"
#endif
#if (BSP_WIFI_MAX_SAVED_NETWORKS < 1U) || (BSP_WIFI_MAX_SAVED_NETWORKS > 32U)
    #error "bspCONFIG_WIFI_MAX_SAVED_NETWORKS must be 1..32"
#endif

    /**
     * @brief Wi-Fi operation modes supported by the BSP interface.
     */
    typedef enum
    {
        eWiFiModeStation = 0, /**< Station (client) mode. */
        eWiFiModeAP,          /**< Access point mode. */
        eWiFiModeP2P, /**< Peer-to-peer mode; may be unsupported by a port. */
        eWiFiModeAPStation, /**< Simultaneous access point and station mode. */
        eWiFiModeMax        /**< End marker; not a supported operation mode. */
    } bspWifiMode_t;

    /**
     * @brief Wi-Fi lifecycle and connection event types.
     *
     * @details
     * Events cover general Wi-Fi state, station and access-point activity,
     * scanning, and WPS results.
     */
    typedef enum
    {
        /* General Wi-Fi events. */
        eBSPWifiEventReady = 0, /**< Wi-Fi subsystem initialized and ready. */
        eBSPWifiEventScanDone,  /**< Wi-Fi scan completed. */
        eBSPWifiEventConnected, /**< Connected to a Wi-Fi network. */
        eBSPWifiEventDisconnected, /**< Disconnected from a Wi-Fi network. */
        eBSPWifiEventConnectionFailed, /**< Connection attempt failed. */
        eBSPWifiEventIPReady,          /**< An IP address was acquired. */
        eBSPWifiEventIPFailed,         /**< IP address acquisition failed. */

        /* Station events. */
        eBSPWifiEventSTAStarted,   /**< Wi-Fi station started. */
        eBSPWifiEventSTAStopped,   /**< Wi-Fi station stopped. */
        eBSPWifiEventSTAConnected, /**< Station connected to an access point. */
        eBSPWifiEventSTADisconnected, /**< Station disconnected from an access point. */
        eBSPWifiEventSTAGotIP, /**< Station received an IP address. */

        /* Access point events. */
        eBSPWifiEventAPStarted,      /**< Access point started. */
        eBSPWifiEventAPStopped,      /**< Access point stopped. */
        eBSPWifiEventAPStateChanged, /**< Access point started or stopped. */
        eBSPWifiEventAPStationConnected, /**< A station connected to the access point. */
        eBSPWifiEventAPStationDisconnected, /**< A station disconnected from the access point. */

        /* WPS events. */
        eBSPWifiEventWPSSuccess, /**< WPS operation succeeded. */
        eBSPWifiEventWPSFailed,  /**< WPS operation failed. */
        eBSPWifiEventWPSTimeout, /**< WPS operation timed out. */

        eBSPWifiEventUnknown,    /**< Event could not be identified. */
        /* End marker. */
        eBSPWifiEventMax /**< Number of event values; must remain last. */

    } bspWifiEvent_t;

    /**
     * @brief Wi-Fi security types.
     */
    typedef enum
    {
        eWiFiSecurityOpen = 0,    /**< Open network without link security. */
        eWiFiSecurityWEP,         /**< WEP security. */
        eWiFiSecurityWPA,         /**< WPA security. */
        eWiFiSecurityWPA2,        /**< WPA2 security. */
        eWiFiSecurityWPA2_ent,    /**< WPA2 Enterprise security. */
        eWiFiSecurityWPA3,        /**< WPA3 security. */
        eWiFiSecurityNotSupported /**< Security type is unknown or unsupported. */
    } bspWifiSecurity_t;

    /**
     * @brief Wi-Fi power-management modes.
     */
    typedef enum
    {
        eBSPWifiPmNormal = 0,  /**< Normal power-management mode. */
        eBSPWifiPmLowPower,    /**< Low-power mode. */
        eBSPWifiPmAlwaysOn,    /**< Keep Wi-Fi active. */
        eBSPWifiPmNotSupported /**< Power-management mode is unsupported. */
    } bspWifiPmMode_t;

    /**
     * @brief Wi-Fi frequency bands.
     */
    typedef enum
    {
        eBSPWifiBand2G = 0, /**< 2.4 GHz band. */
        eBSPWifiBand5G,     /**< 5 GHz band. */
        eBSPWifiBandDual,   /**< Both 2.4 GHz and 5 GHz bands. */
        eBSPWifiBandMax     /**< End marker; not a frequency band. */
    } bspWifiBand_t;

    /**
     * @brief Wi-Fi physical-layer modes.
     */
    typedef enum
    {
        eBSPWifiPhyNone = 0, /**< No PHY mode reported. */
        eBSPWifiPhy11b  = 1, /**< IEEE 802.11b. */
        eBSPWifiPhy11g  = 2, /**< IEEE 802.11g. */
        eBSPWifiPhy11n  = 3, /**< IEEE 802.11n. */
        eBSPWifiPhy11ac = 4, /**< IEEE 802.11ac. */
        eBSPWifiPhy11ax = 5, /**< IEEE 802.11ax. */
        eBSPWifiPhyLR   = 6, /**< Wi-Fi long-range mode. */
    } bspWifiPhyMode_t;

    /**
     * @brief Wi-Fi channel bandwidths.
     */
    typedef enum
    {
        eBSPWifiBW20 = 0, /**< 20 MHz bandwidth. */
        eBSPWifiBW40,     /**< 40 MHz bandwidth. */
        eBSPWifiBW80,     /**< 80 MHz bandwidth. */
        eBSPWifiBW160,    /**< 160 MHz bandwidth. */
        eBSPWifiBWMax     /**< End marker; not a bandwidth. */
    } bspWifiBandwidth_t;

    /**
     * @brief Wi-Fi disconnection and failure reasons.
     */
    typedef enum
    {
        eBSPWifiReasonUnspecified = 0, /**< Reason was not specified. */
        eBSPWifiReasonAPNotFound,      /**< Access point was not found. */
        eBSPWifiReasonAuthExpired,     /**< Authentication expired. */
        eBSPWifiReasonAuthLeaveBSS,    /**< Authentication left the BSS. */
        eBSPWifiReasonAuthFailed,      /**< Authentication failed. */
        eBSPWifiReasonAssocExpired,    /**< Association expired. */
        eBSPWifiReasonAssocTooMany,    /**< Too many associations. */
        eBSPWifiReasonAssocPowerCapBad, /**< Association failed due to power capability. */
        eBSPWifiReasonAssocSupChanBad, /**< Association failed due to supported channels. */
        eBSPWifiReasonAssocFailed, /**< Association failed. */
        eBSPWifiReasonIEInvalid,   /**< Information element is invalid. */
        eBSPWifiReasonMICFailure,  /**< Message integrity check failed. */
        eBSPWifiReason4WayTimeout, /**< Four-way handshake timed out. */
        eBSPWifiReason4WayIEDiffer, /**< Four-way handshake information elements differ. */
        eBSPWifiReason4WayFailed, /**< Four-way handshake failed. */
        eBSPWifiReasonAKMPInvalid, /**< Authentication and key-management protocol is invalid. */
        eBSPWifiReasonPairwiseCipherInvalid, /**< Pairwise cipher is invalid. */
        eBSPWifiReasonGroupCipherInvalid,    /**< Group cipher is invalid. */
        eBSPWifiReasonRSNVersionInvalid,     /**< RSN version is invalid. */
        eBSPWifiReasonRSNCapInvalid, /**< RSN capabilities are invalid. */
        eBSPWifiReasonGroupKeyUpdateTimeout, /**< Group key update timed out. */
        eBSPWifiReasonCipherSuiteRejected,   /**< Cipher suite was rejected. */
        eBSPWifiReason8021XAuthFailed, /**< 802.1X authentication failed. */
        eBSPWifiReasonBeaconTimeout,   /**< Beacon reception timed out. */
        eBSPWifiReasonLinkFailed,      /**< Link failed. */
        eBSPWifiReasonDHCPExpired,     /**< DHCP lease expired. */
        eBSPWifiReasonDHCPFailed,      /**< DHCP operation failed. */
        eBSPWifiReasonMax /**< End marker; not a disconnection reason. */
    } bspWifiReason_t;

    /**
     * @brief IP address types used by the Wi-Fi interface.
     */
    typedef enum
    {
        eBSPWifiIPv4 = 0,      /**< IPv4 address. */
        eBSPWifiIPv6,          /**< IPv6 address. */
        eBSPWifiIPNotSupported /**< IP address type is unsupported. */
    } bspWifiIPAddressType_t;

    /* -------------------------------------------------------------------------- */
    /* Wi-Fi Data Structures */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Stores an IPv4 or IPv6 address.
     *
     * @note The address array contains four 32-bit words; interpretation
     * depends on type and the platform port.
     */
    typedef struct
    {
        bspWifiIPAddressType_t type; /**< Address type. */
        uint32_t address[4]; /**< Address data represented as four words. */
    } bspWifiIPAddress_t;

    /**
     * @brief IP configuration for a Wi-Fi interface.
     */
    typedef struct
    {
        bspWifiIPAddress_t ipAddress; /**< Interface IP address. */
        bspWifiIPAddress_t netMask;   /**< Network mask. */
        bspWifiIPAddress_t gateway;   /**< Gateway address. */
        bspWifiIPAddress_t dns1;      /**< Primary DNS server address. */
        bspWifiIPAddress_t dns2;      /**< Secondary DNS server address. */
    } bspWifiIPConfig_t;

    /**
     * @brief Information about a station connected to an access point.
     */
    typedef struct
    {
        uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< Station MAC address. */
        int8_t rssi;              /**< Received signal strength in dBm. */
        bspWifiPhyMode_t phyMode; /**< Station PHY mode. */
    } bspWifiStationInfo_t;

    /**
     * @brief Wi-Fi hardware capability information.
     *
     * @note Capability fields depend on the platform and driver.
     */
    typedef struct
    {
        bspWifiBand_t band;           /**< Supported Wi-Fi band. */
        bspWifiPhyMode_t phyMode;     /**< Supported PHY mode. */
        bspWifiBandwidth_t bandwidth; /**< Supported channel bandwidth. */
        uint32_t maxAggr; /**< Maximum aggregation size reported by the port. */
        uint16_t supportedFeatures; /**< Port-defined feature bitmask. */
    } bspWifiCapabilityInfo_t;

    /**
     * @brief Generic Wi-Fi statistics.
     *
     * @details
     * A port may populate only the fields it supports. Check validMask before
     * using fields whose validity is indicated by a defined mask bit.
     */
    typedef struct
    {
        /* Packet counters. */
        uint32_t txPackets; /**< Total transmitted packets, when available. */
        uint32_t rxPackets; /**< Total received packets, when available. */
        uint32_t txErrors;  /**< Transmission errors, when available. */
        uint32_t rxErrors;  /**< Reception errors, when available. */
        /* Link quality. */
        int8_t rssi;  /**< RSSI in dBm for the station or an AP-side average. */
        int8_t noise; /**< Noise floor in dBm, when supported. */
        /* Rates. */
        uint16_t txRateKbps; /**< Current transmit rate in kilobits per second. */
        uint16_t rxRateKbps; /**< Current receive rate in kilobits per second. */
        /* Access point data. */
        uint8_t connectedStations; /**< Number of connected stations in AP mode. */
        /* Validity. */
        uint32_t validMask; /**< Bitmask identifying fields reported as valid. */
    } bspWifiStatistics_t;

    /**
     * @brief Wi-Fi network configuration used for station connections and profiles.
     *
     * @note Credential data is sensitive. The application is responsible for
     *       protecting it when storing, copying, or logging this structure.
     */
    typedef struct
    {
        /* Network identity. */
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< SSID bytes; not NUL-terminated. */
        uint8_t ssidLength;                  /**< Number of valid SSID bytes. */
        uint8_t bssid[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< Optional BSSID; all zeroes means ignore. */
        /* Security configuration. */
        bspWifiSecurity_t security; /**< Security type for the network. */
        union
        {
            struct
            {
                uint8_t key[BSP_WIFI_PASSWORD_MAX_LEN]; /**< WEP key bytes. */
                uint8_t keyLength; /**< Number of valid WEP key bytes. */
                uint8_t keyIndex;  /**< Selected WEP key index. */
            } wep;
            struct
            {
                uint8_t passphrase[BSP_WIFI_PASSWORD_MAX_LEN]; /**< WPA passphrase bytes. */
                uint8_t length; /**< Number of valid passphrase bytes. */
            } wpa;
        } credential;
        /* Optional connection hint. */
        uint8_t channel; /**< Preferred channel; zero requests automatic selection. */
    } bspWifiNetworkConfig_t;

    /**
     * @brief Information about a Wi-Fi network found during a scan.
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< Discovered SSID bytes. */
        uint8_t ssidLength;                  /**< Number of valid SSID bytes. */
        uint8_t bssid[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< Access point MAC address. */
        bspWifiSecurity_t security; /**< Discovered network security type. */
        int8_t rssi;                /**< Received signal strength. */
        uint8_t channel;            /**< Network channel. */
        uint8_t num_result; /**< Result count used by the legacy scan API. */
    } bspWifiApScanResult_t;

    /**
     * @brief Configuration for operating as a Wi-Fi access point.
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< Access point SSID bytes. */
        uint8_t ssidLength;                  /**< Number of valid SSID bytes. */
        uint8_t password[BSP_WIFI_PASSWORD_MAX_LEN]; /**< Access point password bytes. */
        uint8_t passwordLength;     /**< Number of valid password bytes. */
        bspWifiSecurity_t security; /**< Access point security type. */
        uint8_t channel;            /**< Wi-Fi channel. */
        uint8_t ssidHidden;         /**< Nonzero hides the SSID. */
        uint8_t maxConnections;    /**< Maximum number of client connections. */
        uint8_t pmMode;            /**< Power-management mode value. */
        uint8_t bandwidth;         /**< Channel bandwidth value. */
        uint8_t phyMode;           /**< PHY mode value. */
        uint32_t maxIdlePeriodSec; /**< Maximum idle period in seconds. */
    } bspWifiApConfig_t;

    /**
     * @brief Configuration for connecting to a Wi-Fi access point as a station.
     *
     * @note Platform-specific power, PHY, and bandwidth tuning is handled by
     *       the port implementation.
     */
    typedef struct
    {
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< Target SSID bytes. */
        uint8_t ssidLength;                  /**< Number of valid SSID bytes. */
        uint8_t password[BSP_WIFI_PASSWORD_MAX_LEN]; /**< WPA/WPA2/WPA3 passphrase bytes. */
        uint8_t passwordLength;     /**< Number of valid password bytes. */
        bspWifiSecurity_t security; /**< Target network security type. */
        uint8_t channel; /**< Optional fixed channel; zero selects automatically. */
    } bspWifiStaConfig_t;

    /**
     * @brief Combined station and access-point configuration.
     */
    typedef struct
    {
        bspWifiStaConfig_t staConfig; /**< Station configuration. */
        bspWifiApConfig_t apConfig;   /**< Access-point configuration. */
    } bspWifiApStaConfig_t;

    /**
     * @brief Runtime connection, link-quality, and peer information.
     *
     * @note Some fields may be unsupported by a platform. Treat this
     * information as available only while Wi-Fi is started.
     */
    typedef struct
    {
        /* Link identity. */
        uint8_t ssid[BSP_WIFI_SSID_MAX_LEN]; /**< Connected SSID bytes. */
        uint8_t ssidLength;                  /**< Number of valid SSID bytes. */
        uint8_t bssid[BSP_WIFI_MAC_ADDR_MAX_LEN]; /**< Access point MAC address. */
        bspWifiSecurity_t security;               /**< Active security type. */
        uint8_t channel;                          /**< Operating channel. */
        /* Connection state. */
        uint8_t staConnected; /**< Nonzero when the station is connected. */
        uint8_t apStarted;    /**< Nonzero when the access point is running. */
        bspWifiReason_t lastDisconnectReason; /**< Most recently reported disconnect reason. */
        /* Signal and link quality. */
        bspWifiBand_t band;       /**< Current frequency band. */
        bspWifiPhyMode_t phyMode; /**< Current PHY mode. */
        int8_t rssi;              /**< Received signal strength in dBm. */
        int8_t noise;             /**< Noise floor in dBm, when available. */
        /* Best-effort traffic statistics. */
        uint32_t txPackets; /**< Transmitted packet count. */
        uint32_t txErrors;  /**< Transmit error count. */
        uint32_t rxPackets; /**< Received packet count. */
        uint32_t rxErrors;  /**< Receive error count. */
        /* IP configuration. */
        bspWifiIPConfig_t ip; /**< Interface IP configuration. */
        /* Access-point information. */
        uint8_t connectedStations; /**< Connected station count in AP mode. */
    } bspWifiConnectionInfo_t;

    /**
     * @brief Event type and associated data delivered by the Wi-Fi event queue.
     *
     * @note The active tEventData member depends on tEventType.
     */
    typedef struct
    {
        bspWifiEvent_t tEventType; /**< Event represented by this context. */
        union
        {
            bspWifiConnectionInfo_t tConnectionInfo; /**< Connection, disconnection, or IP event data. */
            bspWifiApScanResult_t tScanResult; /**< Scan result event data. */
            bspWifiStaConfig_t tStationCfg; /**< Station start or configuration event data. */
            bspWifiApConfig_t tApCfg; /**< Access-point start or configuration event data. */
        } tEventData;
    } bspWifiContext_t;

    /**
     * @brief Optional application-owned station profile persistence callbacks.
     *
     * @details
     * The BSP does not select a filesystem, database, or NVS partition.
     * Callbacks run synchronously in the calling task and may block. They must
     * not call bspWifi* APIs. A callback that retains input data must copy it
     * before returning.
     *
     * A NULL save or remove callback skips that operation and is treated as
     * success. A NULL load callback makes profile retrieval unavailable.
     *
     * @warning Station credentials are sensitive; the application must store
     *          and handle them securely.
     * @note The context pointer must remain valid while the callbacks are registered.
     */
    typedef ret_err_sts_t (*bspWifiProfileSaveCallback_t)(const bspWifiStaConfig_t* station,
                                                          void* context);

    /**
     * @brief Load a station profile from application-owned storage.
     *
     * @param[in] index Profile index to retrieve.
     * @param[out] station Destination for the loaded profile.
     * @param[in] context Application-provided callback context.
     * @return Application-defined status for the load operation.
     */
    typedef ret_err_sts_t (*bspWifiProfileLoadCallback_t)(uint8_t index,
                                                          bspWifiStaConfig_t* station,
                                                          void* context);

    /**
     * @brief Remove a station profile from application-owned storage.
     *
     * @param[in] ssid SSID bytes identifying the profile.
     * @param[in] ssid_len Number of valid SSID bytes.
     * @param[in] context Application-provided callback context.
     * @return Application-defined status for the remove operation.
     */
    typedef ret_err_sts_t (*bspWifiProfileRemoveCallback_t)(const uint8_t* ssid,
                                                            uint8_t ssid_len,
                                                            void* context);

    /**
     * @brief Application callbacks used for station profile persistence.
     *
     * @note The callback context remains application-owned and must stay valid
     *       for as long as these callbacks are registered.
     */
    typedef struct
    {
        bspWifiProfileSaveCallback_t save; /**< Optional profile save callback. */
        bspWifiProfileLoadCallback_t load; /**< Optional profile load callback. */
        bspWifiProfileRemoveCallback_t remove; /**< Optional profile removal callback. */
        void* context; /**< Application-owned callback context. */
    } bspWifiProfileCallbacks_t;

    /**
     * @brief Public handle for a Wi-Fi instance.
     *
     * @pre Zero-initialize the handle before the first bspWifiInit() call.
     * @warning Do not inspect or modify platform_ctx while initialized.
     * @note The ESP32 port has one hardware Wi-Fi controller, so only one
     *       handle can be active at a time.
     */
    typedef struct
    {
        void* platform_ctx; /**< Opaque platform context owned by the BSP. */
    } bspWifiHandle_t;

    /**
     * @brief Initialize Wi-Fi resources for a handle.
     *
     * @param[in,out] handle Zero-initialized handle to initialize.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @note Repeated initialization with the same active handle is idempotent.
     */
    ret_err_sts_t bspWifiInit(bspWifiHandle_t* handle);

    /**
     * @brief Stop Wi-Fi and release resources associated with a handle.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @note The handle may be reused after successful deinitialization.
     */
    ret_err_sts_t bspWifiDeInit(bspWifiHandle_t* handle);

    /**
     * @brief Configure and start Wi-Fi in the selected mode.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[in] mode Station, access-point, or combined mode. P2P may be
     *                 unsupported by the port.
     * @param[in] pvParam Pointer to the mode-specific station, AP, or combined
     *                    configuration. The port copies the configuration.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @note For station mode, success indicates that an IP address was obtained
     *       within the implementation timeout.
     */
    ret_err_sts_t bspWifiOn(bspWifiHandle_t* handle, bspWifiMode_t mode, const void* pvParam);

    /**
     * @brief Stop active Wi-Fi while preserving initialized resources.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     */
    ret_err_sts_t bspWifiOff(bspWifiHandle_t* handle);

    /**
     * @brief Reconnect using the station settings from the previous bspWifiOn() call.
     *
     * @param[in,out] handle Initialized Wi-Fi handle with station configuration.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     */
    ret_err_sts_t bspWifiConnectToAp(bspWifiHandle_t* handle);

    /**
     * @brief Disconnect the station without stopping the Wi-Fi radio.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     */
    ret_err_sts_t bspWifiDisconnectToAp(bspWifiHandle_t* handle);

    /**
     * @brief Retrieve information about stations connected to the access point.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[out] stations Destination array for station information.
     * @param[out] numStations Number of entries copied to stations.
     * @param[in] maxStations Capacity of the stations array.
     * @return RET_ERR_STS_INVALID_SIZE if the array is too short; the first
     *         maxStations entries are still populated and numStations reports
     *         the copied count. Otherwise returns the operation status.
     */
    ret_err_sts_t bspWifiGetConnStations(bspWifiHandle_t* handle,
                                         bspWifiStationInfo_t* stations,
                                         uint8_t* numStations,
                                         uint8_t maxStations);

    /**
     * @brief Disconnect an access-point station by MAC address.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[in] mac MAC address of the station to disconnect.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     */
    ret_err_sts_t bspWifiDisconnectStation(bspWifiHandle_t* handle,
                                           const uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN]);

    /**
     * @brief Start an access point when Wi-Fi is off or configured for AP mode.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[in] apCfg Access-point configuration.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @note Use bspWifiOn() with eWiFiModeAPStation to run station and AP
     *       simultaneously.
     */
    ret_err_sts_t bspWifiStartAP(bspWifiHandle_t* handle, const bspWifiApConfig_t* apCfg);

    /**
     * @brief Stop the access point.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @note In AP+STA mode, the station remains active.
     */
    ret_err_sts_t bspWifiStopAP(bspWifiHandle_t* handle);

    /**
     * @brief Retrieve the current access-point configuration.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[out] apCfg Destination for the access-point configuration.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @warning The returned configuration may contain credentials; handle it
     *          as sensitive data.
     */
    ret_err_sts_t bspWifiGetAPConfig(bspWifiHandle_t* handle, bspWifiApConfig_t* apCfg);

    /**
     * @brief Retrieve supported access-point statistics.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[out] stats Destination for statistics.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @note Check stats->validMask before using fields whose validity is
     *       indicated by a defined mask bit.
     */
    ret_err_sts_t bspWifiGetAPStatistics(bspWifiHandle_t* handle, bspWifiStatistics_t* stats);

    /**
     * @brief Register or clear application-owned station profile callbacks.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[in] callbacks Callback configuration to copy, or NULL to clear all
     *                      registered callbacks.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @note The application retains ownership of callbacks->context. Register
     *       after initialization and do not race this call with deinitialization.
     */
    ret_err_sts_t bspWifiSetProfileCallbacks(bspWifiHandle_t* handle,
                                             const bspWifiProfileCallbacks_t* callbacks);

    /**
     * @brief Store a station profile using the registered save callback.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[in] station Station configuration to pass to the callback.
     * @return Callback status, or RET_ERR_STS_OK when no save callback is registered.
     */
    ret_err_sts_t bspWifiStoreStationProfile(bspWifiHandle_t* handle,
                                             const bspWifiStaConfig_t* station);

    /**
     * @brief Retrieve a station profile using the registered load callback.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[in] index Profile index to retrieve.
     * @param[out] sta_cfg Destination for the retrieved station configuration.
     * @return Callback status, or an error status if loading is unavailable or fails.
     */
    ret_err_sts_t bspWifiGetStationProfile(bspWifiHandle_t* handle,
                                           uint8_t index,
                                           bspWifiStaConfig_t* sta_cfg);

    /**
     * @brief Remove a station profile using the registered remove callback.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[in] ssid SSID bytes identifying the profile.
     * @param[in] ssid_len Number of valid SSID bytes.
     * @return Callback status, or RET_ERR_STS_OK when no remove callback is registered.
     */
    ret_err_sts_t bspWifiRemoveStationProfile(bspWifiHandle_t* handle,
                                              const uint8_t* ssid,
                                              uint8_t ssid_len);

    /**
     * @brief Perform a synchronous Wi-Fi scan and report the copied result count.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[out] results Destination array for scan results.
     * @param[in] max_results Capacity of the results array.
     * @param[out] out_count Number of results copied to the array.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @note The call blocks until the scan completes. The caller supplies the
     *       result array and its capacity.
     */
    ret_err_sts_t bspWifiScan(bspWifiHandle_t* handle,
                              bspWifiApScanResult_t* results,
                              uint8_t max_results,
                              uint8_t* out_count);

    /**
     * @brief Perform a scan using the backward-compatible result-count convention.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[out] results Destination array for scan results.
     * @param[in] max_results Capacity of the results array.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @note When max_results is nonzero, the result count is returned in
     *       results[0].num_result. Prefer bspWifiScan(), which provides out_count.
     */
    ret_err_sts_t bspWifiStartScan(bspWifiHandle_t* handle,
                                   bspWifiApScanResult_t* results,
                                   uint8_t max_results);

    /**
     * @brief Read one asynchronous Wi-Fi notification from the BSP event queue.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @param[out] event Destination for the event type and associated data.
     * @param[in] timeout_ms Timeout in milliseconds; zero polls and UINT32_MAX
     *                       waits indefinitely.
     * @return RET_ERR_STS_OK on success; otherwise an error status.
     * @warning Do not call concurrently with bspWifiDeInit() on the same handle.
     */
    ret_err_sts_t
    bspWifiGetEvent(bspWifiHandle_t* handle, bspWifiContext_t* event, uint32_t timeout_ms);

    /**
     * @brief Request a Wi-Fi hardware or driver reset.
     *
     * @param[in,out] handle Initialized Wi-Fi handle.
     * @return RET_ERR_STS_NOT_SUPPORTED on the ESP32 port; otherwise the
     *         platform-specific operation status.
     */
    ret_err_sts_t bspWifiReset(bspWifiHandle_t* handle);

#ifdef __cplusplus
}
#endif

#endif /* BSP_WIFI_H */