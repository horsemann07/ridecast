/**
 * @file bsp_osal.c
 * @brief BSP OS abstraction layer implementation for CMSIS-RTOS2.
 *
 * This module provides a BSP-independent wrapper around CMSIS-RTOS2
 * synchronization and message-queue APIs.
 *
 * The BSP layer must not directly depend on CMSIS-RTOS2 primitives.
 * All RTOS-specific operations are contained in this module.
 */

 #include <stddef.h>
#include "bsp_osal.h"
#include "cmsis_os2.h"



/*=============================================================================
 * PRIVATE HELPERS
 *============================================================================*/

/**
 * @brief Convert a CMSIS-RTOS status to BSP error status.
 *
 * @param[in] status CMSIS-RTOS status.
 *
 * @return Corresponding BSP error status.
 */
static bsp_err_sts_t bspOsalMapStatus(
    osStatus_t status)
{
    bsp_err_sts_t bspStatus;

    switch(status)
    {
        case osOK:
        {
            bspStatus = BSP_ERR_STS_OK;
            break;
        }

        case osErrorParameter:
        {
            bspStatus = BSP_ERR_STS_INVALID_PARAM;
            break;
        }

        case osErrorTimeout:
        {
            bspStatus = BSP_ERR_STS_TIMEOUT;
            break;
        }

        case osErrorResource:
        {
            bspStatus = BSP_ERR_STS_BUSY;
            break;
        }

        case osErrorNoMemory:
        {
            bspStatus = BSP_ERR_STS_NO_MEM;
            break;
        }

        case osErrorISR:
        {
            bspStatus = BSP_ERR_STS_FAIL;
            break;
        }

        default:
        {
            bspStatus = BSP_ERR_STS_FAIL;
            break;
        }
    }

    return bspStatus;
}


/*=============================================================================
 * MUTEX API
 *============================================================================*/

/**
 * @brief Create a BSP mutex.
 *
 * @return Mutex handle on success.
 * @return NULL on failure.
 */
bspMutexHandle_t bspMutexCreate(void)
{
    osMutexId_t mutex;

    mutex = osMutexNew(NULL);

    return (bspMutexHandle_t)mutex;
}


/*----------------------------------------------------------------------------*/

/**
 * @brief Delete a BSP mutex.
 *
 * @param[in] mutex Mutex handle.
 */
void bspMutexDelete(
    bspMutexHandle_t mutex)
{
    if(mutex != NULL)
    {
        (void)osMutexDelete(
            (osMutexId_t)mutex);
    }
}


/*----------------------------------------------------------------------------*/

/**
 * @brief Acquire a BSP mutex.
 *
 * @param[in] mutex
 *        BSP mutex handle.
 *
 * @param[in] timeoutMs
 *        Timeout in milliseconds.
 *
 * @return BSP_ERR_STS_OK
 *         Mutex acquired.
 *
 * @return BSP_ERR_STS_INVALID_PARAM
 *         Invalid mutex.
 *
 * @return BSP_ERR_STS_TIMEOUT
 *         Mutex could not be acquired before timeout.
 *
 * @return BSP_ERR_STS_FAIL
 *         Other RTOS error.
 */
bsp_err_sts_t bspMutexLock(
    bspMutexHandle_t mutex,
    uint32_t timeoutMs)
{
    osStatus_t status;

    if(mutex == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    status = osMutexAcquire(
        (osMutexId_t)mutex,
        timeoutMs);

    return bspOsalMapStatus(status);
}


/*----------------------------------------------------------------------------*/

/**
 * @brief Release a BSP mutex.
 *
 * @param[in] mutex
 *        BSP mutex handle.
 *
 * @return BSP_ERR_STS_OK
 *         Mutex released.
 *
 * @return BSP_ERR_STS_INVALID_PARAM
 *         Invalid mutex.
 *
 * @return BSP_ERR_STS_FAIL
 *         Mutex release failed.
 */
bsp_err_sts_t bspMutexUnlock(
    bspMutexHandle_t mutex)
{
    osStatus_t status;

    if(mutex == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    status = osMutexRelease(
        (osMutexId_t)mutex);

    return bspOsalMapStatus(status);
}


/*=============================================================================
 * QUEUE API
 *============================================================================*/

/**
 * @brief Create a BSP message queue.
 *
 * @param[in] queueLength
 *        Maximum number of messages.
 *
 * @param[in] itemSize
 *        Size of each message in bytes.
 *
 * @return Queue handle on success.
 * @return NULL on failure.
 */
bspQueueHandle_t bspQueueCreate(
    uint32_t queueLength,
    uint32_t itemSize)
{
    osMessageQueueId_t queue;

    if((queueLength == 0U) ||
       (itemSize == 0U))
    {
        return NULL;
    }

    queue = osMessageQueueNew(
        queueLength,
        itemSize,
        NULL);

    return (bspQueueHandle_t)queue;
}


/*----------------------------------------------------------------------------*/

/**
 * @brief Delete a BSP message queue.
 *
 * @param[in] queue
 *        BSP queue handle.
 *
 * @return CMSIS/BSP status.
 */
osStatus_t bspQueueDelete(
    bspQueueHandle_t queue)
{
    if(queue == NULL)
    {
        return osErrorParameter;
    }

    return osMessageQueueDelete(
        (osMessageQueueId_t)queue);
}


/*----------------------------------------------------------------------------*/

/**
 * @brief Send a message to a BSP queue.
 *
 * This API is intended for thread context.
 *
 * @param[in] queue
 *        BSP queue handle.
 *
 * @param[in] item
 *        Pointer to message.
 *
 * @param[in] timeout
 *        Timeout in milliseconds.
 *
 * @return CMSIS-RTOS status.
 */
osStatus_t bspQueueSend(
    bspQueueHandle_t queue,
    const void* item,
    uint32_t timeout)
{
    if((queue == NULL) ||
       (item == NULL))
    {
        return osErrorParameter;
    }

    return osMessageQueuePut(
        (osMessageQueueId_t)queue,
        item,
        0U,
        timeout);
}


/*----------------------------------------------------------------------------*/

/**
 * @brief Send a message to a BSP queue from ISR context.
 *
 * This function is strictly non-blocking.
 *
 * CMSIS-RTOS2 allows osMessageQueuePut() to be called from ISR
 * when timeout is zero.
 *
 * @param[in] queue
 *        BSP queue handle.
 *
 * @param[in] item
 *        Pointer to message.
 *
 * @return CMSIS-RTOS status.
 */
osStatus_t bspQueueSendFromISR(
    bspQueueHandle_t queue,
    const void* item)
{
    if((queue == NULL) ||
       (item == NULL))
    {
        return osErrorParameter;
    }

    /*
     * IMPORTANT:
     *
     * timeout MUST be zero here.
     *
     * An ISR must never block waiting for queue space.
     */
    return osMessageQueuePut(
        (osMessageQueueId_t)queue,
        item,
        0U,
        0U);
}


/*----------------------------------------------------------------------------*/

/**
 * @brief Receive a message from a BSP queue.
 *
 * This API is intended for thread context.
 *
 * @param[in] queue
 *        BSP queue handle.
 *
 * @param[out] item
 *        Destination message buffer.
 *
 * @param[in] timeout
 *        Timeout in milliseconds.
 *
 * @return CMSIS-RTOS status.
 */
osStatus_t bspQueueReceive(
    bspQueueHandle_t queue,
    void* item,
    uint32_t timeout)
{
    if((queue == NULL) ||
       (item == NULL))
    {
        return osErrorParameter;
    }

    return osMessageQueueGet(
        (osMessageQueueId_t)queue,
        item,
        NULL,
        timeout);
}