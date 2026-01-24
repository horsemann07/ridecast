#include "nal_crypto.h"
#include "nal_platform.h"
#include "nal_log.h"

#if NAL_CONFIG_USE_CRYPTO

static const uint8_t* g_crypto_key = NULL;
static size_t g_crypto_key_len     = 0;


static size_t nalCryptoKeyLenFromType(nalCryptoKeyType_t type)
{
    switch(type)
    {
        case NAL_CRYPTO_KEY_AES_128:
            return 16;
        case NAL_CRYPTO_KEY_AES_192:
            return 24;
        case NAL_CRYPTO_KEY_AES_256:
            return 32;
        default:
            return 0;
    }
}

bsp_err_sts_t nalCryptoSetKey(nalCryptoKeyType_t key_type, const uint8_t* key)
{
    size_t key_len = nalCryptoKeyLenFromType(key_type);

    if(!key || key_len == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    g_crypto_key     = key;
    g_crypto_key_len = key_len;
    return BSP_ERR_STS_OK;
}

bsp_err_sts_t nalCryptoGetKey(const uint8_t** key, size_t* key_len)
{
    if(!key || !key_len)
        return BSP_ERR_STS_INVALID_PARAM;

    if(!g_crypto_key)
    {
        /* Crypto used before key initialization */
        NAL_LOGE("Crypto encrypt called before key initialization");
        return BSP_ERR_STS_NOT_INIT;
    }

    *key     = g_crypto_key;
    *key_len = g_crypto_key_len;
    return BSP_ERR_STS_OK;
}

bsp_err_sts_t
nalCryptoEncrypt(const uint8_t* plaintext, size_t plain_len, uint8_t* ciphertext, size_t* cipher_len)
{
    if(!g_crypto_key)
    {
        /* Crypto used before key initialization */
        NAL_LOGE("Crypto encrypt called before key initialization");
        return BSP_ERR_STS_NOT_INIT;
    }

    return nal_platform_crypto_encrypt(g_crypto_key, g_crypto_key_len, plaintext,
                                       plain_len, ciphertext, cipher_len);
}

bsp_err_sts_t
nalCryptoDecrypt(const uint8_t* ciphertext, size_t cipher_len, uint8_t* plaintext, size_t* plain_len)
{
    if(!g_crypto_key)
    {
        /* Crypto used before key initialization */
        NAL_LOGE("Crypto decrypt called before key initialization");
        return BSP_ERR_STS_NOT_INIT;
    }

    return nal_platform_crypto_decrypt(g_crypto_key, g_crypto_key_len, ciphertext,
                                       cipher_len, plaintext, plain_len);
}

#endif
