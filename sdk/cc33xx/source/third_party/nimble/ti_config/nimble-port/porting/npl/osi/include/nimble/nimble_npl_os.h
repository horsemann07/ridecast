/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _NIMBLE_NPL_OS_H_
#define _NIMBLE_NPL_OS_H_

#include <stdint.h>
#include "osi_kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_NPL_OS_ALIGNMENT    4

#define BLE_NPL_TIME_FOREVER    OSI_WAIT_FOREVER

typedef uint32_t ble_npl_time_t;
typedef int32_t ble_npl_stime_t;

struct ble_npl_event {
    bool queued;
    ble_npl_event_fn *fn;
    void *arg;
};

struct ble_npl_eventq {
    OsiMsgQ_t msgQ;
};

struct ble_npl_callout {
    OsiTimer_t timer;
    struct ble_npl_eventq *evq;
    struct ble_npl_event ev;
};

struct ble_npl_mutex {
    OsiLockObj_t lockObj;
};

struct ble_npl_sem {
    OsiSemaphoreObj_t semaphoreObj;
};

struct ble_npl_task {
    OsiThread_t thrdHandler;
};

typedef void (*ble_npl_task_func_t)(void* pParam);
typedef int ble_npl_stack_t;

int ble_npl_task_init(struct ble_npl_task *t, const char *name, ble_npl_task_func_t func,
         void *arg, uint8_t prio, ble_npl_time_t sanity_itvl,
         ble_npl_stack_t *stack_bottom, uint16_t stack_size);

int ble_npl_task_remove(struct ble_npl_task *t);

uint8_t ble_npl_task_count(void);

void ble_npl_eventq_delete(struct ble_npl_eventq *evq);

#ifdef __cplusplus
}
#endif

#endif  /* _NIMBLE_NPL_OS_H_ */
