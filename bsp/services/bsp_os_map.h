#ifndef BSP_OS_MAP_DEF_H
#define BSP_OS_MAP_DEF_H

#include "cmsis_os2.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ---------------------------------------------------------------
     * THREAD / TASK descriptor
     * --------------------------------------------------------------- */
    typedef struct
    {
        osThreadFunc_t func; /* Thread entry function              */
        void* arg;           /* Thread argument                    */
        osThreadAttr_t attr; /* CMSIS thread attribute block       */
        osThreadId_t handle; /* Thread handle - filled at runtime  */
    } os_thread_map_t;

    /* ---------------------------------------------------------------
     * MESSAGE QUEUE descriptor
     * --------------------------------------------------------------- */
    typedef struct
    {
        const uint16_t msg_count;        /* Max messages in queue            */
        const uint16_t msg_size_bytes;   /* Size of one message in bytes     */
        const osMessageQueueAttr_t attr; /* CMSIS queue attribute block      */
        osMessageQueueId_t handle;       /* Queue handle (filled at runtime) */
    } os_queue_map_t;

    /* ---------------------------------------------------------------
     * MUTEX descriptor
     * --------------------------------------------------------------- */
    typedef struct
    {
        const osMutexAttr_t attr; /* Flags: recursive, prio-inherit, etc. */
        osMutexId_t handle;       /* Mutex handle (filled at runtime)    */
    } os_mutex_map_t;

    /* ---------------------------------------------------------------
     * SEMAPHORE descriptor
     * --------------------------------------------------------------- */
    typedef struct
    {
        const uint32_t max_count;     /* Maximum semaphore count          */
        const uint32_t initial_count; /* Initial semaphore count          */
        const osSemaphoreAttr_t attr; /* CMSIS semaphore attribute block  */
        osSemaphoreId_t handle;       /* Semaphore handle (filled at runtime) */
    } os_semaphore_map_t;

    /* ---------------------------------------------------------------
     * EVENT FLAGS descriptor
     * --------------------------------------------------------------- */
    typedef struct
    {
        const uint32_t flags;          /* Initial event flags value        */
        const osEventFlagsAttr_t attr; /* CMSIS event flags attribute block */
        osEventFlagsId_t handle; /* Event flags handle (filled at runtime) */
    } os_event_flags_map_t;


#ifdef __cplusplus
}
#endif

#endif /* BSP_OS_MAP_DEF_H */