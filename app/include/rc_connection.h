#ifndef RC_CONNECTION_H
#define RC_CONNECTION_H

#include <stdint.h>
#include <stdbool.h>

#include "bsp_err_sts.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* =========================================================================
     * Connection Events / Actions
     * ========================================================================= */
    typedef enum
    {
        eRcConnEvtStart = 0x00,
        eRcConnEvtStop,
        eRcConnEvtConnect,
        eRcConnEvtConnecting,
        eRcConnEvtConnected,
        eRcConnEvtDisconnect,
        eRcConnEvtDisconnected,
        eRcConnEvtReconnect,
        eRcConnEvtReconnecting,
        eRcConnEvtFailed

    } rc_connection_event_t;
    /* =========================================================================
     * RIDECAST TRANSPORT
     * ========================================================================= */

    /**
     * @brief RideCast wireless transport types.
     */
    typedef enum
    {
        eRcTransportNone      = 0x00U,
        eRcTransportWifi      = 0x01U,
        eRcTransportBle       = 0x02U,
        eRcTransportBtClassic = 0x03U

    } rc_wireless_transport_t;
    /* =========================================================================
     * Reconnect Configuration
     * ========================================================================= */

    /**
     * @brief Reconnect configuration.
     *
     * RideCast uses this configuration to determine how many times
     * it should retry a failed wireless connection.
     */
    typedef struct
    {
        bool enable;               /**< Enable automatic reconnect */
        uint8_t maxRetry;          /**< Maximum number of retry attempts */
        uint16_t retryIntervalSec; /**< Delay between retry attempts in seconds */
    } rc_reconnect_config_t;


    /* =========================================================================
     * Connection Status
     * ========================================================================= */

    /**
     * @brief Current connection status.
     */
    typedef struct
    {
        rc_wireless_transport_t wireless_transport; /**< Wireless transport */
        rc_connection_event_t state; /**< Current connection state */
        uint8_t retryCount;          /**< Number of retry attempts */
        uint8_t maxRetry;            /**< Configured maximum retries */
    } rc_connection_status_t;


    /* =========================================================================
     * Initialization
     * ========================================================================= */

    /**
     * @brief Initialize connection manager.
     *
     * Initializes internal connection state and retry configuration.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_connection_init(void);


    /**
     * @brief Deinitialize connection manager.
     *
     * Stops connection management and clears internal state.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_connection_deinit(void);


    /* =========================================================================
     * Connection Control
     * ========================================================================= */

    /**
     * @brief Start connection management for a transport.
     *
     * The actual transport implementation is handled by the corresponding
     * transport module such as rc_wifi.c, rc_ble.c or rc_bt_classic.c.
     *
     * @param[in] transport Wireless transport to start.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_connection_start(rc_wireless_transport_t transport);


    /**
     * @brief Stop connection management for a transport.
     *
     * @param[in] transport Wireless transport to stop.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_connection_stop(rc_wireless_transport_t transport);


    /**
     * @brief Request a reconnect for a transport.
     *
     * Resets the retry counter and starts the reconnect process.
     *
     * @param[in] transport Wireless transport to reconnect.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_connection_reconnect(rc_wireless_transport_t transport);


    /* =========================================================================
     * Configuration
     * ========================================================================= */

    /**
     * @brief Set reconnect configuration.
     *
     * @param[in] transport Transport for which configuration is applied.
     * @param[in] config    Reconnect configuration.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_connection_set_reconnect_config(rc_wireless_transport_t transport,
                                                     const rc_reconnect_config_t* config);


    /**
     * @brief Get reconnect configuration.
     *
     * @param[in] transport Transport whose configuration is requested.
     * @param[out] config   Reconnect configuration.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_connection_get_reconnect_config(rc_wireless_transport_t transport,
                                                     rc_reconnect_config_t* config);


    /* =========================================================================
     * Status
     * ========================================================================= */

    /**
     * @brief Get current connection status.
     *
     * @param[in]  transport Transport whose status is requested.
     * @param[out] status    Current connection status.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_connection_get_status(rc_wireless_transport_t transport,
                                           rc_connection_status_t* status);


    /**
     * @brief Notify connection manager about a state change.
     *
     * Transport modules call this function when the actual wireless
     * connection state changes.
     *
     * @param[in] transport Transport whose state changed.
     * @param[in] state     New connection state.
     *
     * @return RET_ERR_STS_OK on success.
     */
    ret_err_sts_t rc_connection_set_state(rc_wireless_transport_t transport,
                                          rc_connection_event_t state);


    /* =========================================================================
     * Retry Handling
     * ========================================================================= */

    /**
     * @brief Perform one reconnect retry.
     *
     * Called by the connection manager when a retry is required.
     *
     * @param[in] transport Transport to retry.
     *
     * @return RET_ERR_STS_OK on successful retry initiation.
     */
    ret_err_sts_t rc_connection_retry(rc_wireless_transport_t transport);


    /**
     * @brief Get current retry count.
     *
     * @param[in] transport Transport whose retry count is requested.
     *
     * @return Current retry count.
     */
    uint8_t rc_connection_get_retry_count(rc_wireless_transport_t transport);


#ifdef __cplusplus
}
#endif

#endif /* RC_CONNECTION_H */