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

    /* Forward declare struct so pointers can be used before full definition */
    typedef struct nalHandle nalHandle_t;

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
    struct nalHandle
    {
        nalScheme_t scheme; /**< Configured transport scheme (TCP or TLS) */
        int sockfd;    /**< Underlying socket file descriptor (-1 if unused) */

#if defined(NAL_USE_TLS)
        void* tls_ctx; /* opaque TLS context */
        char server_name[128];

        /* CA certificate reference (no ownership) */
        const uint8_t* ca_cert;
        size_t ca_cert_len;
#endif                    // #if defined(NAL_USE_TLS)

        osMutexId_t lock; /**< CMSIS-RTOS mutex for thread-safe operations */

    }; // nalHandle_t


    /* -------------------------------------------------------------------------
     *  Public API (High-Level Contract)
     * -------------------------------------------------------------------------
     *  - All functions return bsp_err_sts_t for error reporting (see bsp_err_sts.h).
     *  - Synchronous APIs block until completion or timeout.
     *  - Event callbacks are global and optional (registered per event type).
     * ------------------------------------------------------------------------- */

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
     *  Connection Management (Synchronous)
     * ======================================================================== */

    /**
     * @brief Establish a TCP or TLS connection to a remote host.
     *
     * Creates a socket, optionally performs a TLS handshake, and blocks
     * until the connection succeeds or the timeout expires.
     *
     * Connection state change events (CONNECTED / ERROR) are reported
     * via registered event callbacks, if any.
     *
     * @param[in,out] handle     Pointer to an initialized nalHandle_t
     * @param[in]     host       Remote hostname or IP address (null-terminated)
     * @param[in]     port       Remote TCP port
     * @param[in]     scheme     Transport scheme (NAL_SCHEME_PLAIN or NAL_SCHEME_TLS)
     * @param[in]     timeout_ms Connection timeout in milliseconds
     *
     * @return
     *  - BSP_ERR_STS_OK              Connection established
     *  - BSP_ERR_STS_TIMEOUT         Connection attempt timed out
     *  - BSP_ERR_STS_INVALID_PARAM   Invalid arguments
     *  - BSP_ERR_STS_FAIL            Socket or TLS handshake failure
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
     * @brief Receive data synchronously from the active connection.
     *
     * Blocks until data is received, the peer closes the connection,
     * or the timeout expires.
     *
     * @param[in,out] handle       Pointer to active nalHandle_t
     * @param[out]    buf          Receive buffer
     * @param[in]     buf_len      Size of receive buffer
     * @param[out]    bytes_recv   Number of bytes actually received
     * @param[in]     timeout_ms   Receive timeout in milliseconds
     *
     * @return
     *  - BSP_ERR_STS_OK              Data received successfully
     *  - BSP_ERR_STS_TIMEOUT         Receive timed out
     *  - BSP_ERR_STS_INVALID_PARAM   Invalid arguments
     *  - BSP_ERR_STS_FAIL            Receive error or connection closed
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
     *  Asynchronous
     * ======================================================================== */

    /**
     * @brief Queue an asynchronous send operation.
     *
     * Copies user data into an internal buffer and schedules it for sending by the async worker thread.
     * Only one outstanding send operation is allowed per connection in this implementation.
     *
     * @param[in] handle Pointer to an active @ref nalHandle_t structure.
     * @param[in] data   Pointer to the data buffer to send.
     * @param[in] len    Length of the data in bytes.
     *
     * @return
     *  - BSP_ERR_STS_OK              : Send queued successfully.
     *  - BSP_ERR_STS_INVALID_PARAM   : Null handle or invalid length.
     *  - BSP_ERR_STS_NOT_INITIALIZED : Async worker not running.
     *  - BSP_ERR_STS_BUSY   : Previous send still pending.
     *  - BSP_ERR_STS_INTERNAL_ERROR  : Memory allocation failure.
     */
    bsp_err_sts_t nalNetworkSendAsync(nalHandle_t* handle, const void* data, size_t len);

    /**
     * @brief Register a receive buffer for asynchronous data notifications.
     *
     * If the user provides a buffer, incoming data is copied into it before invoking
     * the callback with @ref NAL_EVENT_DATA_RECEIVED. If a NULL buffer is given, an
     * internal buffer is used, and its pointer is provided directly in the callback data.
     *
     * @param[in] handle     Pointer to the active @ref nalHandle_t structure.
     * @param[in] recv_buf   Pointer to user-provided receive buffer (optional, can be NULL).
     * @param[in] recv_len   Size of the provided buffer in bytes.
     *
     * @return
     *  - BSP_ERR_STS_OK              : Receive buffer registered successfully.
     *  - BSP_ERR_STS_INVALID_PARAM   : Null handle.
     *  - BSP_ERR_STS_NOT_INITIALIZED : Async worker not active for this handle.
     */
    bsp_err_sts_t nalNetworkRecvAsync(nalHandle_t* handle, void* recv_buf, size_t recv_len);

    /* ========================================================================
     *  TLS Certificate Management
     * ======================================================================== */
    /**
     * @brief Load and store a CA certificate for TLS server verification.
     *
     * The function copies the provided PEM certificate buffer internally so the caller
     * may free or reuse the input buffer afterward. It is required for authenticating
     * servers during TLS connections if certificate verification is enabled.
     *
     * @param[in] handle  Pointer to the initialized @ref nalHandle_t structure.
     * @param[in] ca_pem  Pointer to the CA certificate in PEM format.
     * @param[in] len     Length of the PEM data (in bytes).
     *
     * @return
     *  - BSP_ERR_STS_OK              : Certificate successfully stored.
     *  - BSP_ERR_STS_INVALID_PARAM   : Null handle or invalid length.
     *  - BSP_ERR_STS_INTERNAL_ERROR  : Memory allocation failure.
     */
    bsp_err_sts_t nalSetCaCert(nalHandle_t* handle, const uint8_t* ca_pem, size_t len);

    /**
     * @brief Retrieve the currently stored CA certificate for TLS verification.
     *
     * Returns a pointer to the internally stored CA certificate (if previously set via
     * @ref nalSetCaCert). The data remains valid as long as the handle is initialized.
     *
     * @param[in]  handle      Pointer to the initialized @ref nalHandle_t structure.
     * @param[out] out_ca_pem  Pointer to receive the internal CA PEM data pointer.
     * @param[out] out_len     Pointer to receive the certificate length (in bytes).
     *
     * @return
     *  - BSP_ERR_STS_OK              : Certificate returned successfully.
     *  - BSP_ERR_STS_INVALID_PARAM   : Null argument(s) passed.
     *  - BSP_ERR_STS_NOT_INITIALIZED : No certificate has been set.
     */
    bsp_err_sts_t nalGetCaCert(nalHandle_t* handle, const uint8_t** out_ca_pem, size_t* out_len);

    /* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // NAL_NETWORK_H