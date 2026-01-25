
#ifndef NAL_CONFIG_H
#define NAL_CONFIG_H


#ifdef __cplusplus
extern "C"
{
#endif

#include "sys_config.h"


#define NAL_TLS_ALLOC_STATIC (1)
#define NAL_TLS_ALLOC_HEAP   (2)


/* ============================================================
 * TLS configuration 1 = enable TLS, 0 = disable
 * ============================================================ */
#define nalCONFIG_NAL_USE_TLS                (1)

#define nalCONFIG_NAL_TLS_ALLOC_MODE         NAL_TLS_ALLOC_STATIC
#define nalCONFIG_NAL_TLS_BACKEND_ALLOC_MODE (1)

#define nalCONFIG_NAL_EVENT_THREAD_NAME      ("NalEventThread")
#define nalCONFIG_NAL_EVENT_THREAD_STACK     (2048U)
#define nalCONFIG_NAL_EVENT_THREAD_PRIO      (osPriorityNormal)
#define nalCONFIG_NAL_NET_RECV_TIMEOUT_MS    (5000U)

#define bspNAL_MAX_CONNCETION_SUPPORT        (2U)
/* ============================================================
 * Crypto configuration 1 = enable, 0 = disable
 * ============================================================ */
#define nalCONFIG_NAL_CONFIG_USE_CRYPTO (1)

/* ============================================================
 * Logging configuration
 * ============================================================ */

/* Set global log level */
#define nalCONFIG_NAL_LOG_LEVEL 3 /* INFO */

    /* Select backend if not ESP */
    // #define nalCONFIG_NAL_USE_BSP_LOG 1


    /* ============================================================
     * Network configuration
     * ============================================================ */

// #define NAL_USE_TLS           nalCONFIG_NAL_USE_TLS
#define NAL_TLS_ALLOC_MODE    nalCONFIG_NAL_TLS_ALLOC_MODE
#define NAL_LOG_LEVEL         nalCONFIG_NAL_LOG_LEVEL
#define NAL_CONFIG_USE_CRYPTO nalCONFIG_NAL_CONFIG_USE_CRYPTO
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // NAL_CONFIG_H
