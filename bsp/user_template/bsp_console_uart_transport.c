#include "bsp_console_uart_transport.h"

#include "bsp_cfg_map.h"

#include <string.h>

typedef struct
{
    bspConsoleTransport_t transport;
    bspUartHandle_t* uart;

    bspConsoleRxCallback_t rxCallback;
    void* rxUserContext;

    bool initialized;
} bspConsoleUartTransportContext_t;

/*
 * One transport context per UART mapping index.
 *
 * This prevents logger and CLI from initializing the same UART twice when
 * both use BSP_UART_OWNER_CONSOLE.
 */
static bspConsoleUartTransportContext_t s_uartTransportContext[BSP_UART_OWNER_MAX];

static void
bspConsoleUartRxAdapter(bsp_err_sts_t status, uint8_t* data, uint16_t length, void* userContext)
{
    bspConsoleUartTransportContext_t* ctx = (bspConsoleUartTransportContext_t*)userContext;

    if((ctx == NULL) || (ctx->initialized == false))
    {
        return;
    }

    /*
     * Do not process CLI commands here. The callback only forwards RX data
     * to the registered CLI callback, which places bytes into the CLI ring.
     */
    if(ctx->rxCallback != NULL)
    {
        ctx->rxCallback(data, (size_t)length, status, ctx->rxUserContext);
    }
}

static bsp_err_sts_t bspConsoleUartWrite(void* context, const uint8_t* data, uint16_t length)
{
    bspConsoleUartTransportContext_t* ctx = (bspConsoleUartTransportContext_t*)context;

    if((ctx == NULL) || (ctx->initialized == false) || (ctx->uart == NULL) ||
       (data == NULL) || (length == 0U))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    return bspUartWriteAsync(ctx->uart, data, (size_t)length);
}

static bsp_err_sts_t bspConsoleUartReadStart(void* context, uint8_t* buffer, uint16_t length)
{
    bspConsoleUartTransportContext_t* ctx = (bspConsoleUartTransportContext_t*)context;

    if((ctx == NULL) || (ctx->initialized == false) || (ctx->uart == NULL) ||
       (buffer == NULL) || (length == 0U))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    return bspUartReadAsync(ctx->uart, buffer, (size_t)length);
}

static bsp_err_sts_t
bspConsoleUartSetCallback(void* context, bspConsoleRxCallback_t callback, void* userContext)
{
    bspConsoleUartTransportContext_t* ctx = (bspConsoleUartTransportContext_t*)context;
    bsp_err_sts_t status;

    if((ctx == NULL) || (ctx->initialized == false) || (ctx->uart == NULL))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    /*
     * Store console callback first. UART calls bspConsoleUartRxAdapter(),
     * which converts BSP UART callback arguments to console callback order.
     */
    ctx->rxCallback    = callback;
    ctx->rxUserContext = userContext;

    status = bspUartSetCallback(ctx->uart, (callback != NULL) ? bspConsoleUartRxAdapter : NULL,
                                (callback != NULL) ? ctx : NULL);

    if(status != PLT_ERR_STS_OK)
    {
        ctx->rxCallback    = NULL;
        ctx->rxUserContext = NULL;
    }

    return status;
}

bsp_err_sts_t bsp_console_uart_transport_init(bspUartOwner_t owner)
{
    bspConsoleUartTransportContext_t* ctx;
    bsp_err_sts_t status;

    if(owner >= BSP_UART_OWNER_MAX)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    ctx = &s_uartTransportContext[owner];

    if(ctx->initialized == true)
    {
        return PLT_ERR_STS_OK;
    }

    (void)memset(ctx, 0, sizeof(*ctx));

    /*
     * Uses the user-configured UART mapping:
     *
     * g_bspUartCfg[BSP_UART_OWNER_CONSOLE]
     * g_bspUartCfg[BSP_UART_OWNER_LOG]
     * g_bspUartCfg[BSP_UART_OWNER_CLI]
     */
    ctx->uart = &g_bspUartCfg[owner];

    status = bspUartInit(ctx->uart);

    if(status != PLT_ERR_STS_OK)
    {
        ctx->uart = NULL;
        return status;
    }

    ctx->transport.write        = bspConsoleUartWrite;
    ctx->transport.read_start   = bspConsoleUartReadStart;
    ctx->transport.set_callback = bspConsoleUartSetCallback;
    ctx->transport.context      = ctx;

    ctx->initialized = true;

    return PLT_ERR_STS_OK;
}

bsp_err_sts_t bsp_console_uart_transport_deinit(bspUartOwner_t owner)
{
    bspConsoleUartTransportContext_t* ctx;
    bsp_err_sts_t status;

    if(owner >= BSP_UART_OWNER_MAX)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    ctx = &s_uartTransportContext[owner];

    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_NOT_INIT;
    }

    (void)bspUartSetCallback(ctx->uart, NULL, NULL);

    status = bspUartDeInit(ctx->uart);

    (void)memset(ctx, 0, sizeof(*ctx));

    return status;
}

const bspConsoleTransport_t* bsp_console_uart_transport_get(bspUartOwner_t owner)
{
    if(owner >= BSP_UART_OWNER_MAX)
    {
        return NULL;
    }

    if(s_uartTransportContext[owner].initialized == false)
    {
        return NULL;
    }

    return &s_uartTransportContext[owner].transport;
}