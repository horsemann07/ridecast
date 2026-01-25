
#ifndef NAL_INTERNAL_H
#define NAL_INTERNAL_H

#include "nal_core.h"


/* ============================================================
 * Time helpers (CMSIS-RTOS)
 * ============================================================ */

#define NAL_TICKS_NOW()     (osKernelGetTickCount())
#define NAL_TICKS_PER_SEC() (osKernelGetTickFreq())

#define NAL_MS_TO_TICKS(ms) \
    ((uint32_t)(((uint64_t)(ms) * NAL_TICKS_PER_SEC()) / 1000U))

#define NAL_TICKS_TO_MS(ticks) \
    ((uint32_t)(((uint64_t)(ticks) * 1000U) / NAL_TICKS_PER_SEC()))

/* Timeout check */
#define NAL_TIMEOUT_EXPIRED(start_ticks, timeout_ms) \
    ((timeout_ms) > 0 && (NAL_TICKS_TO_MS(NAL_TICKS_NOW() - (start_ticks)) >= (timeout_ms)))


#define NAL_SERVER_VALID(h) ((h) && (h)->sockfd >= 0)

#define NAL_CLIENT_RESET(h)              \
    do                                   \
    {                                    \
        (h)->sockfd  = -1;               \
        (h)->scheme  = NAL_SCHEME_PLAIN; \
        (h)->tls_ctx = NULL;             \
    } while(0)


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

#endif /* NAL_INTERNAL_H */
