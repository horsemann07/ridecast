
#ifndef CORE_H
#define CORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    /* ========================================================= */
    /* GENERIC VALUES                                            */
    /* ========================================================= */

#ifndef ENABLE
    #define ENABLE (1U)
#endif

#ifndef DISABLE
    #define DISABLE (0U)
#endif

#ifndef TRUE
    #define TRUE (1U)
#endif

#ifndef FALSE
    #define FALSE (0U)
#endif

#ifndef NULL
    #define NULL ((void*)0)
#endif


#ifndef ON
    #define ON (1U)
#endif

#ifndef OFF
    #define OFF (0U)
#endif


    /* ========================================================= */
    /* MATHEMATICAL CONSTANTS (Arduino Style + Embedded Ready)   */
    /* ========================================================= */

#define MATH_PI                   (3.14159265358979323846)
#define MATH_TWO_PI               (6.28318530717958647692)
#define MATH_HALF_PI              (1.57079632679489661923)
#define MATH_E                    (2.71828182845904523536)

/* Degree / Radian Conversion */
#define DEG_TO_RAD(x)             ((x) * (MATH_PI / 180.0))
#define RAD_TO_DEG(x)             ((x) * (180.0 / MATH_PI))

    /* ========================================================= */
    /* PIN / HARDWARE DEFAULTS                                   */
    /* ========================================================= */

#define GPIO_PIN_UNUSED           (0xFFFFFFFFUL)

    /* ========================================================= */
    /* STACK SIZES (CMSIS expects BYTES)                         */
    /* ========================================================= */

#define THREAD_STACK_512B         (512U)
#define THREAD_STACK_1K           (1024U)
#define THREAD_STACK_1_5K         (1536U)
#define THREAD_STACK_2K           (2U * THREAD_STACK_1K)
#define THREAD_STACK_3K           (3U * THREAD_STACK_1K)
#define THREAD_STACK_4K           (4U * THREAD_STACK_1K)
#define THREAD_STACK_5K           (5U * THREAD_STACK_1K)
#define THREAD_STACK_6K           (6U * THREAD_STACK_1K)
#define THREAD_STACK_7K           (7U * THREAD_STACK_1K)
#define THREAD_STACK_8K           (8U * THREAD_STACK_1K)

/* Multiplier (512B blocks) */
#define THREAD_STACK_MULT_512B(x) ((x) * THREAD_STACK_512B)
#define THREAD_STACK_MULT_1KB(x)  ((x) * THREAD_STACK_1K)
    /* ========================================================= */
    /* TIME / DELAY                                              */
    /* ========================================================= */

#define SEC_TO_MS(sec)            ((sec) * 1000U)
#define MS_TO_SEC(ms)             ((ms) / 1000U)
#define MIN_DELAY_MS              (1U)

    /* ========================================================= */
    /* LIMIT VALUES                                              */
    /* ========================================================= */

#define U8_MAX                    (0xFFU)
#define U16_MAX                   (0xFFFFU)
#define U32_MAX                   (0xFFFFFFFFUL)

    /* ========================================================= */
    /* MIN / MAX / CLAMP                                         */
    /* ========================================================= */

#define MIN(a, b)                 (((a) < (b)) ? (a) : (b))
#define MAX(a, b)                 (((a) > (b)) ? (a) : (b))

#define CLAMP(val, min, max) \
    (((val) < (min)) ? (min) : (((val) > (max)) ? (max) : (val)))

#define ABS(x) (((x) < 0) ? -(x) : (x))

    /* ========================================================= */
    /* BIT MANIPULATION                                          */
    /* ========================================================= */
#ifndef BIT
    #define BIT(n) (1UL << (n))
#endif /* #ifndef BIT */
#define BIT_0             (1UL << 0)
#define BIT_1             (1UL << 1)
#define BIT_2             (1UL << 2)
#define BIT_3             (1UL << 3)
#define BIT_4             (1UL << 4)
#define BIT_5             (1UL << 5)
#define BIT_6             (1UL << 6)
#define BIT_7             (1UL << 7)
#define BIT_8             (1UL << 8)
#define BIT_9             (1UL << 9)
#define BIT_10            (1UL << 10)
#define BIT_11            (1UL << 11)
#define BIT_12            (1UL << 12)
#define BIT_13            (1UL << 13)
#define BIT_14            (1UL << 14)
#define BIT_15            (1UL << 15)

#define SET_BIT(reg, bit) ((reg) |= BIT(bit))
#define CLR_BIT(reg, bit) ((reg) &= ~BIT(bit))
#define TOG_BIT(reg, bit) ((reg) ^= BIT(bit))
#define GET_BIT(reg, bit) (((reg) >> (bit)) & 1U)

#include <stdint.h>

/* ========================================================= */
/* BYTE EXTRACTION (8 / 16 / 32 / 64 BIT)                    */
/* ========================================================= */

/* Extract 8-bit values */
#define LOW_BYTE8(x)   ((uint8_t)((uint64_t)(x) & UINT64_C(0xFF)))
#define HIGH_BYTE8(x)  ((uint8_t)(((uint64_t)(x) >> 8) & UINT64_C(0xFF)))

/* Extract 16-bit values */
#define LOW_WORD16(x)  ((uint16_t)((uint64_t)(x) & UINT64_C(0xFFFF)))
#define HIGH_WORD16(x) ((uint16_t)(((uint64_t)(x) >> 16) & UINT64_C(0xFFFF)))

/* Extract 32-bit values */
#define LOW_DWORD32(x) ((uint32_t)((uint64_t)(x) & UINT64_C(0xFFFFFFFF)))
#define HIGH_DWORD32(x) \
    ((uint32_t)(((uint64_t)(x) >> 32) & UINT64_C(0xFFFFFFFF)))

/* ========================================================= */
/* VALUE CONSTRUCTION (16 / 32 / 64 BIT)                     */
/* ========================================================= */

/* Combine two 8-bit values into 16-bit */
#define MAKE_WORD16(lsb, msb) \
    ((uint16_t)(((uint16_t)(uint8_t)(msb) << 8) | (uint8_t)(lsb)))

/* Combine two 16-bit values into 32-bit */
#define MAKE_DWORD32(lsw, msw) \
    ((uint32_t)(((uint32_t)(uint16_t)(msw) << 16) | (uint16_t)(lsw)))

/* Combine two 32-bit values into 64-bit */
#define MAKE_QWORD64(lsdw, msdw) \
    ((uint64_t)(((uint64_t)(uint32_t)(msdw) << 32) | (uint32_t)(lsdw)))

    /* ========================================================= */
    /* MEMORY / ALIGNMENT                                        */
    /* ========================================================= */

#define ALIGN_UP(val, align)     (((val) + ((align) - 1U)) & ~((align) - 1U))
#define ALIGN_DOWN(val, align)   ((val) & ~((align) - 1U))

    /* ========================================================= */
    /* UTILITY MACROS                                            */
    /* ========================================================= */

#define ARRAY_SIZE_STATIC(arr)   (sizeof(arr) / sizeof((arr)[0]))

#define UNUSED(x)                ((void)(x))


    /* ========================================================= */
    /* STATIC ASSERT                                             */
    /* ========================================================= */

#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

#ifdef c_plusplus
}
#endif

#endif /* CORE_H */
