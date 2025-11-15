/*
 * nal_network_async.c
 *
 * Asynchronous support for NAL (Network Abstraction Layer)
 *
 * Requirements:
 *  - The project must provide:
 *      errStatus_t nalNetworkConnect(nalHandle_t*, const char*, uint16_t, nalScheme_t);
 *      void nalNetworkDisconnect(nalHandle_t*);
 *      int32_t nalNetworkSend(nalHandle_t*, const void*, size_t, uint32_t);
 *      int32_t nalNetworkRecv(nalHandle_t*, void*, size_t, uint32_t);
 *      (and nalHandle_t type & ERR_STS_* codes)
 *
 *  - CMSIS-RTOS2 must be available (SYS_USE_CMSIS defined).
 *
 * Design:
 *  - Each async connection spawns a worker thread that:
 *      1) Performs connect (synchronously) and notifies callback of CONNECTED / CONNECT_FAILED
 *      2) Enters a select()/poll() loop on the socket and handles:
 *         - readable => perform nalNetworkRecv (sync read wrapped), notify DATA_RECEIVED
 *         - want_send set => perform nalNetworkSend (sync send wrapped), notify DATA_SENT
 *         - socket closed / error => notify DISCONNECTED
 *
 *  - The user registers callback with nalNetworkConnectAsync().
 *  - Only one outstanding async send is supported per connection (simple buffer copy).
 *    Extend by making a FIFO queue if needed.
 *
 *  - All public functions return errStatus_t to match BSP conventions.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "nal_network.h"

#ifdef SYS_USE_CMSIS
    #include "cmsis_os2.h"
#else
    #error "Async NAL requires CMSIS-RTOS2 (define SYS_USE_CMSIS)"
#endif


/* --- Macro to allow redefinition for safe callback dispatching --- */
#ifndef NAL_CALL_CB_SAFE
    #define NAL_CALL_CB_SAFE(cb, handle, ev, user_ctx, data, len) \
        do                                                        \
        {                                                         \
            if((cb) != NULL)                                      \
                (cb)((handle), (ev), (user_ctx), (data), (len));  \
        } while(0)
#endif


typedef struct
{
    nalAsyncCtx_t* ctx;
    char* host;
    uint16_t port;
    nalScheme_t scheme;
} async_connect_job_t;


/* -------------------------
 * Globals
 * ------------------------- */

static nalAsyncCtx_t g_async_table[NAL_ASYNC_MAX_INSTANCES];

/* -------------------------
 * Helpers
 * ------------------------- */

/* --- Ensure async table zeroed (call from nalNetworkInit or module init) --- */
static void nal_async_table_init(void)
{
    /* static storage is normally zeroed, but call explicitly for safety */
    memset(g_async_table, 0, sizeof(g_async_table));
}

/* ------------------------------------------------------------------------- */

static nalAsyncCtx_t* get_ctx_by_sockfd(int sockfd)
{
    for(int i = 0; i < NAL_ASYNC_MAX_INSTANCES; ++i)
    {
        if(g_async_table[i].used && g_async_table[i].sockfd == sockfd)
            return &g_async_table[i];
    }
    return NULL;
}
/* ------------------------------------------------------------------------- */
static nalAsyncCtx_t* get_free_ctx(void)
{
    for(int i = 0; i < NAL_ASYNC_MAX_INSTANCES; ++i)
    {
        if(!g_async_table[i].used)
            return &g_async_table[i];
    }
    return NULL;
}
/* ------------------------------------------------------------------------- */
static void free_ctx(nalAsyncCtx_t* ctx)
{
    if(!ctx)
        return;

    /* If worker still running, attempt graceful stop; if it fails, terminate the thread */
    if(ctx->running)
    {
        ctx->running = 0;
        /* prefer waiting for the worker to signal exit semaphore */
        if(ctx->exit_sem)
        {
            /* give the worker time to exit cleanly */
            if(osSemaphoreAcquire(ctx->exit_sem, 2000) != osOK)
            {
                /* try terminating the thread as a last resort if supported */
                if(ctx->thread_id)
                {
                    /* osThreadTerminate is platform specific; check existence */
                    osThreadTerminate(ctx->thread_id);
                }
            }
        }
        else
        {
            if(ctx->thread_id)
            {
                /* osThreadTerminate is platform specific; check existence */
                osThreadTerminate(ctx->thread_id);
            }
        }
    }

    /* delete kernel objects and free buffers */
    if(ctx->lock)
    {
        osMutexDelete(ctx->lock);
        ctx->lock = NULL;
    }
    if(ctx->exit_sem)
    {
        osSemaphoreDelete(ctx->exit_sem);
        ctx->exit_sem = NULL;
    }
    if(ctx->send_buf)
    {
        free(ctx->send_buf);
        ctx->send_buf = NULL;
    }

    /* Reset state */
    ctx->used         = 0;
    ctx->sockfd       = -1;
    ctx->handle       = NULL;
    ctx->cb           = NULL;
    ctx->user_ctx     = NULL;
    ctx->recv_buf     = NULL;
    ctx->recv_buf_len = 0;
    ctx->send_len     = 0;
    ctx->want_send    = 0;
    ctx->thread_id    = NULL;
    ctx->running      = 0;

    /* zero-out to avoid stale data references in the global table */
    /* Note: this will also clear the fields above but we already cleaned resources */
    memset(ctx, 0, sizeof(*ctx));
}
/* ------------------------------------------------------------------------- */
/* Notify wrapper: call user callback safely (no locks held) */
static void notify_callback(nalAsyncCtx_t* ctx, nalEvent_t ev, void* data, size_t len)
{
    if(!ctx || !ctx->cb)
    {
        return;
    }
    /* Use the safe macro to allow user redefinition */
    NAL_CALL_CB_SAFE(ctx->cb, ctx->handle, ev, ctx->user_ctx, data, len);
}
/* ------------------------------------------------------------------------- */
/* Thread function for each async connection */
static void async_worker_thread(void* arg)
{
    nalAsyncCtx_t* ctx = (nalAsyncCtx_t*)arg;
    if(!ctx)
        return;

    /* 1) perform connection using existing sync connect API (inside thread) */
    if(!ctx->handle || ctx->handle->sockfd < 0)
    {
        notify_callback(ctx, NAL_EVENT_CONNECT_FAILED, NULL, 0);
        ctx->running = 0;
        if(ctx->exit_sem)
        {
            osSemaphoreRelease(ctx->exit_sem);
        }
        return;
    }

    ctx->sockfd = (int)ctx->handle->sockfd;

    notify_callback(ctx, NAL_EVENT_CONNECTED, NULL, 0);

    /* Reduced select timeout for better shutdown responsiveness */
    const int select_timeout_ms = 200;

    /* Notify connected */
    notify_callback(ctx, NAL_EVENT_CONNECTED, NULL, 0);

    /* Select/poll loop (simple) */
    while(ctx->running)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(ctx->sockfd, &read_fds);

        struct timeval tv;
        tv.tv_sec  = select_timeout_ms / 1000;
        tv.tv_usec = (select_timeout_ms % 1000) * 1000;

        int nfds = ctx->sockfd + 1;
        int sel  = select(nfds, &read_fds, NULL, NULL, &tv);
        if(sel < 0)
        {
            /* EINTR is common; if other error, notify and break */
            if(errno == EINTR)
                continue;
            notify_callback(ctx, NAL_EVENT_ERROR, NULL, 0);
            break;
        }
        else if(sel == 0)
        {
            /* timeout, check if want_send */
        }
        else
        {
            if(FD_ISSET(ctx->sockfd, &read_fds))
            {
                /* Data available to read */
                /* Use a temp buffer if user did not provide one */
                uint8_t tmpbuf[1024];
                void* buf     = tmpbuf;
                size_t buflen = sizeof(tmpbuf);
                if(ctx->recv_buf && ctx->recv_buf_len > 0)
                {
                    buf    = ctx->recv_buf;
                    buflen = ctx->recv_buf_len;
                }
                int32_t r = nalNetworkRecv(ctx->handle, buf, buflen, 0); /* non-blocking since data available */
                if(r > 0)
                {
                    notify_callback(ctx, NAL_EVENT_DATA_RECEIVED, buf, (size_t)r);
                }
                else if(r == 0)
                {
                    /* peer closed */
                    notify_callback(ctx, NAL_EVENT_DISCONNECTED, NULL, 0);
                    break;
                }
                else
                {
                    /* read error */
                    notify_callback(ctx, NAL_EVENT_ERROR, NULL, 0);
                    break;
                }
            }
        }

        /* handle pending send if requested */
        osMutexAcquire(ctx->lock, osWaitForever);
        int want = ctx->want_send;
        osMutexRelease(ctx->lock);
        if(want)
        {
            /* copy send buffer pointer under lock */
            osMutexAcquire(ctx->lock, osWaitForever);
            void* sbuf  = ctx->send_buf;
            size_t slen = ctx->send_len;
            /* clear flags before sending so new async sends can be queued */
            ctx->send_buf  = NULL;
            ctx->send_len  = 0;
            ctx->want_send = 0;
            osMutexRelease(ctx->lock);

            if(sbuf && slen > 0)
            {
                int32_t s = nalNetworkSend(ctx->handle, sbuf, slen, 0);
                if(s >= 0)
                {
                    notify_callback(ctx, NAL_EVENT_DATA_SENT, NULL, (size_t)s);
                    free(sbuf);
                }
                else
                {
                    /* On send error, notify and free (no retry policy here) */
                    notify_callback(ctx, NAL_EVENT_ERROR, NULL, 0);
                    free(sbuf);
                }
            }
        }
        /* small sleep/yield to avoid busy loop (select already blocks) */
    }

    /* cleanup: Ensure underlying handle is disconnected */
    nalNetworkDisconnect(ctx->handle);
    notify_callback(ctx, NAL_EVENT_DISCONNECTED, NULL, 0);

    /* signal that the worker is exiting so stop API can join safely */
    if(ctx->exit_sem)
    {
        osSemaphoreRelease(ctx->exit_sem);
    }

    ctx->running = 0;
    return;
}
/* ------------------------------------------------------------------------- */
/* -------------------------
 * Convenience: Async connect wrapper
 * ------------------------- */

/*
 * Async connect routine: spawns a worker which will attempt connect and then
 * enter the read/send loop. It uses the same worker thread above but performs
 * connect inside the thread. To keep the code compact we implement a thin
 * wrapper that prepares a context, stores host/port/scheme in a small local
 * struct, and starts a thread that does connect then enters the same loop.
 *
 * For simplicity we implement this by performing nalNetworkConnect
 * synchronously inside the worker before starting the select/send loop.
 */

static void async_connect_thread(void* arg)
{
    async_connect_job_t* job = (async_connect_job_t*)arg;
    nalAsyncCtx_t* c         = job->ctx;
    if(!job || !c)
    {
        if(job)
        {
            free(job->host);
            free(job);
        }
        return;
    }

    /* mark running so state is consistent for cleanup paths */
    c->running = 1;

    nalHandle_t* h = c->handle;
    /* attempt connect (synchronous) */
    errStatus_t rc = nalNetworkConnect(h, job->host, job->port, job->scheme);
    if(rc != ERR_STS_OK)
    {
        notify_callback(c, NAL_EVENT_CONNECT_FAILED, NULL, 0);
        c->running = 0;
        /* cleanup */
        free(job->host);
        free(job);
        free_ctx(c);
        return;
    }

    /* now set sockfd to handle->sockfd and continue with main worker loop */
    c->sockfd = (int)h->sockfd;
    notify_callback(c, NAL_EVENT_CONNECTED, NULL, 0);

    /* free host copy and job */
    free(job->host);
    free(job);

    /* reuse async_worker_thread loop body by calling it directly */
    async_worker_thread(c);
}
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/* ============================================================
 * Internal Helper Functions
 * ============================================================ */

static errStatus_t nal_openSocket(nalHandle_t* h, const char* host, uint16_t port)
{
    CHECK_PARAM(h || host);

    struct sockaddr_in server;
    h->sockfd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if(h->sockfd < 0)
    {
        return ERR_STS_INVALID_SOCKET;
    }

    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    server.sin_addr.s_addr = inet_addr(host);

    in_addr_t adr = inet_addr(host);
    if(adr == INADDR_NONE)
    {
        /* Try name resolution */
        struct hostent* he = lwip_gethostbyname(host);
        if(!he || he->h_addr_list == NULL || he->h_addr_list[0] == NULL)
        {
            lwip_close(h->sockfd);
            h->sockfd = -1;
            return ERR_STS_CONNECT_FAILED;
        }
        memcpy(&server.sin_addr.s_addr, he->h_addr_list[0],
               sizeof(server.sin_addr.s_addr));
    }
    else
    {
        server.sin_addr.s_addr = adr;
    }

    if(lwip_connect(h->sockfd, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        lwip_close(h->sockfd);
        h->sockfd = -1;
        return ERR_STS_CONNECT_FAILED;
    }

    return ERR_STS_OK;
}
/* ------------------------------------------------------------------------- */
#if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)
static errStatus_t nal_tlsHandshake(nalHandle_t* handle)
{
    CHECK_PARAM(handle);
    int ret;

    mbedtls_net_init(&handle->net_ctx);
    mbedtls_ssl_init(&handle->ssl);
    mbedtls_ssl_config_init(&handle->conf);
    mbedtls_ctr_drbg_init(&handle->ctr_drbg);
    mbedtls_entropy_init(&handle->entropy);
    mbedtls_x509_crt_init(&handle->cacert);

    const char* pers = "nal_tls";
    if((ret = mbedtls_ctr_drbg_seed(&handle->ctr_drbg, mbedtls_entropy_func,
                                    &handle->entropy, (const unsigned char*)pers,
                                    strlen(pers))) != 0)
    {
        goto tls_fail;
    }

    if((ret = mbedtls_ssl_config_defaults(&handle->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        goto tls_fail;
    }

    mbedtls_ssl_conf_authmode(&handle->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_rng(&handle->conf, mbedtls_ctr_drbg_random, &handle->ctr_drbg);

    if((ret = mbedtls_ssl_setup(&handle->ssl, &handle->conf)) != 0)
    {
        goto tls_fail;
    }

    if(handle->server_name[0] != '\0')
    {
        ret = mbedtls_ssl_set_hostname(&handle->ssl, handle->server_name);
        if(ret != 0)
            goto tls_fail;
    }
    handle->net_ctx.fd = handle->sockfd;
    mbedtls_ssl_set_bio(&handle->ssl, &handle->net_ctx, mbedtls_net_send,
                        mbedtls_net_recv, NULL);

    /* Consider a handshake timeout wrapper if needed */
    if((ret = mbedtls_ssl_handshake(&handle->ssl)) != 0)
    {
        ret = ERR_STS_HANDSHAKE_FAIL;
        goto tls_fail;
    }

    handle->tls_initialized = 1;
    return ERR_STS_OK;

tls_fail:
    /* free any partial initialization */
    mbedtls_x509_crt_free(&handle->cacert);
    mbedtls_ssl_free(&handle->ssl);
    mbedtls_ssl_config_free(&handle->conf);
    mbedtls_ctr_drbg_free(&handle->ctr_drbg);
    mbedtls_entropy_free(&handle->entropy);
    mbedtls_net_free(&handle->net_ctx);
    handle->tls_initialized = 0;
    return ERR_STS_INTERNAL_ERROR;
}
#endif

/* ------------------------------------------------------------------------- */
/*
 * ============================================================
 * Public API
 * ============================================================
 */

errStatus_t nalNetworkInit(nalHandle_t* handle)
{
    CHECK_PARAM(handle);

    memset(handle, 0, sizeof(*handle));

    handle->scheme          = NAL_SCHEME_PLAIN;
    handle->sockfd          = -1;
    handle->recv_timeout_ms = 0;
    handle->send_timeout_ms = 0;

#ifdef SYS_USE_CMSIS
    handle->lock = osMutexNew(NULL);
    if(handle->lock == NULL)
    {
        return ERR_STS_FAIL;
    }
#else
    handle->lock = NULL;
#endif

#if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)
    /* initialize mbedTLS contexts but delay heavy operations until connect */
    mbedtls_ssl_init(&handle->ssl);
    mbedtls_ssl_config_init(&handle->conf);
    mbedtls_ctr_drbg_init(&handle->ctr_drbg);
    mbedtls_entropy_init(&handle->entropy);
    mbedtls_x509_crt_init(&handle->cacert);
    handle->tls_initialized = 0;
#endif

    /* ensure global async table inited */
    nal_async_table_init();

    return ERR_STS_OK;
}
/* ------------------------------------------------------------------------- */
errStatus_t nalNetworkDeinit(nalHandle_t* handle)
{
    CHECK_PARAM(handle);

    /* ensure we close any open socket */
    if(handle->sockfd >= 0)
    {
        lwip_close(handle->sockfd);
        handle->sockfd = -1;
    }

#ifdef SYS_USE_CMSIS
    if(handle->lock)
    {
        osMutexDelete(handle->lock);
        handle->lock = NULL;
    }
#endif

#if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)
    if(handle->tls_initialized)
    {
        /* free TLS runtime resources */
        mbedtls_ssl_free(&handle->ssl);
        mbedtls_ssl_config_free(&handle->conf);
        mbedtls_ctr_drbg_free(&handle->ctr_drbg);
        mbedtls_entropy_free(&handle->entropy);
        mbedtls_x509_crt_free(&handle->cacert);
        handle->tls_initialized = 0;
    }
    else
    {
        /* still free inited contexts (safe even if handshake not done) */
        mbedtls_x509_crt_free(&handle->cacert);
        mbedtls_ssl_free(&handle->ssl);
        mbedtls_ssl_config_free(&handle->conf);
        mbedtls_ctr_drbg_free(&handle->ctr_drbg);
        mbedtls_entropy_free(&handle->entropy);
    }
#endif

    /* clear full struct to a safe state */
    memset(handle, 0, sizeof(*handle));
    handle->sockfd = -1;

    return ERR_STS_OK;
}
/* ------------------------------------------------------------------------- */
errStatus_t nalNetworkConnect(nalHandle_t* handle, const char* host, uint16_t port, nalScheme_t scheme)
{
    errStatus_t sts = nal_openSocket(handle, host, port);
    if(sts != ERR_STS_OK)
    {
        return sts;
    }

    handle->scheme = scheme; /* ensure scheme remembered */

#if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)
    if(scheme == NAL_SCHEME_TLS)
    {
        strncpy(handle->server_name, host, sizeof(handle->server_name) - 1);
        sts = nal_tlsHandshake(handle);
        if(sts != ERR_STS_OK)
        {
            lwip_close(handle->sockfd);
            return sts;
        }
        handle->tls_initialized = 1;
    }
#endif
    return ERR_STS_OK;
}
/* ------------------------------------------------------------------------- */
errStatus_t nalNetworkDisconnect(nalHandle_t* handle)
{
    CHECK_PARAM(handle);

#if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)
    if(handle->scheme == NAL_SCHEME_TLS && handle->tls_initialized)
    {
        mbedtls_ssl_close_notify(&handle->ssl);
        mbedtls_ssl_free(&handle->ssl);
        mbedtls_ssl_config_free(&handle->conf);
        mbedtls_ctr_drbg_free(&handle->ctr_drbg);
        mbedtls_entropy_free(&handle->entropy);
        handle->tls_initialized = 0;
    }
#endif

    if(handle->sockfd >= 0)
    {
        lwip_close(handle->sockfd);
        handle->sockfd = -1;
    }
    return ERR_STS_OK;
}
/* ------------------------------------------------------------------------- */
int32_t nalNetworkSend(nalHandle_t* handle, const void* buf, size_t len, uint32_t timeout_ms)
{
    if(!handle || handle->sockfd < 0 || !buf)
    {
        return -1;
    }

    if(timeout_ms > 0)
    {
        struct timeval tv;
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        lwip_setsockopt(handle->sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    }

#if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)
    if(handle->scheme == NAL_SCHEME_TLS)
    {
        return mbedtls_ssl_write(&handle->ssl, buf, len);
    }
#endif

    return lwip_send(handle->sockfd, buf, len, 0);
}

/* ------------------------------------------------------------------------- */
int32_t nalNetworkRecv(nalHandle_t* handle, void* buf, size_t len, uint32_t timeout_ms)
{
    if(!handle || handle->sockfd < 0 || !buf)
    {
        return -1;
    }

    if(timeout_ms > 0)
    {
        struct timeval tv;
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        lwip_setsockopt(handle->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

#if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)
    if(handle->scheme == NAL_SCHEME_TLS)
    {
        return mbedtls_ssl_read(&handle->ssl, buf, len);
    }
#endif

    return lwip_recv(handle->sockfd, buf, len, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * ============================================================
 * Public API - Asynchronous
 * ============================================================
 */

errStatus_t nalNetworkStartAsync(nalHandle_t* handle, nalEventCallback_t cb, void* user_ctx)
{
    if(!handle || !cb)
    {
        return ERR_STS_INVALID_PARAM;
    }

    /* must have a valid socket (connected) */
    if(handle->sockfd < 0)
    {
        return ERR_STS_CONNECT_FAILED;
    }

    nalAsyncCtx_t* ctx = get_free_ctx();
    if(!ctx)
    {
        return ERR_STS_RESOURCE_BUSY;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->used         = true;
    ctx->sockfd       = (int)handle->sockfd;
    ctx->handle       = handle;
    ctx->cb           = cb;
    ctx->user_ctx     = user_ctx;
    ctx->recv_buf     = NULL;
    ctx->recv_buf_len = 0;
    ctx->send_buf     = NULL;
    ctx->send_len     = 0;
    ctx->want_send    = 0;
    ctx->lock         = osMutexNew(NULL);
    if(ctx->lock == NULL)
    {
        /* failed to create mutex */
        free_ctx(ctx);
        return ERR_STS_FAIL;
    }
    /* create exit semaphore for join/stop */
    ctx->exit_sem = osSemaphoreNew(1, 0, NULL);
    if(ctx->exit_sem == NULL)
    {
        free_ctx(ctx);
        return ERR_STS_FAIL;
    }
    ctx->running = true;

    char name[16];
    snprintf(name, sizeof(name), "nal_async_%d", ctx->sockfd);
    osThreadAttr_t attr = { .name       = (const char*)name,
                            .stack_size = NAL_ASYNC_THREAD_STACK,
                            .priority   = NAL_ASYNC_THREAD_PRIO };
    ctx->thread_id = osThreadNew((osThreadFunc_t)async_worker_thread, ctx, &attr);
    if(ctx->thread_id == NULL)
    {
        osMutexDelete(ctx->lock);
        osSemaphoreDelete(ctx->exit_sem);
        free_ctx(ctx);
        return ERR_STS_RESOURCE_BUSY;
    }

    return ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
errStatus_t nalNetworkStopAsync(nalHandle_t* handle)
{
    if(!handle)
    {
        return ERR_STS_INVALID_PARAM;
    }

    nalAsyncCtx_t* ctx = get_ctx_by_sockfd((int)handle->sockfd);
    if(!ctx)
    {
        return ERR_STS_NOT_INIT;
    }

    /* signal thread to stop */
    ctx->running = 0;

    /* wait for worker to signal exit semaphore (safe join) */
    if(ctx->exit_sem)
    {
        if(osSemaphoreAcquire(ctx->exit_sem, 5000) != osOK)
        {
            /* timeout waiting for thread exit; proceed with best-effort cleanup */
        }
    }

    /* cleanup context */
    free_ctx(ctx);
    return ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */

errStatus_t nalNetworkConnectAsync(nalHandle_t* handle,
                                   const char* host,
                                   uint16_t port,
                                   nalScheme_t scheme,
                                   nalEventCallback_t cb,
                                   void* user_ctx)
{
    if(!handle || !host || !cb)
    {
        return ERR_STS_INVALID_PARAM;
    }

    nalAsyncCtx_t* c = get_free_ctx();
    if(!c)
    {
        return ERR_STS_RESOURCE_BUSY;
    }

    memset(c, 0, sizeof(*c));
    c->used         = 1;
    c->sockfd       = -1; /* not connected yet */
    c->handle       = handle;
    c->cb           = cb;
    c->user_ctx     = user_ctx;
    c->recv_buf     = NULL;
    c->recv_buf_len = 0;
    c->send_buf     = NULL;
    c->send_len     = 0;
    c->want_send    = 0;
    c->lock         = osMutexNew(NULL);
    if(c->lock == NULL)
    {
        free_ctx(c);
        return ERR_STS_FAIL;
    }
    /* create exit semaphore for join/stop */
    c->exit_sem = osSemaphoreNew(1, 0, NULL);
    if(c->exit_sem == NULL)
    {
        osMutexDelete(c->lock);
        free_ctx(c);
        return ERR_STS_FAIL;
    }
    c->running = 1;

    /* store host copy in job */
    async_connect_job_t* job = malloc(sizeof(*job));
    if(!job)
    {
        osMutexDelete(c->lock);
        osSemaphoreDelete(c->exit_sem);
        free_ctx(c);
        return ERR_STS_INTERNAL_ERROR;
    }
    job->ctx    = c;
    job->host   = strdup(host);
    job->port   = port;
    job->scheme = scheme;

    char name[20];
    snprintf(name, sizeof(name), "nal_conn_%d", rand() & 0xFFFF);
    osThreadAttr_t attr = { .name       = name,
                            .stack_size = NAL_ASYNC_THREAD_STACK,
                            .priority   = NAL_ASYNC_THREAD_PRIO };
    c->thread_id = osThreadNew((osThreadFunc_t)async_connect_thread, job, &attr);
    if(c->thread_id == NULL)
    {
        free(job->host);
        free(job);
        osMutexDelete(c->lock);
        osSemaphoreDelete(c->exit_sem);
        free_ctx(c);
        return ERR_STS_RESOURCE_BUSY;
    }

    return ERR_STS_OK;
}


/* ------------------------------------------------------------------------- */
errStatus_t nalNetworkSendAsync(nalHandle_t* handle, const void* data, size_t len)
{
    if(!handle || !data || len == 0)
    {
        return ERR_STS_INVALID_PARAM;
    }

    nalAsyncCtx_t* c = get_ctx_by_sockfd((int)handle->sockfd);
    if(!c)
    {
        return ERR_STS_NOT_INIT;
    }

    osMutexAcquire(c->lock, osWaitForever);
    if(c->send_buf != NULL)
    {
        /* already a send pending */
        osMutexRelease(c->lock);
        return ERR_STS_RESOURCE_BUSY;
    }

    void* copy = malloc(len);
    if(!copy)
    {
        osMutexRelease(c->lock);
        return ERR_STS_INTERNAL_ERROR;
    }
    memcpy(copy, data, len);
    c->send_buf  = copy;
    c->send_len  = len;
    c->want_send = 1;
    osMutexRelease(c->lock);

    return ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
errStatus_t nalNetworkRecvAsync(nalHandle_t* handle, void* recv_buf, size_t recv_len)
{
    if(!handle)
    {
        return ERR_STS_INVALID_PARAM;
    }

    nalAsyncCtx_t* c = get_ctx_by_sockfd((int)handle->sockfd);
    if(!c)
    {
        return ERR_STS_INVALID_PARAM;
    }

    osMutexAcquire(c->lock, osWaitForever);
    c->recv_buf     = recv_buf;
    c->recv_buf_len = recv_len;
    osMutexRelease(c->lock);

    return ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
errStatus_t nalSetCaCert(nalHandle_t* handle, const uint8_t* ca_pem, size_t len)
{
#if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)
    if(!handle || !ca_pem || len == 0)
    {
        return ERR_STS_INVALID_PARAM;
    }

    // Free any previously loaded cert
    if(handle->ca_cert_buf)
    {
        free(handle->ca_cert_buf);
        handle->ca_cert_buf = NULL;
        handle->ca_cert_len = 0;
    }

    // Allocate and copy new CA cert
    handle->ca_cert_buf = (uint8_t*)malloc(len);
    if(!handle->ca_cert_buf)
    {
        return ERR_STS_NO_MEM;
    }

    memcpy(handle->ca_cert_buf, ca_pem, len);
    handle->ca_cert_len = len;
#endif // #if defined(SYS_USE_MBEDTLS ) || defined(ESP_PLATFORM_MBEDTLS)
    return ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
errStatus_t nalGetCaCert(nalHandle_t* handle, const uint8_t** out_ca_pem, size_t* out_len)
{
#if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)
    if(!handle || !out_ca_pem || !out_len)
    {
        return ERR_STS_INVALID_PARAM;
    }

    if(!handle->ca_cert_buf || handle->ca_cert_len == 0)
    {
        return ERR_STS_NOT_FOUND;
    }

    *out_ca_pem = handle->ca_cert_buf;
    *out_len    = handle->ca_cert_len;
#endif // #if defined(SYS_USE_MBEDTLS ) || defined(ESP_PLATFORM_MBEDTLS)
    return ERR_STS_OK;
}
/* ------------------------------------------------------------------------- */