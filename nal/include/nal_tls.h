#ifndef NAL_TLS_H
#define NAL_TLS_H


#include "nal_config.h"
#include "nal_platform.h"
#include "nal_core.h"

#include "cmsis_os2.h"

#include <stddef.h>
#include <stdint.h>
#include "bsp_err_sts.h"


#ifdef __cplusplus
extern "C"
{
#endif

#if defined(NAL_USE_TLS)

    #define NAL_MAX_TLS_SESSIONS bspNAL_MAX_CONNCETION_SUPPORT

    /**
     * @brief Opaque TLS context used by NAL.
     *
     * This structure encapsulates all TLS-related state
     * (certificates, handshake state, encryption context).
     *
     * The internal layout is implementation-specific and
     * must not be accessed directly by NAL core or applications.
     */
    typedef struct nalTlsCtx nalTlsCtx_t;

    /* =========================================================================
     *  TLS Lifecycle
     * ========================================================================= */

    /**
     * @brief Create and initialize a TLS context.
     *
     * This function allocates and initializes all TLS-related
     * runtime structures. It does NOT perform a network handshake.
     *
     * Typical usage:
     *  - Called after TCP socket is connected
     *  - Followed by nalTlsHandshake()
     *
     * @param[out] handle NAL handle
     *
     * @return
     *  - BSP_ERR_STS_OK              Success
     *  - BSP_ERR_STS_NO_MEM          Memory allocation failed
     *  - BSP_ERR_STS_INTERNAL_ERROR  TLS backend initialization failed
     */
    bsp_err_sts_t nalTlsInit(nalHandle_t* handle);

    /**
     * @brief Perform TLS handshake on an already-connected socket.
     *
     * This function binds the TLS context to an existing TCP socket
     * and performs the TLS handshake (client side).
     *
     * The socket must already be connected.
     *
     * @param[in,out] handle NAL handle
     *
     * @return
     *  - BSP_ERR_STS_OK              Handshake successful
     *  - BSP_ERR_STS_TIMEOUT         Handshake timed out
     *  - BSP_ERR_STS_HANDSHAKE_FAIL  TLS handshake failed
     *  - BSP_ERR_STS_FAIL            Generic TLS error
     */
    bsp_err_sts_t nalTlsHandshake(nalTlsCtx_t* handle);

    /**
     * @brief Gracefully shutdown a TLS session.
     *
     * Sends TLS close-notify if possible.
     * Best-effort operation; failure is non-fatal.
     *
     * @param[in,out] handle NAL handle
     */
    bsp_err_sts_t nalTlsShutdown(nalHandle_t* handle);

    /**
     * @brief Free and destroy a TLS context.
     *
     * This releases all memory and cryptographic state.
     * Safe to call multiple times.
     *
     * @param[in,out] handle NAL handle
     */
    bsp_err_sts_t nalTlsDeinit(nalHandle_t* handle);

    /* =========================================================================
     *  TLS Data I/O
     * ========================================================================= */

    /**
     * @brief Send encrypted data over TLS.
     *
     * This function behaves like a blocking send on a stream:
     *  - Handles partial writes
     *  - Respects timeout
     *  - Returns only when data is sent or an error occurs
     *
     * @param[in]     handle       NAL handle (for lock & state)
     * @param[in]     data         Plaintext data to send
     * @param[in]     len          Number of bytes to send
     * @param[out]    bytes_sent   Number of bytes actually sent
     * @param[in]     timeout_ms   Timeout in milliseconds (0 = infinite)
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_TIMEOUT
     *  - BSP_ERR_STS_CONN_LOST
     *  - BSP_ERR_STS_FAIL
     */
    bsp_err_sts_t
    nalTlsSend(nalHandle_t* handle, const void* data, size_t len, size_t* bytes_sent, uint32_t timeout_ms);

    /**
     * @brief Receive decrypted data over TLS.
     *
     * Blocks until data is available, timeout expires,
     * or connection is closed.
     *
     * @param[in]     handle       NAL handle
     * @param[out]    buf          Receive buffer
     * @param[in]     buf_len      Buffer size
     * @param[out]    bytes_recv   Number of bytes received
     * @param[in]     timeout_ms   Timeout in milliseconds
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_TIMEOUT
     *  - BSP_ERR_STS_CONN_LOST
     *  - BSP_ERR_STS_FAIL
     */
    bsp_err_sts_t
    nalTlsRecv(nalHandle_t* handle, void* buf, size_t buf_len, size_t* bytes_recv, uint32_t timeout_ms);

    /* =========================================================================
     *  Certificate Management
     * ========================================================================= */

    /**
     * @brief Store CA certificate for TLS verification.
     *
     * The certificate is copied internally. The caller
     * may free the buffer after this call.
     *
     * @param[in] handle Pointer to nalHandle_t
     * @param[in] ca_pem CA certificate in PEM format
     * @param[in] len    Length of PEM data
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_NO_MEM
     *  - BSP_ERR_STS_INVALID_PARAM
     */
    bsp_err_sts_t nalTlsSetCaCert(nalHandle_t* handle, const uint8_t* ca_pem, size_t len);

    /**
     * @brief Retrieve stored CA certificate.
     *
     * @param[in]  handle      Pointer to nalHandle_t
     * @param[out] out_ca_pem  Pointer to CA cert buffer
     * @param[out] out_len     Length of cert
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_NOT_INITIALIZED
     */
    bsp_err_sts_t
    nalTlsGetCaCert(nalHandle_t* handle, const uint8_t** out_ca_pem, size_t* out_len);

    /**
     * @brief Retrieve stored CA certificate.
     *
     * @param[in]  handle      Pointer to nalHandle_t
     * @param[out] out_ca_pem  Pointer to CA cert buffer
     * @param[out] out_len     Length of cert
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_NOT_INITIALIZED
     */
    bsp_err_sts_t
    nalTlsGetCaCert(nalHandle_t* handle, const uint8_t** out_ca_pem, size_t* out_len);


    /**
     * @brief Decrypt TLS-encrypted data using the active session.
     *
     * This function decrypts data that was encrypted using the same
     * TLS session keys.
     *
     * Use cases:
     *  - Receiving encrypted payloads over non-socket links
     *  - Offline decryption
     *  - Custom transport layers
     *
     * Requirements:
     *  - TLS must be connected and handshake completed
     *  - Input data must be valid TLS-encrypted payload
     *
     * @param[in]     handle      Pointer to active nalHandle_t
     * @param[in]     ciphertext  Encrypted input buffer
     * @param[in]     cipher_len  Length of encrypted data
     * @param[out]    plaintext   Output plaintext buffer
     * @param[in,out] plain_len   IN: size of plaintext buffer
     *                            OUT: actual decrypted length
     *
     * @return
     *  - BSP_ERR_STS_OK              Decryption successful
     *  - BSP_ERR_STS_NOT_CONNECTED   TLS session not active
     *  - BSP_ERR_STS_BUF_TOO_SMALL   Output buffer too small
     *  - BSP_ERR_STS_FAIL            Decryption failed
     */
    bsp_err_sts_t nalTlsDecrypt(nalHandle_t* handle,
                                const uint8_t* ciphertext,
                                size_t cipher_len,
                                uint8_t* plaintext,
                                size_t* plain_len);

    bsp_err_sts_t nalTlsNetworkAccept(nalHandle_t* handle, uint32_t timeout_ms);


#endif /* NAL_USE_TLS */

#ifdef __cplusplus
}
#endif

#endif /* NAL_TLS_H */
