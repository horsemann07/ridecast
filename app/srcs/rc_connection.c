/*
 * =========================================================================
 * @file    rc_connection.c
 *
 * @brief   RideCast Common Connection Management
 *
 * Description:
 *   Implements common connection state and retry management used by
 *   RideCast wireless transports.
 *
 * This module does NOT:
 *   - Implement Wi-Fi
 *   - Implement BLE
 *   - Implement BT Classic
 *   - Access UART/SPI
 *   - Interpret application payload
 *
 * Transport-specific modules are responsible for the actual connection.
 *
 * =========================================================================
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "rc_connection.h"


/* =========================================================================
 * Configuration
 * =========================================================================
 */

#define RC_CONNECTION_MAX_TRANSPORTS    (4U)
#define RC_CONNECTION_DEFAULT_MAX_RETRY (3U)
#define RC_CONNECTION_DEFAULT_RETRY_SEC (1U)


/* =========================================================================
 * Internal Context
 * =========================================================================
 */

typedef struct
{
    rc_wireless_transport_t transport;
    rc_reconnect_config_t reconnectConfig;
    rc_connection_event_t state;
    uint8_t retryCount;
    bool initialized;
} rc_connection_ctx_t;


/* =========================================================================
 * Static Context
 * =========================================================================
 */

static rc_connection_ctx_t g_rcConnection[RC_CONNECTION_MAX_TRANSPORTS];


/* =========================================================================
 * Local Functions
 * =========================================================================
 */

static rc_connection_ctx_t* rc_connection_get_context(rc_wireless_transport_t transport);
static bool rc_connection_is_valid_transport(rc_wireless_transport_t transport);


/* =========================================================================
 * Transport Validation
 * =========================================================================
 */

static bool rc_connection_is_valid_transport(rc_wireless_transport_t transport)
{
    if((transport <= eRcTransportNone) || (transport > eRcTransportBtClassic))
    {
        return false;
    }

    return true;
}


/* =========================================================================
 * Get Context
 * =========================================================================
 */

static rc_connection_ctx_t* rc_connection_get_context(rc_wireless_transport_t transport)
{
    if(rc_connection_is_valid_transport(transport) == false)
    {
        return NULL;
    }

    return &g_rcConnection[(uint8_t)transport];
}


/* =========================================================================
 * Initialize
 * =========================================================================
 */

plt_err_sts_t rc_connection_init(void)
{
    uint8_t index;

    (void)memset(g_rcConnection, 0, sizeof(g_rcConnection));

    for(index = 0U; index < RC_CONNECTION_MAX_TRANSPORTS; index++)
    {
        g_rcConnection[index].transport = (rc_wireless_transport_t)index;
        g_rcConnection[index].reconnectConfig.enable = true;
        g_rcConnection[index].reconnectConfig.maxRetry = RC_CONNECTION_DEFAULT_MAX_RETRY;
        g_rcConnection[index].reconnectConfig.retryIntervalSec = RC_CONNECTION_DEFAULT_RETRY_SEC;
        g_rcConnection[index].state       = eRcConnEvtStop;
        g_rcConnection[index].retryCount  = 0U;
        g_rcConnection[index].initialized = true;
    }
    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Deinitialize
 * =========================================================================
 */

plt_err_sts_t rc_connection_deinit(void)
{
    uint8_t index;

    for(index = 0U; index < RC_CONNECTION_MAX_TRANSPORTS; index++)
    {
        if(g_rcConnection[index].initialized == true)
        {
            g_rcConnection[index].state       = eRcConnEvtStop;
            g_rcConnection[index].retryCount  = 0U;
            g_rcConnection[index].initialized = false;
        }
    }

    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Start Connection
 * =========================================================================
 */

plt_err_sts_t rc_connection_start(rc_wireless_transport_t transport)
{
    rc_connection_ctx_t* ctx;

    ctx = rc_connection_get_context(transport);

    if(ctx == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_NOT_INITIALIZED;
    }

    if(ctx->state == eRcConnEvtConnected)
    {
        return PLT_ERR_STS_ALREADY_CONNECTED;
    }

    if((ctx->state == eRcConnEvtConnecting) || (ctx->state == eRcConnEvtReconnecting))
    {
        return PLT_ERR_STS_IN_PROGRESS;
    }

    ctx->retryCount = 0U;
    ctx->state      = eRcConnEvtStart;
    /*
     * Transport-specific module performs the actual connection.
     */
    ctx->state = eRcConnEvtConnect;

    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Stop Connection
 * =========================================================================
 */

plt_err_sts_t rc_connection_stop(rc_wireless_transport_t transport)
{
    rc_connection_ctx_t* ctx;

    ctx = rc_connection_get_context(transport);

    if(ctx == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_NOT_INITIALIZED;
    }

    if(ctx->state == eRcConnEvtStop)
    {
        return PLT_ERR_STS_OK;
    }

    ctx->retryCount = ctx->reconnectConfig.maxRetry;

    ctx->state = eRcConnEvtStop;

    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Reconnect
 * =========================================================================
 */

plt_err_sts_t rc_connection_reconnect(rc_wireless_transport_t transport)
{
    rc_connection_ctx_t* ctx;

    ctx = rc_connection_get_context(transport);

    if(ctx == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_NOT_INITIALIZED;
    }

    if(ctx->state == eRcConnEvtConnected)
    {
        return PLT_ERR_STS_ALREADY_CONNECTED;
    }

    if(ctx->reconnectConfig.enable == false)
    {
        return PLT_ERR_STS_UNSUPPORTED;
    }

    ctx->retryCount = 0U;

    ctx->state = eRcConnEvtReconnect;

    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Set Reconnect Configuration
 * =========================================================================
 */

plt_err_sts_t rc_connection_set_reconnect_config(rc_wireless_transport_t transport,
                                                 const rc_reconnect_config_t* config)
{
    rc_connection_ctx_t* ctx;

    if(config == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    ctx = rc_connection_get_context(transport);

    if(ctx == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_NOT_INITIALIZED;
    }

    if(config->maxRetry == 0U)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    ctx->reconnectConfig = *config;

    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Get Reconnect Configuration
 * =========================================================================
 */
plt_err_sts_t rc_connection_get_reconnect_config(rc_wireless_transport_t transport,
                                                 rc_reconnect_config_t* config)
{
    rc_connection_ctx_t* ctx;

    if(config == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    ctx = rc_connection_get_context(transport);

    if(ctx == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_NOT_INITIALIZED;
    }

    *config = ctx->reconnectConfig;

    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Get Connection Status
 * =========================================================================
 */

plt_err_sts_t rc_connection_get_status(rc_wireless_transport_t transport,
                                       rc_connection_status_t* status)
{
    rc_connection_ctx_t* ctx;

    if(status == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    ctx = rc_connection_get_context(transport);

    if(ctx == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_NOT_INITIALIZED;
    }

    status->wireless_transport = ctx->transport;
    status->state              = ctx->state;
    status->retryCount         = ctx->retryCount;
    status->maxRetry           = ctx->reconnectConfig.maxRetry;

    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Set Connection State
 * =========================================================================
 */

plt_err_sts_t rc_connection_set_state(rc_wireless_transport_t transport, rc_connection_event_t state)
{
    rc_connection_ctx_t* ctx;

    ctx = rc_connection_get_context(transport);

    if(ctx == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_NOT_INITIALIZED;
    }

    ctx->state = state;

    if(state == eRcConnEvtConnected)
    {
        ctx->retryCount = 0U;
    }

    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Retry
 * =========================================================================
 */

plt_err_sts_t rc_connection_retry(rc_wireless_transport_t transport)
{
    rc_connection_ctx_t* ctx;

    ctx = rc_connection_get_context(transport);

    if(ctx == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_NOT_INITIALIZED;
    }

    if(ctx->reconnectConfig.enable == false)
    {
        return PLT_ERR_STS_UNSUPPORTED;
    }

    if(ctx->retryCount >= ctx->reconnectConfig.maxRetry)
    {
        ctx->state = eRcConnEvtFailed;

        return PLT_ERR_STS_RETRY_LIMIT;
    }

    ctx->retryCount++;

    ctx->state = eRcConnEvtReconnecting;

    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Get Retry Count
 * =========================================================================
 */

uint8_t rc_connection_get_retry_count(rc_wireless_transport_t transport)
{
    rc_connection_ctx_t* ctx;

    ctx = rc_connection_get_context(transport);

    if(ctx == NULL)
    {
        return 0U;
    }

    return ctx->retryCount;
}