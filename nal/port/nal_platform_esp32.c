


#include "nal_config.h"
#include "bsp_err_sts.h"
#include "cmsis_os2.h"
#include "nal_log.h"
#include "nal_crypto.h"

#include "nal_platform.h"
#include "nal_core.h"

#include <string.h>
#include <stdlib.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_random.h"

#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/pk.h"

#include "lwip/sockets.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/sys.h"


#if defined(NAL_USE_TLS)


    #ifndef NAL_TLS_ALLOC_MODE
        #define NAL_TLS_ALLOC_MODE NAL_TLS_ALLOC_STATIC
    #endif // NAL_TLS_ALLOC_MODE

    #if (NAL_TLS_ALLOC_MODE == NAL_TLS_ALLOC_HEAP)
        #include "esp_heap_caps.h"
    #endif //(NAL_TLS_ALLOC_MODE == NAL_TLS_ALLOC_HEAP)


    /*
     * =========================================================================
     *  Macros
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
            if((h) && (h)->lock)                          \
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
            if((h) && (h)->lock)           \
            {                              \
                osMutexRelease((h)->lock); \
            }                              \
        } while(0)


/**
 * @brief Maximum number of concurrent TLS backend instances.
 *
 * This value must be tuned per product based on:
 *  - maximum simultaneous TLS connections
 *  - available RAM
 */
    #define ESP32_TLS_BACKEND_MAX 2

/*
 * =========================================================================
 *  Private Types
 * =========================================================================
 */
/**
 * @brief ESP32-specific TLS backend context.
 *
 * This structure encapsulates all mbedTLS objects required
 * to establish and maintain a single TLS session.
 *
 * This type is INTERNAL to nal_platform_esp32.c and must
 * never be exposed outside this file.
 */
typedef struct
{
    mbedtls_ssl_context ssl; /**< mbedTLS SSL context (session state) */
    mbedtls_ssl_config conf; /**< mbedTLS SSL configuration */
    mbedtls_ctr_drbg_context ctr_drbg; /**< Deterministic random generator */
    mbedtls_entropy_context entropy;   /**< Entropy source */
    mbedtls_x509_crt x509_cert;        /**< Parsed CA certificate chain */
    mbedtls_pk_context pkey;           /**< Private key */

    #if (NAL_TLS_ALLOC_MODE == NAL_TLS_ALLOC_STATIC)
    bool in_use;                       /**< Pool ownership flag */
    #endif

    bool x509_cert_set;                /**< CA certificate configured */
    bool sni_set;                      /**< Server Name Indication configured */
    int sockfd;                        /**< Socket file descriptor */

} esp32_tls_backend_t;


/*
 * =========================================================================
 *  Private Variables
 * =========================================================================
 */
/** Static pool of TLS backend contexts */
static esp32_tls_backend_t g_tls_backend_pool[ESP32_TLS_BACKEND_MAX];

/** Mutex protecting backend pool allocation */
static osMutexId_t g_tls_backend_lock;

/**
 * @brief TLS DRBG personalization counter.
 *
 * Used to generate unique personalization strings
 * for each TLS backend instance.
 */
static uint32_t g_tls_pers_counter = 0;


/*
 * =========================================================================
 *  Private API
 * =========================================================================
 */
/**
 * @brief Allocate a TLS backend context from heap.
 *
 * @return Pointer to allocated backend context, or NULL on failure.
 */
static esp32_tls_backend_t* esp32_tls_backend_alloc(void)
{
    #if (NAL_TLS_ALLOC_MODE == NAL_TLS_ALLOC_HEAP)

    esp32_tls_backend_t* ctx =
    (esp32_tls_backend_t*)calloc(1, sizeof(esp32_tls_backend_t));

    if(!ctx)
    {
        NAL_LOGE("TLS backend alloc failed (heap)");
        return NULL;
    }

    return ctx;

    #else  /* NAL_TLS_ALLOC_STATIC */

    esp32_tls_backend_t* ctx = NULL;

    if(osMutexAcquire(g_tls_backend_lock, osWaitForever) != osOK)
    {
        NAL_LOGE("TLS backend pool lock failed");
        return NULL;
    }

    for(int i = 0; i < ESP32_TLS_BACKEND_MAX; i++)
    {
        if(!g_tls_backend_pool[i].in_use)
        {
            memset(&g_tls_backend_pool[i], 0, sizeof(esp32_tls_backend_t));
            g_tls_backend_pool[i].in_use = true;
            ctx                          = &g_tls_backend_pool[i];
            break;
        }
    }

    osMutexRelease(g_tls_backend_lock);

    if(!ctx)
    {
        NAL_LOGE("TLS backend pool exhausted (max=%d)", ESP32_TLS_BACKEND_MAX);
    }

    return ctx;

    #endif /* NAL_TLS_ALLOC_MODE */
}

/**
 * @brief Free a TLS backend context allocated from heap.
 *
 * @param ctx Pointer to backend context.
 */
static void esp32_tls_backend_free(esp32_tls_backend_t* ctx)
{
    if(!ctx)
        return;

    #if (NAL_TLS_ALLOC_MODE == NAL_TLS_ALLOC_HEAP)

    free(ctx);

    #else /* STATIC */

    if(osMutexAcquire(g_tls_backend_lock, osWaitForever) == osOK)
    {
        ctx->in_use = false;
        osMutexRelease(g_tls_backend_lock);
    }

    #endif
}

static void nal_platform_set_socket_timeout(int sockfd, uint32_t timeout_ms, bool is_recv)
{
    struct timeval tv;

    tv.tv_sec  = timeout_ms / 1000U;
    tv.tv_usec = (timeout_ms % 1000U) * 1000U;

    int opt = is_recv ? SO_RCVTIMEO : SO_SNDTIMEO;

    setsockopt(sockfd, SOL_SOCKET, opt, &tv, sizeof(tv));
}

static void tls_ctx_cleanup(esp32_tls_backend_t* ctx)
{
    if(!ctx)
        return;

    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_x509_crt_free(&ctx->x509_cert);
    mbedtls_pk_free(&ctx->pkey);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_entropy_free(&ctx->entropy);
}
/*
 * =========================================================================
 *  Public API
 * =========================================================================
 */
void* nal_platform_tls_create(void)
{
    char pers[32];
    uint32_t id = ++g_tls_pers_counter;

    /* Personalization string for DRBG */
    snprintf(pers, sizeof(pers), "nal_tls_%lu", (unsigned long)id);

    esp32_tls_backend_t* ctx = esp32_tls_backend_alloc();
    if(!ctx)
    {
        NAL_LOGE("TLS backend allocation failed");
        return NULL;
    }

    /* ---- Init all contexts ---- */
    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
    mbedtls_entropy_init(&ctx->entropy);
    mbedtls_x509_crt_init(&ctx->x509_cert); /* Used as CA store */
    mbedtls_pk_init(&ctx->pkey);            /* For optional client cert */

    /* ---- Seed DRBG ---- */
    int ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy,
                                    (const unsigned char*)pers, strlen(pers));
    if(ret != 0)
    {
        NAL_LOGE("TLS DRBG seed failed (err=%d)", ret);
        goto fail;
    }

    /* ---- Default TLS client config ---- */
    ret = mbedtls_ssl_config_defaults(&ctx->conf, MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if(ret != 0)
    {
        NAL_LOGE("TLS config defaults failed (err=%d)", ret);
        goto fail;
    }

    /* ---- Attach RNG ---- */
    mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);

    /* ---- Attach RNG ---- */
    mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);

    /* ====== LOAD CA CERT HERE ====== */
    // ret = mbedtls_x509_crt_parse(&ctx->x509_cert, ca_pem, ca_len);
    // if(ret != 0)
    // {
    //     NAL_LOGE("CA cert parse failed: -0x%x", -ret);
    //     goto fail;
    // }

    // mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->x509_cert, NULL);
    // ctx->x509_cert_set = true;

    /* ---- Require server verification ---- */
    mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);

    /*
     * NOTE:
     * CA certificate must be loaded later via:
     *   mbedtls_x509_crt_parse()
     * and attached using:
     *   mbedtls_ssl_conf_ca_chain()
     */

    /* ---- Final SSL context ---- */
    ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf);
    if(ret != 0)
    {
        NAL_LOGE("TLS SSL setup failed (err=%d)", ret);
        goto fail;
    }

    /*
     * Hostname (SNI + cert verification) must be set later using:
     *   mbedtls_ssl_set_hostname()
     */
    ctx->x509_cert_set = false;
    ctx->sni_set       = false;

    return (void*)ctx;

fail:
    mbedtls_x509_crt_free(&ctx->x509_cert);
    mbedtls_pk_free(&ctx->pkey);
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_entropy_free(&ctx->entropy);

    esp32_tls_backend_free(ctx);
    return NULL;
}


/* ---------------------------------------------------------------------- */
bsp_err_sts_t nal_platform_tls_destroy(void* backend_ctx)
{
    if(backend_ctx == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    esp32_tls_backend_t* ctx = (esp32_tls_backend_t*)backend_ctx;


    /*
     * Free parsed CA certificate (if any).
     * Safe to call even if never set.
     */
    if(ctx->x509_cert_set)
    {
        mbedtls_x509_crt_free(&ctx->x509_cert);
        ctx->x509_cert_set = false;
    }

    /*
     * Free SSL context.
     * This clears session keys and handshake state.
     */
    mbedtls_ssl_free(&ctx->ssl);

    /*
     * Free SSL configuration.
     */
    mbedtls_ssl_config_free(&ctx->conf);

    /*
     * Free DRBG and entropy contexts.
     */
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);

    mbedtls_entropy_free(&ctx->entropy);

    /*
     * Release backend context memory
     * (heap or static pool, depending on build).
     */
    esp32_tls_backend_free(ctx);

    if(g_tls_pers_counter)
    {
        g_tls_pers_counter--;
    }

    return BSP_ERR_STS_OK;
}

/* ---------------------------------------------------------------------- */
bsp_err_sts_t nal_platform_tls_handshake(void* backend_ctx, int sockfd)
{
    if(backend_ctx == NULL || sockfd < 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    esp32_tls_backend_t* ctx = (esp32_tls_backend_t*)backend_ctx;

    /*
     * Bind the existing socket to mbedTLS.
     * We do NOT own the socket.
     */
    mbedtls_ssl_set_bio(&ctx->ssl, (void*)(intptr_t)sockfd, mbedtls_net_send,
                        mbedtls_net_recv, NULL);

    ctx->sockfd = sockfd;
    /*
     * Perform TLS handshake.
     * mbedTLS may require multiple read/write cycles.
     */
    int ret;
    while((ret = mbedtls_ssl_handshake(&ctx->ssl)) != 0)
    {
        if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            /* Retry until handshake completes */
            continue;
        }

        /* Real handshake failure */
        NAL_LOGE("TLS handshake failed (err=%d)", ret);
        return BSP_ERR_STS_HANDSHAKE_FAIL;
    }

    return BSP_ERR_STS_OK;
}

/* ---------------------------------------------------------------------- */
bsp_err_sts_t nal_platform_tls_reset(void* backend_ctx)
{
    if(backend_ctx == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    esp32_tls_backend_t* ctx = (esp32_tls_backend_t*)backend_ctx;

    /*
     * Reset SSL session state.
     * Keeps configuration (CA, RNG, auth mode).
     */
    int ret = mbedtls_ssl_session_reset(&ctx->ssl);
    if(ret != 0)
    {
        return BSP_ERR_STS_FAIL;
        /* Rare but useful when debugging handshake retries */
        NAL_LOGD("TLS session reset failed (err=%d)", ret);
    }

    return BSP_ERR_STS_OK;
}

/* ---------------------------------------------------------------------- */
bsp_err_sts_t nal_platform_tls_send(void* backend_ctx,
                                    const uint8_t* data,
                                    size_t len,
                                    size_t* bytes_written,
                                    uint32_t timeout_ms)
{
    if(!backend_ctx || !data || !bytes_written || len == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    *bytes_written = 0;

    esp32_tls_backend_t* ctx = (esp32_tls_backend_t*)backend_ctx;

    nal_platform_set_socket_timeout(ctx->sockfd, timeout_ms, false);

    int ret = mbedtls_ssl_write(&ctx->ssl, data, len);
    if(ret > 0)
    {
        *bytes_written = (size_t)ret;
        return BSP_ERR_STS_OK;
    }

    if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        /* Retry at upper layer */
        return BSP_ERR_STS_TIMEOUT;
    }

    if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
    {
        return BSP_ERR_STS_CONN_LOST;
    }

    /* Real TLS error */
    NAL_LOGE("TLS write failed (err=%d)", ret);
    return BSP_ERR_STS_FAIL;
}

/* ---------------------------------------------------------------------- */
bsp_err_sts_t
nal_platform_tls_recv(void* backend_ctx, uint8_t* buf, size_t buf_len, size_t* bytes_read, uint32_t timeout_ms)
{
    if(!backend_ctx || !buf || !bytes_read || buf_len == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    *bytes_read = 0;

    esp32_tls_backend_t* ctx = (esp32_tls_backend_t*)backend_ctx;

    nal_platform_set_socket_timeout(ctx->sockfd, timeout_ms, true);

    int ret = mbedtls_ssl_read(&ctx->ssl, buf, buf_len);
    if(ret > 0)
    {
        *bytes_read = (size_t)ret;
        return BSP_ERR_STS_OK;
    }

    if(ret == 0 || ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
    {
        return BSP_ERR_STS_CONN_LOST;
    }

    if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        return BSP_ERR_STS_TIMEOUT;
    }

    /* Real TLS error */
    NAL_LOGE("TLS read failed (err=%d)", ret);
    return BSP_ERR_STS_FAIL;
}

/* ---------------------------------------------------------------------- */
bsp_err_sts_t nal_platform_tls_shutdown(void* backend_ctx)
{
    if(backend_ctx == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    esp32_tls_backend_t* ctx = (esp32_tls_backend_t*)backend_ctx;

    /*
     * Best-effort TLS shutdown.
     * This sends close_notify if the session is active.
     */
    int ret = mbedtls_ssl_close_notify(&ctx->ssl);
    if(ret < 0)
    {
        /* Non-fatal, useful only for debugging */
        NAL_LOGD("TLS close_notify failed (err=%d)", ret);
        return BSP_ERR_STS_FAIL;
    }
    return BSP_ERR_STS_OK;
}

/* ---------------------------------------------------------------------- */
bsp_err_sts_t nal_platform_tls_set_server_name(void* backend_ctx, const char* server_name)
{
    if(backend_ctx == NULL || server_name == NULL || server_name[0] == '\0')
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    esp32_tls_backend_t* ctx = (esp32_tls_backend_t*)backend_ctx;

    /*
     * Configure Server Name Indication (SNI).
     * Must be set before TLS handshake.
     */
    int ret = mbedtls_ssl_set_hostname(&ctx->ssl, server_name);
    if(ret != 0)
    {
        NAL_LOGE("TLS set server name failed (err=%d)", ret);
        return BSP_ERR_STS_FAIL;
    }

    ctx->sni_set = true;
    return BSP_ERR_STS_OK;
}

/* ---------------------------------------------------------------------- */
bsp_err_sts_t nal_platform_tls_server_init(void* tls_backend, const nalTlsServerCreds_t* creds)
{
    if(!tls_backend || !creds || !creds->cert_pem || !creds->key_pem)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    esp32_tls_backend_t* ctx = (esp32_tls_backend_t*)tls_backend;
    int ret;

    /* ---- Init all mbedTLS contexts ---- */
    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);
    mbedtls_x509_crt_init(&ctx->x509_cert);
    mbedtls_pk_init(&ctx->pkey);
    mbedtls_entropy_init(&ctx->entropy);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);

    /* ---- Seed RNG (MANDATORY for production TLS) ---- */
    ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func,
                                &ctx->entropy, NULL, 0);
    if(ret != 0)
    {
        NAL_LOGE("DRBG seed failed: -0x%x", -ret);
        tls_ctx_cleanup(ctx);
        return BSP_ERR_STS_FAIL;
    }

    /* ---- Parse server certificate ---- */
    ret = mbedtls_x509_crt_parse(&ctx->x509_cert, creds->cert_pem, creds->cert_len);
    if(ret != 0)
    {
        NAL_LOGE("cert parse failed: -0x%x", -ret);
        tls_ctx_cleanup(ctx);
        return BSP_ERR_STS_FAIL;
    }

    /* ---- Parse private key (with RNG wired) ---- */
    ret = mbedtls_pk_parse_key(&ctx->pkey, creds->key_pem, creds->key_len, NULL,
                               0, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    if(ret != 0)
    {
        NAL_LOGE("key parse failed: -0x%x", -ret);
        tls_ctx_cleanup(ctx);
        return BSP_ERR_STS_FAIL;
    }

    /* ---- Configure TLS as SERVER ---- */
    ret = mbedtls_ssl_config_defaults(&ctx->conf, MBEDTLS_SSL_IS_SERVER,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if(ret != 0)
    {
        NAL_LOGE("ssl_config_defaults failed: -0x%x", -ret);
        tls_ctx_cleanup(ctx);
        return BSP_ERR_STS_FAIL;
    }

    // ret = mbedtls_x509_crt_parse(&ctx->x509_cert_set, creds->ca_pem,
    // creds->ca_len); if(ret != 0)
    // {
    //     NAL_LOGE("CA cert parse failed: -0x%x", -ret);
    //     tls_ctx_cleanup(ctx);
    //     return BSP_ERR_STS_FAIL;
    // }

    // mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->x509_cert_set, NULL);
    // ctx->x509_cert_set = true;

    /* ---- Security posture ---- */
    /* NOTE:
     * Use VERIFY_REQUIRED + CA chain for mTLS.
     * VERIFY_NONE is acceptable ONLY for controlled/internal links.
     */
    mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE);

    /* ---- Attach RNG to TLS ---- */
    mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);

    /* ---- Attach server cert + key ---- */
    ret = mbedtls_ssl_conf_own_cert(&ctx->conf, &ctx->x509_cert, &ctx->pkey);
    if(ret != 0)
    {
        NAL_LOGE("ssl_conf_own_cert failed: -0x%x", -ret);
        tls_ctx_cleanup(ctx);
        return BSP_ERR_STS_FAIL;
    }

    /* ---- Final SSL context setup ---- */
    ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf);
    if(ret != 0)
    {
        NAL_LOGE("ssl_setup failed: -0x%x", -ret);
        tls_ctx_cleanup(ctx);
        return BSP_ERR_STS_FAIL;
    }

    return BSP_ERR_STS_OK;
}

/* ---------------------------------------------------------------------- */
bsp_err_sts_t nal_platform_tls_server_handshake(void* tls_backend)
{
    if(!tls_backend)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    esp32_tls_backend_t* ctx = (esp32_tls_backend_t*)tls_backend;

    if(ctx->sockfd < 0)
    {
        return BSP_ERR_STS_NO_CONN;
    }

    mbedtls_ssl_set_bio(&ctx->ssl, &ctx->sockfd, mbedtls_net_send, mbedtls_net_recv, NULL);

    int ret;
    while((ret = mbedtls_ssl_handshake(&ctx->ssl)) != 0)
    {
        if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            continue;
        }

        NAL_LOGE("TLS handshake failed: -0x%x", -ret);
        return BSP_ERR_STS_FAIL;
    }

    return BSP_ERR_STS_OK;
}

/* ---------------------------------------------------------------------- */
int nal_platform_accept(int listen_fd, uint32_t timeout_ms)
{
    if(listen_fd < 0)
    {
        return -1;
    }

    /* Configure accept timeout */
    if(timeout_ms > 0)
    {
        struct timeval tv;
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        lwip_setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = lwip_accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);

    if(client_fd < 0)
    {
        if(errno == EWOULDBLOCK || errno == EAGAIN)
        {
            /* Timeout */
            return -1;
        }

        NAL_LOGE("accept failed errno=%d", errno);
        return -1;
    }

    return client_fd;
}
/* ---------------------------------------------------------------------- */
#endif /* NAL_CONFIG_USE_TLS */
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
#if defined(NAL_CONFIG_USE_CRYPTO)

bsp_err_sts_t nal_platform_crypto_encrypt(const uint8_t* key,
                                          size_t key_len,
                                          const uint8_t* plaintext,
                                          size_t plain_len,
                                          uint8_t* ciphertext,
                                          size_t* cipher_len)
{
    if(!key || !plaintext || !ciphertext || !cipher_len)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* Validate AES key length (128 / 192 / 256) */
    if(key_len != 16 && key_len != 24 && key_len != 32)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    size_t required_len = NAL_CRYPTO_AES_GCM_NONCE_LEN + plain_len + NAL_CRYPTO_AES_GCM_TAG_LEN;

    if(*cipher_len < required_len)
    {
        return BSP_ERR_STS_BUF_TOO_SMALL;
    }

    uint8_t* nonce = ciphertext;
    uint8_t* enc   = ciphertext + NAL_CRYPTO_AES_GCM_NONCE_LEN;
    uint8_t* tag   = enc + plain_len;

    /* Generate random nonce */
    for(size_t i = 0; i < NAL_CRYPTO_AES_GCM_NONCE_LEN; i += 4)
    {
        uint32_t r = esp_random();
        memcpy(&nonce[i], &r,
               (NAL_CRYPTO_AES_GCM_NONCE_LEN - i >= 4) ? 4 : (NAL_CRYPTO_AES_GCM_NONCE_LEN - i));
    }

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);

    int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, key_len * 8);
    if(ret != 0)
    {
        NAL_LOGE("AES-GCM setkey failed (%d)", ret);
        goto fail;
    }

    ret = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, plain_len, nonce,
                                    NAL_CRYPTO_AES_GCM_NONCE_LEN, NULL, 0, plaintext,
                                    enc, NAL_CRYPTO_AES_GCM_TAG_LEN, tag);
    if(ret != 0)
    {
        NAL_LOGE("AES-GCM encrypt failed (%d)", ret);
        goto fail;
    }

    *cipher_len = required_len;
    mbedtls_gcm_free(&gcm);
    return BSP_ERR_STS_OK;

fail:
    mbedtls_gcm_free(&gcm);
    return BSP_ERR_STS_FAIL;
}

/* ---------------------------------------------------------------------- */
bsp_err_sts_t nal_platform_crypto_decrypt(const uint8_t* key,
                                          size_t key_len,
                                          const uint8_t* ciphertext,
                                          size_t cipher_len,
                                          uint8_t* plaintext,
                                          size_t* plain_len)
{
    if(!key || !ciphertext || !plaintext || !plain_len)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(key_len != 16 && key_len != 24 && key_len != 32)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(cipher_len < (NAL_CRYPTO_AES_GCM_NONCE_LEN + NAL_CRYPTO_AES_GCM_TAG_LEN))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    size_t enc_len = cipher_len - NAL_CRYPTO_AES_GCM_NONCE_LEN - NAL_CRYPTO_AES_GCM_TAG_LEN;

    if(*plain_len < enc_len)
    {
        return BSP_ERR_STS_BUF_TOO_SMALL;
    }

    const uint8_t* nonce = ciphertext;
    const uint8_t* enc   = ciphertext + NAL_CRYPTO_AES_GCM_NONCE_LEN;
    const uint8_t* tag   = enc + enc_len;

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);

    int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, key_len * 8);
    if(ret != 0)
    {
        NAL_LOGE("AES-GCM setkey failed (%d)", ret);
        goto fail;
    }

    ret = mbedtls_gcm_auth_decrypt(&gcm, enc_len, nonce,
                                   NAL_CRYPTO_AES_GCM_NONCE_LEN, NULL, 0, tag,
                                   NAL_CRYPTO_AES_GCM_TAG_LEN, enc, plaintext);
    if(ret != 0)
    {
        /* Authentication failure */
        NAL_LOGE("AES-GCM auth failed (%d)", ret);
        goto fail;
    }

    *plain_len = enc_len;
    mbedtls_gcm_free(&gcm);
    return BSP_ERR_STS_OK;

fail:
    mbedtls_gcm_free(&gcm);
    return BSP_ERR_STS_FAIL;
}

#endif // #if defined(NAL_CONFIG_USE_CRYPTO)


//*******************************************************************************
// End of nal_platform_esp32.c
//*******************************************************************************
