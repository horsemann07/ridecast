
#ifndef BSP_CORE_H
#define BSP_CORE_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* ========================================================= */
    /* GENERIC VALUES                                            */
    /* ========================================================= */

#define BSP_ENABLE  (1U)
#define BSP_DISABLE (0U)

#define BSP_TRUE    (1U)
#define BSP_FALSE   (0U)

#define BSP_NULL    ((void*)0)

    /* ========================================================= */
    /* MATHEMATICAL CONSTANTS (Arduino Style + Embedded Ready)   */
    /* ========================================================= */

#define BSP_MATH_PI      (3.14159265358979323846)
#define BSP_MATH_TWO_PI  (6.28318530717958647692)
#define BSP_MATH_HALF_PI (1.57079632679489661923)
#define BSP_MATH_E       (2.71828182845904523536)

/* Degree / Radian Conversion */
#define BSP_DEG_TO_RAD(x) ((x) * (BSP_MATH_PI / 180.0))
#define BSP_RAD_TO_DEG(x) ((x) * (180.0 / BSP_MATH_PI))

    /* ========================================================= */
    /* PIN / HARDWARE DEFAULTS                                   */
    /* ========================================================= */

#define GPIO_PIN_UNUSED (0xFFFFFFFFUL)

    /* ========================================================= */
    /* SIZE DEFINITIONS                                          */
    /* ========================================================= */

#define BSP_SIZE_1B (1U)
#define BSP_SIZE_1K (1024U)
#define BSP_SIZE_1M (1024U * 1024U)

    /* ========================================================= */
    /* STACK SIZES (CMSIS expects BYTES)                         */
    /* ========================================================= */

#define THREAD_STACK_512B (512U)
#define THREAD_STACK_1K   (1U * BSP_SIZE_1K)
#define THREAD_STACK_1_5K (1536U)
#define THREAD_STACK_2K   (2U * BSP_SIZE_1K)
#define THREAD_STACK_3K   (3U * BSP_SIZE_1K)
#define THREAD_STACK_4K   (4U * BSP_SIZE_1K)
#define THREAD_STACK_5K   (5U * BSP_SIZE_1K)
#define THREAD_STACK_6K   (6U * BSP_SIZE_1K)
#define THREAD_STACK_7K   (7U * BSP_SIZE_1K)
#define THREAD_STACK_8K   (8U * BSP_SIZE_1K)

/* Multiplier (512B blocks) */
#define THREAD_STACK_MULT_512B(x) ((x) * THREAD_STACK_512B)

    /* ========================================================= */
    /* TIME / DELAY                                              */
    /* ========================================================= */

#define BSP_SEC_TO_MS(sec)  ((sec) * 1000U)
#define BSP_MS_TO_SEC(ms)   ((ms) / 1000U)
#define BSP_MS_TO_TICKS(ms) ((uint32_t)(ms))

#define BSP_MIN_DELAY_MS    (1U)

    /* ========================================================= */
    /* LIMIT VALUES                                              */
    /* ========================================================= */

#define BSP_U8_MAX  (0xFFU)
#define BSP_U16_MAX (0xFFFFU)
#define BSP_U32_MAX (0xFFFFFFFFUL)

    /* ========================================================= */
    /* MIN / MAX / CLAMP                                         */
    /* ========================================================= */

#define BSP_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define BSP_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define BSP_CLAMP(val, min, max) \
    (((val) < (min)) ? (min) : (((val) > (max)) ? (max) : (val)))

#define BSP_ABS(x) (((x) < 0) ? -(x) : (x))

    /* ========================================================= */
    /* BIT MANIPULATION                                          */
    /* ========================================================= */

#define BSP_BIT(n)            (1UL << (n))
#define BSP_BIT_0             (1UL << 0)
#define BSP_BIT_1             (1UL << 1)
#define BSP_BIT_2             (1UL << 2)
#define BSP_BIT_3             (1UL << 3)
#define BSP_BIT_4             (1UL << 4)
#define BSP_BIT_5             (1UL << 5)
#define BSP_BIT_6             (1UL << 6)
#define BSP_BIT_7             (1UL << 7)
#define BSP_BIT_8             (1UL << 8)

#define BSP_SET_BIT(reg, bit) ((reg) |= BSP_BIT(bit))
#define BSP_CLR_BIT(reg, bit) ((reg) &= ~BSP_BIT(bit))
#define BSP_TOG_BIT(reg, bit) ((reg) ^= BSP_BIT(bit))
#define BSP_GET_BIT(reg, bit) (((reg) >> (bit)) & 1U)

    /* ========================================================= */
    /* BYTE / WORD OPERATIONS                                    */
    /* ========================================================= */

#define BSP_LOW_BYTE(x)         ((uint8_t)((x) & 0xFFU))
#define BSP_HIGH_BYTE(x)        ((uint8_t)(((x) >> 8) & 0xFFU))

#define BSP_MAKE_WORD(lsb, msb) (((uint16_t)(msb) << 8) | (lsb))

    /* ========================================================= */
    /* MEMORY / ALIGNMENT                                        */
    /* ========================================================= */

#define BSP_ALIGN_UP(val, align)   (((val) + ((align) - 1U)) & ~((align) - 1U))

#define BSP_ALIGN_DOWN(val, align) ((val) & ~((align) - 1U))

    /* ========================================================= */
    /* UTILITY MACROS                                            */
    /* ========================================================= */

#define BSP_ARRAY_SIZE_STATIC(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BSP_UNUSED(x)              ((void)(x))

    /* ========================================================= */
    /* DEBUG / ASSERT (OPTIONAL SWITCHABLE)                      */
    /* ========================================================= */

#ifndef BSP_ASSERT_ENABLE
    #define BSP_ASSERT_ENABLE (1U)
#endif

#if (BSP_ASSERT_ENABLE == 1U)
    #include <assert.h>
    #define BSP_ASSERT(x) assert(x)
#else
    #define BSP_ASSERT(x) ((void)0)
#endif

    /* ========================================================= */
    /* STATIC ASSERT                                             */
    /* ========================================================= */

#define BSP_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

    /* ========================================================= */

#ifdef __cplusplus
}
#endif

#endif /* BSP_CORE_H */
