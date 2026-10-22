#ifndef RC_BT_CLASSIC_H
#define RC_BT_CLASSIC_H

#include <stdint.h>
#include <stdbool.h>

#include "rc_protocol.h"
#include "rc_connection.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* =========================================================================
 * Bluetooth Classic Configuration
 * ========================================================================= */

/**
 * @brief Maximum Bluetooth Classic device name length.
 */
#define RC_BT_CLASSIC_DEVICE_NAME_LEN 32U


    /**
     * @brief Bluetooth Classic configuration.
     */
    typedef struct
    {
        char deviceName[RC_BT_CLASSIC_DEVICE_NAME_LEN];
    } rc_bt_classic_config_t;


    /* =========================================================================
     * Bluetooth Classic Status
     * ========================================================================= */

    /**
     * @brief Bluetooth Classic connection state.
     */
    typedef enum
    {
        eRcBtClassicStateStopped = 0x00, /**< Bluetooth Classic is stopped */
        eRcBtClassicStateStarting,       /**< Bluetooth Classic is starting */
        eRcBtClassicStateDiscoverable,   /**< Device is discoverable */
        eRcBtClassicStateConnecting,     /**< Connection in progress */
        eRcBtClassicStateConnected,      /**< Connection established */
        eRcBtClassicStateDisconnected,   /**< Connection lost */
        eRcBtClassicStateReconnecting,   /**< Reconnection in progress */
        eRcBtClassicStateFailed          /**< Connection failed */
    } rc_bt_classic_state_t;


    /**
     * @brief Bluetooth Classic status information.
     */
    typedef struct
    {
        rc_bt_classic_state_t state; /**< Current Bluetooth Classic state */
        uint8_t retryCount;          /**< Current retry count */
        uint8_t maxRetry;            /**< Maximum configured retries */
    } rc_bt_classic_status_t;


    /* =========================================================================
     * Initialization
     * ========================================================================= */

    /**
     * @brief Initialize Bluetooth Classic module.
     *
     * Initializes the Bluetooth Classic stack/hardware and internal state.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_init(void);


    /**
     * @brief Deinitialize Bluetooth Classic module.
     *
     * Stops Bluetooth Classic and releases associated resources.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_deinit(void);


    /* =========================================================================
     * Configuration
     * ========================================================================= */

    /**
     * @brief Set Bluetooth Classic configuration.
     *
     * Stores the Bluetooth Classic device name.
     * Configuration does not automatically start Bluetooth Classic.
     *
     * @param[in] config Bluetooth Classic configuration.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_set_config(const rc_bt_classic_config_t* config);


    /**
     * @brief Get current Bluetooth Classic configuration.
     *
     * @param[out] config Bluetooth Classic configuration.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_get_config(rc_bt_classic_config_t* config);


    /* =========================================================================
     * Bluetooth Classic Control
     * ========================================================================= */

    /**
     * @brief Start Bluetooth Classic.
     *
     * Starts the Bluetooth Classic stack and makes the device
     * available for connection.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_start(void);


    /**
     * @brief Stop Bluetooth Classic.
     *
     * Stops Bluetooth Classic and disconnects any active connection.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_stop(void);


    /**
     * @brief Start Bluetooth Classic discoverability.
     *
     * Makes RideCast discoverable by remote Bluetooth devices.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_start_discoverable(void);


    /**
     * @brief Stop Bluetooth Classic discoverability.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_stop_discoverable(void);


    /**
     * @brief Disconnect the current Bluetooth Classic connection.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_disconnect(void);


    /**
     * @brief Request Bluetooth Classic reconnect.
     *
     * Starts the Bluetooth Classic reconnection procedure according
     * to the configured reconnect policy.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_reconnect(void);


    /* =========================================================================
     * Status
     * ========================================================================= */

    /**
     * @brief Get current Bluetooth Classic status.
     *
     * @param[out] status Current Bluetooth Classic status.
     *
     * @return PLT_ERR_STS_OK on success.
     */
    bsp_err_sts_t rc_bt_classic_get_status(rc_bt_classic_status_t* status);


    /**
     * @brief Get current Bluetooth Classic state.
     *
     * @return Current Bluetooth Classic state.
     */
    rc_bt_classic_state_t rc_bt_classic_get_state(void);


    /* =========================================================================
     * Connection Events
     * ========================================================================= */

    /**
     * @brief Notify Bluetooth Classic module that a connection was established.
     *
     * Called by the Bluetooth Classic stack/driver when a remote device
     * connects to RideCast.
     */
    void rc_bt_classic_on_connected(void);


    /**
     * @brief Notify Bluetooth Classic module that the connection was lost.
     *
     * Called by the Bluetooth Classic stack/driver when the connection
     * is lost.
     */
    void rc_bt_classic_on_disconnected(void);


    /**
     * @brief Process Bluetooth Classic connection management.
     *
     * Handles Bluetooth Classic state processing and reconnect operations.
     * Should be called periodically from the RideCast task/thread.
     */
    void rc_bt_classic_process(void);


#ifdef __cplusplus
}
#endif

#endif /* RC_BT_CLASSIC_H */