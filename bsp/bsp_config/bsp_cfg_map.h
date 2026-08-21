
#ifndef BSP_CFG_MAP_H
#define BSP_CFG_MAP_H


    /**********************************************************************
     *
     * ALL THREADS (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add thread entries to app_bsp_thread_owner_t enum.
     * - Initialize corresponding os_thread_map_t entry in g_appThreadCfg  array.
     * - Thread handles are filled at runtime after thread creation.
     ***********************************************************************/
    typedef enum
    {
        /* --------------------------------------------------------
         *        BSP THREAD CONFIGURATION
         * -------------------------------------------------------- */
        BSP_THREAD_OWNER_LOGGER     = 0, /*< Logger thread. */
        BSP_THREAD_OWNER_UART_ASYNC = 1, /**< UART asynchronous thread. */
        BSP_THREAD_OWNER_MAX             /**< Maximum number of threads. */
    } app_bsp_thread_owner_t;


    /**********************************************************************
     * ALL QUEUES (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add queue entries to bsp_queue_owner_t enum.
     * - Initialize corresponding os_queue_map_t entry in g_appBspQueueObj  array.
     * - Queue handles are filled at runtime after queue creation.
     ***********************************************************************/
    typedef enum
    {
        BSP_WIFI_EVENT_QUEUE = 0, /*< WIFI event queue length. */
        BSP_QUEUE_OWNER_MAX       /*< Maximum number of queues. */
    } bsp_queue_owner_t;


    /**********************************************************************
     * ALL MUTEXES (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add mutex entries to bsp_mutex_owner_t enum.
     * - Initialize corresponding os_mutex_map_t entry in g_bspMutexCfg  array.
     * - Mutex handles are filled at runtime after mutex creation.
     ***********************************************************************/
    typedef enum
    {
        BSP_MUTEX_OWNER_LOG        = 0, /*< Log message mutex. */
        BSP_MUTEX_OWNER_WIFI       = 1, /*< WiFi module mutex. */
        BSP_MUTEX_OWNER_UART_ASYNC = 2, /*< UART asynchronous mutex. */
        BSP_MUTEX_OWNER_MAX             /*< Maximum number of mutexes. */
    } bsp_mutex_owner_t;


    /**********************************************************************
     * ALL SEMAPHORES (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add semaphore entries to bsp_semaphore_owner_t enum.
     * - Initialize corresponding os_semaphore_map_t entry in  g_appBspSemaphoreObj           array.
     * - Semaphore handles are filled at runtime after semaphore creation.
     ***********************************************************************/
    typedef enum
    {
        BSP_SEMAPHORE_OWNER_LOG  = 0, /*< Log message semaphore. */
        BSP_SEMAPHORE_OWNER_WIFI = 1, /*< WiFi module semaphore. */
        BSP_SEMAPHORE_OWNER_MAX       /*< Maximum number of semaphores. */
    } bsp_semaphore_owner_t;


    /**********************************************************************
     * ALL EVENT FLAGS (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add event flag entries to bsp_event_flags_owner_t enum.
     * - Initialize corresponding os_event_flags_map_t entry in
     * g_appBspEventFlagsObj array.
     * - Event flag handles are filled at runtime after event flag creation.
     ***********************************************************************/
    typedef enum
    {
        BSP_EVENT_FLAGS_OWNER_LOG  = 0, /*< Log message event flags. */
        BSP_EVENT_FLAGS_OWNER_WIFI = 1, /*< WiFi module event flags. */
        BSP_EVENT_FLAGS_OWNER_MAX       /*< Maximum number of event flags. */
    } bsp_event_flags_owner_t;


    /**********************************************************************
     * PERIPHERAL OWNER
     ***********************************************************************/
    /**
    * @brief BSP UART ownership identifiers.
    */
    typedef enum
    {
        BSP_UART_OWNER_CONSOLE = 0U, /**< Console UART used by LOG and CLI. */
        BSP_UART_OWNER_MAX             /**< Maximum number of UART owners. */

    } bsp_uart_owner_t;


    /**********************************************************************
     * EXTERNAL VARIABLES
     ***********************************************************************/
    extern os_thread_map_t g_bspThreadCfg[BSP_THREAD_OWNER_MAX];
    extern os_queue_map_t g_bspQueueCfg[BSP_QUEUE_OWNER_MAX];
    extern os_mutex_map_t g_bspMutexCfg[BSP_MUTEX_OWNER_MAX];
    extern os_semaphore_map_t g_bspSemaphoreCfg[BSP_SEMAPHORE_OWNER_MAX];
    extern os_event_flags_map_t g_bspEventFlagsCfg[BSP_EVENT_FLAGS_OWNER_MAX];

    /*---------------PERIPHERAL ----------------------*/
    extern bsp_uart_map_t g_bspUartCfg[BSP_UART_OWNER_MAX];

#endif /* BSP_CFG_MAP_H */