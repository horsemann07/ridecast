/*
 * Copyright (c) 2024, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "FreeRTOS.h"
#include <time.h>
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "portmacro.h"
#include <stdlib.h>
#include <osi_kernel.h>


/*******************************************************************************

    DEFINES and SETTINGS

********************************************************************************/
//#define PRINT_DBG_MALLOC_FREE
//#define PRINT_DBG_TOTAL_MALLOC_FREE
//#define ASSERT_ON_MALLOC_FAIL
#undef ASSERT_ON_MALLOC_FAIL
/* System tick period in microseconds */
#define TICK_PERIOD_US (1000000 / configTICK_RATE_HZ)

#define TICK_DURATION                     1  //should check the required tick duration

#define RELEASE_ASSERT //For debugging without any assert please open this define

/*******************************************************************************

    MISCELLANEOUS

********************************************************************************/
/*!
    \brief  convert tick to msec
*/
uint32_t TICK_TO_mSEC(uint32_t Tick)
{
    return ((Tick) * TICK_DURATION);
}

/*!
    \brief  convert msec to tick
*/
uint32_t mSEC_TO_TICK(uint32_t mSec)
{
    return ((mSec) / TICK_DURATION);
}

/*!
    \brief  assert function for the upper mac library

    \param  condition   -   if TRUE, assert will occur

    \return
    \note
    \warning
*/
void ASSERT_GENERAL(uint32_t condition)
{
#ifndef RELEASE_ASSERT
    uint32_t lr;

    lr = (uint32_t)__get_LR();

    if(FALSE == condition)
    {
        osi_EnterCriticalSection();
        Report("ASSSSEEEERRRTTT!!!! in 0x%x\n\r",lr);
        while(1);
    }
#endif
}


/*******************************************************************************

    Critical section

********************************************************************************/



/*!
    \brief  This function use to entering into critical section. It is provide a basic critical section implementation that works
     by simply disabling interrupts.
    \param  void
    \return - void
    \note
    \warning
*/
uint32_t osi_EnterCritical(void)
{
    portDISABLE_INTERRUPTS();
    vTaskSuspendAll();
    return 0;
}

/*!
    \brief  This function use to exit critical section
    \param  void
    \return - void
    \note
    \warning
*/
uint32_t osi_ExitCritical(uint32_t ulKey)
{
    xTaskResumeAll();
    portENABLE_INTERRUPTS();
    return 0;

}


/*******************************************************************************

    SYNC

********************************************************************************/


/*!
	\brief 	This function creates a sync object

	The sync object is used for synchronization between different thread or ISR and
	a thread.

	\param	pSyncObj	-	pointer to the sync object control block

	\return upon successful creation the function should return 0
			Otherwise, a negative value indicating the error code shall be returned
	\note
	\warning
*/
OsiReturnVal_e osi_SyncObjCreate(OsiSyncObj_t* pSyncObj)
{
    //Check for NULL
    if(NULL == pSyncObj)
    {
        return OSI_INVALID_PARAMS;
    }
    SemaphoreHandle_t *pl_SyncObj = (SemaphoreHandle_t *)pSyncObj;

    *pl_SyncObj = xSemaphoreCreateBinary();

    if((SemaphoreHandle_t)(*pSyncObj) != NULL)
    {
        return OSI_OK; 
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}

/*!
	\brief 	This function deletes a sync object

	\param	pSyncObj	-	pointer to the sync object control block

	\return upon successful deletion the function should return 0
			Otherwise, a negative value indicating the error code shall be returned
	\note
	\warning
*/
OsiReturnVal_e osi_SyncObjDelete(OsiSyncObj_t* pSyncObj)
{
	//Check for NULL
	if(NULL == pSyncObj)
	{
		return OSI_INVALID_PARAMS;
	}
    vSemaphoreDelete(*pSyncObj );
    return OSI_OK;
}

/*!
	\brief 		This function generates a sync signal for the object.

	All suspended threads waiting on this sync object are resumed

	\param		pSyncObj	-	pointer to the sync object control block

	\return 	upon successful signaling the function should return 0
				Otherwise, a negative value indicating the error code shall be returned
	\note		the function could be called from ISR context
	\warning
*/
OsiReturnVal_e osi_SyncObjSignal(OsiSyncObj_t* pSyncObj)
{
	//Check for NULL
	if(NULL == pSyncObj)
	{
		return OSI_INVALID_PARAMS;
	}

    if(pdTRUE != xSemaphoreGive( *pSyncObj ))
	{
        //In case of Semaphore, you are expected to get this if multiple sem
        // give is called before sem take
        return OSI_OK;
	}
	
    return OSI_OK;
}
/*!
	\brief 		This function generates a sync signal for the object
				from ISR context.

	All suspended threads waiting on this sync object are resumed

	\param		pSyncObj	-	pointer to the sync object control block

	\return 	upon successful signalling the function should return 0
				Otherwise, a negative value indicating the error code shall be returned
	\note		the function is called from ISR context
	\warning
*/
OsiReturnVal_e osi_SyncObjSignalFromISR(OsiSyncObj_t* pSyncObj)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	//Check for NULL
	if(NULL == pSyncObj)
	{
		return OSI_INVALID_PARAMS;
	}

	if(pdTRUE == xSemaphoreGiveFromISR( *pSyncObj, &xHigherPriorityTaskWoken ))
	{
		if( xHigherPriorityTaskWoken )
		{
		    /*
		     * taskYIELD (); cannot use this, as this cause entering critical section from ISR
		    */
		    extern void vPortYeildFromISR( uint32_t xSwitchRequired );
		    vPortYeildFromISR(pdTRUE);
		}
		return OSI_OK;
	}
	else
	{
		//In case of Semaphore, you are expected to get this if multiple sem
		// give is called before sem take
		return OSI_OK;
	}
}

/*!
	\brief 	This function waits for a sync signal of the specific sync object

	\param	pSyncObj	-	pointer to the sync object control block
	\param	Timeout		-	numeric value specifies the maximum number of mSec to
							stay suspended while waiting for the sync signal
							Currently, the simple link driver uses only two values:
								- OSI_WAIT_FOREVER
								- OSI_NO_WAIT

	\return upon successful reception of the signal within the timeout window return 0
			Otherwise, a negative value indicating the error code shall be returned
	\note
	\warning
*/
OsiReturnVal_e osi_SyncObjWait(OsiSyncObj_t* pSyncObj , OsiTime_t Timeout)
{
	//Check for NULL
    TickType_t frtsTimeout = portMAX_DELAY;

    if(OSI_WAIT_FOREVER != Timeout)
    {
        frtsTimeout = Timeout/portTICK_PERIOD_MS;
    }


	if(NULL == pSyncObj)
	{
		return OSI_INVALID_PARAMS;
	}
    if(pdTRUE == xSemaphoreTake( (SemaphoreHandle_t)*pSyncObj, frtsTimeout ))
    {
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}

/*!
	\brief 	This function clears a sync object

	\param	pSyncObj	-	pointer to the sync object control block

	\return upon successful clearing the function should return 0
			Otherwise, a negative value indicating the error code shall be returned
	\note
	\warning
*/
OsiReturnVal_e osi_SyncObjClear(OsiSyncObj_t* pSyncObj)
{
	//Check for NULL
	if(NULL == pSyncObj)
	{
		return OSI_INVALID_PARAMS;
	}

    if (OSI_OK == osi_SyncObjWait(pSyncObj,0) )
    {
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}

/*******************************************************************************

    LOCK

********************************************************************************/


/*!
    \brief  This function creates a Task.

    Creates a new Task and add it to the last of tasks that are ready to run

    \param  pEntry  -   pointer to the Task Function
    \param  pcName  -   Task Name String
    \param  usStackDepth    -   Stack Size in bytes
    \param  pvParameters    -   pointer to structure to be passed to the Task Function
    \param  uxPriority  -   Task Priority - 0 is lowest (IDLE) and (configMAX_PRIORITIES - 1) max
    \param  pTaskHandle -  output ptr filled with taskHandler

    \return upon successful creation the function should return 0
            Otherwise, a negative value indicating the error code shall be returned
    \note
    \warning
*/
OsiReturnVal_e osi_TaskCreate(P_OSI_TASK_ENTRY pEntry,const signed char * const pcName,
                              unsigned short usStackDepth, void *pvParameters,
                              unsigned long uxPriority, OsiTaskHandle* pTaskHandle)
{

    if(pdPASS == xTaskCreate( pEntry, (char const*)pcName,
                                (usStackDepth/(sizeof( portSTACK_TYPE ))),
                                pvParameters,(unsigned portBASE_TYPE)uxPriority,
                                (TaskHandle_t*)pTaskHandle ))
    {
        return OSI_OK;
    }

    return OSI_OPERATION_FAILED;
}


/*!
    \brief  This function Deletes a Task.

    Deletes a  Task and remove it from list of running task

    \param  pTaskHandle -   Task Handle

    \note
    \warning
*/
void osi_TaskDelete(OsiTaskHandle* pTaskHandle)
{
    vTaskDelete((TaskHandle_t)*pTaskHandle);
}


/*******************************************************************************

    LOCK

********************************************************************************/
/*!
	\brief 	This function creates a locking object.

	The locking object is used for protecting a shared resources between different
	threads. Allow only one thread to access a section of code at a time.

	\param	pLockObj	-	pointer to the locking object control block

	\return upon successful creation the function should return 0
			Otherwise, a negative value indicating the error code shall be returned
	\note
	\warning
*/
OsiReturnVal_e osi_LockObjCreate(OsiLockObj_t*  pLockObj)
{
    //Check for NULL
    if(NULL == pLockObj)
    {
            return OSI_INVALID_PARAMS;
    }
    *pLockObj = (OsiLockObj_t)xSemaphoreCreateMutex();
    if(pLockObj != NULL)
    {  
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}



/*!
	\brief 	This function deletes a locking object.

	\param	pLockObj	-	pointer to the locking object control block

	\return upon successful deletion the function should return 0
			Otherwise, a negative value indicating the error code shall be returned
	\note
	\warning
*/
OsiReturnVal_e osi_LockObjDelete(OsiLockObj_t* pLockObj)
{
    vSemaphoreDelete((SemaphoreHandle_t)*pLockObj );
    return OSI_OK;
}

/*!
	\brief 	This function locks a locking object.

	All other threads that call this function before this thread calls
	the osi_LockObjUnlock would be suspended

	\param	pLockObj	-	pointer to the locking object control block
	\param	Timeout		-	numeric value specifies the maximum number of mSec to
							stay suspended while waiting for the locking object
							Currently, the simple link driver uses only two values:
								- OSI_WAIT_FOREVER
								- OSI_NO_WAIT


	\return upon successful reception of the locking object the function should return 0
			Otherwise, a negative value indicating the error code shall be returned
	\note
	\warning
*/
OsiReturnVal_e osi_LockObjLock(OsiLockObj_t* pLockObj , OsiTime_t Timeout)
{
    //Check for NULL
    if(NULL == pLockObj)
    {
            return OSI_INVALID_PARAMS;
    }
    //Take Semaphore
    if(pdTRUE == xSemaphoreTake( *pLockObj, ( TickType_t ) (Timeout/portTICK_PERIOD_MS) ))
    {
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}

/*!
	\brief 	This function unlock a locking object.

	\param	pLockObj	-	pointer to the locking object control block

	\return upon successful unlocking the function should return 0
			Otherwise, a negative value indicating the error code shall be returned
	\note
	\warning
*/
OsiReturnVal_e osi_LockObjUnlock(OsiLockObj_t* pLockObj)
{
	//Check for NULL
	if(NULL == pLockObj)
	{
		return OSI_INVALID_PARAMS;
	}
	//Release Semaphore
    if(pdTRUE == xSemaphoreGive( *pLockObj ))
    {
    	return OSI_OK;
    }
    else
    {
    	return OSI_OPERATION_FAILED;
    }
}

/*******************************************************************************

    SEMAPHORE

********************************************************************************/

/*!
    \brief  This function creates a Semaphore object
            The Semaphore object is used for accounting resources which can be shared
            and obtained/released in different contexts, as well as having multicplicity of more than 1.

    \param  pSemaphoreObj        -   pointer to the Semaphore object (area allocated by the caller and remains in scope
                                                                      throughout the lifetime of the object).
            pSemaphoreObjName    -   pointer to the name of the semaphore object
            initialCount         -   count of the semaphore at creation.
                                     (the amount of resources available).
            maxCount             -   Max multiplicity of the semaphore.
                             Cannot release to a value which is larger than this value.

    \return upon successful unlocking the function should return 0
            Otherwise, a negative value indicating the error code shall be returned
*/
OsiReturnVal_e osi_SemaphoreObjCreate(OsiSemaphoreObj_t*      pSemaphoreObj     ,
                                      const char*             pSemaphoreObjName ,
                                      const uint32            initialCount      ,
                                      const uint32            maxCount          )
{
    //Check for NULL
    if(NULL == pSemaphoreObj)
    {
        return OSI_INVALID_PARAMS;
    }

    pSemaphoreObj->max_count = maxCount;

    SemaphoreHandle_t *pl_SemaphoreObj = (SemaphoreHandle_t *)pSemaphoreObj;

    *pl_SemaphoreObj = xSemaphoreCreateCounting(maxCount, initialCount);

    if((SemaphoreHandle_t)(pSemaphoreObj->Semaphore) != NULL)
    {
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}

/*!
    \brief This function deletes a Semaphore object.
           After this function is called, the area holding the semaphore objetc is no longer
           in used and can be reused.

    \param  pSemaphoreObj        -   pointer to the semaphore object to release

    \return upon successful unlocking the function should return 0
            Otherwise, a negative value indicating the error code shall be returned
*/
OsiReturnVal_e osi_SemaphoreObjDelete(OsiSemaphoreObj_t* pSemaphoreObj)
{
    //Check for NULL
    if(NULL == pSemaphoreObj)
    {
        return OSI_INVALID_PARAMS;
    }
    vSemaphoreDelete( pSemaphoreObj->Semaphore );
    return OSI_OK;
}

/*!
    \brief This function attempts to obtain a semaphore instance.
           If no instance is available, it waits for the designated timeout.

    \param pSemaphoreObj   -   pointer semaphore object to obtain instance of.
           Timeout         -   numeric value specifies the maximum number of mSec to
                               stay suspended while waiting for an instance to become available.
    \return upon successful unlocking the function should return 0
            Otherwise, a negative value indicating the error code shall be returned
*/
OsiReturnVal_e osi_SemaphoreObjObtain(OsiSemaphoreObj_t* pSemaphoreObj , OsiTime_t Timeout)
{
    //Check for NULL
    TickType_t frtsTimeout = portMAX_DELAY;

    if(OSI_WAIT_FOREVER != Timeout)
    {
        frtsTimeout = Timeout/portTICK_PERIOD_MS;
    }

    if(NULL == pSemaphoreObj)
    {
        return OSI_INVALID_PARAMS;
    }
    if(NULL == pSemaphoreObj->Semaphore)
    {
        return OSI_INVALID_PARAMS;
    }

    if(pdTRUE == xSemaphoreTake( (SemaphoreHandle_t)pSemaphoreObj->Semaphore, frtsTimeout ))
    {
        return OSI_OK;
    }
    else
    {
        return OSI_TIMEOUT;
    }
}

/*!
    \brief This function releases an instance of a semaphore.
           Note that if the available multiplicity of the semaphore is already at its
           maximal value, this function would fail.

    \param pSemaphoreObj        -   pointer to the semaphore object to release.

    \return upon successful unlocking the function should return 0
            Otherwise, a negative value indicating the error code shall be returned
*/
OsiReturnVal_e osi_SemaphoreObjRelease(OsiSemaphoreObj_t* pSemaphoreObj)
{
    //Check for NULL
    if(NULL == pSemaphoreObj)
    {
        return OSI_INVALID_PARAMS;
    }

    if(pdTRUE != xSemaphoreGive( pSemaphoreObj->Semaphore ))
    {
        //In case of Semaphore, you are expected to get this if multiple sem
        // give is called before sem take
        return OSI_OK;
    }

    return OSI_OK;
}

/*!
    \brief This function returns the current count of an instance of a semaphore.
           Note that if the available multiplicity of the semaphore is already at its
           maximal value, this function would fail.

    \param pSemaphoreObj        -   pointer to the semaphore object to get count for.

    \return the function should return the count of a semaphore
*/
uint32_t osi_SemaphoreObjGetCount(OsiSemaphoreObj_t* pSemaphoreObj)
{
    //Check for NULL
    if(NULL == pSemaphoreObj)
    {
        return OSI_INVALID_PARAMS;
    }

    return uxSemaphoreGetCount( pSemaphoreObj->Semaphore );
}

/*-----------------------------------------------------------*/
/*!
    \brief  Get time from the operating system
    \brief
    \brief
    \param
    \return - time in  seconds
    \note
    \warning
*/
uint64_t osi_GetDateTimeS()
{
    //struct timespec ts = {0};

    //clock_gettime(CLOCK_REALTIME,&ts);
    return 0;

}
/*-----------------------------------------------------------*/
/*!
    \brief  Set time to the operating system
    \brief
    \brief
    \param
    \return - time in  seconds
    \note
    \warning
*/
void osi_SetDateTimeS(uint64_t newtime)
{
    //struct timespec ts = {0};
    //ts.tv_sec = newtime;

    //clock_settime(CLOCK_REALTIME,&ts);

}
/*******************************************************************************

    MESSAGE QUEUE

********************************************************************************/
/*!
	\brief 	This function is used to create the MsgQ

	\param	pMsgQ	-	pointer to the message queue
	\param	pMsgQName	-	msg queue name
	\param	MsgSize	-	size of message on the queue
	\param	MaxMsgs	-	max. number of msgs that the queue can hold

	\return - OsiReturnVal_e
	\note
	\warning
*/
OsiReturnVal_e osi_MsgQCreate(OsiMsgQ_t*         pMsgQ ,
                              char*              pMsgQName,
                              uint32_t             MsgSize,
                              uint32_t             MaxMsgs)
{
	//Check for NULL
	if(NULL == pMsgQ)
	{
		return OSI_INVALID_PARAMS;
	}

	QueueHandle_t handle =0;

	//Create Queue
	handle = xQueueCreate( MaxMsgs, MsgSize );
	if (handle==0)
	{
		return OSI_OPERATION_FAILED;
	}

	*pMsgQ = (OsiMsgQ_t)handle;
	return OSI_OK;
}

/*!
	\brief 	This function is used to delete the MsgQ

	\param	pMsgQ	-	pointer to the message queue

	\return - OsiReturnVal_e
	\note
	\warning
*/
OsiReturnVal_e osi_MsgQDelete(OsiMsgQ_t* pMsgQ)
{
	//Check for NULL
	if(NULL == pMsgQ)
	{
		return OSI_INVALID_PARAMS;
	}
	vQueueDelete((QueueHandle_t) *pMsgQ );
    return OSI_OK;
}

/*!
	\brief 	This function is used to write data to the MsgQ

	\param	pMsgQ	-	pointer to the message queue
	\param	pMsg	-	pointer to the Msg strut to read into
	\param	Timeout	-	timeout to wait for the Msg to be available

	\return - OsiReturnVal_e
	\note
	\warning
*/
OsiReturnVal_e osi_MsgQWrite(OsiMsgQ_t* pMsgQ, void* pMsg , OsiTime_t Timeout, uint8_t flags)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	//Check for NULL
	if(NULL == pMsgQ)
	{
		return OSI_INVALID_PARAMS;
	}


    if(pdPASS == xQueueSendFromISR((QueueHandle_t) *pMsgQ, pMsg, &xHigherPriorityTaskWoken ))
    {
		taskYIELD ();
		return OSI_OK;
    }
	else
	{
		return OSI_OPERATION_FAILED;
	}
}

/*!
	\brief 	This function is used to read data from the MsgQ

	\param	pMsgQ	-	pointer to the message queue
	\param	pMsg	-	pointer to the Msg strut to read into
	\param	Timeout	-	timeout to wait for the Msg to be available

	\return - OsiReturnVal_e
	\note
	\warning
*/

OsiReturnVal_e osi_MsgQRead(OsiMsgQ_t* pMsgQ, void* pMsg , OsiTime_t Timeout)
{
	//Check for NULL
	if(NULL == pMsgQ)
	{
		return OSI_INVALID_PARAMS;
	}

	if ( Timeout == OSI_WAIT_FOREVER )
	{
		Timeout = portMAX_DELAY ;
	}

	//Receive Item from Queue
	if( pdTRUE  == xQueueReceive((QueueHandle_t)*pMsgQ,pMsg,Timeout) )
	{
		return OSI_OK;
	}
	else
	{
		return OSI_OPERATION_FAILED;
	}
}

/*!
	\brief 	This function returns the number of messages ready in the Queue.

	\param	pMsgQ	-	pointer to the message queue

	\return - The number of messages available in the queue.
	\note
	\warning
*/
uint32_t osi_MsgQCount(OsiMsgQ_t* pMsgQ)
{
    uint32_t Enqueued;

    if (NULL != pMsgQ)
    {
        Enqueued = uxQueueMessagesWaiting((QueueHandle_t)*pMsgQ);
        return Enqueued;
    }

    return 0;
}

/*!
	\brief 	This function returns whether there are any messages ready in the Queue.

	\param	pMsgQ	-	pointer to the message queue

	\return - BOOLEAN
	\note
	\warning
*/
BOOLEAN osi_MsgQIsEmpty(OsiMsgQ_t* pMsgQ)
{
    return (osi_MsgQCount(pMsgQ) == 0);
}

/*******************************************************************************

    Memory

********************************************************************************/

/*!
    \brief  Allocate dynamic memory

    \param  size

    \return - ptr to buffer or NULL
    \note
    \warning
*/
#ifdef PRINT_DBG_TOTAL_MALLOC_FREE
    volatile INT32 totalloc = 0;
#endif
void *os_malloc(size_t size)
{
    void *ptr = NULL;
#ifdef PRINT_DBG_MALLOC_FREE
    Report("+++++++++++++MALLOC  LR 0x%x ++++++++++\n\r", __get_LR());
#endif //PRINT_DBG_MALLOC_FREE
    if(0 != size)
    {
        osi_EnterCritical();
        ptr = malloc(size);
        osi_ExitCritical(0);

    }
#ifdef PRINT_DBG_MALLOC_FREE
    Report("+++++++++++++MALLOC = 0x%x ++++++++++\n\r", (uint32_t)ptr);
#endif //PRINT_DBG_MALLOC_FREE

#ifdef ASSERT_ON_MALLOC_FAIL
    if(!ptr)
    {
        assert(0);
    }
#endif
#ifdef PRINT_DBG_TOTAL_MALLOC_FREE
    osi_EnterCritical();
    totalloc  = totalloc + (uint32_t)(*((uint32_t *)ptr-2) - 1);
    osi_ExitCritical(0);
    //Report("+++++++++++++ os_malloc total_loc = %d\n\r", totalloc);
#endif
    return ptr;
}

/*!
    \brief  Re-allocate dynamic memory

    \param  ptr Old buffer from os_malloc() or os_realloc()
    \param  newsize of the new buffer

    \return - Allocated buffer or %NULL on failure
    \note
    \warning -  Caller is responsible for freeing the returned buffer with os_free().
                If re-allocation fails, %NULL is returned and the original buffer (ptr) is
                not freed and caller is still responsible for freeing it.
*/
void* os_realloc(void *ptr, size_t newsize)
{
    void *ptrnew = NULL;
#ifdef PRINT_DBG_MALLOC_FREE
    Report("+++++++++++++REALLOC  LR 0x%x ++++++++++\n\r", __get_LR());
#endif //PRINT_DBG_MALLOC_FREE
    if(0 != newsize)
    {
#ifdef PRINT_DBG_TOTAL_MALLOC_FREE
        if (ptr)
        {
            osi_EnterCritical();
            totalloc  = totalloc - (uint32_t)(*((uint32_t *)ptr-2) - 1);
            osi_ExitCritical(0);
        }
#endif
        osi_EnterCritical();
        ptrnew = realloc(ptr,newsize);
        osi_ExitCritical(0);
    }
#ifdef PRINT_DBG_MALLOC_FREE
    Report("+++++++++++++REALLOC old ptr 0x%x newptr = 0x%x ++++++++++\n\r", (uint32_t)ptr,ptrnew);
#endif //PRINT_DBG_MALLOC_FREE

#ifdef ASSERT_ON_MALLOC_FAIL
    if(!ptrnew)
    {
        assert(0);
    }
#endif
#ifdef PRINT_DBG_TOTAL_MALLOC_FREE
    osi_EnterCritical();
    totalloc  = totalloc + (uint32_t)(*((uint32_t *)ptrnew-2) - 1);
    osi_ExitCritical(0);
        //Report("+++++++++++++ os_realloc total_loc = %d\n\r", totalloc);
#endif
    return ptrnew;
}


/*!
    \brief  Re-Allocate and zero memory blocks

    \param  ptr to old array
    \param  nmemb number of blocks to allocate
    \param  size size of each block in bytes

    \return - Allocated buffer or %NULL on failure
    \note
*/
void * os_realloc_array(void *ptr, size_t nmemb, size_t size)
{
    if (size && nmemb > (~(size_t) 0) / size)
        return NULL;
    return os_realloc(ptr, nmemb * size);
}


/*!
    \brief  Allocate and zero memory blocks

    \param  nmemb number of blocks to allocate
    \param  size size of each block in bytes

    \return - Allocated buffer or %NULL on failure
    \note
*/
void *os_calloc(size_t nmemb, size_t size)
{
    void *ptr = NULL;

#ifdef PRINT_DBG_MALLOC_FREE   
    Report("+++++++++++++CALLOC  LR 0x%x ++++++++++\n\r", __get_LR());
#endif //PRINT_DBG_MALLOC_FREE    
    if(0 != size)
    {
        osi_EnterCritical();
        ptr = calloc(nmemb, size);
        osi_ExitCritical(0);
    }


#ifdef PRINT_DBG_MALLOC_FREE       
    Report("+++++++++++++CALLOC = 0x%x ++++++++++\n\r", (uint32_t)ptr);
#endif
#ifdef ASSERT_ON_MALLOC_FAIL
    if(!ptr)
    {
        assert(0);
    }
#endif
#ifdef PRINT_DBG_TOTAL_MALLOC_FREE
    osi_EnterCritical();
    totalloc  = totalloc + (uint32_t)(*((uint32_t *)ptr-2) - 1);
    osi_ExitCritical(0);
    //Report("+++++++++++++ os_calloc total_loc = %d\n\r", totalloc);
#endif
    return ptr;

}

/*!
    \brief  Allocate and zero memory

    \param  size - in bytes

    \return - Allocated buffer or %NULL on failure
    \note
*/
void* os_zalloc(size_t size)
{
    void *ptr = NULL;
#ifdef PRINT_DBG_MALLOC_FREE
    Report("+++++++++++++ZALLOC  LR 0x%x ++++++++++\n\r", __get_LR());
#endif //
    if(0 != size)
    {
        osi_EnterCritical();
        ptr = calloc(1,size);
        osi_ExitCritical(0);
    }
#ifdef PRINT_DBG_MALLOC_FREE
    Report("+++++++++++++ZALLOC = 0x%x ++++++++++\n\r", (uint32_t)ptr);
#endif
#ifdef ASSERT_ON_MALLOC_FAIL
    if(!ptr)
    {
        assert(0);
    }
#endif
#ifdef PRINT_DBG_TOTAL_MALLOC_FREE
    osi_EnterCritical();
    totalloc  = totalloc + (uint32_t)(*((uint32_t *)ptr-2) - 1);
    osi_ExitCritical(0);
    //Report("+++++++++++++ os_malloc total_loc = %d\n\r", totalloc);
#endif
    return ptr;
}


/*!
    \brief  Free memory

    \param  ptr - pointer to release

    \return
    \note
*/
void os_free(void *ptr)
{
    if(!ptr)
        return;

#ifdef PRINT_DBG_TOTAL_MALLOC_FREE
    osi_EnterCritical();
    totalloc  = totalloc - (uint32_t)(*((uint32_t *)ptr-2) - 1);
    //Report("+++++++++++++ os_free total_loc = %d\n\r", totalloc);
    if(totalloc < 0)
    {
        assert(0);
    }
    osi_ExitCritical(0);
#endif
#ifdef PRINT_DBG_MALLOC_FREE 
    Report("+++++++++++++FREE  LR 0x%x ++++++++++\n\r", __get_LR());
    Report("+++++++++++++FREE = 0x%x ++++++++++\n\r", (uint32_t)ptr);
#endif //PRINT_DBG_MALLOC_FREE 
#ifdef PRINT_DBG_TOTAL_MALLOC_FREE
    if(totalloc < 0)
      {
          assert(0);
      }
    totalloc  = totalloc - (uint32_t)(*((uint32_t *)ptr-2) - 1);
    Report("+++++++++++++ os_free total_loc = %d\n\r", totalloc);
#endif
    osi_EnterCritical();
    free(ptr);
    osi_ExitCritical(0);
}




/*******************************************************************************

    Clocks / Timers

********************************************************************************/
/*!
    \brief  Put the thread to sleep
    \param  sec
    \return - OSI_OK
    \note
    \warning
*/
OsiReturnVal_e osi_Sleep(OsiTime_t sec)
{
    TickType_t xDelay;

    if (sec > 0xFFFFFFFF / configTICK_RATE_HZ) {
        xDelay = 0xFFFFFFFF;
    }
    else {
        xDelay = sec * configTICK_RATE_HZ;
    }

    vTaskDelay(xDelay);
    return OSI_OK;
}

/*!
    \brief  Put the thread to sleep in micro seconds
    \param  usec - time in micro seconds
    \return - OSI_OK
    \note
    \warning
*/
OsiReturnVal_e osi_uSleep(OsiTime_t usec)
{
    TickType_t xDelay;

    /* Take the ceiling */
    xDelay = (usec + TICK_PERIOD_US - 1) / TICK_PERIOD_US;

    vTaskDelay(xDelay);
    return OSI_OK;
}

/*!
    \brief  Get free running time in mili seconds
    \param
    \return - time in mili seconds
    \note
    \warning
*/
uint32_t osi_GetTimeMS()
{
    uint32_t Ticks = (uint32_t)xTaskGetTickCount();//tx_time_get();

    return TICK_TO_mSEC(Ticks);
}


/*******************************************************************************

    TIMERS

*******************************************************************************/
typedef void (*func)(void *);

void timerGeneralCallBack(TimerHandle_t tmr)
{
    OsiTimer_t *OsiTmr = pvTimerGetTimerID(tmr);
    ((func)OsiTmr->callBackFunc)(OsiTmr->params);
}


/*!
    \brief  Creates a timer in the OS
    \param pTimer - timer object preallocated allocated
    \param pTimerName - timer name
    \param pExpiryFunc - expiry callback function
    \param pParam - expiry callback parameters
    \return - OSI_xxx
    \note
    \warning
*/
OsiReturnVal_e osi_TimerCreate(OsiTimer_t*                      pTimer,
                               char*                            pTimerName,
                               P_TIMER_EXPIRY_LEGACY_FUNCTION   pExpiryFunc,
                               void*                            pParam)
{
    if ((NULL == pTimer) ||
        (NULL == pExpiryFunc))
    {
        return OSI_INVALID_PARAMS;
    }

    pTimer->osTimerHandler = xTimerCreate (pTimerName,
                             0xFFFFFFFF,
                             pdTRUE,
                             ( void * ) 0,
                             (TimerCallbackFunction_t)timerGeneralCallBack);


    pTimer->callBackFunc = pExpiryFunc;
    pTimer->params = pParam;

    if (NULL == pTimer->osTimerHandler)
    {
        return OSI_OPERATION_FAILED;
    }
    else
    {
        vTimerSetTimerID(((TimerHandle_t)(pTimer->osTimerHandler)),pTimer);
        return OSI_OK;
    }
}



/*!
    \brief Deletes a timer in the OS
    \param pTimer - timer object
    \return - OSI_xxx
    \note
    \warning
*/
OsiReturnVal_e osi_TimerDelete(OsiTimer_t* pTimer)
{
    uint32_t RetVal;

    if (NULL == pTimer)
    {
        return OSI_INVALID_PARAMS;
    }

    RetVal = xTimerDelete(pTimer->osTimerHandler, 0);

    if (pdPASS  == RetVal)
    {
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}

/*!
    \brief Start a created timer
    \param pTimer - timer object
    \param DurationMiliSec
    \param Periodic - is this timer periodic
    \return - OSI_xxx
    \note
    \warning
*/
OsiReturnVal_e osi_TimerStart(OsiTimer_t*     pTimer,
                              OsiTime_t       DurationMiliSec,
                              BOOLEAN         Periodic)
{
    uint32_t DurationInTicks;

    // ensure that the timer is not running by calling to the deactivate function
    if (OSI_OK != osi_TimerStop(pTimer))
    {
        return OSI_OPERATION_FAILED;
    }

    DurationInTicks = mSEC_TO_TICK(DurationMiliSec);

    if (pdPASS != xTimerChangePeriod(pTimer->osTimerHandler, DurationInTicks, 0))
    {
        return OSI_OPERATION_FAILED;
    }

    vTimerSetReloadMode(pTimer->osTimerHandler, Periodic);

    if (pdPASS != xTimerStart(pTimer->osTimerHandler, 0))
    {
        return OSI_OPERATION_FAILED;
    }
    else
    {
        return OSI_OK;
    }
}


/*!
    \brief Stops a created timer
    \param pTimer - timer object
    \return - OSI_xxx
    \note
    \warning
*/
OsiReturnVal_e osi_TimerStop(OsiTimer_t* pTimer)
{
    uint32_t RetVal;

    if (NULL == pTimer)
    {
        return OSI_INVALID_PARAMS;
    }

    RetVal = xTimerStop(pTimer->osTimerHandler, 0);

    if (pdPASS == RetVal)
    {
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}

/*!
    \brief  Check if given timer is active
    \param pTimer - timer object
    \return - OSI_xxx
    \note
    \warning
*/
OsiReturnVal_e osi_TimerIsActive(OsiTimer_t* pTimer)
{
    uint32_t RetVal;

    if (NULL == pTimer)
    {
        return OSI_INVALID_PARAMS;
    }

    RetVal = xTimerIsTimerActive(pTimer->osTimerHandler);

    if (pdPASS == RetVal)
    {
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}

/*!
    \brief  Get the remaining time for a timer to expire
    \param pTimer - timer object
    \return - expiry in mili seconds
    \note
    \warning
*/
OsiTime_t osi_TimerGetRemainingTime(OsiTimer_t* pTimer)
{
    uint32_t       RetVal;
    uint32_t       RemainingTicks;
    OsiTime_t    RemainingDuration = 0;

    if (NULL != pTimer)
    {
        RetVal = xTimerGetExpiryTime(pTimer->osTimerHandler);

        if (RetVal > 0)
        {
            RemainingTicks = RetVal;
            RemainingDuration = TICK_TO_mSEC(RemainingTicks);
        }
    }

    return RemainingDuration;
}


/*******************************************************************************

    THREADS

********************************************************************************/
/*!
    \brief  Creates a thread in the OS
    \param pThread - return handler for the created thread
    \param pThreadName - thread name
    \param StackSize - stack size to be dynamically allocated inside the create function
    \param Priority - priority - 0 is lowest (IDLE) and (configMAX_PRIORITIES - 1) max
    \param pEntryFunc - thread entry function
    \param pParam - parameters to move to the thread entry function
    \return - OSI_xxx
    \note
    \warning
*/
OsiReturnVal_e osi_ThreadCreate(OsiThread_t*                 pThread,
                                char*                        pThreadName,
                                uint32_t                       StackSize,
                                uint32_t                       Priority,
                                P_THREAD_ENTRY_FUNCTION      pEntryFunc,
                                void*                        pParam
                                )
{
    uint32_t RetVal;

    if ((NULL == pThread) ||
        (StackSize < OSI_MIN_THREAD_STACK_SIZE) ||
        (Priority >=32) ||
        (NULL == pEntryFunc))
    {
        return OSI_INVALID_PARAMS;
    }

    RetVal = osi_TaskCreate((P_OSI_TASK_ENTRY)pEntryFunc,
                            (const signed char *)pThreadName,
                            StackSize,
                            pParam,
                            Priority,
                            (OsiTaskHandle *)pThread);

    if (0 == RetVal)
    {
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}


/*!
    \brief  Deletes a thread in the OS
    \param pThread - return handler for the created thread
    \return - OSI_xxx
    \note
    \warning thread should to be in a safe code - while (1) loop for example
             in order to be deleted
*/
OsiReturnVal_e osi_ThreadDelete(OsiThread_t* pThread)
{
    if (NULL == pThread)
    {
        return OSI_INVALID_PARAMS;
    }

    vTaskDelete((TaskHandle_t)*pThread);

    return OSI_OK;
}

/*!
    \brief  Get currently running thread
    \param
    \return - currently running thread
    \note
    \warning
*/
OsiThread_t osi_GetCurrentThread()
{
    return xTaskGetCurrentTaskHandle();
}

/*!
    \brief  Get currently running thread
    \param
    \return - currently running thread
    \note
    \warning
*/
#define HEAP_THRESHOLD_FOR_TX (20000)
size_t osi_GetFreeHeapSize()
{
    //return (size_t)xPortGetFreeHeapSize();
    //this function is not implement yet on this version of free rtos
    return (HEAP_THRESHOLD_FOR_TX + 1);
}

/*
void ASSERT_GENERAL(uint32_t condition)
{
    assert(condition);
}*/
