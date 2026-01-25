/**
 * @file nal_network.h
 * @brief Network Abstraction Layer (NAL) public API
 *
 * This header exposes the public, portable API for the Network Abstraction Layer
 * used across the project. NAL provides a small, consistent interface for
 * synchronous and asynchronous network I/O over plain TCP or TLS-over-TCP.
 *
 * Purpose and scope
 *  - Decouple application logic from platform socket/TLS implementations (lwIP,
 *    BSD sockets, mbedTLS, etc.).
 *  - Provide both blocking (synchronous) and callback-driven (asynchronous)
 *    operation modes so code can choose the right concurrency model.
 *  - Keep the API minimal and testable on host (POSIX) and embedded targets.
 *
 * Threading model
 *  - The async APIs may spawn a worker thread for each connection. Callbacks
 *    are invoked from that worker thread context. Callbacks should therefore
 *    be short and non-blocking; if longer processing is needed dispatch to
 *    another thread or task.
 *
 * TLS and transport
 *  - TLS is optional. If a TLS backend (for example mbedTLS) is available
 *    and enabled at build time, NAL can perform TLS handshakes and encrypted I/O.
 *  - Use the transport selection parameter to pick plain TCP or TLS per
 *    connection.
 */

#ifndef NAL_NETWORK_H
#define NAL_NETWORK_H


#ifdef __cplusplus
extern "C"
{
#endif

#include "sys_config.h"
#include "nal_config.h"


#include "bsp_err_sts.h"

#include "cmsis_os2.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


    /* ==========================================
     *           ENUMS & STRUCTURES
     * ========================================== */

    /**
     * @brief Transport scheme selection
     */
    typedef enum
    {
        NAL_SCHEME_PLAIN = 0, /**< Plain TCP (stream) */
        NAL_SCHEME_TLS = 1, /**< TLS over TCP (requires TLS backend at build-time) */
    } nalScheme_t;

    /* -------------------------
     * Async event types & callback
     * ------------------------- */
    /**
     * @brief Asynchronous connection events
     *
     * Callbacks receive these events from the async worker.
     */
    typedef enum
    {
        NAL_EVENT_CONNECTED = 1,  /**< Connection established */
        NAL_EVENT_CONNECT_FAILED, /**< Connection attempt failed */
        NAL_EVENT_DISCONNECTED,   /**< Remote or local disconnected */
        NAL_EVENT_DATA_RECEIVED, /**< Data has been received (callback provides pointer+len) */
        NAL_EVENT_DATA_SENT, /**< Previously queued data was sent (len passed in callback) */
        NAL_EVENT_TIMEOUT, /**< Operation timed out */
        NAL_EVENT_ERROR    /**< Generic error occurred; check logs or state */
    } nalEvent_t;


    typedef enum
    {
        NAL_IOCTL_SET_CALLBACK,
        NAL_IOCTL_GET_SOCKET_FD,
        NAL_IOCTL_SET_TIMEO_SEND,
        NAL_IOCTL_SET_TIMEO_RECV,
        NAL_IOCTL_SET_TIMEO_CONNECT,
        NAL_IOCTL_IS_CONNECTED,
    } nalIoctlReq_t;

    typedef enum
    {
        NAL_ROLE_NONE = 0,
        NAL_ROLE_CLIENT,
        NAL_ROLE_SERVER
    } nalRole_t;

    typedef struct
    {
        const uint8_t* ca_cert;
        size_t ca_cert_len;
    } nalTlsClientCreds_t;


    typedef struct
    {
        const uint8_t* cert_pem; /**< Server certificate (PEM) */
        size_t cert_len;

        const uint8_t* key_pem;  /**< Server private key (PEM) */
        size_t key_len;

        /* Optional certificate chain */
        const uint8_t* chain_pem;
        size_t chain_len;

    } nalTlsServerCreds_t;

    /**
     * @brief Opaque NAL handle structure.
     *
     * The nalHandle_t structure encapsulates all state needed for a single
     * network connection. Callers should treat it as an opaque container
     * allocated on the stack or statically.
     *
     * @note This structure should be initialized via nal_init() before use.
     * @warning Do not access members directly; use NAL API functions instead.
     */
    typedef struct
    {
        nalScheme_t scheme; /**< Configured transport scheme (TCP or TLS) */
        nalRole_t role;
        int sockfd;    /**< Underlying socket file descriptor (-1 if unused) */
        int keepAlive;
        int keepIdle;
        int keepInterval;
        int keepCount;
        int broadcast;

#if defined(NAL_USE_TLS)
        void* tls_ctx; /* opaque TLS context */
        char server_name[128];

        /* ---- TLS configuration (core-owned) ---- */
        union
        {
            const nalTlsClientCreds_t* client;
            const nalTlsServerCreds_t* server;
        } tls_creds;

        /* CA certificate reference (no ownership) */
        const uint8_t* ca_cert;
        size_t ca_cert_len;
#endif                    // #if defined(NAL_USE_TLS)

        osMutexId_t lock; /**< CMSIS-RTOS mutex for thread-safe operations */
        int8_t id;        /**< Unique session ID (for internal use) */
    } nalHandle_t;


    /**
     * @brief Callback function prototype for receiving asynchronous NAL events.
     *
     * This function type is used by all asynchronous NAL operations (connect, send,
     * receive, disconnect). It is invoked by the async worker thread when an event
     * occurs. Each callback provides contextual information such as the originating
     * handle, event type, user context, and optional data buffer.
     *
     * Example events:
     *  - NAL_EVENT_CONNECTED:  Connection successfully established.
     *  - NAL_EVENT_DATA_RECEIVED: Data received from the remote peer.
     *  - NAL_EVENT_DISCONNECTED: Connection closed or lost.
     *  - NAL_EVENT_ERROR: Generic error during operation.
     *
     * @param[in,out] handle   Pointer to the nalHandle_t that triggered the event.
     *                         Used for referencing connection-specific state.
     * @param[in]     event    Event type identifier (see nalEvent_t enum).
     * @param[in]     user_ctx User context pointer originally passed to the async
     *                         connect function. Can be used to map events to app objects.
     * @param[in]     data     Optional pointer to event-specific data:
     *                         - For NAL_EVENT_DATA_RECEIVED: pointer to received bytes.
     *                         - For other events: may be NULL.
     * @param[in]     length   Size (in bytes) of the data buffer, if applicable.
     *                         Zero for events without payload data.
     * @return  None.
     */
    typedef void (*nalEventCallback_t)(nalHandle_t* handle,
                                       nalEvent_t event,
                                       void* user_ctx,
                                       void* data,
                                       size_t length);


    /* ========================================================================
     *  NAL Subsystem Lifecycle
     * ======================================================================== */

    /**
     * @brief Initialize the Network Abstraction Layer (NAL).
     *
     * Performs one-time global initialization of networking resources
     * (sockets, TLS backend, mutexes). Must be called before any other
     * NAL API.
     *
     * @param[out] handle Optional pointer to a nalHandle_t to initialize.
     *                    Can be NULL if only global init is required.
     *
     * @return
     *  - BSP_ERR_STS_OK              Initialization successful
     *  - BSP_ERR_STS_INTERNAL_ERROR  Platform or TLS initialization failed
     */
    bsp_err_sts_t nalNetworkInit(nalHandle_t* handle);

    /**
     * @brief Deinitialize the Network Abstraction Layer (NAL).
     *
     * Releases global resources allocated during nalNetworkInit().
     * Safe to call multiple times (idempotent).
     *
     * @param[in] handle Optional pointer to nalHandle_t to clean up.
     *                   Can be NULL for global-only cleanup.
     *
     * @return
     *  - BSP_ERR_STS_OK              Deinitialization successful
     *  - BSP_ERR_STS_INTERNAL_ERROR  Cleanup failed
     */
    bsp_err_sts_t nalNetworkDeinit(nalHandle_t* handle);

    /* ========================================================================
     *   CLIENT CONNECTION API
     * ======================================================================== */


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

    bsp_err_sts_t nalNetworkConnect(nalHandle_t* handle,
                                    const char* host,
                                    uint16_t port,
                                    nalScheme_t scheme,
                                    uint32_t timeout_ms);

    /**
     * @brief Disconnect an active network connection.
     *
     * Performs a graceful TLS shutdown (if applicable) and closes
     * the underlying socket. Safe to call even if already disconnected.
     *
     * @param[in,out] handle Pointer to nalHandle_t
     *
     * @return
     *  - BSP_ERR_STS_OK              Disconnected successfully
     *  - BSP_ERR_STS_INTERNAL_ERROR  Socket close or cleanup failed
     */
    bsp_err_sts_t nalNetworkDisconnect(nalHandle_t* handle);

    /* ========================================================================
     *  Synchronous Data I/O
     * ======================================================================== */

    /**
     * @brief Send data synchronously over the active connection.
     *
     * Blocks until all data is sent, a timeout occurs, or an error happens.
     *
     * @param[in,out] handle       Pointer to active nalHandle_t
     * @param[in]     data         Data buffer to send
     * @param[in]     len          Number of bytes to send
     * @param[out]    bytes_sent   Actual number of bytes sent
     * @param[in]     timeout_ms   Send timeout in milliseconds
     *
     * @return
     *  - BSP_ERR_STS_OK              Data sent successfully
     *  - BSP_ERR_STS_TIMEOUT         Send timed out
     *  - BSP_ERR_STS_INVALID_PARAM   Invalid arguments
     *  - BSP_ERR_STS_FAIL            Send failure
     */
    bsp_err_sts_t nalNetworkSendSync(nalHandle_t* handle,
                                     const void* data,
                                     size_t len,
                                     size_t* bytes_sent,
                                     uint32_t timeout_ms);

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

    bsp_err_sts_t nalNetworkRecvSync(nalHandle_t* handle,
                                     void* buf,
                                     size_t buf_len,
                                     size_t* bytes_recv,
                                     uint32_t timeout_ms);

    /* ========================================================================
     *  Control / IOCTL
     * ======================================================================== */

    /**
     * @brief Perform control operations on a NAL handle.
     *
     * Used for advanced or platform-specific controls such as:
     *  - Set socket options
     *  - Query connection state
     *  - Configure timeouts
     *
     * @param[in,out] handle Pointer to nalHandle_t
     * @param[in]     req    IOCTL request identifier
     * @param[in,out] arg    Request-specific argument
     *
     * @return
     *  - BSP_ERR_STS_OK              Operation successful
     *  - BSP_ERR_STS_INVALID_PARAM   Invalid request or arguments
     *  - BSP_ERR_STS_FUNC_NOT_SUPPORTED Unsupported request
     */
    bsp_err_sts_t nalNetworkIoctl(nalHandle_t* handle, nalIoctlReq_t req, void* arg);

    /* ========================================================================
     *  Event Callback Registration
     * ======================================================================== */

    /**
     * @brief Enable asynchronous NAL events using a user-provided queue.
     *
     * NAL will post network events (RX, disconnect, error, etc.)
     * into this queue. The application is responsible for:
     *  - Creating the queue
     *  - Creating a task that waits on the queue
     *
     * @param[in] evt_queue  QueueHandle_t created by the application
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_INVALID_PARAM
     */
    bsp_err_sts_t nalEnableAsyncEvents(nalHandle_t* handle, osMessageQueueId_t queue);

    /* ========================================================================
     *  Server-side APIs
     * ======================================================================== */

    /**
     * @brief Start a TCP/TLS server.
     *
     * Creates a listening socket bound to the given port.
     * For TLS, initializes server TLS context but does NOT perform handshake.
     *
     * @param[in,out] handle   NAL handle (role must be SERVER)
     * @param[in]     port     Local port to listen on
     * @param[in]     scheme   NAL_SCHEME_PLAIN or NAL_SCHEME_TLS
     * @param[in]     backlog  Max pending connections
     *
     * @return BSP_ERR_STS_OK on success
     */
    bsp_err_sts_t
    nalNetworkStartServer(nalHandle_t* handle, uint16_t port, nalScheme_t scheme, uint32_t backlog);

    /**
     * @brief Accept an incoming client connection.
     *
     * This call blocks until:
     *  - a client connects
     *  - timeout expires
     *
     * On success, a NEW nalHandle_t is populated for the client.
     *
     * @param[in]  server     Listening server handle
     * @param[out] client     Client connection handle
     * @param[in]  timeout_ms Accept timeout (0 = block forever)
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_TIMEOUT
     *  - BSP_ERR_STS_FAIL
     */
    bsp_err_sts_t nalNetworkAccept(nalHandle_t* server, nalHandle_t* client, uint32_t timeout_ms);

    /**
     * @brief Stop server and close listening socket.
     */
    bsp_err_sts_t nalNetworkStopServer(nalHandle_t* handle);
    /* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // NAL_NETWORK_H