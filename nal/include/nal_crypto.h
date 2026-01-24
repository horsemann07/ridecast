/**
 * @file    nal_crypto.h
 * @brief   Generic cryptographic utilities for NAL.
 *
 * This module provides simple encrypt/decrypt APIs for
 * non-socket transports such as UART, CAN, SPI, files,
 * or custom links.
 *
 * This is NOT TLS.
 */

#ifndef NAL_CRYPTO_H
#define NAL_CRYPTO_H

#include "nal_config.h"
#include "bsp_err_sts.h"

#include "cmsis_os2.h"

#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif

#if NAL_CONFIG_USE_CRYPTO

    /**
     * @brief Supported symmetric crypto key types.
     *
     * This enum defines the encryption strength and algorithm
     * used by the NAL crypto module.
     */
    typedef enum
    {
        NAL_CRYPTO_KEY_AES_128 = 0, /**< AES-GCM with 128-bit key */
        NAL_CRYPTO_KEY_AES_192,     /**< AES-GCM with 192-bit key */
        NAL_CRYPTO_KEY_AES_256      /**< AES-GCM with 256-bit key */
    } nalCryptoKeyType_t;


    /**
     * @brief AES-GCM parameter identifiers.
     *
     * Defines fixed parameters used by the AES-GCM
     * encryption scheme.
     */
    typedef enum
    {
        NAL_CRYPTO_AES_GCM_NONCE_LEN = 12U, /**< GCM standard nonce length (bytes) */
        NAL_CRYPTO_AES_GCM_TAG_LEN = 16U, /**< GCM authentication tag length (bytes) */
        NAL_CRYPTO_AES_GCM_IV_LEN = 12U /**< GCM standard IV length (bytes) */
    } nalCryptoAesGcmParam_t;

    /**
     * @brief Configure symmetric encryption key.
     *
     * @param[in] key_type  Crypto key type (algorithm + strength)
     * @param[in] key       Pointer to key material
     *
     * @return BSP_ERR_STS_OK or error
     */
    bsp_err_sts_t nalCryptoSetKey(nalCryptoKeyType_t key_type, const uint8_t* key);


    /**
     * @brief Retrieve configured encryption key.
     *
     * @param[out] key     Pointer to key pointer
     * @param[out] key_len Length of key
     *
     * @return BSP_ERR_STS_OK or BSP_ERR_STS_NOT_INITIALIZED
     */
    bsp_err_sts_t nalCryptoGetKey(const uint8_t** key, size_t* key_len);

    /**
     * @brief Encrypt plaintext data using platform crypto backend.
     */
    bsp_err_sts_t nalCryptoEncrypt(const uint8_t* plaintext,
                                   size_t plain_len,
                                   uint8_t* ciphertext,
                                   size_t* cipher_len);

    /**
     * @brief Decrypt encrypted data using platform crypto backend.
     */
    bsp_err_sts_t nalCryptoDecrypt(const uint8_t* ciphertext,
                                   size_t cipher_len,
                                   uint8_t* plaintext,
                                   size_t* plain_len);

#endif /* NAL_CONFIG_USE_CRYPTO */

#ifdef __cplusplus
}
#endif


#endif /* NAL_CRYPTO_H */
