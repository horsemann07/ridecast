/*
 *  Copyright (C) 2022 Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "lwip/tcpip.h"
#include "arch/sys_arch.h"

#include "osi_kernel.h"


/********************Structure Definition****************/



/********************Static Functions********************/
#if SYS_LIGHTWEIGHT_PROT
static sys_mutex_t sys_arch_protect_mutex;
static sys_prot_t sys_arch_protect_nesting;
#endif


/*****************************************************/
/********************Lib Functions********************/
/*****************************************************/

/*------------------- init -------------------*/
void sys_init() 
{
#if SYS_LIGHTWEIGHT_PROT
  OsiReturnVal_e ret;
  /* initialize sys_arch_protect global mutex */
  ret = osi_LockObjCreate((OsiLockObj_t*)&sys_arch_protect_mutex);
  LWIP_ASSERT("failed to create sys_arch_protect mutex",
    ret == OSI_OK);
#endif /* SYS_LIGHTWEIGHT_PROT && LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX */
}

/*------------------ Time --------------------*/

/* Return time is millisecs */
u32_t sys_now()
{
    return osi_GetTimeMS();
}

/* Returns time in clock ticks */
u32_t sys_jiffies() 
{
    return osi_GetTimeMS();
}

/*------------------- Semaphores -------------------*/

/* Creates new semaphore */
err_t sys_sem_new(sys_sem_t *sem, u8_t initial_count)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("initial_count invalid (not 0 or 1)",
      (initial_count == 0) || (initial_count == 1));

    osi_SyncObjCreate(&(sem->sem));
    if(&(sem->sem) == NULL) {
      SYS_STATS_INC(sem.err);
      return ERR_MEM;
    }
    SYS_STATS_INC_USED(sem);

    if(initial_count == 1) {
      OsiReturnVal_e ret = osi_LockObjUnlock(&(sem->sem));
      LWIP_ASSERT("sys_sem_new: initial give failed", ret == OSI_OK);
    }
    return ERR_OK;
}

/* Free semaphore */
void sys_sem_free(sys_sem_t *sem)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->sem != NULL", sem->sem != NULL);

    SYS_STATS_DEC(sem.used);
    osi_LockObjDelete(&(sem->sem));
    sem->sem = NULL;
}

/* Posts to a semaphore*/
void sys_sem_signal(sys_sem_t *sem)
{
    OsiReturnVal_e ret;
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->sem != NULL", sem->sem != NULL);

    ret = osi_LockObjUnlock(&(sem->sem));
    /* queue full is OK, this is a signal only... */
    LWIP_ASSERT("sys_sem_signal: sane return value",
      (ret == OSI_OK));
}

/*
 * Pends a semaphore for a given period.
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout_ms)
{
    OsiReturnVal_e ret;
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->sem != NULL", sem->sem != NULL);

    if(!timeout_ms){
      /* wait infinite */
      ret = osi_SyncObjWait(&(sem->sem), OSI_WAIT_FOREVER);
      LWIP_ASSERT("taking semaphore failed", ret == OSI_OK);
    } else {
      ret = osi_SyncObjWait(&(sem->sem), timeout_ms);
      LWIP_ASSERT("taking semaphore failed", ((ret == OSI_OK) || (ret == OSI_TIMEOUT)) );
    }

    /* Old versions of lwIP required us to return the time waited.
       This is not the case any more. Just returning != SYS_ARCH_TIMEOUT
       here is enough. */
    if (ret == OSI_TIMEOUT){
      return (SYS_ARCH_TIMEOUT);
    } else {
      return (1);
    }
}

void
sys_arch_msleep(u32_t delay_ms)
{
    os_sleep(0, delay_ms * 1000);
}


/*------------------- Mutexes -------------------*/

/* Create a new mutex.  */
err_t sys_mutex_new(sys_mutex_t *mutex)
{
    OsiReturnVal_e ret;
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
  
    osi_LockObjCreate(&(mutex->mut));
    if(mutex->mut == NULL) {
        SYS_STATS_INC(mutex.err);
        return ERR_MEM;
    }
    SYS_STATS_INC_USED(mutex);
    return ERR_OK;
}

/* Frees a mutex */
void sys_mutex_free(sys_mutex_t *mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);

    SYS_STATS_DEC(mutex.used);
    osi_LockObjDelete(&(mutex->mut));
    mutex->mut = NULL;
}

/* Locks a mutex */
void sys_mutex_lock(sys_mutex_t *mutex)
{
    OsiReturnVal_e ret;
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);

    // sys_arch_sem_wait(mutex, 0);
    // ret = xSemaphoreTakeRecursive((QueueHandle_t )mutex->mut, portMAX_DELAY);
    ret = osi_LockObjLock(&(mutex->mut), OSI_WAIT_FOREVER); // TODO - Check if blocking 
    LWIP_ASSERT("failed to take the mutex", ret == OSI_OK);
}

/* Unlocks a mutex */
void sys_mutex_unlock(sys_mutex_t *mutex)
{
    OsiReturnVal_e ret;
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);

    ret = osi_LockObjUnlock(&(mutex->mut));
    LWIP_ASSERT("failed to give the mutex", ret == OSI_OK);
}

/*------------------- Mailboxes -------------------*/

/* Creates a new mailbox */
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    LWIP_ASSERT("size > 0", size > 0);

    osi_MsgQCreate(&(mbox->mbx), "lwip_port_1", sizeof(void *), size); // TODO: Check
    //mbox->mbx = xQueueCreate((UBaseType_t)size, sizeof(void *));
    if(mbox->mbx == NULL) {
      SYS_STATS_INC(mbox.err);
      return ERR_MEM;
    }
    SYS_STATS_INC_USED(mbox);
    return ERR_OK;
}

/* Frees a mailbox */
void sys_mbox_free(sys_mbox_t *mbox)
{
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);

// #if LWIP_FREERTOS_CHECK_QUEUE_EMPTY_ON_FREE
//   {
//     UBaseType_t msgs_waiting = uxQueueMessagesWaiting((QueueHandle_t )mbox->mbx);
//     LWIP_ASSERT("mbox quence not empty", msgs_waiting == 0);

//     if (msgs_waiting != 0) {
//       SYS_STATS_INC(mbox.err);
//     }
//   }
// #endif

    osi_MsgQDelete(&(mbox->mbx));
    //vQueueDelete((QueueHandle_t )mbox->mbx);

    SYS_STATS_DEC(mbox.used);
}

/* Posts to a mailbox */
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    OsiReturnVal_e ret;
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);

    ret = osi_MsgQWrite(&(mbox->mbx), &msg, OSI_WAIT_FOREVER,OSI_FLAG_NOT_FROM_INTR);

    LWIP_ASSERT("mbox post failed", ret == OSI_OK);
}

/* Post and return whether post was successful or not */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    OsiReturnVal_e ret;
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);

    ret = osi_MsgQWrite(&(mbox->mbx), &msg, OSI_NO_WAIT,OSI_FLAG_NOT_FROM_INTR);
    if (ret == OSI_OK) {
      return ERR_OK;
    } else {
      LWIP_ASSERT("mbox trypost failed", ret == OSI_OPERATION_FAILED);
      SYS_STATS_INC(mbox.err);
      return ERR_MEM;
    }
}

err_t
sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    return sys_mbox_trypost(mbox, msg);
}

/* Fetches from the mailbox and return time taken to do so */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout_ms)
{
    OsiReturnVal_e ret;
    void *msg_dummy;
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);

    if (!msg) {
      msg = &msg_dummy;
    }

    if (!timeout_ms) {
      /* wait infinite */
      ret = osi_MsgQRead(&(mbox->mbx), &(*msg), OSI_WAIT_FOREVER);
      LWIP_ASSERT("mbox fetch failed", ret == OSI_OK);
    } else {
    ret = osi_MsgQRead(&(mbox->mbx), &(*msg), timeout_ms);
    if (ret == OSI_OPERATION_FAILED) {
      /* timed out */
      *msg = NULL;
      return SYS_ARCH_TIMEOUT;
    }
    LWIP_ASSERT("mbox fetch failed", ret == OSI_OK);
  }

  /* Old versions of lwIP required us to return the time waited.
     This is not the case any more. Just returning != SYS_ARCH_TIMEOUT
     here is enough. */
  return 1;
}

/* Fetches from a mailbox and returns error if fetch fails */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    OsiReturnVal_e ret;
    void *msg_dummy;
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);

    if (!msg) {
      msg = &msg_dummy;
    }

    ret = osi_MsgQRead(&(mbox->mbx), &(*msg), OSI_NO_WAIT);
    if (ret == OSI_OPERATION_FAILED) {
      *msg = NULL;
      return SYS_MBOX_EMPTY;
    }
    LWIP_ASSERT("mbox fetch failed", ret == OSI_OK);

    /* Old versions of lwIP required us to return the time waited.
       This is not the case any more. Just returning != SYS_ARCH_TIMEOUT
       here is enough. */
    return 1;
}


/*------------------- Threads -------------------*/

/* create new thread */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
  OsiReturnVal_e ret;
  OsiThread_t   thread_handler;
  sys_thread_t lwip_thread;

  //TaskHandle_t rtos_task;
  //size_t rtos_stacksize;

  LWIP_ASSERT("invalid stacksize", stacksize > 0);
  // TODO Ask
// #if LWIP_FREERTOS_THREAD_STACKSIZE_IS_STACKWORDS
//   rtos_stacksize = (size_t)stacksize;
// #else
//   rtos_stacksize = (size_t)stacksize / sizeof(StackType_t);
// #endif

  /* lwIP's lwip_thread_fn matches FreeRTOS' TaskFunction_t, so we can pass the
     thread function without adaption here. */
  ret = osi_ThreadCreate(&thread_handler, (char*)name, stacksize, prio, thread, arg);
  //ret = xTaskCreate(thread, name, (configSTACK_DEPTH_TYPE)rtos_stacksize, arg, prio, &rtos_task);
  LWIP_ASSERT("task creation failed", ret == OSI_OK);

  lwip_thread.thread_handle = thread_handler;
  return lwip_thread;
}

/*
 * threadFunc is void func pointer with void* arg
 * threadArg is void*
 */

// typedef void Thread_FuncPtr(void*);

// Void taskFunc(UArg threadFunc, UArg threadArg)
// {
//     ((Thread_FuncPtr*)(threadFunc)) ((void*)threadArg);
// }

// /* Initializes thread */
// sys_thread_t sys_thread_new(const char *name, void (* thread)(void *arg), void *arg, int stacksize, int prio)
// {
//     sys_thread_t newThread = (sys_thread_t)mem_malloc(sizeof(struct sys_thread));

//     if (newThread != NULL)
//     {
//         Error_init(&(newThread->eb));

//         /* Init thread params */
//         Task_Params_init(&(newThread->taskParams));
//         newThread->taskParams.instance->name = name;
//         newThread->taskParams.stackSize = stacksize;
//         newThread->taskParams.priority = prio;
//         newThread->taskParams.arg0 = (UArg)thread;
//         newThread->taskParams.arg1 = (UArg)arg;

//         newThread->taskHandle = Task_create(taskFunc, &(newThread->taskParams), &(newThread->eb));
//     }

//     return newThread;
// }

/*-----------------------Thread protection---------------------*/
/*
 * Protects the thread. Ported from the linux port shipped with the lwip-contrib
 */

#if SYS_LIGHTWEIGHT_PROT

sys_prot_t sys_arch_protect(void)
{
//#if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
  OsiReturnVal_e ret;
  //LWIP_ASSERT("sys_arch_protect_mutex != NULL", sys_arch_protect_mutex != NULL);

  //ret = xSemaphoreTakeRecursive(sys_arch_protect_mutex, portMAX_DELAY);
  ret = osi_LockObjLock(&(sys_arch_protect_mutex.mut), OSI_WAIT_FOREVER); // TODO - Check if blocking 
  LWIP_ASSERT("sys_arch_protect failed to take the mutex", ret == OSI_OK);
//#else /* LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX */
 // taskENTER_CRITICAL(); 
//#endif /* LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX */
// #if LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
//   {
    /* every nested call to sys_arch_protect() returns an increased number */
    //sys_prot_t ret = sys_arch_protect_nesting;
    sys_arch_protect_nesting++;
    //LWIP_ASSERT("sys_arch_protect overflow", sys_arch_protect_nesting > ret);
    //return ret;
    return sys_arch_protect_nesting-1;
  // }
// #else
//   return 1;
// #endif
}

void sys_arch_unprotect(sys_prot_t pval)
{
// #if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
    OsiReturnVal_e ret;
// #endif
// #if LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
  LWIP_ASSERT("unexpected sys_arch_protect_nesting", sys_arch_protect_nesting > 0);
  sys_arch_protect_nesting--;
  LWIP_ASSERT("unexpected sys_arch_protect_nesting", sys_arch_protect_nesting == pval);
// #endif

// #if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
  LWIP_ASSERT("sys_arch_protect_mutex.mut != NULL", sys_arch_protect_mutex.mut != NULL);

  //ret = xSemaphoreGiveRecursive(sys_arch_protect_mutex);
  ret = osi_LockObjUnlock(&(sys_arch_protect_mutex.mut));
  LWIP_ASSERT("sys_arch_unprotect failed to give the mutex", ret == OSI_OK);
// #else /* LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX */
//   taskEXIT_CRITICAL();
// #endif /* LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX */
//   LWIP_UNUSED_ARG(pval);
}

#endif /* SYS_LIGHTWEIGHT_PROT */

//#if LWIP_FREERTOS_CHECK_CORE_LOCKING
#if LWIP_TCPIP_CORE_LOCKING

/** Flag the core lock held. A counter for recursive locks. */
static u8_t lwip_core_lock_count;
static OsiTaskHandle lwip_core_lock_holder_thread;

void sys_lock_tcpip_core(void)
{
   sys_mutex_lock(&lock_tcpip_core);
   if (lwip_core_lock_count == 0) {
     lwip_core_lock_holder_thread = osi_GetCurrentThread();
   }
   lwip_core_lock_count++;
}

void sys_unlock_tcpip_core(void)
{
   lwip_core_lock_count--;
   if (lwip_core_lock_count == 0) {
       lwip_core_lock_holder_thread = 0;
   }
   sys_mutex_unlock(&lock_tcpip_core);
}

#endif /* LWIP_TCPIP_CORE_LOCKING */

#if !NO_SYS
static OsiTaskHandle lwip_tcpip_thread;
#endif

void sys_mark_tcpip_thread(void)
{
#if !NO_SYS
  lwip_tcpip_thread = osi_GetCurrentThread();
#endif
}

void sys_check_core_locking(void)
{
  /* Embedded systems should check we are NOT in an interrupt context here */
  /* E.g. core Cortex-M3/M4 ports:
         configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 );

     Instead, we use more generic FreeRTOS functions here, which should fail from ISR: */
//  taskENTER_CRITICAL();
//  taskEXIT_CRITICAL();

#if !NO_SYS
  if (lwip_tcpip_thread != 0) {
    OsiTaskHandle current_thread = osi_GetCurrentThread();

#if LWIP_TCPIP_CORE_LOCKING
    LWIP_ASSERT("Function called without core lock",
                current_thread == lwip_core_lock_holder_thread && lwip_core_lock_count > 0);
#else /* LWIP_TCPIP_CORE_LOCKING */
    LWIP_ASSERT("Function called from wrong thread", current_thread == lwip_tcpip_thread);
#endif /* LWIP_TCPIP_CORE_LOCKING */
  }
#endif /* !NO_SYS */
}
//#endif /* LWIP_FREERTOS_CHECK_CORE_LOCKING*/

