#ifndef RC_BLE_H
#define RC_BLE_H

#include <stdint.h>
#include <stdbool.h>

#include "bsp_err_sts.h"
#include "rc_protocol.h"
#include "conn_manager.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* =========================================================================
 * BLE Configuration
 * ========================================================================= */

/**
 * @brief Maximum BLE device name length.
 */
#define RC_BLE_DEVICE_NAME_LEN 32U


    /**
     * @brief BLE configuration.
     */
    typedef struct
    {
        char deviceName[RC_BLE_DEVICE_NAME_LEN];
    } rc_ble_config_t;


    /* =========================================================================
     * BLE Status
     * ========================================================================= */

    /**
     * @brief BLE connection state.
     */
    typedef enum
    {
        eRcBleStateStopped = 0x00, /**< BLE is stopped */
        eRcBleStateStarting,       /**< BLE is starting */
        eRcBleStateAdvertising,    /**< BLE advertising is active */
        eRcBleStateConnecting,     /**< Connection in progress */
        eRcBleStateConnected,      /**< BLE connection established */
        eRcBleStateDisconnected,   /**< BLE connection lost */
        eRcBleStateReconnecting,   /**< Reconnection in progress */
        eRcBleStateFailed          /**< Connection failed */
    } rc_ble_state_t;


    /**
     * @brief BLE status information.
     */
    typedef struct
    {
        rc_ble_state_t state; /**< Current BLE state */
        uint8_t retryCount;   /**< Current retry count */
        uint8_t maxRetry;     /**< Maximum configured retries */
    } rc_ble_status_t;


    /* =========================================================================
     * Initialization
     * ========================================================================= */

    /**
     * @brief Initialize BLE module.
     *
     * Initializes the BLE hardware/stack and internal state.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_init(void);


    /**
     * @brief Deinitialize BLE module.
     *
     * Stops BLE and releases BLE resources.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_deinit(void);


    /* =========================================================================
     * Configuration
     * ========================================================================= */

    /**
     * @brief Set BLE configuration.
     *
     * Stores the BLE device name.
     * Configuration does not automatically start BLE.
     *
     * @param[in] config BLE configuration.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_set_config(const rc_ble_config_t* config);


    /**
     * @brief Get current BLE configuration.
     *
     * @param[out] config Current BLE configuration.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_get_config(rc_ble_config_t* config);


    /* =========================================================================
     * BLE Control
     * ========================================================================= */

    /**
     * @brief Start BLE.
     *
     * Starts the BLE stack and begins advertising using
     * the configured device name.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_start(void);


    /**
     * @brief Stop BLE.
     *
     * Stops advertising and disconnects any active BLE connection.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_stop(void);


    /**
     * @brief Start BLE advertising.
     *
     * Starts advertising so that the mobile application
     * can discover and connect to RideCast.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_start_advertising(void);


    /**
     * @brief Stop BLE advertising.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_stop_advertising(void);


    /**
     * @brief Disconnect the current BLE connection.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_disconnect(void);


    /**
     * @brief Request BLE reconnect.
     *
     * Starts the BLE reconnection procedure according to
     * the configured reconnect policy.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_reconnect(void);


    /* =========================================================================
     * Status
     * ========================================================================= */

    /**
     * @brief Get current BLE status.
     *
     * @param[out] status Current BLE status.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_ble_get_status(rc_ble_status_t* status);


    /**
     * @brief Get current BLE state.
     *
     * @return Current BLE state.
     */
    rc_ble_state_t rc_ble_get_state(void);


    /* =========================================================================
     * Connection Events
     * ========================================================================= */

    /**
     * @brief Notify BLE module that a connection was established.
     *
     * Called by the BLE stack/driver when a mobile device connects.
     */
    void rc_ble_on_connected(void);


    /**
     * @brief Notify BLE module that the connection was lost.
     *
     * Called by the BLE stack/driver when the mobile device disconnects.
     */
    void rc_ble_on_disconnected(void);


    /**
     * @brief Process BLE connection management.
     *
     * Handles BLE state processing and reconnect operations.
     * Should be called periodically from the RideCast task/thread.
     */
    void rc_ble_process(void);


#ifdef __cplusplus
}
#endif

#endif /* RC_BLE_H */