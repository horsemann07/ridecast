


#include "nal_config.h"
#include "nal_tls.h"
#include "nal_platform.h"
#include "nal_log.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cmsis_os2.h"

#if defined(NAL_USE_TLS)


    #ifndef NAL_TLS_ALLOC_MODE
        #define NAL_TLS_ALLOC_MODE NAL_TLS_ALLOC_STATIC
    #endif

/* =========================================================================
 *  TLS Context Private
 * ========================================================================= */

/*
 * Internal TLS context.
 * This structure is NOT exposed outside nal_tls.c.
 */
typedef struct nalTlsCtx
{
    #if (NAL_TLS_ALLOC_MODE == NAL_TLS_ALLOC_STATIC)
    bool in_use;        /* pool ownership flag */
    #endif

    void* platform_ctx; /* TLS backend (mbedTLS / wolfSSL / etc.) */
    int sockfd;

} nalTlsCtx_t;




/* =========================================================================
 *  Private Functions
 * ========================================================================= */

static nalTlsCtx_t* nalTlsAlloc(void);
static void nalTlsFree(nalTlsCtx_t* ctx);

/*
 * Initialize TLS pool once.
 * Must be called before any pool allocation.
 */
static bsp_err_sts_t nalTlsPoolInitOnce(void)
{
    static bool initialized = false;

    if(initialized)
    {
        return BSP_ERR_STS_OK;
    }

    memset(g_tls_pool, 0, sizeof(g_tls_pool));
    g_tls_pool_lock = osMutexNew(NULL);
    if(g_tls_pool_lock == NULL)
    {
        return BSP_ERR_STS_NO_MEM;
    }
    initialized = true;
    return BSP_ERR_STS_OK;
}

/*
 * Allocate TLS context from static pool.
 */
static nalTlsCtx_t* nalTlsAlloc(void)
{
    #if (NAL_TLS_ALLOC_MODE == NAL_TLS_ALLOC_HEAP)

    nalTlsCtx_t* ctx = (nalTlsCtx_t*)calloc(1, sizeof(nalTlsCtx_t));
    return ctx;

    #else /* NAL_TLS_ALLOC_STATIC */

    nalTlsCtx_t* ctx = NULL;

    bsp_err_sts_t rc = nalTlsPoolInitOnce();
    if(rc != BSP_ERR_STS_OK)
    {
        return NULL;
    }

    if(osMutexAcquire(g_tls_pool_lock, osWaitForever) != osOK)
        return NULL;

    for(int i = 0; i < NAL_MAX_TLS_SESSIONS; i++)
    {
        if(!g_tls_pool[i].in_use)
        {
            memset(&g_tls_pool[i], 0, sizeof(nalTlsCtx_t));
            g_tls_pool[i].in_use = true;
            ctx                  = &g_tls_pool[i];
            break;
        }
    }

    osMutexRelease(g_tls_pool_lock);
    return ctx;

    #endif
}

/*
 * Release TLS context back to static pool.
 */
static void nalTlsFree(nalTlsCtx_t* ctx)
{
    if(!ctx)
        return;

    #if (NAL_TLS_ALLOC_MODE == NAL_TLS_ALLOC_HEAP)

    free(ctx);
    ctx = NULL;

    #else /* NAL_TLS_ALLOC_STATIC */

    if(osMutexAcquire(g_tls_pool_lock, osWaitForever) != osOK)
        return;

    memset(ctx, 0, sizeof(nalTlsCtx_t));
    ctx->in_use = false;

    osMutexRelease(g_tls_pool_lock);

    #endif
}

/* =========================================================================
 *  TLS backend initialization
 * ========================================================================= */

/*
 * Initialize TLS backend state.
 *
 * Responsibilities:
 *  - Allocate backend TLS objects
 *  - Initialize crypto primitives
 *
 * MUST NOT:
 *  - Bind socket
 *  - Perform handshake
 */
static bsp_err_sts_t nalTlsBackendInit(nalTlsCtx_t* ctx)
{
    if(!ctx)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    ctx->platform_ctx = nal_platform_tls_create();
    if(ctx->platform_ctx == NULL)
    {
        return BSP_ERR_STS_FAIL;
    }

    return BSP_ERR_STS_OK;
}

/* =========================================================================
 *  Public TLS API
 * ========================================================================= */

/*
 * Create and initialize a TLS context.
 * Memory source (heap or static pool) is selected at compile time.
 */
bsp_err_sts_t nalTlsInit(nalHandle_t* handle)
{
    if(!handle)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    handle->tls_ctx = (nalTlsCtx_t*)nalTlsAlloc();
    if(handle->tls_ctx == NULL)
    {
        return BSP_ERR_STS_NO_MEM;
    }

    bsp_err_sts_t rc = nalTlsBackendInit(handle->tls_ctx);
    if(rc != BSP_ERR_STS_OK)
    {
        nalTlsFree(handle->tls_ctx);
        handle->tls_ctx = NULL;
        return rc;
    }

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalTlsDeinit(nalHandle_t* handle)
{
    if(handle == NULL || handle->tls_ctx == NULL)
    {
        return BSP_ERR_STS_OK;
    }

    nalTlsCtx_t* ctx = (nalTlsCtx_t*)handle->tls_ctx;
    /*
     * Tear down TLS backend state if it exists.
     * This must be safe even if handshake never completed.
     */
    if(ctx->platform_ctx != NULL)
    {
        nal_platform_tls_destroy(ctx->platform_ctx);
        ctx->platform_ctx = NULL;
    }

    /*
     * Release the TLS context memory.
     * This routes correctly to heap or static pool
     * depending on build configuration.
     */
    nalTlsFree(ctx);
    handle->tls_ctx = NULL;
    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalTlsHandshake(nalHandle_t* handle)
{
    if(handle == NULL || handle->tls_ctx == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    nalTlsCtx_t* ctx = (nalTlsCtx_t*)handle->tls_ctx;
    int sockfd       = handle->sockfd;
    /*
     * Perform client-side TLS handshake.
     * The socket is already connected at TCP level.
     */
    bsp_err_sts_t rc = nal_platform_tls_handshake(ctx->platform_ctx, sockfd);
    if(rc != BSP_ERR_STS_OK)
    {
        /*
         * Reset backend state so the context
         * can be reused or safely destroyed.
         */
        nal_platform_tls_reset(ctx->platform_ctx);
        return rc;
    }

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalTlsShutdown(nalHandle_t* handle)
{
    if(handle == NULL || handle->tls_ctx == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    nalTlsCtx_t* ctx = (nalTlsCtx_t*)handle->tls_ctx;
    /*
     * Best-effort TLS shutdown.
     * This sends close_notify if a session exists.
     * Failure is intentionally ignored.
     */
    if(ctx->platform_ctx != NULL)
    {
        nal_platform_tls_shutdown(ctx->platform_ctx);
    }

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */

bsp_err_sts_t
nalTlsSend(nalHandle_t* handle, const void* data, size_t len, size_t* bytes_sent, uint32_t timeout_ms)
{
    if(!handle || !data || len == 0 || !bytes_sent)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    *bytes_sent = 0;

    NAL_LOCK(handle);

    nalTlsCtx_t* ctx = (nalTlsCtx_t*)handle->tls_ctx;

    /*
     * Validate TLS state
     */
    if(handle->scheme != NAL_SCHEME_TLS || handle->tls_ctx == NULL || handle->sockfd < 0)
    {
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_UNSUPPORTED;
    }

    uint32_t start_tick = osKernelGetTickCount();

    /*
     * Blocking-style send loop
     */
    while(*bytes_sent < len)
    {
        uint32_t elapsed_ms = 0;

        if(timeout_ms > 0)
        {
            elapsed_ms = osKernelGetTickCount() - start_tick;

            if(NAL_TIMEOUT_EXPIRED(elapsed_ms, timeout_ms))
            {
                NAL_UNLOCK(handle);
                return BSP_ERR_STS_TIMEOUT;
            }
        }

        uint32_t remaining_timeout = (timeout_ms == 0) ? 0 : (timeout_ms - elapsed_ms);

        size_t written = 0;

        bsp_err_sts_t rc =
        nal_platform_tls_send(ctx->platform_ctx, (const uint8_t*)data + *bytes_sent,
                              len - *bytes_sent, &written, remaining_timeout);

        if(rc != BSP_ERR_STS_OK)
        {
            NAL_UNLOCK(handle);
            return rc;
        }

        if(written == 0)
        {
            /*
             * No forward progress → connection likely lost
             */
            NAL_UNLOCK(handle);
            return BSP_ERR_STS_CONN_LOST;
        }

        *bytes_sent += written;
    }

    NAL_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
bsp_err_sts_t
nalTlsRecv(nalHandle_t* handle, void* buf, size_t buf_len, size_t* bytes_recv, uint32_t timeout_ms)
{
    if(!handle || !buf || buf_len == 0 || !bytes_recv)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    *bytes_recv = 0;

    NAL_LOCK(handle);

    /*
     * Validate TLS state
     */
    if(handle->scheme != NAL_SCHEME_TLS || handle->tls_ctx == NULL || handle->sockfd < 0)
    {
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_NO_CONN;
    }

    nalTlsCtx_t* ctx = (nalTlsCtx_t*)handle->tls_ctx;

    uint32_t start_tick = osKernelGetTickCount();

    /*
     * Blocking-style receive
     *
     * Unlike send, a single successful read is enough.
     */
    while(true)
    {
        uint32_t elapsed_ms = 0;

        if(timeout_ms > 0)
        {
            elapsed_ms = osKernelGetTickCount() - start_tick;

            if(NAL_TIMEOUT_EXPIRED(elapsed_ms, timeout_ms))
            {
                NAL_UNLOCK(handle);
                return BSP_ERR_STS_TIMEOUT;
            }
        }

        uint32_t remaining_timeout = (timeout_ms == 0) ? 0 : (timeout_ms - elapsed_ms);

        size_t read = 0;

        bsp_err_sts_t rc = nal_platform_tls_recv(ctx->platform_ctx, (uint8_t*)buf,
                                                 buf_len, &read, remaining_timeout);

        if(rc == BSP_ERR_STS_TIMEOUT)
        {
            /* retry until overall timeout expires */
            continue;
        }

        if(rc != BSP_ERR_STS_OK)
        {
            NAL_UNLOCK(handle);
            return rc;
        }

        if(read == 0)
        {
            /*
             * Peer performed orderly shutdown
             */
            NAL_UNLOCK(handle);
            return BSP_ERR_STS_CONN_LOST;
        }

        *bytes_recv = read;
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_OK;
    }
}

/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalTlsSetCaCert(nalHandle_t* handle, const uint8_t* ca_pem, size_t len)
{
    if(!handle || !ca_pem || len == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(handle->sockfd >= 0)
    {
        return BSP_ERR_STS_BUSY;
    }

    NAL_LOCK(handle);

    /*
     * Store reference only.
     * Ownership and lifetime belong to caller.
     */
    handle->ca_cert     = ca_pem;
    handle->ca_cert_len = len;

    NAL_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}

bsp_err_sts_t nalTlsGetCaCert(nalHandle_t* handle, const uint8_t** out_ca_pem, size_t* out_len)
{
    if(!handle || !out_ca_pem || !out_len)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    NAL_LOCK(handle);

    if(handle->ca_cert == NULL || handle->ca_cert_len == 0)
    {
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_NOT_FOUND;
    }

    /*
     * Return reference to CA certificate.
     * Ownership and lifetime remain with the caller.
     */
    *out_ca_pem = handle->ca_cert;
    *out_len    = handle->ca_cert_len;

    NAL_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalTlsDecrypt(nalHandle_t* handle,
                            const uint8_t* ciphertext,
                            size_t cipher_len,
                            uint8_t* plaintext,
                            size_t* plain_len)
{
    if(!handle || !plaintext || !plain_len)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    NAL_LOCK(handle);

    if(handle->tls_ctx == NULL || handle->sockfd < 0)
    {
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_NO_CONN;
    }

    /*
     * NOTE:
     * TLS is a streaming protocol. Decryption is performed
     * by the TLS backend as part of the receive path.
     *
     * 'ciphertext' and 'cipher_len' are not used directly.
     * Data must already be available in the TLS engine.
     */
    size_t out_len = *plain_len;

    bsp_err_sts_t rc =
    nal_platform_tls_recv(((nalTlsCtx_t*)handle->tls_ctx)->platform_ctx,
                          plaintext, out_len, &out_len, 0 /* blocking */);

    if(rc != BSP_ERR_STS_OK)
    {
        NAL_UNLOCK(handle);
        return rc;
    }

    *plain_len = out_len;

    NAL_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalTlsServerStart(nalHandle_t* handle, nalTlsServerCreds_t* creds)
{
    if(!handle || !handle->tls_ctx || !creds)
        return BSP_ERR_STS_INVALID_PARAM;

    uint8_t id = handle->id;

    bsp_err_sts_t sts =
    nal_platform_tls_server_init(((nalTlsCtx_t*)handle->tls_ctx)->platform_ctx, creds);

    if(sts != BSP_ERR_STS_OK)
    {
        nal_platform_tls_free(((nalTlsCtx_t*)handle->tls_ctx)->platform_ctx);
        nalTlsFree(handle->tls_ctx);
        return sts;
    }
}


/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalTlsHandshakeServer(nalTlsCtx_t* ctx)
{
    if(!ctx || ctx->sockfd < 0)
        return BSP_ERR_STS_INVALID_PARAM;

    bsp_err_sts_t sts = nal_platform_tls_server_handshake(ctx->platform_ctx);

    return sts;
}


bsp_err_sts_t nalTlsNetworkAccept(nalHandle_t* handle, uint32_t timeout_ms)
{
    if(!handle || !handle->tls_ctx)
        return BSP_ERR_STS_INVALID_PARAM;

    int fd = nal_platform_tls_network_accept(handle->tls_ctx, timeout_ms);
    if(fd < 0)
    {
        return BSP_ERR_STS_TIMEOUT;
    }

    handle->sockfd = fd;

    nalTlsCtx_t* ctx = (nalTlsCtx_t*)handle->tls_ctx;
    ctx->sockfd      = fd;
    return BSP_ERR_STS_OK;
}
/* ------------------------------------------------------------------------- */
#endif /* NAL_USE_TLS */

       //
// =========================================================================
//  End of nal_tls.c
// =========================================================================
///|editable_region_end|>
