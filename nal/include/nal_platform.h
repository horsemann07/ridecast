/**
 * @file    nal_nal_platform_tls.h
 * @brief   Platform-specific TLS backend interface for NAL (ESP32).
 *
 * This file declares the platform abstraction functions required by the
 * NAL TLS layer to perform secure communication on ESP32 platforms.
 *
 * The implementation of these APIs is responsible for:
 *  - Initializing and managing the TLS engine (mbedTLS)
 *  - Binding TLS to an already-connected TCP socket
 *  - Performing TLS handshake
 *  - Encrypting and decrypting data
 *  - Handling TLS shutdown and cleanup
 *
 * IMPORTANT:
 *  - This interface is INTERNAL to NAL.
 *  - Application code MUST NOT call these functions directly.
 *  - All socket lifecycle management remains in NAL core.
 *
 * Design principles:
 *  - No socket ownership
 *  - No policy decisions
 *  - No retries or blocking logic beyond TLS primitives
 *  - Minimal logging (only failures or critical events)
 *
 * @note    This backend is designed for ESP-IDF v5+ using mbedTLS.
 *
 * @copyright
 * Copyright (c) 2026
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only within the context of the NAL project.
 */

#ifndef NAL_PLATFORM
#define NAL_PLATFORM

#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os2.h"

#include <stdint.h>
#include <stddef.h>

#include "bsp_err_sts.h"

    /* =========================================================================
     *  TLS Backend Lifecycle
     * ========================================================================= */

    /**
     * @brief Create and initialize a platform-specific TLS backend context.
     *
     * This function allocates and initializes all TLS-related backend state
     * required for a single TLS session.
     *
     * What this function DOES:
     *  - Initializes mbedTLS structures
     *  - Seeds cryptographic RNG
     *  - Prepares TLS configuration (client mode)
     *
     * What this function DOES NOT do:
     *  - Bind to a socket
     *  - Perform TLS handshake
     *  - Configure CA certificates or SNI
     *
     * @return
     *  - Pointer to opaque TLS backend context on success
     *  - NULL on failure (allocation or initialization error)
     *
     * @note
     *  The returned context must be destroyed using
     *  nal_platform_tls_destroy().
     */
    void* nal_platform_tls_create(void);

    /**
     * @brief Destroy a platform-specific TLS backend context.
     *
     * This function releases all TLS-related resources, clears cryptographic
     * material, and frees backend memory.
     *
     * Safe to call:
     *  - If handshake never completed
     *  - After a failed handshake
     *  - Multiple times (NULL-safe)
     *
     * @param[in] backend_ctx
     *  Pointer to TLS backend context returned by
     *  nal_platform_tls_create().
     *
     * @note
     *  This function does NOT close the underlying socket.
     */
    bsp_err_sts_t nal_platform_tls_destroy(void* backend_ctx);

    /**
     * @brief Reset TLS session state after a failed handshake.
     *
     * This function clears handshake and session state while preserving
     * TLS configuration (CA certificate, SNI, RNG, auth mode).
     *
     * Typical use case:
     *  - Handshake failure
     *  - Retry handshake or destroy context
     *
     * @param[in] backend_ctx
     *  Pointer to TLS backend context.
     */
    bsp_err_sts_t nal_platform_tls_reset(void* backend_ctx);

    /* =========================================================================
     *  TLS Configuration (Must be called BEFORE handshake)
     * ========================================================================= */

    /**
     * @brief Configure CA certificate for server verification.
     *
     * This function configures the trusted CA certificate chain used
     * to verify the server during TLS handshake.
     *
     * @param[in] backend_ctx
     *  Pointer to TLS backend context.
     *
     * @param[in] ca_pem
     *  Pointer to CA certificate in PEM format.
     *  The buffer must remain valid until handshake completes.
     *
     * @param[in] len
     *  Length of CA certificate buffer in bytes.
     *
     * @return
     *  - BSP_ERR_STS_OK    Configuration successful
     *  - BSP_ERR_STS_FAIL  Certificate parsing or configuration failed
     *
     * @note
     *  Must be called before nal_platform_tls_handshake().
     */
    bsp_err_sts_t
    nal_platform_tls_set_ca_cert(void* backend_ctx, const uint8_t* ca_pem, size_t len);

    /**
     * @brief Configure Server Name Indication (SNI).
     *
     * Sets the server hostname used during TLS handshake for
     * certificate verification and virtual hosting.
     *
     * @param[in] backend_ctx
     *  Pointer to TLS backend context.
     *
     * @param[in] server_name
     *  Null-terminated server hostname string.
     *
     * @return
     *  - BSP_ERR_STS_OK    SNI configured successfully
     *  - BSP_ERR_STS_FAIL  Failed to set server name
     *
     * @note
     *  Must be called before nal_platform_tls_handshake().
     */
    bsp_err_sts_t nal_platform_tls_set_server_name(void* backend_ctx, const char* server_name);

    /**
     * @brief Perform TLS client-side handshake.
     *
     * This function binds the TLS backend to an already-connected
     * TCP socket and performs the TLS handshake.
     *
     * @param[in] backend_ctx
     *  Pointer to TLS backend context.
     *
     * @param[in] sockfd
     *  Connected TCP socket file descriptor.
     *
     * @return
     *  - BSP_ERR_STS_OK              Handshake successful
     *  - BSP_ERR_STS_HANDSHAKE_FAIL  TLS handshake failed
     *  - BSP_ERR_STS_INVALID_PARAM   Invalid input parameters
     *
     * @note
     *  The socket must already be connected.
     *  This function does NOT take ownership of the socket.
     */
    bsp_err_sts_t nal_platform_tls_handshake(void* backend_ctx, int sockfd);

    /**
     * @brief Send encrypted data over TLS.
     *
     * Encrypts and sends application data using the active TLS session.
     *
     * @param[in] backend_ctx
     *  Pointer to TLS backend context.
     *
     * @param[in] data
     *  Pointer to plaintext data buffer.
     *
     * @param[in] len
     *  Number of bytes to send.
     *
     * @param[out] bytes_written
     *  Number of bytes successfully sent.
     *
     * @param[in] timeout_ms
     *  Send timeout in milliseconds (0 = blocking).
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_TIMEOUT
     *  - BSP_ERR_STS_CONN_LOST
     *  - BSP_ERR_STS_FAIL
     */
    bsp_err_sts_t nal_platform_tls_send(void* backend_ctx,
                                        const uint8_t* data,
                                        size_t len,
                                        size_t* bytes_written,
                                        uint32_t timeout_ms);

    /**
     * @brief Receive decrypted data over TLS.
     *
     * Receives and decrypts data from the active TLS session.
     *
     * @param[in] backend_ctx
     *  Pointer to TLS backend context.
     *
     * @param[out] buf
     *  Buffer to receive plaintext data.
     *
     * @param[in] buf_len
     *  Size of receive buffer.
     *
     * @param[out] bytes_read
     *  Number of bytes actually received.
     *
     * @param[in] timeout_ms
     *  Receive timeout in milliseconds.
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_TIMEOUT
     *  - BSP_ERR_STS_CONN_LOST
     *  - BSP_ERR_STS_FAIL
     */
    bsp_err_sts_t nal_platform_tls_recv(void* backend_ctx,
                                        uint8_t* buf,
                                        size_t buf_len,
                                        size_t* bytes_read,
                                        uint32_t timeout_ms);

    /**
     * @brief Perform graceful TLS shutdown.
     *
     * Sends TLS close-notify to the peer if possible.
     * This is a best-effort operation; failure is non-fatal.
     *
     * @param[in] backend_ctx
     *  Pointer to TLS backend context.
     *
     * @note
     *  This function does NOT close the socket.
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_INVALID_PARAM
     *  - BSP_ERR_STS_FAIL
     */
    bsp_err_sts_t nal_platform_tls_shutdown(void* backend_ctx);


    /**
     * @brief Encrypt data using platform crypto backend.
     *
     * Uses authenticated encryption (AES-GCM).
     *
     * Output format:
     *   [ NONCE | CIPHERTEXT | AUTH_TAG ]
     *
     * - NONCE     : 12 bytes
     * - AUTH_TAG  : 16 bytes
     *
     * @param[in]     key
     *  Pointer to symmetric encryption key.
     *
     * @param[in]     key_len
     *  Length of key in bytes (16 / 24 / 32).
     *
     * @param[in]     plaintext
     *  Input plaintext buffer.
     *
     * @param[in]     plain_len
     *  Length of plaintext in bytes.
     *
     * @param[out]    ciphertext
     *  Output buffer for encrypted data.
     *
     * @param[in,out] cipher_len
     *  IN : size of ciphertext buffer
     *  OUT: number of bytes written
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_INVALID_PARAM
     *  - BSP_ERR_STS_BUF_TOO_SMALL
     *  - BSP_ERR_STS_FAIL
     */
    bsp_err_sts_t nal_platform_crypto_encrypt(const uint8_t* key,
                                              size_t key_len,
                                              const uint8_t* plaintext,
                                              size_t plain_len,
                                              uint8_t* ciphertext,
                                              size_t* cipher_len);

    /**
     * @brief Decrypt data using platform crypto backend.
     *
     * Verifies authentication tag before returning plaintext.
     *
     * @param[in]     key
     *  Pointer to symmetric encryption key.
     *
     * @param[in]     key_len
     *  Length of key in bytes (16 / 24 / 32).
     *
     * @param[in]     ciphertext
     *  Encrypted input buffer (NONCE + CIPHERTEXT + TAG).
     *
     * @param[in]     cipher_len
     *  Length of encrypted buffer in bytes.
     *
     * @param[out]    plaintext
     *  Output plaintext buffer.
     *
     * @param[in,out] plain_len
     *  IN : size of plaintext buffer
     *  OUT: number of bytes decrypted
     *
     * @return
     *  - BSP_ERR_STS_OK
     *  - BSP_ERR_STS_INVALID_PARAM
     *  - BSP_ERR_STS_BUF_TOO_SMALL
     *  - BSP_ERR_STS_FAIL
     */
    bsp_err_sts_t nal_platform_crypto_decrypt(const uint8_t* key,
                                              size_t key_len,
                                              const uint8_t* ciphertext,
                                              size_t cipher_len,
                                              uint8_t* plaintext,
                                              size_t* plain_len);


#ifdef __cplusplus
}
#endif

#endif /* NAL_PLATFORM */
