
/**
 * ============================================================================
 * @file    conn_manager.h
 * @brief   RideCast common wireless connection manager.
 *
 * @details
 * Coordinates Wi-Fi, BLE and Bluetooth Classic connection lifecycles.
 *
 * The vehicle cluster is the MASTER. This module never starts wireless
 * communication automatically during system initialization.
 *
 * RESPONSIBILITIES:
 *   - Process cluster-authorized START and STOP requests.
 *   - Track independent connection states per wireless transport.
 *   - Handle transport connection and disconnection events.
 *   - Schedule bounded reconnect attempts.
 *   - Enforce the configured retry interval and retry limit.
 *   - Request operations through registered transport callbacks.
 *
 * NOT RESPONSIBLE FOR:
 *   - Wi-Fi AP and TCP socket operations.
 *   - BLE or Bluetooth Classic driver operations.
 *   - Authentication, encryption or pairing implementation.
 *   - UART protocol encoding or application payload processing.
 *
 * ============================================================================
 * CONNECTION FLOW
 * ============================================================================
 *
 *               Vehicle Cluster (MASTER)
 *                          |
 *                          | START / STOP
 *                          v
 *                    ridecast_main
 *                          |
 *                          v
 *                   conn_manager
 *                          |
 *             +------------+------------+
 *             |            |            |
 *             v            v            v
 *           Wi-Fi         BLE       BT Classic
 *             |            |            |
 *             +------------+------------+
 *                          |
 *                          | Connection events
 *                          v
 *                   conn_manager
 *                          |
 *                          +-- Update transport state
 *                          +-- Reset retries on success
 *                          +-- Schedule retry on failure
 *                          +-- Stop after retry limit
 *
 * ============================================================================
 * RECONNECT POLICY
 * ============================================================================
 *
 * Initial START is not counted as a retry.
 *
 * Default policy:
 *   - Retry enabled.
 *   - Maximum five additional attempts.
 *   - Configurable fixed delay between retry attempts.
 *
 * Each transport maintains its own independent retry counter.
 *
 * Automatic retries occur only while the transport remains authorized
 * by the cluster. STOP disables authorization and cancels retries.
 *
 * ============================================================================
 * RTOS OWNERSHIP
 * ============================================================================
 *
 * app_main.c:
 *   - Creates the manager message queue and mutex.
 *
 * conn_manager_init():
 *   - Obtains centrally created RTOS objects.
 *   - Initializes manager state.
 *   - Creates the worker from the application thread map.
 *
 * conn_manager_deinit():
 *   - Stops the manager worker and transport services.
 *   - Does not delete centrally owned queues or mutexes.
 *
 * ============================================================================
 * WARNINGS
 * ============================================================================
 *
 * @warning
 *   All manager APIs are task-context APIs, not ISR APIs.
 *
 *   Transport callbacks must not block indefinitely.
 *
 *   Connection events are queued and processed asynchronously.
 *   A successful START request means accepted for processing,
 *   not that the transport is already connected.
 *
 *   BLE, Wi-Fi and Bluetooth Classic must register actual operation
 *   callbacks before receiving START requests.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Renamed to conn_manager and introduced common transport
 *             operations, serialized commands and bounded retries.
 * 2026-05-23  Initial connection management implementation.
 * ============================================================================
 */

#ifndef CONN_MANAGER_H
#define CONN_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#include "bsp_err_sts.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* =========================================================================
 * RIDECAST WIRELESS TRANSPORT
 * ========================================================================= */

/**
 * @brief Wireless transport identifiers.
 *
 * @note
 *   Values must remain aligned with rc_protocol.h.
 *   eRcTransportNone represents device-level commands, not a radio.
 */
typedef enum
{
    eRcTransportNone      = 0x00U,
    eRcTransportWifi      = 0x01U,
    eRcTransportBle       = 0x02U,
    eRcTransportBtClassic = 0x03U

} rc_wireless_transport_t;


/* =========================================================================
 * CONNECTION EVENTS
 * ========================================================================= */

/**
 * @brief Connection lifecycle events.
 *
 * @note
 *   Existing event values are preserved.
 *
 *   Transport modules should report Connecting, Connected,
 *   Disconnected, Reconnecting or Failed.
 *
 *   START, STOP and RECONNECT commands are issued through
 *   the manager's control APIs.
 */
typedef enum
{
    eRcConnEvtStart = 0x00U,
    eRcConnEvtStop,
    eRcConnEvtConnect,
    eRcConnEvtConnecting,
    eRcConnEvtConnected,
    eRcConnEvtDisconnect,
    eRcConnEvtDisconnected,
    eRcConnEvtReconnect,
    eRcConnEvtReconnecting,
    eRcConnEvtFailed

} conn_manager_event_t;


/* =========================================================================
 * RECONNECT CONFIGURATION
 * ========================================================================= */

/**
 * @brief Reconnection policy for one transport.
 */
typedef struct
{
    bool enable;
    uint8_t maxRetry;
    uint16_t retryIntervalSec;

} conn_manager_reconnect_config_t;


/* =========================================================================
 * CONNECTION STATUS
 * ========================================================================= */

/**
 * @brief Current connection state and retry information.
 */
typedef struct
{
    rc_wireless_transport_t wireless_transport;

    conn_manager_event_t state;

    uint8_t retryCount;
    uint8_t maxRetry;

    bool authorized;
    bool retryPending;

} conn_manager_status_t;


/* =========================================================================
 * TRANSPORT OPERATIONS
 * ========================================================================= */

/**
 * @brief Transport operation implemented by its application module.
 *
 * @param[in] user Registered transport context.
 *
 * @return RET_ERR_STS_OK if the operation request was accepted.
 */
typedef ret_err_sts_t (*conn_manager_operation_t)(void* user);


/**
 * @brief Operations registered by each wireless transport.
 *
 * start:
 *   Start using previously configured transport parameters.
 *
 * stop:
 *   Stop the transport and release its active connection.
 *
 * reconnect:
 *   Perform one new connection attempt.
 *   May be NULL; in that case start is used for retry.
 */
typedef struct
{
    conn_manager_operation_t start;
    conn_manager_operation_t stop;
    conn_manager_operation_t reconnect;

} conn_manager_ops_t;


/* =========================================================================
 * MANAGER QUEUE MESSAGE
 * ========================================================================= */

/**
 * @brief Internal command categories.
 *
 * @note
 *   Publicly declared because rc_os_map.c needs sizeof(conn_manager_msg_t)
 *   to configure the centrally owned message queue.
 */
typedef enum
{
    eConnMgrMsgStart = 0U,
    eConnMgrMsgStop,
    eConnMgrMsgReconnect,
    eConnMgrMsgTransportEvent

} conn_manager_msg_type_t;


/**
 * @brief One connection-manager queue message.
 */
typedef struct
{
    conn_manager_msg_type_t type;
    rc_wireless_transport_t transport;
    conn_manager_event_t event;

} conn_manager_msg_t;


/* =========================================================================
 * INITIALIZATION
 * ========================================================================= */

/**
 * @brief Initialize the connection manager and its worker task.
 *
 * @return RET_ERR_STS_OK on success.
 */
ret_err_sts_t conn_manager_init(void);


/**
 * @brief Stop the connection manager.
 *
 * @warning
 *   Call while transport service queues and tasks are still available.
 */
ret_err_sts_t conn_manager_deinit(void);


/* =========================================================================
 * TRANSPORT REGISTRATION
 * ========================================================================= */

/**
 * @brief Register the operations for a wireless transport.
 *
 * @param[in] transport Wireless transport.
 * @param[in] ops       Transport operation callbacks.
 * @param[in] user      Callback context.
 *
 * @return RET_ERR_STS_OK on success.
 *
 * @note
 *   Registration does not start the transport.
 *   START and STOP operations are required.
 *   Reconnect is optional.
 */
ret_err_sts_t conn_manager_register_transport(
    rc_wireless_transport_t transport,
    const conn_manager_ops_t* ops,
    void* user);


/* =========================================================================
 * CONNECTION CONTROL
 * ========================================================================= */

/**
 * @brief Request START for one transport.
 *
 * Resets the retry counter and enables connection authorization.
 *
 * @return RET_ERR_STS_OK if the command was queued.
 */
ret_err_sts_t conn_manager_start(
    rc_wireless_transport_t transport);


/**
 * @brief Request STOP for one transport.
 *
 * Disables authorization and cancels pending retries when processed.
 *
 * @return RET_ERR_STS_OK if the command was queued.
 */
ret_err_sts_t conn_manager_stop(
    rc_wireless_transport_t transport);


/**
 * @brief Request an explicit reconnect.
 *
 * @note
 *   This is an external reconnect request and resets the retry counter.
 *   Automatic retries do not reset the counter.
 */
ret_err_sts_t conn_manager_reconnect(
    rc_wireless_transport_t transport);


/**
 * @brief Request START for all registered transports.
 *
 * @note
 *   Transport-specific configurations must already be prepared.
 *   Submission is not atomic across the three transports.
 */
ret_err_sts_t conn_manager_start_all(void);


/**
 * @brief Request STOP for all registered transports.
 *
 * @note
 *   Submission is not atomic across the three transports.
 */
ret_err_sts_t conn_manager_stop_all(void);


/* =========================================================================
 * TRANSPORT EVENT REPORTING
 * ========================================================================= */

/**
 * @brief Report an actual transport state change.
 *
 * @details
 * Called by Wi-Fi, BLE or Bluetooth Classic application code.
 *
 * Connection failure or disconnection may schedule a retry when
 * authorization and retry policy allow it.
 *
 * @return RET_ERR_STS_OK if the event was queued.
 */
ret_err_sts_t conn_manager_report_event(
    rc_wireless_transport_t transport,
    conn_manager_event_t event);


/* =========================================================================
 * RECONNECT CONFIGURATION
 * ========================================================================= */

/**
 * @brief Configure the reconnection policy for one transport.
 */
ret_err_sts_t conn_manager_set_reconnect_config(
    rc_wireless_transport_t transport,
    const conn_manager_reconnect_config_t* config);


/**
 * @brief Obtain the reconnection policy for one transport.
 */
ret_err_sts_t conn_manager_get_reconnect_config(
    rc_wireless_transport_t transport,
    conn_manager_reconnect_config_t* config);


/* =========================================================================
 * CONNECTION STATUS
 * ========================================================================= */

/**
 * @brief Read the current connection state.
 */
ret_err_sts_t conn_manager_get_status(
    rc_wireless_transport_t transport,
    conn_manager_status_t* status);


/**
 * @brief Read the number of automatic retry requests issued.
 *
 * @return Retry count, or zero for an invalid/uninitialized transport.
 */
uint8_t conn_manager_get_retry_count(
    rc_wireless_transport_t transport);



#ifdef __cplusplus
}
#endif

#endif /* CONN_MANAGER_H */
