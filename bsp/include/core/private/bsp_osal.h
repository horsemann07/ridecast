#ifndef BSP_OSAL_H
#define BSP_OSAL_H

#include "bsp_err_sts.h"

typedef void* bspMutexHandle_t;
typedef void* bspQueueHandle_t;


/*=============================================================================
 * MUTEX
 *============================================================================*/

bspMutexHandle_t bspMutexCreate(void);

void bspMutexDelete(
    bspMutexHandle_t mutex);

bsp_err_sts_t bspMutexLock(
    bspMutexHandle_t mutex,
    uint32_t timeoutMs);

bsp_err_sts_t bspMutexUnlock(
    bspMutexHandle_t mutex);


/*=============================================================================
 * QUEUE
 *============================================================================*/

bspQueueHandle_t bspQueueCreate(
    uint32_t queueLength,
    uint32_t itemSize);

osStatus_t bspQueueDelete(
    bspQueueHandle_t queue);

osStatus_t bspQueueSend(
    bspQueueHandle_t queue,
    const void* item,
    uint32_t timeout);

osStatus_t bspQueueSendFromISR(
    bspQueueHandle_t queue,
    const void* item);

osStatus_t bspQueueReceive(
    bspQueueHandle_t queue,
    void* item,
    uint32_t timeout);
#endif