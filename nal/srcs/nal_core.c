/*
 * nal_core.c
 *
 *
 *  - All public functions return bsp_err_sts_t to match BSP conventions.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "nal_config.h"
#include "nal_internal.h"
#include "nal_tls.h"
#include "nal_log.h"


#include "lwip/sockets.h" /* socket(), connect(), bind(), listen(), accept(), send(), recv() */
#include "lwip/netdb.h" /* gethostbyname(), getaddrinfo() */
#include "lwip/errno.h" /* lwIP errno values */
#include "lwip/sys.h"   /* lwip_select(), timeouts */

#include "lwip/inet.h"
#include "lwip/ip_addr.h"

#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"

#include "lwip/timeouts.h"


#define NAL_EVENT_TASK_STACK_SIZE (nalCONFIG_NAL_EVENT_THREAD_STACK)
#define NAL_EVENT_TASK_NAME       (nalCONFIG_NAL_EVENT_THREAD_NAME)
#define NAL_EVENT_TASK_PRIORITY   (nalCONFIG_NAL_EVENT_THREAD_PRIO)
#define NAL_EVENT_TIMEOUT_MS      (nalCONFIG_NAL_NET_RECV_TIMEOUT_MS)


#ifndef NAL_TLS_ALLOC_MODE
    #define NAL_TLS_ALLOC_MODE NAL_TLS_ALLOC_STATIC
#endif

#define NAL_MAX_CONNECTIONS (bspNAL_MAX_CONNCETION_SUPPORT)

/*
 * =========================================================================
 *  Internal Structures
 * =========================================================================
 */
/**
 * @brief Asynchronous NAL event message.
 *
 * This structure represents a single network-related event generated
 * by the NAL asynchronous worker and delivered to the application
 * via a message queue.
 *
 * Ownership rules:
 *  - The @ref handle is owned by the caller and must remain valid.
 *  - The @ref data buffer ownership depends on the event type.
 */
typedef struct
{
    nalHandle_t* handle; /**< NAL handle associated with the event */
    nalEvent_t event;    /**< Type of network event */
    void* data;          /**< Optional event-specific data buffer */
    size_t length;       /**< Length of event-specific data in bytes */
} nalEventMsg_t;


/**
 * @brief Internal asynchronous NAL context.
 *
 * Holds runtime state required to support asynchronous network
 * event handling, including the worker task and event queue.
 *
 * This context is INTERNAL to NAL and must not be accessed
 * directly by application code.
 */
typedef struct
{
    osMessageQueueId_t evt_queue; /**< Message queue for async NAL events */
    osThreadId_t worker_tid;      /**< Worker thread handling network events */
    uint8_t enabled; /**< Non-zero if async event worker is running */
} nalAsyncCtx_t;


/**
 * @brief Global asynchronous NAL context instance.
 *
 * This singleton context manages the asynchronous event subsystem
 * for the NAL module.
 */
static nalAsyncCtx_t g_nal_async[NAL_MAX_CONNECTIONS] = { 0 };


/**
 * @brief Flag to indicate if a session is in use.
 *
 */
static int8_t g_nal_session_id[NAL_MAX_CONNECTIONS] = { 0 };

/* ============================================================
 * Internal Helper Functions
 * ============================================================ */

static int8_t get_free_session_id(void)
{
    for(int i = 0; i < NAL_MAX_CONNECTIONS; i++)
    {
        if(g_nal_session_id[i] == false)
        {
            g_nal_session_id[i] = true;
            return i;
        }
    }
    return -1;
}

/**
 * @brief Internal NAL network event worker task.
 *
 * - Monitors socket activity
 * - Receives data
 * - Generates nalEventMsg_t
 * - Sends it to application queue
 */
static void nalNetEventTask(void* arg)
{
    nalEventMsg_t msg;
    uint8_t rx_buf[512];
    nalHandle_t* handle = (nalHandle_t*)arg;

    for(;;)
    {
        /* Example: simple blocking receive */
        size_t rx_len = 0;
        bsp_err_sts_t ret;

        ret = nalNetworkRecvSync(handle, rx_buf, sizeof(rx_buf), &rx_len, NAL_EVENT_TIMEOUT_MS);

        if(ret == BSP_ERR_STS_OK && rx_len > 0)
        {
            msg.handle = handle;
            msg.event  = NAL_EVENT_DATA_RECEIVED;
            msg.data   = rx_buf;
            msg.length = rx_len;

            /* Send event to application */
            osMessageQueuePut(g_nal_async[handle->id].evt_queue, &msg, 0, 0);
        }
        else if(ret == BSP_ERR_STS_OK && rx_len == 0)
        {
            msg.handle = handle;
            msg.event  = NAL_EVENT_DISCONNECTED;
            msg.data   = NULL;
            msg.length = 0;

            /* Send event to application */
            osMessageQueuePut(g_nal_async[handle->id].evt_queue, &msg, 0, 0);
        }
        else if(ret == BSP_ERR_STS_TIMEOUT)
        {
            msg.handle = handle;
            msg.event  = NAL_EVENT_TIMEOUT;
            msg.data   = NULL;
            msg.length = 0;

            /* Send event to application */
            osMessageQueuePut(g_nal_async[handle->id].evt_queue, &msg, 0, 0);
        }
        else
        {
            msg.handle = handle;
            msg.event  = NAL_EVENT_ERROR;
            msg.data   = NULL;
            msg.length = 0;

            /* Send event to application */
            osMessageQueuePut(g_nal_async[handle->id].evt_queue, &msg, 0, 0);
        }
    }
}


/* ------------------------------------------------------------------------- */
/*
 * ============================================================
 * Public API
 * ============================================================
 */
bsp_err_sts_t nalNetworkInit(nalHandle_t* handle)
{
    if(handle == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /*
     * Always start from a clean slate.
     * This prevents undefined behavior if the handle
     * was stack-allocated or reused.
     */
    memset(handle, 0, sizeof(nalHandle_t));

    handle->sockfd = -1;
    handle->scheme = NAL_SCHEME_PLAIN;

    int8_t session_id = get_free_session_id();
    if(session_id >= 0)
    {
        handle->id = session_id;
    }
    else
    {
        handle->id = -1;
        return BSP_ERR_STS_NO_MEM;
    }
    NAL_LOGI("Session ID: %d", handle->id);

    /*
     * Create per-handle mutex.
     * This allows multiple connections to be used safely
     * from different tasks without a global bottleneck.
     */
    if(handle->lock == NULL)
    {
        handle->lock = osMutexNew(NULL);
        if(handle->lock == NULL)
        {
            NAL_LOGI("Failed to create mutex");
            return BSP_ERR_STS_NO_MEM;
        }
    }

#if defined(NAL_USE_TLS)
    handle->tls_ctx = NULL; /* created on connect */
#endif

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalNetworkDeinit(nalHandle_t* handle)
{
    CHECK_PARAM(handle);

    NAL_LOCK(handle);
    /* ensure we close any open socket */
    if(handle->sockfd >= 0)
    {
        NAL_LOGI("Closing socket %d", handle->sockfd);
        lwip_close(handle->sockfd);
        handle->sockfd = -1;
    }


#if defined(NAL_USE_TLS)
    if(handle->tls_ctx)
    {
        NAL_LOGI("Closing TLS context");
        nalTlsDeinit(handle->tls_ctx);
        handle->tls_ctx = NULL;
    }
#endif

    NAL_UNLOCK(handle);

    osMutexDelete(handle->lock);
    handle->lock                 = NULL;
    g_nal_session_id[handle->id] = 0;
    memset(handle, 0, sizeof(nalHandle_t));
    handle->id = -1;

    return BSP_ERR_STS_OK;
}
/* ------------------------------------------------------------------------- */
bsp_err_sts_t
nalNetworkConnect(nalHandle_t* handle, const char* host, uint16_t port, nalScheme_t scheme, uint32_t timeout_ms)
{
    if(!handle || !host || port == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    NAL_LOCK(handle);

    NAL_LOGI("Connecting to %s:%d", host, port);

    /* -------------------------------------------------
     * 1. Prevent reconnect on active socket
     * -------------------------------------------------
     * A handle can own only one socket at a time.
     * This avoids fd leaks and undefined behavior.
     */
    if(handle->sockfd >= 0)
    {
        NAL_LOGI("Socket already connected");
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_BUSY;
    }

    handle->scheme = scheme;
    handle->role   = NAL_ROLE_CLIENT;

    /* -------------------------------------------------
     * 2. Create TCP socket
     * -------------------------------------------------
     * AF_INET      -> IPv4
     * SOCK_STREAM  -> TCP
     */
    int sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        NAL_LOGI("Socket creation failed: %d", (errno));
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_INVALID_SOCKET;
    }

    /* -------------------------------------------------
     * 3. Make socket non-blocking
     * -------------------------------------------------
     * Required so connect() does not block forever.
     * Timeout will be handled using select().
     */
    int flags = lwip_fcntl(sock, F_GETFL, 0);
    lwip_fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    /* -------------------------------------------------
     * 4. Prepare destination address
     * ------------------------------------------------- */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    /* -------------------------------------------------
     * 5. Resolve host
     * -------------------------------------------------
     * First try direct IP (faster).
     * If that fails, fall back to DNS.
     */
    in_addr_t ip = inet_addr(host);
    if(ip == INADDR_NONE)
    {
        struct hostent* he = lwip_gethostbyname(host);
        if(!he || !he->h_addr_list || !he->h_addr_list[0])
        {
            lwip_close(sock);
            NAL_LOGI("Host resolution failed: %d", (errno));
            NAL_UNLOCK(handle);
            return BSP_ERR_STS_NOT_FOUND;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], sizeof(addr.sin_addr));
    }
    else
    {
        /* IP string case */
        addr.sin_addr.s_addr = ip;
    }

    /* -------------------------------------------------
     * 6. Start non-blocking connect
     * -------------------------------------------------
     * Expected outcomes:
     *  - rc == 0           -> connected immediately
     *  - errno=EINPROGRESS -> connection in progress
     */
    int rc = lwip_connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if(rc < 0 && errno != EINPROGRESS)
    {
        lwip_close(sock);
        NAL_LOGI("Connect failed immediately: %d", errno);
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_CONN_FAILED;
    }

    /* -------------------------------------------------
     * 7. Wait for connect completion using select()
     * -------------------------------------------------
     * Socket becomes writable when:
     *  - connection succeeds OR
     *  - connection fails
     */
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);

    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    rc = lwip_select(sock + 1, NULL, &wfds, NULL, (timeout_ms > 0) ? &tv : NULL);

    if(rc == 0)
    {
        /* Timeout expired */
        lwip_close(sock);
        NAL_LOGI("Connect timeout (%lu ms)", timeout_ms);
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_TIMEOUT;
    }
    else if(rc < 0)
    {
        /* Select internal error */

        lwip_close(sock);
        NAL_LOGI("Select failed: %d", errno);
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_FAIL;
    }

    /* Defensive check */
    if(!FD_ISSET(sock, &wfds))
    {
        lwip_close(sock);
        NAL_LOGI("Socket not writable after select");
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_FAIL;
    }

    /* -------------------------------------------------
     * 8. Verify final socket error
     * -------------------------------------------------
     * Even if select() succeeds, the connection may
     * still have failed. SO_ERROR tells the truth.
     */
    int so_err    = 0;
    socklen_t len = sizeof(so_err);
    lwip_getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_err, &len);

    if(so_err != 0)
    {
        lwip_close(sock);
        NAL_UNLOCK(handle);
        NAL_LOGI("Socket error: %d", (so_err));
        return BSP_ERR_STS_CONN_FAILED;
    }

    /* -------------------------------------------------
     * 9. Restore blocking mode
     * -------------------------------------------------
     * recv()/send() APIs assume blocking sockets.
     */
    lwip_fcntl(sock, F_SETFL, flags);

    /* -------------------------------------------------
     * 10. Attach socket to handle
     * ------------------------------------------------- */
    handle->sockfd = sock;

#if defined(NAL_USE_TLS)
    /* -------------------------------------------------
     * 11. TLS initialization and handshake (optional)
     * ------------------------------------------------- */
    if(handle->scheme == NAL_SCHEME_TLS)
    {
        bsp_err_sts_t tls_rc = nalTlsInit(handle);
        if(tls_rc != BSP_ERR_STS_OK)
        {
            lwip_close(sock);
            handle->sockfd = -1;
            NAL_LOGI("TLS init failed: %d", (tls_rc));
            NAL_UNLOCK(handle);
            return tls_rc;
        }

        tls_rc = nalTlsHandshake((nalTlsCtx_t*)handle->tls_ctx);
        if(tls_rc != BSP_ERR_STS_OK)
        {
            nalTlsDeinit(handle);
            lwip_close(sock);
            handle->sockfd = -1;
            NAL_LOGI("TLS handshake failed: %s", bsp_err_sts_to_str(tls_rc));
            NAL_UNLOCK(handle);
            return tls_rc;
        }
    }
#endif
    /* -------------------------------------------------
     * 12. Success
     * ------------------------------------------------- */

    NAL_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
/**
 * @brief Disconnect an active network connection and release resources.
 *
 * This function cleanly closes an active TCP or TLS connection associated
 * with the given NAL handle.
 *
 * Why this function exists:
 *  - Embedded systems must explicitly release sockets.
 *  - Leaving sockets open causes resource leaks and unstable networking.
 *  - TLS connections require proper shutdown before closing the socket.
 *
 * What this function does:
 *  1. If TLS is enabled and active:
 *       - Perform a TLS shutdown (best-effort).
 *       - Free TLS runtime objects.
 *  2. Close the underlying TCP socket if it is open.
 *  3. Reset the handle state so it can be reused safely.
 *
 * Design rules:
 *  - Safe to call multiple times (idempotent).
 *  - Never blocks for long periods.
 *  - Leaves handle in a known "disconnected" state.
 *
 * This function does NOT:
 *  - Free the nalHandle_t itself.
 *  - Deinitialize global networking (use nalNetworkDeinit for that).
 *
 * @param[in,out] handle Pointer to an initialized NAL handle.
 *
 * @return BSP_ERR_STS_OK            Disconnect successful or already disconnected
 * @return BSP_ERR_STS_INVALID_PARAM Invalid handle
 * @return BSP_ERR_STS_INTERNAL_ERROR Socket close or TLS cleanup failure
 */
bsp_err_sts_t nalNetworkDisconnect(nalHandle_t* handle)
{
    if(handle == NULL)
    {
        NAL_LOGI("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* If already disconnected, treat as success */
    if(handle->sockfd < 0)
    {
        NAL_LOGI("Socket already disconnected");
        return BSP_ERR_STS_OK;
    }

#if defined(NAL_USE_TLS)
    /* Ask TLS layer to shutdown if active */
    if(handle->scheme == NAL_SCHEME_TLS)
    {
        nalTlsShutdown(handle);
    }
#endif /* NAL_USE_TLS */

    /* Close TCP socket */
    lwip_close(handle->sockfd);
    handle->sockfd = -1;

    /* Reset transport scheme to default */
    handle->scheme = NAL_SCHEME_PLAIN;

    NAL_LOGI("Socket %d disconnected", handle->sockfd);
    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
/**
 * @brief Send data synchronously over an active NAL connection.
 *
 * This function transmits the given data buffer over an already-established
 * network connection (TCP or TLS) and blocks until one of the following occurs:
 *  - All bytes are successfully sent
 *  - The specified timeout expires
 *  - A transport error or disconnect is detected
 *
 * The function supports both plain TCP and TLS connections:
 *  - For plain TCP, data is sent directly using the underlying socket.
 *  - For TLS connections, the send operation is delegated to the TLS layer
 *    (nal_tls.c), keeping encryption logic isolated from core networking code.
 *
 * Internally, the function handles partial sends and retries as required by
 * stream-based transports (TCP/TLS). It guarantees that either:
 *  - All requested bytes are sent, or
 *  - A clear error/timeout status is returned.
 *
 * Timeout behavior:
 *  - If timeout_ms is 0, the function blocks indefinitely until completion
 *    or error.
 *  - If timeout_ms > 0, the function repeatedly attempts to send remaining
 *    data until the timeout expires.
 *
 * Thread safety:
 *  - This function assumes the caller serializes access per nalHandle_t.
 *  - It does not perform internal locking to avoid unnecessary overhead
 *    in single-threaded or task-owned usage patterns.
 *
 * Typical usage:
 * @code
 * nalHandle_t net;
 * size_t sent;
 *
 * nalNetworkInit(&net);
 * nalNetworkConnect(&net, "192.168.1.10", 8000, NAL_SCHEME_PLAIN, 2000);
 *
 * if (nalNetworkSendSync(&net, "HELLO", 5, &sent, 1000) == BSP_ERR_STS_OK)
 * {
 *     // Data successfully sent
 * }
 *
 * nalNetworkDisconnect(&net);
 * nalNetworkDeinit(&net);
 * @endcode
 *
 * @param[in,out] handle      Pointer to an initialized and connected nalHandle_t
 * @param[in]     data        Pointer to data buffer to transmit
 * @param[in]     len         Number of bytes to send
 * @param[out]    bytes_sent  Number of bytes successfully sent
 * @param[in]     timeout_ms  Send timeout in milliseconds (0 = wait forever)
 *
 * @return
 *  - BSP_ERR_STS_OK            All data sent successfully
 *  - BSP_ERR_STS_TIMEOUT       Timeout expired before sending all data
 *  - BSP_ERR_STS_NOT_CONNECTED Handle is not connected
 *  - BSP_ERR_STS_INVALID_PARAM Invalid argument(s)
 *  - BSP_ERR_STS_CONN_LOST     Connection closed by peer
 *  - BSP_ERR_STS_FAIL          Transport-level send failure
 */

bsp_err_sts_t
nalNetworkSendSync(nalHandle_t* handle, const void* data, size_t len, size_t* bytes_sent, uint32_t timeout_ms)
{
    if(handle == NULL || data == NULL || bytes_sent == NULL || len == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* Must be connected */
    if(handle->sockfd < 0)
    {
        NAL_LOGI("Socket already disconnected");
        return BSP_ERR_STS_NO_CONN;
    }

    *bytes_sent = 0;

#if defined(NAL_USE_TLS)
    /* TLS path */
    if(handle->scheme == NAL_SCHEME_TLS)
    {
        return nalTlsSend(handle, data, len, bytes_sent, timeout_ms);
    }
    else
    {
        NAL_LOGI("Invalid scheme: %d", handle->scheme);
        return BSP_ERR_STS_INVALID_PARAM;
    }
#endif /* NAL_USE_TLS */

    /* ===============================
     * Plain TCP socket path
     * =============================== */

    const uint8_t* ptr = (const uint8_t*)data;
    size_t remaining   = len;
    uint32_t start_ms  = osKernelGetTickCount();

    NAL_LOGI("Sending %zu bytes to socket %d", len, handle->sockfd);

    while(remaining > 0)
    {
        /* Check timeout */
        if(timeout_ms > 0)
        {
            uint32_t elapsed = NAL_TICKS_TO_MS(osKernelGetTickCount() - start_ms);
            if(elapsed >= timeout_ms)
            {
                NAL_LOGE("Timeout expired before sending all data");
                return BSP_ERR_STS_TIMEOUT;
            }
        }

        ssize_t ret = lwip_send(handle->sockfd, ptr, remaining, 0);

        if(ret < 0)
        {
            /* EWOULDBLOCK / EAGAIN → retry until timeout */
            if(errno == EWOULDBLOCK || errno == EAGAIN)
            {
                NAL_LOGE("EWOULDBLOCK / EAGAIN");
                osDelay(NAL_MS_TO_TICKS(20)); /* brief delay before retry */
                continue;
            }

            NAL_LOGE("Transport-level send failure");
            return BSP_ERR_STS_FAIL;
        }

        if(ret == 0)
        {
            /* Peer closed connection */
            NAL_LOGE("Peer closed connection");
            return BSP_ERR_STS_CONN_LOST;
        }

        ptr += ret;
        remaining -= (size_t)ret;
        *bytes_sent += (size_t)ret;
    }

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
bsp_err_sts_t
nalNetworkRecvSync(nalHandle_t* handle, void* buf, size_t buf_len, size_t* bytes_recv, uint32_t timeout_ms)
{
    /* Validate input parameters */
    if(handle == NULL || buf == NULL || bytes_recv == NULL || buf_len == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    *bytes_recv = 0;

    /* Socket must be valid (connected) */
    if(handle->sockfd < 0)
    {
        NAL_LOGE("Socket already disconnected");
        return BSP_ERR_STS_NO_CONN;
    }

#if defined(NAL_USE_TLS)
    /*
     * TLS path:
     * Data reception is delegated entirely to the TLS layer.
     * TLS handles its own buffering, timeouts, and decryption.
     */
    if(handle->scheme == NAL_SCHEME_TLS)
    {
        NAL_LOGE("TLS scheme detected");
        return nalTlsRecv(handle, buf, buf_len, bytes_recv, timeout_ms);
    }
#endif /* NAL_USE_TLS */

    /*
     * Configure receive timeout.
     * SO_RCVTIMEO controls how long recv() will block.
     *
     * timeout_ms > 0:
     *   recv() blocks up to timeout_ms.
     *
     * timeout_ms == 0:
     *   recv() should not block (best-effort poll).
     */
    /* Non-blocking behavior: zero timeout */
    struct timeval tv = { 0 };
    if(timeout_ms > 0)
    {
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
    }

    /* Apply socket receive timeout */
    lwip_setsockopt(handle->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /*
     * Attempt to receive data.
     * For TCP:
     *   >0  : number of bytes received
     *    0  : peer performed orderly shutdown
     *   <0  : error (check errno)
     */
    int ret = lwip_recv(handle->sockfd, buf, buf_len, 0);

    if(ret > 0)
    {
        /* Data successfully received */
        *bytes_recv = (size_t)ret;
        return BSP_ERR_STS_OK;
    }

    if(ret == 0)
    {
        /*
         * Peer closed the connection cleanly (FIN received).
         * Socket is no longer usable.
         */
        NAL_LOGE("Peer performed an orderly shutdown");
        lwip_close(handle->sockfd);
        handle->sockfd = -1;
        return BSP_ERR_STS_CONN_LOST;
    }

    /*
     * ret < 0: error case
     */
    if(errno == EWOULDBLOCK || errno == EAGAIN)
    {
        /*
         * No data available within timeout.
         * This is not a fatal error.
         */
        NAL_LOGE("EWOULDBLOCK / EAGAIN");
        return BSP_ERR_STS_TIMEOUT;
    }

    /*
     * Any other error indicates a transport failure.
     */
    NAL_LOGE("Transport-level receive failure (errno=%d)", errno);
    return BSP_ERR_STS_FAIL;
}


/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalSetCaCert(nalHandle_t* handle, const uint8_t* ca_pem, size_t len)
{
    if(handle == NULL || ca_pem == NULL || len == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

#if !defined(NAL_USE_TLS)
    /* TLS not enabled in this build */
    return BSP_ERR_STS_UNSUPPORTED;
#else
    return nalTlsSetCaCert(handle, ca_pem, len);
#endif /* NAL_USE_TLS */
}


/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalGetCaCert(nalHandle_t* handle, const uint8_t** out_ca_pem, size_t* out_len)
{
    if(handle == NULL || out_ca_pem == NULL || out_len == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

#if !defined(NAL_USE_TLS)
    /* TLS not enabled in this build */
    return BSP_ERR_STS_UNSUPPORTED;
#else
    return nalTlsGetCaCert(handle, out_ca_pem, out_len);
#endif /* NAL_USE_TLS */
}
/* ------------------------------------------------------------------------- */

bsp_err_sts_t nalEnableAsyncEvents(nalHandle_t* handle, osMessageQueueId_t queue)
{
    if(handle == NULL || queue == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(g_nal_async[handle->id].enabled)
    {
        return BSP_ERR_STS_OK;
    }

    g_nal_async[handle->id].evt_queue = queue;
    g_nal_async[handle->id].enabled   = 1;

    const osThreadAttr_t attr = { .name       = NAL_EVENT_TASK_NAME,
                                  .priority   = NAL_EVENT_TASK_PRIORITY,
                                  .stack_size = NAL_EVENT_TASK_STACK_SIZE };

    g_nal_async[handle->id].worker_tid = osThreadNew(nalNetEventTask, handle, &attr);

    if(g_nal_async[handle->id].worker_tid == NULL)
    {
        g_nal_async[handle->id].enabled = 0;
        return BSP_ERR_STS_NO_MEM;
    }

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */

bsp_err_sts_t
nalNetworkStartServer(nalHandle_t* handle, uint16_t port, nalScheme_t scheme, uint32_t backlog)
{
    if(!handle)
        return BSP_ERR_STS_INVALID_PARAM;

    NAL_LOCK(handle);

    /* Protect handle state against concurrent access */
    handle->scheme = scheme;
    handle->role   = NAL_ROLE_SERVER;

    /* Create a TCP socket (IPv4, stream-oriented) */
    handle->sockfd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(handle->sockfd < 0)
    {
        NAL_LOGE("Server socket creation failed");
        goto err;
    }

    NAL_LOGI("Server socket created %d", handle->sockfd);

    /* Allow address reuse to avoid TIME_WAIT bind failures */
    int optval = 1;
    if(lwip_setsockopt(handle->sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        NAL_LOGE("Server setsockopt failed");
        goto err;
    }

    /* Prepare server bind address (any local interface) */
    struct sockaddr_in addr = { 0 };
    addr.sin_family         = AF_INET;
    addr.sin_port           = htons(port);
    addr.sin_addr.s_addr    = htonl(INADDR_ANY);

    /* Bind socket to local port */
    if(lwip_bind(handle->sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        NAL_LOGE("Server bind failed");
        goto err;
    }
    NAL_LOGI("Server bound, port %d", port);

    /* Put socket into listening mode */
    if(lwip_listen(handle->sockfd, backlog) < 0)
    {
        NAL_LOGE("Server listen failed");
        goto err;
    }


    NAL_LOGI("Server listening on port %d", port);

#if defined(NAL_USE_TLS)
    /* Initialize server-side TLS context if TLS is requested */
    if(scheme == NAL_SCHEME_TLS)
    {
        /* Server must have certificate + key */
        if(!handle->tls_creds.server)
        {
            NAL_LOGE("No TLS credentials provided");
            goto err;
        }

        /* Allocate TLS server configuration context */
        handle->tls_ctx = nalTlsAlloc();
        if(!handle->tls_ctx)
        {
            NAL_LOGE("TLS context allocation failed");
            goto err;
        }

        /* Load certificate and private key into TLS config */
        if(nalTlsServerInit(handle->tls_ctx, handle->tls_creds.server) != BSP_ERR_STS_OK)
        {
            NAL_LOGE("TLS server initialization failed");
            goto err;
        }
    }
#endif

    /* Server successfully started */
    NAL_LOGI("Server started on port %d", port);
    NAL_UNLOCK(handle);
    return BSP_ERR_STS_OK;

err:
    /* Cleanup on failure */
    if(handle->sockfd >= 0)
    {
        lwip_close(handle->sockfd);
    }

    handle->sockfd = -1;
    NAL_LOGE("Server start failed");
    NAL_UNLOCK(handle);
    return BSP_ERR_STS_FAIL;
}


/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalNetworkAccept(nalHandle_t* server, nalHandle_t* client, uint32_t timeout_ms)
{
    if(!server || !client)
        return BSP_ERR_STS_INVALID_PARAM;

    fd_set rfds;
    struct timeval tv;
    char addr_str[128];

    /* Lock server to serialize accept operations */
    NAL_LOCK(server);

    /* Prepare select timeout if requested */
    if(timeout_ms > 0)
    {
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        /* IMPORTANT: initialize fd_set */
        FD_ZERO(&rfds);
        FD_SET(server->sockfd, &rfds);

        /* Wait for incoming connection */
        int ret = lwip_select(server->sockfd + 1, &rfds, NULL, NULL, &tv);

        /* Timeout expired without connection */
        if(ret == 0)
        {
            NAL_LOGI("Accept timed out");
            NAL_UNLOCK(server);
            return BSP_ERR_STS_TIMEOUT;
        }
        /* select() error */
        if(ret < 0)
        {
            /* select error */
            NAL_LOGE("Select error");
            NAL_UNLOCK(server);
            return BSP_ERR_STS_FAIL;
        }
    }

    /* Accept incoming connection */
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    int client_fd = lwip_accept(server->sockfd, (struct sockaddr*)&addr, &addr_len);
    if(client_fd < 0)
    {
        NAL_LOGI("Accept failed");
        NAL_UNLOCK(server);

        if(errno == EWOULDBLOCK || errno == EAGAIN)
            return BSP_ERR_STS_TIMEOUT;

        return BSP_ERR_STS_FAIL;
    }

    /* Configure TCP keepalive to detect dead peers */
    setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &server->keepAlive, sizeof(int));
    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPIDLE, &server->keepIdle, sizeof(int));
    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPINTVL, &server->keepInterval, sizeof(int));
    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPCNT, &server->keepCount, sizeof(int));

    /* Convert client address to string */
    if(addr.ss_family == PF_INET)
    {
        inet_ntoa_r(((struct sockaddr_in*)&addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
    }
    else if(addr.ss_family == PF_INET6)
    {
        inet6_ntoa_r(((struct sockaddr_in6*)&addr)->sin6_addr, addr_str,
                     sizeof(addr_str) - 1);
    }

    /* Initialize client handle */
    client->sockfd       = client_fd;
    client->scheme       = server->scheme;
    client->role         = NAL_ROLE_CLIENT;
    client->keepAlive    = server->keepAlive;
    client->keepIdle     = server->keepIdle;
    client->keepInterval = server->keepInterval;
    client->keepCount    = server->keepCount;

#if defined(NAL_USE_TLS)
    /* Perform server-side TLS handshake per client */
    if(server->scheme == NAL_SCHEME_TLS)
    {
        client->tls_ctx = nalTlsAlloc();
        if(!client->tls_ctx)
        {
            NAL_LOGI("TLS context allocation failed");
            goto err;
        }

        if(nalTlsServerAttach(client->tls_ctx, client_fd, server->tls_ctx) != BSP_ERR_STS_OK)
        {
            NAL_LOGI("TLS server attach failed");
            goto err;
        }

        if(nalTlsServerHandshake(client->tls_ctx) != BSP_ERR_STS_OK)
        {
            NAL_LOGI("TLS server handshake failed");
            goto err;
        }
    }
#endif

    /* Client successfully accepted */
    NAL_LOGI("Client accepted ip address: %s", addr_str);
    NAL_UNLOCK(server);
    return BSP_ERR_STS_OK;


#if defined(NAL_USE_TLS)
err:
    if(client->tls_ctx)
    {
        NAL_LOGI("TLS context freed");
        nalTlsFree(client->tls_ctx);
    }
#endif
    lwip_close(client_fd);
    client->sockfd = -1;
    NAL_LOGI("Client accept failed");
    NAL_UNLOCK(server);
    return BSP_ERR_STS_FAIL;
}

/* ------------------------------------------------------------------------- */
bsp_err_sts_t nalNetworkStopServer(nalHandle_t* handle)
{
    if(!handle)
        return BSP_ERR_STS_INVALID_PARAM;

    NAL_LOCK(handle);
    if(handle->sockfd >= 0)
    {
        NAL_LOGI("Closing server socket");
        lwip_close(handle->sockfd);
        handle->sockfd = -1;
    }

#if defined(NAL_USE_TLS)
    if(handle->tls_ctx)
    {
        NAL_LOGI("TLS context freed");
        nalTlsDeinit(handle);
        handle->tls_ctx = NULL;
    }
#endif

    NAL_LOGI("Server stopped");
    NAL_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}
/* ------------------------------------------------------------------------- */
/**
 ********************************************************************************
 * End of nal_core.c
 ********************************************************************************
 */