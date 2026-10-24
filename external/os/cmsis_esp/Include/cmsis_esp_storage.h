
/* ============================================================================
 * File        : cmsis_esp_storage.h
 * Description : ESP-IDF 5.5 CMSIS-RTOS2 static allocation storage types
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Change Log (newest first):
 * 2026-10-23  Jha Raghav  Initial storage contracts for ESP-IDF port
 * ============================================================================ */

#ifndef CMSIS_ESP_STORAGE_H_
#define CMSIS_ESP_STORAGE_H_

/* -------------------------------------------------------------------------- */
/* Standard Library Headers                                                   */
/* -------------------------------------------------------------------------- */

#include <stdint.h>
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* CMSIS-RTOS2 Timer Callback Type                                            */
/* -------------------------------------------------------------------------- */
/*
 * cmsis_os2.h includes this storage header before it defines osTimerFunc_t.
 * Keep a single shared typedef so both headers agree on the signature without
 * entering a circular include loop.
 */
#ifndef CMSIS_ESP_OS_TIMER_FUNC_T_DEFINED
#define CMSIS_ESP_OS_TIMER_FUNC_T_DEFINED
typedef void (*osTimerFunc_t)(void* argument);
#endif

/* -------------------------------------------------------------------------- */
/* FreeRTOS Headers                                                           */
/* -------------------------------------------------------------------------- */

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

/* -------------------------------------------------------------------------- */
/* C++ Compatibility                                                          */
/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C"
{
#endif

    /* -------------------------------------------------------------------------- */
    /* Timer Callback Context */
    /* -------------------------------------------------------------------------- */
    /**
     * @brief Callback context associated with a CMSIS software timer.
     *
     * @details
     * Stores the application callback, callback argument,
     * and allocation ownership information.
     *
     * The FreeRTOS timer ID references this context.
     *
     * @note
     * A nonzero dynamic_alloc value indicates that
     * the context was dynamically allocated.
     *
     * @warning
     * The context must remain valid until timer deletion
     * and all associated timer daemon operations complete.
     */
    typedef struct
    {
        osTimerFunc_t func;    /**< Application timer callback. */
        void* arg;             /**< Application callback argument. */
        uint8_t dynamic_alloc; /**< Dynamic allocation ownership flag. */
    } cmsis_esp_timer_context_t;

    /* -------------------------------------------------------------------------- */
    /* Static Timer Storage */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Storage for a statically allocated CMSIS software timer.
     *
     * @details
     * Combines the native FreeRTOS timer control block
     * with the CMSIS timer callback context.
     *
     * Applications may allocate an instance of this structure
     * and supply its address through osTimerAttr_t.cb_mem.
     *
     * @note
     * The control block capacity must be at least
     * sizeof(cmsis_esp_static_timer_t).
     *
     * @warning
     * The application must preserve this storage until
     * the timer has been completely deleted.
     */
    typedef struct
    {
        StaticTimer_t cb; /**< FreeRTOS timer control block. */
        cmsis_esp_timer_context_t callback; /**< CMSIS timer callback context. */
    } cmsis_esp_static_timer_t;

    /* -------------------------------------------------------------------------- */
    /* Mutex Metadata */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Internal metadata associated with a CMSIS mutex.
     *
     * @details
     * Stores the native FreeRTOS mutex handle,
     * recursive mutex configuration, ownership information,
     * and optional mutex name.
     *
     * The CMSIS mutex identifier references this metadata.
     *
     * @note
     * A nonzero dynamic_alloc value indicates that
     * the metadata was dynamically allocated.
     */
    typedef struct
    {
        SemaphoreHandle_t handle; /**< Native FreeRTOS mutex handle. */
        uint8_t recursive;        /**< Recursive mutex configuration flag. */
        uint8_t dynamic_alloc;    /**< Metadata allocation ownership flag. */
        const char* name;         /**< Optional mutex name. */
    } cmsis_esp_mutex_meta_t;

    /* -------------------------------------------------------------------------- */
    /* Static Mutex Storage */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Storage for a statically allocated CMSIS mutex.
     *
     * @details
     * Combines the CMSIS mutex metadata and native
     * FreeRTOS semaphore control block.
     *
     * Applications may supply an instance of this structure
     * through osMutexAttr_t.cb_mem.
     *
     * @note
     * The control block capacity must be at least
     * sizeof(cmsis_esp_static_mutex_t).
     *
     * @warning
     * The storage must remain valid until the mutex
     * has been deleted and is no longer referenced.
     */
    typedef struct
    {
        cmsis_esp_mutex_meta_t meta; /**< CMSIS mutex metadata. */
        StaticSemaphore_t cb;        /**< FreeRTOS mutex control block. */
    } cmsis_esp_static_mutex_t;

    /* -------------------------------------------------------------------------- */
    /* Message Queue Metadata */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Internal metadata associated with a CMSIS message queue.
     *
     * @details
     * Stores the native FreeRTOS queue handle,
     * configured message count and size,
     * optional queue name, and metadata ownership.
     *
     * @note
     * A nonzero meta_dynamic value indicates that
     * the metadata was dynamically allocated.
     *
     * @note
     * Queue message priorities are not stored by this
     * metadata structure.
     */
    typedef struct
    {
        QueueHandle_t handle; /**< Native FreeRTOS queue handle. */
        uint32_t msg_count;   /**< Maximum number of queue messages. */
        uint32_t msg_size;    /**< Size of each message in bytes. */
        uint8_t meta_dynamic; /**< Metadata allocation ownership flag. */
        const char* name;     /**< Optional queue name. */
    } cmsis_esp_queue_meta_t;

    /* -------------------------------------------------------------------------- */
    /* Static Message Queue Storage */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Control block storage for a static CMSIS message queue.
     *
     * @details
     * Combines CMSIS queue metadata with the native
     * FreeRTOS queue control block.
     *
     * Message payload storage is supplied separately
     * through osMessageQueueAttr_t.mq_mem.
     *
     * @note
     * The control block capacity must be at least
     * sizeof(cmsis_esp_static_queue_t).
     *
     * @warning
     * Both control block storage and message payload
     * storage must remain valid until queue deletion.
     */
    typedef struct
    {
        cmsis_esp_queue_meta_t meta; /**< CMSIS queue metadata. */
        StaticQueue_t cb;            /**< FreeRTOS queue control block. */
    } cmsis_esp_static_queue_t;

    /* -------------------------------------------------------------------------- */
    /* Memory Pool Control Structure */
    /* -------------------------------------------------------------------------- */

    /**
     * @brief Internal control structure for a CMSIS memory pool.
     *
     * @details
     * Maintains the synchronization semaphore,
     * allocated memory region, block usage information,
     * free-list pointer, and pool configuration.
     *
     * The structure also records ownership information
     * for control block and data storage.
     *
     * @note
     * A nonzero cb_dynamic value indicates dynamically
     * allocated control block storage.
     *
     * @note
     * A nonzero data_dynamic value indicates dynamically
     * allocated memory pool data storage.
     *
     * @warning
     * These fields are internal implementation details.
     * Application code must not modify them directly.
     */
    typedef struct
    {
        SemaphoreHandle_t sem;        /**< Pool synchronization semaphore. */
        StaticSemaphore_t static_sem; /**< Static semaphore control block. */

        uint8_t* memory;              /**< Pool data storage base address. */
        uint8_t* used;        /**< Per-block allocation tracking storage. */
        void* free_head;      /**< Head of the available block list. */

        uint32_t capacity;    /**< Total number of memory blocks. */
        uint32_t block_bytes; /**< Requested payload size per block. */
        uint32_t stride;      /**< Aligned storage size per block. */
        uint32_t next_unused; /**< Index of the next unused block. */

        const char* name;     /**< Optional memory pool name. */

        uint8_t cb_dynamic;   /**< Control block allocation ownership. */
        uint8_t data_dynamic; /**< Pool data allocation ownership. */
    } cmsis_esp_pool_control_t;

/* -------------------------------------------------------------------------- */
/* Memory Pool Control Block Size                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Calculate memory pool control block storage capacity.
 *
 * @details
 * Calculates the number of bytes required for the
 * memory pool control structure and one allocation
 * tracking byte per pool block.
 *
 * @param[in] count
 * Number of memory pool blocks.
 *
 * @return
 * Required control block storage capacity in bytes.
 *
 * @warning
 * The caller must ensure that the addition does not
 * overflow size_t and that the supplied memory
 * satisfies the control structure alignment requirements.
 */
#define CMSIS_ESP_POOL_CB_BYTES(count) \
    (sizeof(cmsis_esp_pool_control_t) + (size_t)(count))

/* -------------------------------------------------------------------------- */
/* Memory Pool Block Stride                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Calculate the aligned storage stride for a pool block.
 *
 * @details
 * Ensures each block can store at least one pointer-sized
 * free-list link.
 *
 * The resulting block stride is rounded upward to
 * a multiple of sizeof(void *).
 *
 * @param[in] size
 * Requested memory pool block size in bytes.
 *
 * @return
 * Calculated storage stride in bytes.
 *
 * @warning
 * The rounding expression assumes that sizeof(void *)
 * is a power of two.
 *
 * @warning
 * The caller must ensure that the intermediate addition
 * does not overflow size_t.
 */
#define CMSIS_ESP_POOL_STRIDE_BYTES(size)                                   \
    (((((size_t)(size) > sizeof(void*)) ? (size_t)(size) : sizeof(void*)) + \
      (sizeof(void*) - 1U)) &                                               \
     ~(sizeof(void*) - 1U))

/* -------------------------------------------------------------------------- */
/* Memory Pool Data Size                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Calculate total memory pool data storage capacity.
 *
 * @details
 * Multiplies the configured number of memory blocks
 * by the aligned storage stride of each block.
 *
 * @param[in] count
 * Number of memory pool blocks.
 *
 * @param[in] size
 * Requested payload size per memory block.
 *
 * @return
 * Total memory pool data storage requirement in bytes.
 *
 * @warning
 * The caller must verify that multiplication does not
 * overflow size_t before allocating the memory region.
 */
#define CMSIS_ESP_POOL_DATA_BYTES(count, size) \
    ((size_t)(count) * CMSIS_ESP_POOL_STRIDE_BYTES(size))

    /* -------------------------------------------------------------------------- */
    /* C++ Compatibility */
    /* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

/* -------------------------------------------------------------------------- */

#endif /* CMSIS_ESP_STORAGE_H_ */
