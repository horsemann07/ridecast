
/**
 * ============================================================================
 * @file    conn_manager.c
 * @brief   RideCast common wireless connection management.
 *
 * @details
 * Provides one connection-policy owner for Wi-Fi, BLE and BT Classic.
 *
 * Actual transport operations are delegated through registered callbacks.
 *
 * START / STOP:
 *   Vehicle Cluster -> RideCast -> Manager queue -> Transport operation.
 *
 * CONNECTION EVENTS:
 *   Transport -> Manager queue -> Update state / retry scheduling.
 *
 * RETRIES:
 *   - Initial START does not increment retryCount.
 *   - Each additional retry increments retryCount.
 *   - A successful connection resets retryCount.
 *   - STOP cancels pending retries.
 *   - Retry exhaustion leaves the transport in FAILED state.
 *
 * All control messages and transport events are serialized by one
 * manager task. Individual transport services own their drivers.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Implemented serialized control, per-transport retries and
 *             operation registration; renamed rc_connection.c.
 * 2026-05-23  Initial connection management implementation.
 * ============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "app_config.h"
#include "app_logging.h"
#include "conn_manager.h"
#include "cmsis_os2.h"

/* =========================================================================
 * LOCAL CONSTANTS
 * ========================================================================= */

/* Includes the reserved transport-none slot at index zero. */
#define CONN_MANAGER_TRANSPORT_COUNT ((uint8_t)eRcTransportBtClassic + 1U)

/* =========================================================================
 * INTERNAL CONTEXT
 * ========================================================================= */

typedef struct
{
    rc_wireless_transport_t transport;
    conn_manager_ops_t ops;
    void* user;
    conn_manager_reconnect_config_t reconnectConfig;
    conn_manager_event_t state;
    uint32_t nextRetryTick;
    uint8_t retryCount;
    bool registered;
    bool authorized;
    bool retryPending;
} conn_manager_ctx_t;


/* =========================================================================
 * STATIC STATE
 * ========================================================================= */

static conn_manager_ctx_t g_connManager[CONN_MANAGER_TRANSPORT_COUNT];
static osMessageQueueId_t g_connManagerQ = NULL;
static osMutexId_t g_connManagerMutex    = NULL;
static osThreadId_t g_connManagerTask    = NULL;

static volatile bool g_connManagerRunning = false;
static bool g_connManagerInitialized      = false;


/* -------------------------------------------------------------------------- */
/* Private Function Declarations                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Validate a wireless transport identifier.
 *
 * @param[in] transport Wireless transport identifier to validate.
 *
 * @retval true The transport identifier is within the supported range.
 * @retval false The transport identifier is outside the supported range.
 */
static bool conn_mgr_is_valid_transport(rc_wireless_transport_t transport);

/* -------------------------------------------------------------------------- */

/**
 * @brief Convert milliseconds into CMSIS-RTOS2 kernel ticks.
 *
 * @param[in] milliseconds Requested duration in milliseconds.
 *
 * @return Converted timeout in kernel ticks.
 *
 * @note The conversion rounds upward to avoid producing a shorter
 * duration than requested.
 *
 * @note The result is restricted to the signed 32-bit tick-difference range.
 * A zero kernel tick frequency results in a return value of one tick.
 */
static uint32_t conn_mgr_ms_to_ticks(uint32_t milliseconds);

/* -------------------------------------------------------------------------- */

/**
 * @brief Acquire the connection manager mutex.
 *
 * @retval true The mutex was acquired successfully.
 * @retval false The mutex is unavailable or acquisition failed.
 *
 * @note The function waits indefinitely for the mutex when it exists.
 *
 * @warning A successful acquisition must be followed by conn_mgr_unlock().
 */
static bool conn_mgr_lock(void);

/* -------------------------------------------------------------------------- */

/**
 * @brief Release the connection manager mutex.
 *
 * @return None.
 *
 * @pre The calling thread must hold the connection manager mutex.
 */
static void conn_mgr_unlock(void);

/* -------------------------------------------------------------------------- */

/**
 * @brief Update a transport's failure state and schedule a retry if permitted.
 *
 * @param[in,out] ctx Pointer to the transport connection context.
 *
 * @return None.
 *
 * @details Marks the transport as failed and clears any existing retry
 * request. A new retry is scheduled only when the transport remains
 * authorized, automatic reconnection is enabled, and the configured
 * retry limit has not been reached.
 *
 * @pre The caller must hold the connection manager mutex.
 *
 * @warning This function modifies shared connection state and must not
 * be called without the manager mutex.
 */
static void conn_mgr_failure_locked(conn_manager_ctx_t* ctx);

/* -------------------------------------------------------------------------- */

/**
 * @brief Submit a command or transport event to the connection manager queue.
 *
 * @param[in] type Connection manager message type.
 * @param[in] transport Target wireless transport.
 * @param[in] event Transport event associated with the message.
 *
 * @retval RET_ERR_STS_OK Message successfully queued.
 * @retval RET_ERR_STS_INVALID_PARAM Invalid wireless transport identifier.
 * @retval RET_ERR_STS_NOT_INITIALIZED Manager unavailable or transport
 * not registered.
 * @retval RET_ERR_STS_FAIL Connection manager mutex acquisition failed.
 * @retval RET_ERR_STS_BUSY Message queue operation failed.
 *
 * @note All messages use the same queue priority to preserve FIFO ordering.
 *
 * @note Successful submission indicates queue acceptance, not completion
 * of the requested transport operation.
 */
static ret_err_sts_t conn_mgr_post(conn_manager_msg_type_t type,
                                   rc_wireless_transport_t transport,
                                   conn_manager_event_t event);

/* -------------------------------------------------------------------------- */

/**
 * @brief Process a transport event and update connection manager state.
 *
 * @param[in] msg Pointer to the received transport event message.
 *
 * @return None.
 *
 * @details Updates the connection state, resets retry tracking after
 * successful connection, or schedules retries following transport failure.
 *
 * @note Events for unregistered or unauthorized transports are ignored.
 *
 * @pre msg must point to a valid connection manager message containing
 * a valid transport identifier.
 */
static void conn_mgr_handle_event(const conn_manager_msg_t* msg);

/* -------------------------------------------------------------------------- */

/**
 * @brief Process a connection manager START, STOP, or RECONNECT command.
 *
 * @param[in] msg Pointer to the received control message.
 *
 * @return None.
 *
 * @details Updates transport authorization and connection state before
 * executing the registered transport operation.
 *
 * @note Transport callbacks execute outside the manager mutex to avoid
 * deadlocks when those callbacks report connection events.
 *
 * @note A failed transport operation updates the manager failure state.
 *
 * @pre msg must point to a valid connection manager message containing
 * a valid transport identifier.
 */
static void conn_mgr_handle_control(const conn_manager_msg_t* msg);

/* -------------------------------------------------------------------------- */

/**
 * @brief Process scheduled automatic reconnection attempts.
 *
 * @return None.
 *
 * @details Examines registered transports for expired retry deadlines.
 * When retry conditions are satisfied, increments the retry counter
 * and invokes the configured reconnect or start callback.
 *
 * @note Transport callbacks execute outside the manager mutex.
 *
 * @note A successfully accepted retry request does not indicate
 * that the transport has established a connection.
 *
 * @note Retry scheduling and execution are restricted by the
 * configured maximum retry count.
 */
static void conn_mgr_process_retries(void);

/* -------------------------------------------------------------------------- */


/* =========================================================================
 * CONNECTION MANAGER TASK
 * ========================================================================= */

/**
 * @brief Process serialized commands, events and retry deadlines.
 */
void vConnManagerTask(void* arg)
{
    conn_manager_msg_t msg;
    uint32_t pollTicks;

    (void)arg;

    pollTicks = conn_mgr_ms_to_ticks(configCONN_MANAGER_WORKER_POLL_MS);

    ALOGI("Connection manager task started");

    while(g_connManagerRunning)
    {
        if(osMessageQueueGet(g_connManagerQ, &msg, NULL, pollTicks) == osOK)
        {
            if(conn_mgr_is_valid_transport(msg.transport))
            {
                if(msg.type == eConnMgrMsgTransportEvent)
                {
                    conn_mgr_handle_event(&msg);
                }
                else
                {
                    conn_mgr_handle_control(&msg);
                }
            }
        }

        /* Polling is only for deadlines, not transport sockets. */
        conn_mgr_process_retries();
    }

    ALOGI("Connection manager task stopped");

    osThreadExit();
}


/* =========================================================================
 * INITIALIZATION
 * ========================================================================= */

/**
 * @brief Initialize all transport contexts.
 *
 * @details
 * Retrieves RTOS objects created by app_main.c and initializes
 * connection management state.
 *
 * @note
 *   This function does not create RTOS threads, queues or mutexes.
 *   The connection manager task is created by app_main.c.
 *
 * @return RET_ERR_STS_OK on successful initialization.
 */
ret_err_sts_t conn_manager_init(void)
{
    uint8_t index;

    if(g_connManagerInitialized)
    {
        return RET_ERR_STS_ALREADY_INIT;
    }

    /*
     * Retrieve centrally created RTOS objects.
     */
    g_connManagerQ     = APP_QUEUE_HANDLE(APP_Q_OWNER_CONN_MANAGER);
    g_connManagerMutex = APP_MUTEX_HANDLE(APP_MUTEX_OWNER_CONN_MANAGER);
    g_connManagerTask  = APP_THREAD_HANDLE(APP_THREAD_OWNER_CONN_MANAGER);

    if((g_connManagerQ == NULL) || (g_connManagerMutex == NULL))
    {
        ALOGE("Connection manager RTOS objects unavailable");
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /*
     * The worker may not have been created yet if app_main.c
     * starts application threads after module initialization.
     *
     * Do not require g_connManagerTask to be non-NULL here.
     */

    (void)memset(g_connManager, 0, sizeof(g_connManager));

    for(index = (uint8_t)eRcTransportWifi; index < CONN_MANAGER_TRANSPORT_COUNT; index++)
    {
        g_connManager[index].transport = (rc_wireless_transport_t)index;
        g_connManager[index].state     = eRcConnEvtStop;
        g_connManager[index].reconnectConfig.enable = true;
        g_connManager[index].reconnectConfig.maxRetry = configCONN_MANAGER_DEFAULT_MAX_RETRY;
        g_connManager[index].reconnectConfig.retryIntervalSec =
        configCONN_MANAGER_DEFAULT_RETRY_INTERVAL_SEC;
    }

    (void)osMessageQueueReset(g_connManagerQ);

    g_connManagerRunning     = true;
    g_connManagerInitialized = true;

    ALOGI("Connection manager initialized");

    return RET_ERR_STS_OK;
}

/* =========================================================================
 * TRANSPORT REGISTRATION
 * ========================================================================= */

/**
 * @brief Register actual operations for Wi-Fi, BLE or BT Classic.
 */
ret_err_sts_t conn_manager_register_transport(rc_wireless_transport_t transport,
                                              const conn_manager_ops_t* ops,
                                              void* user)
{
    conn_manager_ctx_t* ctx;

    if((!conn_mgr_is_valid_transport(transport)) || (ops == NULL) ||
       (ops->start == NULL) || (ops->stop == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(!g_connManagerInitialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(!conn_mgr_lock())
    {
        return RET_ERR_STS_FAIL;
    }

    ctx = &g_connManager[(uint8_t)transport];

    if(ctx->authorized || ctx->registered)
    {
        conn_mgr_unlock();
        return RET_ERR_STS_BUSY;
    }

    ctx->ops        = *ops;
    ctx->user       = user;
    ctx->registered = true;

    conn_mgr_unlock();

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * CONNECTION CONTROL API
 * ========================================================================= */

ret_err_sts_t conn_manager_start(rc_wireless_transport_t transport)
{
    return conn_mgr_post(eConnMgrMsgStart, transport, eRcConnEvtStart);
}


ret_err_sts_t conn_manager_stop(rc_wireless_transport_t transport)
{
    return conn_mgr_post(eConnMgrMsgStop, transport, eRcConnEvtStop);
}


ret_err_sts_t conn_manager_reconnect(rc_wireless_transport_t transport)
{
    return conn_mgr_post(eConnMgrMsgReconnect, transport, eRcConnEvtReconnect);
}


/**
 * @brief Send a control command to every registered transport.
 *
 * @note
 *   A transport without registered operations is skipped.
 *   Already-queued requests are not rolled back if a later post fails.
 */
static ret_err_sts_t conn_mgr_all(bool start)
{
    uint8_t index;
    bool registered;
    bool anyRegistered = false;
    ret_err_sts_t result;
    ret_err_sts_t firstError = RET_ERR_STS_OK;

    if(!g_connManagerInitialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    for(index = (uint8_t)eRcTransportWifi; index < CONN_MANAGER_TRANSPORT_COUNT; index++)
    {
        if(!conn_mgr_lock())
        {
            return RET_ERR_STS_FAIL;
        }

        registered = g_connManager[index].registered;

        conn_mgr_unlock();

        if(!registered)
        {
            continue;
        }

        anyRegistered = true;

        if(start)
        {
            result = conn_manager_start((rc_wireless_transport_t)index);
        }
        else
        {
            result = conn_manager_stop((rc_wireless_transport_t)index);
        }

        if((result != RET_ERR_STS_OK) && (firstError == RET_ERR_STS_OK))
        {
            firstError = result;
        }
    }

    if(!anyRegistered)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    return firstError;
}


ret_err_sts_t conn_manager_start_all(void)
{
    return conn_mgr_all(true);
}


ret_err_sts_t conn_manager_stop_all(void)
{
    return conn_mgr_all(false);
}


/* =========================================================================
 * TRANSPORT EVENT REPORTING
 * ========================================================================= */

ret_err_sts_t conn_manager_report_event(rc_wireless_transport_t transport,
                                        conn_manager_event_t event)
{
    switch(event)
    {
        case eRcConnEvtConnecting:
        case eRcConnEvtConnected:
        case eRcConnEvtDisconnected:
        case eRcConnEvtReconnecting:
        case eRcConnEvtFailed:
        {
            break;
        }

        default:
        {
            return RET_ERR_STS_INVALID_PARAM;
        }
    }

    return conn_mgr_post(eConnMgrMsgTransportEvent, transport, event);
}


/* =========================================================================
 * RECONNECT CONFIGURATION
 * ========================================================================= */

ret_err_sts_t conn_manager_set_reconnect_config(rc_wireless_transport_t transport,
                                                const conn_manager_reconnect_config_t* config)
{
    conn_manager_ctx_t* ctx;

    if((!conn_mgr_is_valid_transport(transport)) || (config == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(config->enable && ((config->maxRetry == 0U) || (config->retryIntervalSec == 0U)))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(!g_connManagerInitialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(!conn_mgr_lock())
    {
        return RET_ERR_STS_FAIL;
    }

    ctx                  = &g_connManager[(uint8_t)transport];
    ctx->reconnectConfig = *config;

    if(!config->enable)
    {
        ctx->retryPending = false;
    }

    conn_mgr_unlock();

    return RET_ERR_STS_OK;
}


ret_err_sts_t conn_manager_get_reconnect_config(rc_wireless_transport_t transport,
                                                conn_manager_reconnect_config_t* config)
{
    if((!conn_mgr_is_valid_transport(transport)) || (config == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(!g_connManagerInitialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(!conn_mgr_lock())
    {
        return RET_ERR_STS_FAIL;
    }

    *config = g_connManager[(uint8_t)transport].reconnectConfig;

    conn_mgr_unlock();

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * CONNECTION STATUS
 * ========================================================================= */

ret_err_sts_t conn_manager_get_status(rc_wireless_transport_t transport,
                                      conn_manager_status_t* status)
{
    conn_manager_ctx_t* ctx;

    if((!conn_mgr_is_valid_transport(transport)) || (status == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(!g_connManagerInitialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(!conn_mgr_lock())
    {
        return RET_ERR_STS_FAIL;
    }

    ctx = &g_connManager[(uint8_t)transport];

    status->wireless_transport = ctx->transport;
    status->state              = ctx->state;
    status->retryCount         = ctx->retryCount;
    status->maxRetry           = ctx->reconnectConfig.maxRetry;
    status->authorized         = ctx->authorized;
    status->retryPending       = ctx->retryPending;

    conn_mgr_unlock();

    return RET_ERR_STS_OK;
}


uint8_t conn_manager_get_retry_count(rc_wireless_transport_t transport)
{
    conn_manager_status_t status;

    if(conn_manager_get_status(transport, &status) != RET_ERR_STS_OK)
    {
        return 0U;
    }

    return status.retryCount;
}


/* =========================================================================
 * DEINITIALIZATION
 * ========================================================================= */

ret_err_sts_t conn_manager_deinit(void)
{
    uint8_t index;

    if(!g_connManagerInitialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /*
     * Stop the manager worker before clearing its callbacks.
     * Do not delete the centrally owned task configuration.
     */
    g_connManagerRunning = false;

    if(g_connManagerTask != NULL)
    {
        (void)osThreadTerminate(g_connManagerTask);

        g_connManagerTask = NULL;

        g_appThreadCfg[APP_THREAD_OWNER_CONN_MANAGER].handle = NULL;
    }

    /*
     * Request transport shutdown while the transport service
     * tasks and command queues are still operational.
     */
    for(index = (uint8_t)eRcTransportWifi; index < CONN_MANAGER_TRANSPORT_COUNT; index++)
    {
        if(g_connManager[index].registered && (g_connManager[index].ops.stop != NULL))
        {
            (void)g_connManager[index].ops.stop(g_connManager[index].user);
        }

        g_connManager[index].authorized   = false;
        g_connManager[index].retryPending = false;
        g_connManager[index].state        = eRcConnEvtStop;
    }

    g_connManagerInitialized = false;

    (void)osMessageQueueReset(g_connManagerQ);

    (void)memset(g_connManager, 0, sizeof(g_connManager));

    g_connManagerQ     = NULL;
    g_connManagerMutex = NULL;

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * LOCAL HELPERS
 * ========================================================================= */

/**
 * @brief Check whether a transport is a real wireless interface.
 */
static bool conn_mgr_is_valid_transport(rc_wireless_transport_t transport)
{
    return ((transport >= eRcTransportWifi) && (transport <= eRcTransportBtClassic));
}


/**
 * @brief Convert milliseconds to CMSIS kernel ticks.
 *
 * @note
 *   Uses rounding up to avoid scheduling earlier than requested.
 *   Tick deadlines are intended to remain within the signed
 *   32-bit tick-difference range.
 */
static uint32_t conn_mgr_ms_to_ticks(uint32_t milliseconds)
{
    uint32_t frequency = osKernelGetTickFreq();
    uint64_t ticks;

    if(frequency == 0U)
    {
        return 1U;
    }

    ticks = (((uint64_t)milliseconds * frequency) + 999ULL) / 1000ULL;

    if(ticks == 0ULL)
    {
        ticks = 1ULL;
    }

    if(ticks > (uint64_t)INT32_MAX)
    {
        ticks = (uint64_t)INT32_MAX;
    }

    return (uint32_t)ticks;
}


/**
 * @brief Acquire the centrally owned manager mutex.
 */
static bool conn_mgr_lock(void)
{
    return ((g_connManagerMutex != NULL) &&
            (osMutexAcquire(g_connManagerMutex, osWaitForever) == osOK));
}


/**
 * @brief Release the manager mutex.
 */
static void conn_mgr_unlock(void)
{
    (void)osMutexRelease(g_connManagerMutex);
}


/**
 * @brief Schedule another retry after a transport failure.
 *
 * @warning
 *   Caller must hold the manager mutex.
 */
static void conn_mgr_failure_locked(conn_manager_ctx_t* ctx)
{
    ctx->state        = eRcConnEvtFailed;
    ctx->retryPending = false;

    if(!ctx->authorized)
    {
        return;
    }

    if(!ctx->reconnectConfig.enable)
    {
        return;
    }

    if(ctx->retryCount >= ctx->reconnectConfig.maxRetry)
    {
        ALOGW("Transport %u retry limit reached (%u)",
              (unsigned int)ctx->transport, (unsigned int)ctx->retryCount);

        return;
    }

    ctx->nextRetryTick =
    osKernelGetTickCount() +
    conn_mgr_ms_to_ticks((uint32_t)ctx->reconnectConfig.retryIntervalSec * 1000U);

    ctx->retryPending = true;

    ALOGW("Transport %u: retry %u/%u scheduled", (unsigned int)ctx->transport,
          (unsigned int)(ctx->retryCount + 1U),
          (unsigned int)ctx->reconnectConfig.maxRetry);
}


/**
 * @brief Queue a manager command or transport event.
 *
 * @note
 *   All commands use the same queue priority to retain FIFO order.
 *   In particular, a queued STOP must not be overtaken by a START.
 */
static ret_err_sts_t conn_mgr_post(conn_manager_msg_type_t type,
                                   rc_wireless_transport_t transport,
                                   conn_manager_event_t event)
{
    conn_manager_msg_t msg;
    bool registered;

    if(!conn_mgr_is_valid_transport(transport))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if((!g_connManagerInitialized) || (!g_connManagerRunning) || (g_connManagerQ == NULL))
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(!conn_mgr_lock())
    {
        return RET_ERR_STS_FAIL;
    }

    registered = g_connManager[(uint8_t)transport].registered;

    conn_mgr_unlock();

    if(!registered)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    msg.type      = type;
    msg.transport = transport;
    msg.event     = event;

    if(osMessageQueuePut(g_connManagerQ, &msg, 0U, 0U) != osOK)
    {
        ALOGW("Connection manager queue full");
        return RET_ERR_STS_BUSY;
    }

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * TRANSPORT EVENT HANDLING
 * ========================================================================= */

/**
 * @brief Apply an event reported by a transport service.
 *
 * @details
 * This function only updates manager state and retry scheduling.
 * It does not call the transport while holding the mutex.
 */
static void conn_mgr_handle_event(const conn_manager_msg_t* msg)
{
    conn_manager_ctx_t* ctx;

    if(!conn_mgr_lock())
    {
        return;
    }

    ctx = &g_connManager[(uint8_t)msg->transport];

    if(!ctx->registered)
    {
        conn_mgr_unlock();
        return;
    }

    /*
     * Ignore late events after STOP.
     * A disconnected transport must not restart itself after STOP.
     */
    if(!ctx->authorized)
    {
        conn_mgr_unlock();
        return;
    }

    switch(msg->event)
    {
        case eRcConnEvtConnecting:
        case eRcConnEvtReconnecting:
        {
            ctx->state = msg->event;
            break;
        }

        case eRcConnEvtConnected:
        {
            ctx->state        = eRcConnEvtConnected;
            ctx->retryCount   = 0U;
            ctx->retryPending = false;

            ALOGI("Transport %u connected", (unsigned int)ctx->transport);
            break;
        }

        case eRcConnEvtDisconnected:
        case eRcConnEvtFailed:
        {
            conn_mgr_failure_locked(ctx);
            break;
        }

        default:
        {
            ALOGW("Ignored transport event: %u", (unsigned int)msg->event);
            break;
        }
    }

    conn_mgr_unlock();
}


/* =========================================================================
 * TRANSPORT CONTROL HANDLING
 * ========================================================================= */

/**
 * @brief Execute an accepted START, STOP or RECONNECT command.
 *
 * @note
 *   Transport callbacks are executed outside the manager mutex.
 *   This prevents a deadlock if a transport reports an event.
 */
static void conn_mgr_handle_control(const conn_manager_msg_t* msg)
{
    conn_manager_ctx_t* ctx;
    conn_manager_operation_t operation = NULL;
    void* user                         = NULL;
    ret_err_sts_t result;

    if(!conn_mgr_lock())
    {
        return;
    }

    ctx = &g_connManager[(uint8_t)msg->transport];

    if(!ctx->registered)
    {
        conn_mgr_unlock();
        return;
    }

    switch(msg->type)
    {
        case eConnMgrMsgStart:
        {
            ctx->authorized   = true;
            ctx->retryCount   = 0U;
            ctx->retryPending = false;
            ctx->state        = eRcConnEvtConnecting;

            operation = ctx->ops.start;
            user      = ctx->user;
            break;
        }

        case eConnMgrMsgStop:
        {
            ctx->authorized   = false;
            ctx->retryCount   = 0U;
            ctx->retryPending = false;
            ctx->state        = eRcConnEvtStop;

            operation = ctx->ops.stop;
            user      = ctx->user;
            break;
        }

        case eConnMgrMsgReconnect:
        {
            if(!ctx->authorized)
            {
                conn_mgr_unlock();
                return;
            }

            /*
             * Explicit reconnection starts a fresh retry sequence.
             * Automatic retries do not reset the retry counter.
             */
            ctx->retryCount   = 0U;
            ctx->retryPending = false;
            ctx->state        = eRcConnEvtReconnecting;

            operation =
            (ctx->ops.reconnect != NULL) ? ctx->ops.reconnect : ctx->ops.start;

            user = ctx->user;
            break;
        }

        default:
        {
            conn_mgr_unlock();
            return;
        }
    }

    conn_mgr_unlock();

    if(operation == NULL)
    {
        return;
    }

    result = operation(user);

    if(result != RET_ERR_STS_OK)
    {
        ALOGE("Transport %u operation %u failed", (unsigned int)msg->transport,
              (unsigned int)msg->type);

        if(conn_mgr_lock())
        {
            ctx = &g_connManager[(uint8_t)msg->transport];

            if(msg->type == eConnMgrMsgStop)
            {
                /*
                 * Do not claim the transport actually stopped
                 * if the STOP request was rejected.
                 */
                ctx->state = eRcConnEvtFailed;
            }
            else
            {
                conn_mgr_failure_locked(ctx);
            }

            conn_mgr_unlock();
        }
    }
}


/* =========================================================================
 * AUTOMATIC RETRY PROCESSING
 * ========================================================================= */

/**
 * @brief Issue scheduled retries when their deadlines have elapsed.
 *
 * @details
 * One retry request is issued at a time for each transport.
 * The transport reports a later Connected or Failed event.
 *
 * Successful enqueueing is not considered connection success.
 */
static void conn_mgr_process_retries(void)
{
    uint8_t index;

    for(index = (uint8_t)eRcTransportWifi; index < CONN_MANAGER_TRANSPORT_COUNT; index++)
    {
        conn_manager_ctx_t* ctx;
        conn_manager_operation_t operation = NULL;
        void* user                         = NULL;
        ret_err_sts_t result;
        uint32_t now;

        if(!conn_mgr_lock())
        {
            return;
        }

        ctx = &g_connManager[index];
        now = osKernelGetTickCount();

        if(ctx->registered && ctx->authorized && ctx->retryPending &&
           ((int32_t)(now - ctx->nextRetryTick) >= 0))
        {
            ctx->retryPending = false;

            if(ctx->retryCount < ctx->reconnectConfig.maxRetry)
            {
                ctx->retryCount++;
                ctx->state = eRcConnEvtReconnecting;

                operation = (ctx->ops.reconnect != NULL) ? ctx->ops.reconnect :
                                                           ctx->ops.start;

                user = ctx->user;

                ALOGI("Transport %u reconnect attempt %u/%u",
                      (unsigned int)index, (unsigned int)ctx->retryCount,
                      (unsigned int)ctx->reconnectConfig.maxRetry);
            }
            else
            {
                ctx->state = eRcConnEvtFailed;
            }
        }

        conn_mgr_unlock();

        if(operation == NULL)
        {
            continue;
        }

        /*
         * Never execute a transport callback while holding the
         * connection-manager mutex.
         */
        result = operation(user);

        if(result != RET_ERR_STS_OK)
        {
            ALOGE("Transport %u retry request failed", (unsigned int)index);

            if(conn_mgr_lock())
            {
                conn_mgr_failure_locked(&g_connManager[index]);
                conn_mgr_unlock();
            }
        }
    }
}


/* =========================================================================
 * FILE END
 * ========================================================================= */
