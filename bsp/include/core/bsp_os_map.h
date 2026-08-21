#ifndef BSP_OS_MAP_DEF_H
#define BSP_OS_MAP_DEF_H

#include "cmsis_os2.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ---------------------------------------------------------------
     * THREAD / TASK descriptor (NO function pointer)
     * --------------------------------------------------------------- */
    typedef struct
    {
        osThreadAttr_t attr; /* Full CMSIS thread attribute block    */
    } os_thread_map_t;

    /* ---------------------------------------------------------------
     * MESSAGE QUEUE descriptor
     * --------------------------------------------------------------- */
    typedef struct
    {
        uint16_t msg_count;        /* Max messages in queue            */
        uint16_t msg_size_bytes;   /* Size of one message in bytes     */
        osMessageQueueAttr_t attr; /* CMSIS queue attribute block      */
    } os_queue_map_t;

    /* ---------------------------------------------------------------
     * MUTEX descriptor
     * --------------------------------------------------------------- */
    typedef struct
    {
        osMutexAttr_t attr; /* Flags: recursive, prio-inherit, etc. */
    } os_mutex_map_t;

    /* ---------------------------------------------------------------
     * SEMAPHORE descriptor
     * --------------------------------------------------------------- */
    typedef struct
    {
        uint32_t max_count;     /* Maximum semaphore count          */
        uint32_t initial_count; /* Initial semaphore count          */
        osSemaphoreAttr_t attr; /* CMSIS semaphore attribute block  */
    } os_semaphore_map_t;

    /* ---------------------------------------------------------------
     * EVENT FLAGS descriptor
     * --------------------------------------------------------------- */
    typedef struct
    {
        uint32_t flags;          /* Initial event flags value        */
        osEventFlagsAttr_t attr; /* CMSIS event flags attribute block */
    } os_event_flags_map_t;


#ifdef __cplusplus
}
#endif

#endif /* BSP_OS_MAP_DEF_H */