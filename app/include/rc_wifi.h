#ifndef RC_WIFI_H
#define RC_WIFI_H

#include <stdint.h>
#include <stdbool.h>

#include "rc_protocol.h"
#include "rc_connection.h"
#include "bsp_err_sts.h"

#ifdef __cplusplus
extern "C"
{
#endif


    /* =========================================================================
     * Wi-Fi Configuration
     * ========================================================================= */

    /**
     * @brief Wi-Fi operating mode.
     */
    typedef enum
    {
        eRcWifiModeSta = 0x00, /**< Station mode - connect to external AP */
        eRcWifiModeAp  = 0x01  /**< Access Point mode - create local AP */
    } rc_wifi_mode_t;


    /**
     * @brief Wi-Fi configuration.
     */
    typedef struct
    {
        rc_wifi_mode_t mode; /**< Wi-Fi operating mode */
        char ssid[RC_WIFI_SSID_LEN];
        char password[RC_WIFI_PASSWORD_LEN];
    } rc_wifi_config_t;


    /* =========================================================================
     * Wi-Fi Status
     * ========================================================================= */

    /**
     * @brief Wi-Fi connection status.
     */
    typedef enum
    {
        eRcWifiStateStopped = 0x00, /**< Wi-Fi is stopped */
        eRcWifiStateStarting,       /**< Wi-Fi is starting */
        eRcWifiStateConnecting,     /**< Connecting to AP / client */
        eRcWifiStateConnected,      /**< Wi-Fi connection established */
        eRcWifiStateDisconnected,   /**< Wi-Fi connection lost */
        eRcWifiStateReconnecting,   /**< Reconnection in progress */
        eRcWifiStateFailed          /**< Connection failed */
    } rc_wifi_state_t;


    /**
     * @brief Wi-Fi status information.
     */
    typedef struct
    {
        rc_wifi_state_t state; /**< Current Wi-Fi state */
        uint8_t retryCount;    /**< Current retry count */
        uint8_t maxRetry;      /**< Maximum configured retries */
    } rc_wifi_status_t;


    /* =========================================================================
     * Initialization
     * ========================================================================= */

    /**
     * @brief Initialize Wi-Fi module.
     *
     * Initializes the Wi-Fi hardware/driver and internal state.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_wifi_init(void);


    /**
     * @brief Deinitialize Wi-Fi module.
     *
     * Stops Wi-Fi and releases Wi-Fi resources.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_wifi_deinit(void);


    /* =========================================================================
     * Configuration
     * ========================================================================= */

    /**
     * @brief Set Wi-Fi configuration.
     *
     * Stores the Wi-Fi operating mode, SSID and password.
     *
     * The configuration does not automatically start Wi-Fi.
     * Host should explicitly request Wi-Fi start.
     *
     * @param[in] config Wi-Fi configuration.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_wifi_set_config(const rc_wifi_config_t* config);


    /**
     * @brief Get current Wi-Fi configuration.
     *
     * @param[out] config Current Wi-Fi configuration.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_wifi_get_config(rc_wifi_config_t* config);


    /* =========================================================================
     * Wi-Fi Control
     * ========================================================================= */

    /**
     * @brief Start Wi-Fi.
     *
     * Starts Wi-Fi according to the configured mode:
     * - STA mode: connect to configured AP.
     * - AP mode: start configured Access Point.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_wifi_start(void);


    /**
     * @brief Stop Wi-Fi.
     *
     * Stops the Wi-Fi interface and disconnects any active connection.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_wifi_stop(void);


    /**
     * @brief Connect Wi-Fi.
     *
     * Initiates a Wi-Fi connection using the configured parameters.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_wifi_connect(void);


    /**
     * @brief Disconnect Wi-Fi.
     *
     * Disconnects the current Wi-Fi connection.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_wifi_disconnect(void);


    /**
     * @brief Request Wi-Fi reconnect.
     *
     * Starts a new connection attempt using the configured parameters.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_wifi_reconnect(void);


    /* =========================================================================
     * Status
     * ========================================================================= */

    /**
     * @brief Get current Wi-Fi status.
     *
     * @param[out] status Current Wi-Fi status.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_wifi_get_status(rc_wifi_status_t* status);


    /**
     * @brief Get current Wi-Fi state.
     *
     * @return Current Wi-Fi state.
     */
    rc_wifi_state_t rc_wifi_get_state(void);


    /* =========================================================================
     * Connection Events
     * ========================================================================= */

    /**
     * @brief Notify Wi-Fi module that connection was established.
     *
     * Called by the Wi-Fi driver when the connection succeeds.
     */
    void rc_wifi_on_connected(void);


    /**
     * @brief Notify Wi-Fi module that connection was lost.
     *
     * Called by the Wi-Fi driver when the connection is lost.
     */
    void rc_wifi_on_disconnected(void);


    /**
     * @brief Process Wi-Fi connection management.
     *
     * Handles connection/reconnection processing and should be called
     * periodically from the RideCast task/thread.
     */
    void rc_wifi_process(void);


#ifdef __cplusplus
}
#endif

#endif /* RC_WIFI_H */