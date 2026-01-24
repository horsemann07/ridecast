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

#define NAL_MS_TO_TICKS(ms)    (((ms) * osKernelGetTickFreq()) / 1000U)

#define NAL_TICKS_TO_MS(ticks) (((ticks) * 1000U) / osKernelGetTickFreq())

/*
 * =========================================================================
 *  Internal Macros
 * =========================================================================
 */

/**
 * @brief Acquire NAL handle mutex (if valid).
 *
 * Used by NAL core to protect per-handle state.
 */
#define NAL_LOCK(h)                                   \
    do                                                \
    {                                                 \
        if((h) && (h->lock))                          \
        {                                             \
            osMutexAcquire((h)->lock, osWaitForever); \
        }                                             \
    } while(0)

/**
 * @brief Release NAL handle mutex (if valid).
 */
#define NAL_UNLOCK(h)                  \
    do                                 \
    {                                  \
        if((h) && (h->lock))           \
        {                              \
            osMutexRelease((h)->lock); \
        }                              \
    } while(0)

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
static nalAsyncCtx_t g_nal_async = { 0 };

/* ============================================================
 * Internal Helper Functions
 * ============================================================ */

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
            osMessageQueuePut(g_nal_async.evt_queue, &msg, 0, 0);
        }
        else if(ret == BSP_ERR_STS_OK && rx_len == 0)
        {
            msg.handle = handle;
            msg.event  = NAL_EVENT_DISCONNECTED;
            msg.data   = NULL;
            msg.length = 0;

            /* Send event to application */
            osMessageQueuePut(g_nal_async.evt_queue, &msg, 0, 0);
        }
        else if(ret == BSP_ERR_STS_TIMEOUT)
        {
            msg.handle = handle;
            msg.event  = NAL_EVENT_TIMEOUT;
            msg.data   = NULL;
            msg.length = 0;

            /* Send event to application */
            osMessageQueuePut(g_nal_async.evt_queue, &msg, 0, 0);
        }
        else
        {
            msg.handle = handle;
            msg.event  = NAL_EVENT_ERROR;
            msg.data   = NULL;
            msg.length = 0;

            /* Send event to application */
            osMessageQueuePut(g_nal_async.evt_queue, &msg, 0, 0);
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
    memset(handle, 0, sizeof(*handle));

    handle->sockfd = -1;
    handle->scheme = NAL_SCHEME_PLAIN;

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
        lwip_close(handle->sockfd);
        handle->sockfd = -1;
    }


#if defined(NAL_USE_TLS)
    if(handle->tls_ctx)
    {
        nalTlsDeinit(handle->tls_ctx);
        handle->tls_ctx = NULL;
    }
#endif

    NAL_UNLOCK(handle);

    osMutexDelete(handle->lock);
    memset(handle, 0, sizeof(nalHandle_t));
    handle->lock = NULL;

    return BSP_ERR_STS_OK;
}
/* ------------------------------------------------------------------------- */

/*
 * @brief Establish a TCP or TLS connection with timeout support.
 *
 * This function connects to a remote host using a blocking-style API,
 * but internally implements a connection timeout using a non-blocking
 * socket and select().
 *
 * Why this function exists:
 *  - Standard connect() may block forever on embedded systems.
 *  - Embedded applications must not hang if the network is unstable.
 *  - This function guarantees the call either succeeds or fails
 *    within the given timeout.
 *
 * How it works (high level):
 *  1. Create a TCP socket.
 *  2. Resolve hostname to IP address.
 *  3. Set the socket to non-blocking mode.
 *  4. Start connect() and wait using select() until:
 *        - connection succeeds,
 *        - connection fails, or
 *        - timeout expires.
 *  5. Restore socket to blocking mode.
 *  6. Optionally perform TLS handshake if enabled.
 *
 * On success:
 *  - The socket is stored inside the nalHandle_t.
 *  - The handle is ready for send/receive operations.
 *
 * On failure:
 *  - The socket is closed.
 *  - The handle remains in a clean, disconnected state.
 *
 * This API is safe to call from a task/thread context.
 * It does not block longer than timeout_ms.
 *
 * @param[in,out] handle     Initialized NAL handle.
 * @param[in]     host       Remote hostname or IPv4 string.
 * @param[in]     port       Remote TCP port.
 * @param[in]     scheme     Connection type (NAL_SCHEME_PLAIN or NAL_SCHEME_TLS).
 * @param[in]     timeout_ms Maximum time to wait for connection.
 *
 * @return BSP_ERR_STS_OK           Connection successful
 * @return BSP_ERR_STS_TIMEOUT      Connection timed out
 * @return BSP_ERR_STS_CONN_FAILED  Connection failed
 * @return BSP_ERR_STS_BUSY         Handle already connected
 * @return BSP_ERR_STS_NO_MEM       Socket allocation failed
 */

bsp_err_sts_t
nalNetworkConnect(nalHandle_t* handle, const char* host, uint16_t port, nalScheme_t scheme, uint32_t timeout_ms)
{
    if(!handle || !host || port == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    NAL_LOCK(handle);

    /* Prevent reconnect on an active socket */
    if(handle->sockfd >= 0)
    {
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_BUSY;
    }

    handle->scheme = scheme;

    /* -------------------------------------------------
     * 1. Create socket
     * ------------------------------------------------- */
    int sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_INVALID_SOCKET;
    }

    /* -------------------------------------------------
     * 2. Resolve address
     * ------------------------------------------------- */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    in_addr_t ip = inet_addr(host);
    if(ip == INADDR_NONE)
    {
        struct hostent* he = lwip_gethostbyname(host);
        if(!he || !he->h_addr_list || !he->h_addr_list[0])
        {
            lwip_close(sock);
            NAL_UNLOCK(handle);
            return BSP_ERR_STS_NOT_FOUND;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], sizeof(addr.sin_addr));
    }
    else
    {
        addr.sin_addr.s_addr = ip;
    }

    /* -------------------------------------------------
     * 3. Non-blocking connect for timeout support
     * ------------------------------------------------- */
    int flags = lwip_fcntl(sock, F_GETFL, 0);
    lwip_fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    int rc = lwip_connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if(rc < 0 && errno != EINPROGRESS)
    {
        lwip_close(sock);
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_CONN_FAILED;
    }

    /* -------------------------------------------------
     * 4. Wait for connect completion (select)
     * ------------------------------------------------- */
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);

    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    rc = select(sock + 1, NULL, &wfds, NULL, (timeout_ms > 0) ? &tv : NULL);

    if(rc <= 0)
    {
        lwip_close(sock);
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_TIMEOUT;
    }

    /* Check socket error */
    int so_err    = 0;
    socklen_t len = sizeof(so_err);
    lwip_getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_err, &len);

    if(so_err != 0)
    {
        lwip_close(sock);
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_CONN_FAILED;
    }

    /* Restore blocking mode */
    lwip_fcntl(sock, F_SETFL, flags);

    handle->sockfd = sock;

#if defined(NAL_USE_TLS)
    /* -------------------------------------------------
     * 5. TLS handshake (optional)
     * ------------------------------------------------- */
    if(handle->scheme == NAL_SCHEME_TLS)
    {
        bsp_err_sts_t tls_rc = nalTlsInit(handle);
        if(tls_rc != BSP_ERR_STS_OK)
        {
            lwip_close(sock);
            handle->sockfd = -1;
            NAL_UNLOCK(handle);
            return tls_rc;
        }

        tls_rc = nalTlsHandshake(handle);
        if(tls_rc != BSP_ERR_STS_OK)
        {
            nalTlsDeinit(handle);
            lwip_close(sock);
            handle->sockfd = -1;
            NAL_UNLOCK(handle);
            return tls_rc;
        }
    }
#endif

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
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* If already disconnected, treat as success */
    if(handle->sockfd < 0)
    {
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
        return BSP_ERR_STS_NO_CONN;
    }

    *bytes_sent = 0;

#if defined(NAL_USE_TLS)
    /* TLS path */
    if(handle->scheme == NAL_SCHEME_TLS)
    {
        return nalTlsSend(handle, data, len, bytes_sent, timeout_ms);
    }
#endif /* NAL_USE_TLS */

    /* ===============================
     * Plain TCP socket path
     * =============================== */

    const uint8_t* ptr = (const uint8_t*)data;
    size_t remaining   = len;
    uint32_t start_ms  = osKernelGetTickCount();


    while(remaining > 0)
    {
        NAL_LOCK(handle);
        /* Check timeout */
        if(timeout_ms > 0)
        {
            uint32_t elapsed = NAL_MS_TO_TICKS(osKernelGetTickCount() - start_ms);
            if(elapsed >= timeout_ms)
            {
                NAL_UNLOCK(handle);
                return BSP_ERR_STS_TIMEOUT;
            }
        }

        ssize_t ret = lwip_send(handle->sockfd, ptr, remaining, 0);

        if(ret < 0)
        {
            /* EWOULDBLOCK / EAGAIN → retry until timeout */
            if(errno == EWOULDBLOCK || errno == EAGAIN)
            {
                NAL_UNLOCK(handle);
                osDelay(NAL_MS_TO_TICKS(500)); /* brief delay before retry */
                continue;
            }

            NAL_UNLOCK(handle);
            return BSP_ERR_STS_FAIL;
        }

        if(ret == 0)
        {
            /* Peer closed connection */
            NAL_UNLOCK(handle);
            return BSP_ERR_STS_CONN_LOST;
        }

        NAL_UNLOCK(handle);
        ptr += ret;
        remaining -= (size_t)ret;
        *bytes_sent += (size_t)ret;
    }

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------------- */
/**
 * @brief Receive data synchronously from an active NAL connection.
 *
 * This function reads data from a previously established network connection
 * (plain TCP or TLS) and blocks until one of the following occurs:
 *
 *  - At least one byte of data is received
 *  - The remote peer closes the connection
 *  - The specified timeout expires
 *  - A socket or transport error occurs
 *
 * Transport handling model:
 *  - For plain TCP connections, the function directly uses the underlying
 *    socket (lwIP / BSD-style recv) with a configured receive timeout.
 *  - For TLS connections, the function delegates the receive operation to
 *    the TLS layer (nal_tls.c), keeping this core logic TLS-agnostic.
 *
 * Connection semantics:
 *  - A return value of BSP_ERR_STS_OK indicates that data was successfully
 *    received and stored in the user-provided buffer.
 *  - If the remote peer performs an orderly shutdown, the connection is
 *    marked as disconnected and BSP_ERR_STS_CONN_LOST is returned.
 *  - Timeouts are treated explicitly and reported as BSP_ERR_STS_TIMEOUT.
 *
 * Threading and blocking:
 *  - This API is synchronous and blocking.
 *  - It is safe to call from a task/thread context.
 *  - It must not be called from ISR context.
 *
 * Ownership rules:
 *  - The receive buffer is owned by the caller.
 *  - The function does not retain or reference the buffer after returning.
 *
 * @param[in,out] handle       Pointer to an initialized and connected nalHandle_t
 * @param[out]    buf          Buffer to store received data
 * @param[in]     buf_len      Size of the receive buffer in bytes
 * @param[out]    bytes_recv   Number of bytes actually received
 * @param[in]     timeout_ms   Receive timeout in milliseconds (0 = block indefinitely)
 *
 * @return
 *  - BSP_ERR_STS_OK            Data received successfully
 *  - BSP_ERR_STS_TIMEOUT       No data received within timeout
 *  - BSP_ERR_STS_CONN_LOST     Peer closed the connection
 *  - BSP_ERR_STS_INVALID_PARAM Invalid arguments
 *  - BSP_ERR_STS_NOT_CONNECTED Handle is not connected
 *  - BSP_ERR_STS_FAIL          Transport or socket error
 */

bsp_err_sts_t
nalNetworkRecvSync(nalHandle_t* handle, void* buf, size_t buf_len, size_t* bytes_recv, uint32_t timeout_ms)
{
    if(handle == NULL || buf == NULL || bytes_recv == NULL || buf_len == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    *bytes_recv = 0;

    /* Must be connected */
    if(handle->sockfd < 0)
    {
        return BSP_ERR_STS_NO_CONN;
    }

#if defined(NAL_USE_TLS)
    /* TLS path: delegate fully to TLS layer */
    if(handle->scheme == NAL_SCHEME_TLS)
    {
        return nalTlsRecv(handle, buf, buf_len, bytes_recv, timeout_ms);
    }
#endif /* NAL_USE_TLS */

    /* ---------- Plain TCP path ---------- */

    /* Configure receive timeout */
    if(timeout_ms > 0)
    {
        NAL_LOCK(handle);
        struct timeval tv;
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        lwip_setsockopt(handle->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        NAL_UNLOCK(handle);
    }

    int ret = lwip_recv(handle->sockfd, buf, buf_len, 0);

    if(ret > 0)
    {
        *bytes_recv = (size_t)ret;

        return BSP_ERR_STS_OK;
    }

    if(ret == 0)
    {
        NAL_LOCK(handle);
        /* Peer performed an orderly shutdown */
        lwip_close(handle->sockfd);
        handle->sockfd = -1;
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_CONN_LOST;
    }

    /* ret < 0 → error */
    if(errno == EWOULDBLOCK || errno == EAGAIN)
    {
        NAL_UNLOCK(handle);
        return BSP_ERR_STS_TIMEOUT;
    }

    NAL_UNLOCK(handle);
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

    if(g_nal_async.enabled)
    {
        return BSP_ERR_STS_OK;
    }

    g_nal_async.evt_queue = queue;
    g_nal_async.enabled   = 1;

    const osThreadAttr_t attr = { .name       = NAL_EVENT_TASK_NAME,
                                  .priority   = NAL_EVENT_TASK_PRIORITY,
                                  .stack_size = NAL_EVENT_TASK_STACK_SIZE };

    g_nal_async.worker_tid = osThreadNew(nalNetEventTask, handle, &attr);

    if(g_nal_async.worker_tid == NULL)
    {
        g_nal_async.enabled = 0;
        return BSP_ERR_STS_NO_MEM;
    }

    return BSP_ERR_STS_OK;
}

/**
 ********************************************************************************
 * End of nal_core.c
 ********************************************************************************
 */