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

#include <stdint.h>
#include <stddef.h>
#include "err_status.h"

#include "sys_config.h"

/* Optionally include CMSIS for mutex type */
#ifdef SYS_USE_CMSIS
    #include "cmsis_os2.h"
#endif


    /* Platform headers (lwIP, CMSIS, mbedTLS) are optional at compile-time */
    /*-------------------------------------------------------------
     *  Socket (lwIP / POSIX)
     *------------------------------------------------------------*/

#if defined(ESP_PLATFORM) && defined(ESP_PLATFORM_LWIP)

    /* ESP-IDF provides lwIP */
    #include "lwip/sockets.h"
    #include "lwip/netdb.h"
    #include "lwip/errno.h"

#elif defined(SYS_USE_LWIP)

    /* User-provided lwIP */
    #include "lwip/sockets.h"
    #include "lwip/netdb.h"

#else
    /* Generic POSIX sockets */
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

    /* Optionally include mbedTLS types when TLS is enabled */

    /*-------------------------------------------------------------
     *  TLS / mbedTLS support
     *------------------------------------------------------------*/

#if defined(ESP_PLATFORM) && defined(ESP_PLATFORM_MBEDTLS)

    /* Use ESP-IDF’s mbedTLS */
    #include "mbedtls/platform.h"
    #include "mbedtls/net_sockets.h"
    #include "mbedtls/ssl.h"
    #include "mbedtls/ssl_ciphersuites.h"
    #include "mbedtls/entropy.h"
    #include "mbedtls/ctr_drbg.h"
    #include "mbedtls/x509_crt.h"

#elif defined(SYS_USE_MBEDTLS)

    /* Use user-provided mbedTLS (for STM32/Linux builds) */
    #include "mbedtls/platform.h"
    #include "mbedtls/net_sockets.h"
    #include "mbedtls/ssl.h"
    #include "mbedtls/ssl_ciphersuites.h"
    #include "mbedtls/entropy.h"
    #include "mbedtls/ctr_drbg.h"
    #include "mbedtls/x509_crt.h"

#else
/* No TLS → Provide stub types if needed */
typedef void* tls_context_t;
#endif

    /* -------------------------
     * Internal async context
     * ------------------------- */

#define NAL_ASYNC_MAX_INSTANCES (nalCONFIG_NAL_ASYNC_MAX_INSTANCES)
#define NAL_ASYNC_THREAD_STACK  (nalCONFIG_NAL_ASYNC_THREAD_STACK)
#define NAL_ASYNC_THREAD_PRIO   (nalCONFIG_NAL_ASYNC_THREAD_PRIO)

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
        NAL_EVENT_ERROR /**< Generic error occurred; check logs or state */
    } nalEvent_t;

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
     * @brief Asynchronous context for a single nal network handle.
     *
     * Internal per-instance state used by the nal implementation. Fields are
     * intentionally exposed so the C file can store per-handle async state
     * without a separate private header.
     */
    typedef struct
    {
        int used;              /**< 0 = free, 1 = in-use */
        int sockfd;            /**< socket fd for this async instance */
        nalHandle_t* handle;   /**< back-reference to parent handle */
        nalEventCallback_t cb; /**< user callback */
        void* user_ctx;        /**< user context passed to callback */
        void* recv_buf;        /**< optional receive buffer provided by user */
        size_t recv_buf_len;   /**< length of recv_buf */
        void* send_buf;        /**< pointer to pending send buffer (malloc'd) */
        size_t send_len;       /**< length of pending send buffer */
        int want_send;         /**< flag set when send_buf is ready */
#ifdef SYS_USE_CMSIS
        osMutexId_t lock;      /**< mutex protecting per-ctx state */
        osThreadId_t thread_id;   /**< worker thread id */
        osSemaphoreId_t exit_sem; /**< signalled by worker on exit */
#else
    void* lock;      /**< placeholder when CMSIS not present */
    void* thread_id; /**< placeholder */
    void* exit_sem;  /**< placeholder */
#endif
        volatile int running;     /**< 0 = stopped, 1 = running */
    } nalAsyncCtx_t;


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
        int sockfd; /**< Underlying socket file descriptor (-1 if unused) */

        uint32_t recv_timeout_ms; /**< Default receive timeout in milliseconds (0 = blocking) */
        uint32_t send_timeout_ms; /**< Default send timeout in milliseconds (0 = blocking) */

#if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)
        char server_name[128];
        mbedtls_ssl_context ssl; /**< mbedTLS SSL context for encrypted communication */
        mbedtls_ssl_config conf;           /**< mbedTLS SSL configuration */
        mbedtls_ctr_drbg_context ctr_drbg; /**< mbedTLS random number generator context */
        mbedtls_entropy_context entropy; /**< mbedTLS entropy source for RNG seeding */
        mbedtls_x509_crt cacert; /**< mbedTLS CA certificate chain for verification */
        mbedtls_net_context net_ctx; /**< mbedTLS network context wrapping sockfd */
        uint8_t tls_initialized; /**< TLS context initialization flag (0 or 1) */
        uint8_t* ca_cert_buf; /**< Raw CA certificate buffer */
        size_t ca_cert_len;   /**< Length of CA certificate buffer */
#else
    void* tls_placeholder; /**< Placeholder to maintain ABI compatibility */
#endif // #if defined(SYS_USE_MBEDTLS) || defined(ESP_PLATFORM_MBEDTLS)

#ifdef SYS_USE_CMSIS
        osMutexId_t lock; /**< CMSIS-RTOS mutex for thread-safe operations */
#else
    void* lock; /**< Placeholder when CMSIS-RTOS is not available */
#endif

        nalAsyncCtx_t async_ctx; /**< Asynchronous operation context and state */
    }; // nalHandle_t


    /* -------------------------------------------------------------------------
     *  Public API (High-Level Contract)
     * -------------------------------------------------------------------------
     *  - All functions return errStatus_t for error reporting (see err_status.h).
     *  - Synchronous functions block until completion or until a timeout occurs.
     *  - Asynchronous functions return immediately and use callbacks for results.
     * ------------------------------------------------------------------------- */

    /**
     * @brief One-time NAL subsystem initialization.
     *
     * Initializes global networking and TLS context resources. Must be called once
     * before any other NAL API. Can optionally perform per-handle initialization.
     *
     * @param[out] handle   Optional pointer to nalHandle_t that will be initialized
     *                      for future operations. Can be NULL if not needed.
     *
     * @return
     *  - ERR_STS_OK on success
     *  - ERR_STS_INTERNAL_ERROR on fatal platform/TLS init failure
     */
    errStatus_t nalNetworkInit(nalHandle_t* handle);

    /**
     * @brief One-time NAL subsystem deinitialization.
     *
     * Frees global resources allocated by nalNetworkInit(). Safe to call
     * multiple times (idempotent). The caller must ensure that no active
     * handles are still using the NAL APIs before deinitializing.
     *
     * @param[in] handle   pointer to nalHandle_t previously
     * initialized. Can be NULL if global-only cleanup is needed.
     *
     * @return
     *  - ERR_STS_OK on success
     *  - ERR_STS_INTERNAL_ERROR if internal cleanup fails
     */
    errStatus_t nalNetworkDeinit(nalHandle_t* handle);

    /**
     * @brief Initiate an asynchronous connection to a remote host using TCP or TLS.
     *
     * Starts a non-blocking connection operation handled by the internal async worker.
     * The provided callback (`nalEventCallback_t`) is used to notify the caller about
     * connection results and subsequent I/O events (e.g., connected, data received, etc.).
     *
     * Once called successfully, this function returns immediately, while connection
     * progress and completion are reported via callback events such as:
     *  - NAL_EVENT_CONNECTING
     *  - NAL_EVENT_CONNECTED
     *  - NAL_EVENT_CONNECT_FAILED
     *
     * @param[in,out] handle    Pointer to a valid nalHandle_t instance.
     *                          Must be initialized via nalNetworkInit() or equivalent.
     * @param[in]     host      Target server hostname or IP address (null-terminated string).
     * @param[in]     port      Remote port number to connect to.
     * @param[in]     scheme    Connection scheme:
     *                          - NAL_SCHEME_TCP for plain TCP connection
     *                          - NAL_SCHEME_TLS for encrypted TLS connection
     * @param[in]     cb        User-provided callback function that receives asynchronous
     *                          connection and I/O events (see nalEventCallback_t).
     *                          Must remain valid for the lifetime of the async operation.
     * @param[in]     user_ctx  Optional user context pointer that will be passed to the
     *                          callback in every event. Can be NULL.
     * @return
     *  - ERR_STS_OK on successful connection initiation
     *  - ERR_STS_INVALID_PARAM if parameters are invalid (e.g., NULL host or callback)
     *  - ERR_STS_BUSY if a previous async operation is still active
     *  - ERR_STS_INTERNAL_ERROR if resource or socket creation fails
     */
    errStatus_t
    nalNetworkConnect(nalHandle_t* handle, const char* host, uint16_t port, nalScheme_t scheme);

    /**
     * @brief Synchronously disconnect and clean protocol state for a handle.
     *
     * Performs a TLS shutdown if applicable and closes the underlying socket.
     * Safe to call multiple times; redundant calls have no effect.
     *
     * @param[in,out] handle   Pointer to the nalHandle_t representing the connection.
     *
     * @return
     *  - ERR_STS_OK on success
     *  - ERR_STS_INTERNAL_ERROR if socket closure or cleanup fails
     */
    errStatus_t nalNetworkDisconnect(nalHandle_t* handle);

    /**
     * @brief Synchronous send.
     *
     * Sends data over the connected transport (TCP/TLS). Blocks until the entire
     * buffer is sent, a timeout occurs, or an error is detected.
     *
     * @param[in,out] handle     Pointer to nalHandle_t of the active connection.
     * @param[in]     buf        Pointer to buffer containing data to send.
     * @param[in]     len        Length of data in bytes.
     * @param[in]     timeout_ms Timeout in milliseconds.
     *
     * @return
     *  - Number of bytes sent (>=0) on success
     *  - Negative error code on failure
     */
    int32_t nalNetworkSend(nalHandle_t* handle, const void* buf, size_t len, uint32_t timeout_ms);

    /**
     * @brief Synchronous receive.
     *
     * Waits for data from the remote peer. Returns once data is received, the
     * connection is closed, or a timeout/error occurs.
     *
     * @param[in,out] handle     Pointer to nalHandle_t of the active connection.
     * @param[out]    buf        Pointer to buffer where received data will be stored.
     * @param[in]     len        Maximum number of bytes to read.
     * @param[in]     timeout_ms Timeout in milliseconds.
     *
     * @return
     *  - Number of bytes received (>=0) on success
     *  - 0 on orderly connection close
     *  - Negative error code on failure
     */
    int32_t nalNetworkRecv(nalHandle_t* handle, void* buf, size_t len, uint32_t timeout_ms);

    /* ------------------------------- Async API ------------------------------- */

    /**
     * @brief Start asynchronous connection and worker thread.
     *
     * Spawns a background worker that handles connect, read, and write events.
     * Events are delivered to the provided callback function.
     *
     * @param[in,out] handle    Pointer to nalHandle_t that will manage async I/O.
     * @param[in]     cb        Callback invoked on connection and I/O events.
     * @param[in]     user_ctx  User context pointer passed to callback for each event.
     *
     * @return
     *  - ERR_STS_OK on success
     *  - ERR_STS_THREAD_FAIL if worker thread creation fails
     *  - ERR_STS_INTERNAL_ERROR on internal resource failure
     */
    errStatus_t nalNetworkStartAsync(nalHandle_t* handle, nalEventCallback_t cb, void* user_ctx);


    /**
     * @brief Stop asynchronous operations and safely terminate the async worker thread.
     *
     * This function signals the running async worker (created by @ref nalNetworkStartAsync or
     * @ref nalNetworkConnectAsync) to stop and waits for it to exit gracefully.
     * It ensures all resources (thread, semaphores, mutexes) related to the async context
     * are released cleanly.
     *
     * @param[in] handle Pointer to the initialized @ref nalHandle_t structure representing the connection.
     *
     * @return
     *  - ERR_STS_OK                : Successfully stopped async worker and released resources.
     *  - ERR_STS_INVALID_PARAM     : Null handle passed.
     *  - ERR_STS_NOT_INITIALIZED   : Async worker not running for this handle.
     */
    errStatus_t nalNetworkStopAsync(nalHandle_t* handle);

    /**
     * @brief Establish an asynchronous TCP or TLS-over-TCP connection and start event monitoring.
     *
     * This function spawns a worker thread that performs connection establishment and
     * continuously monitors socket events. All network events (connect, receive, send, disconnect, etc.)
     * are reported via the user-provided callback function.
     *
     * @param[in]  handle     Pointer to an initialized @ref nalHandle_t structure.
     * @param[in]  host       Remote hostname or IP address string.
     * @param[in]  port       Remote TCP port number.
     * @param[in]  scheme     Connection scheme (e.g., NAL_SCHEME_TCP or NAL_SCHEME_TLS).
     * @param[in]  cb         User callback function to handle network events.
     * @param[in]  user_ctx   User context pointer passed unchanged to the callback.
     *
     * @return
     *  - ERR_STS_OK              : Async connection started successfully.
     *  - ERR_STS_INVALID_PARAM   : Null or invalid arguments.
     *  - ERR_STS_RESOURCE_BUSY   : No available async context or thread creation failed.
     *  - ERR_STS_FAIL            : Internal failure during setup.
     */
    errStatus_t nalNetworkConnectAsync(nalHandle_t* handle,
                                       const char* host,
                                       uint16_t port,
                                       nalScheme_t scheme,
                                       nalEventCallback_t cb,
                                       void* user_ctx);

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
     *  - ERR_STS_OK              : Send queued successfully.
     *  - ERR_STS_INVALID_PARAM   : Null handle or invalid length.
     *  - ERR_STS_NOT_INITIALIZED : Async worker not running.
     *  - ERR_STS_RESOURCE_BUSY   : Previous send still pending.
     *  - ERR_STS_INTERNAL_ERROR  : Memory allocation failure.
     */
    errStatus_t nalNetworkSendAsync(nalHandle_t* handle, const void* data, size_t len);

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
     *  - ERR_STS_OK              : Receive buffer registered successfully.
     *  - ERR_STS_INVALID_PARAM   : Null handle.
     *  - ERR_STS_NOT_INITIALIZED : Async worker not active for this handle.
     */
    errStatus_t nalNetworkRecvAsync(nalHandle_t* handle, void* recv_buf, size_t recv_len);

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
     *  - ERR_STS_OK              : Certificate successfully stored.
     *  - ERR_STS_INVALID_PARAM   : Null handle or invalid length.
     *  - ERR_STS_INTERNAL_ERROR  : Memory allocation failure.
     */
    errStatus_t nalSetCaCert(nalHandle_t* handle, const uint8_t* ca_pem, size_t len);

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
     *  - ERR_STS_OK              : Certificate returned successfully.
     *  - ERR_STS_INVALID_PARAM   : Null argument(s) passed.
     *  - ERR_STS_NOT_INITIALIZED : No certificate has been set.
     */
    errStatus_t nalGetCaCert(nalHandle_t* handle, const uint8_t** out_ca_pem, size_t* out_len);

    /* ------------------------------------------------------------------------- */
    /**
     * @section nal_notes Notes and limitations
     *
     * - Async callbacks run in worker thread context. Keep them short.
     * - Default async implementation may support only one outstanding queued
     *   send per connection. Consider using a FIFO for higher throughput.
     * - Non-blocking sockets and EAGAIN handling improve responsiveness; the
     *   implementation may choose blocking I/O with select/poll in the worker.
     * - For embedded builds using CMSIS/FreeRTOS ensure the kernel is started
     *   before invoking async APIs.
     */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // NAL_NETWORK_H