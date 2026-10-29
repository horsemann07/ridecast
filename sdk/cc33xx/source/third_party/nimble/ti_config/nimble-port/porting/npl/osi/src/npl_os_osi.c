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

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "nimble/nimble_npl.h"
#include "nimble/nimble_npl_os.h"
#include "osi_kernel.h"

#define MSGQ_MSG_SIZE     sizeof(struct ble_npl_eventq *)
#define MSGQ_MAX_NUM_MSGS 32

/*
 * Generic
 */

bool
ble_npl_os_started(void)
{
    return TRUE; /* Always Started */
}

void *
ble_npl_get_current_task_id(void)
{
    return osi_GetCurrentThread();
}

/*
 * Tasks
 */
int
ble_npl_task_init(struct ble_npl_task *t, const char *name, ble_npl_task_func_t func,
        void *arg, uint8_t prio, ble_npl_time_t sanity_itvl,
        ble_npl_stack_t *stack_bottom, uint16_t stack_size)
{
    OsiReturnVal_e rc = OSI_OK;

    rc = osi_ThreadCreate(&(t->thrdHandler),
                          (char*)name,
                          stack_size,
                          prio,
                          func,
                          arg);
    assert(OSI_OK == rc);

    return (0);
}

int
ble_npl_task_remove(struct ble_npl_task *t)
{
    OsiReturnVal_e rc = OSI_OK;

    rc = osi_ThreadDelete(&(t->thrdHandler));
    assert(OSI_OK == rc);

    return (0);
}

uint8_t
ble_npl_task_count(void)
{
    return 0; //Not Supported
}

/*
 * Event queue
 */

void
ble_npl_eventq_init(struct ble_npl_eventq *evq)
{
    OsiReturnVal_e rc = OSI_OK;

    rc = osi_MsgQCreate(&(evq->msgQ),
                        "nimbleQueue",
                        MSGQ_MSG_SIZE,
                        MSGQ_MAX_NUM_MSGS);
    assert(OSI_OK == rc);
}

void
ble_npl_eventq_delete(struct ble_npl_eventq *evq)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!evq) {
        return;
    }

    rc = osi_MsgQDelete(&(evq->msgQ));
    assert(OSI_OK == rc);
}

struct ble_npl_event *
ble_npl_eventq_get(struct ble_npl_eventq *evq, ble_npl_time_t tmo)
{
    OsiReturnVal_e rc = OSI_OK;
    struct ble_npl_event *ev = NULL;

    rc = osi_MsgQRead(&(evq->msgQ), &ev, BLE_NPL_TIME_FOREVER);
    assert(OSI_OK == rc);

    if (ev) {
        ev->queued = false;
    }

    return ev;
}

void
ble_npl_eventq_put(struct ble_npl_eventq *evq, struct ble_npl_event *ev)
{
    OsiReturnVal_e rc = OSI_OK;

    if (ev->queued) {
        return;
    }

    ev->queued = true;

    rc = osi_MsgQWrite(&(evq->msgQ), &ev , BLE_NPL_TIME_FOREVER,OSI_FLAG_NOT_FROM_INTR);
    assert(OSI_OK == rc);
}

void
ble_npl_eventq_remove(struct ble_npl_eventq *evq,
                      struct ble_npl_event *ev)
{
    OsiReturnVal_e rc = OSI_OK;
    struct ble_npl_event *tmp_ev;
    int count;
    int i;

    if (!ev->queued) {
        return;
    }

    /*
     * XXX We cannot extract element from inside a OSI queue so as a quick
     * workaround we'll just remove all elements and add them back except the
     * one we need to remove. This is silly, but works for now - we probably
     * better use counting semaphore with os_queue to handle this in future.
     */

    osi_EnterCriticalSection();

    count = osi_MsgQCount(&(evq->msgQ));
    for (i = 0; i < count; i++) {
        rc = osi_MsgQRead(&(evq->msgQ), &tmp_ev, OSI_NO_WAIT);
        assert(OSI_OK == rc);

        if (tmp_ev == ev) {
            continue;
        }

        rc = osi_MsgQWrite(&(evq->msgQ), &tmp_ev, OSI_NO_WAIT,OSI_FLAG_NOT_FROM_INTR);
        assert(OSI_OK == rc);
    }

    osi_ExitCriticalSection(0);
}

bool
ble_npl_eventq_is_empty(struct ble_npl_eventq *evq)
{
    return osi_MsgQIsEmpty(&(evq->msgQ));
}

void
ble_npl_event_run(struct ble_npl_event *ev)
{
    ev->fn(ev);
}

void
ble_npl_event_init(struct ble_npl_event *ev, ble_npl_event_fn *fn,
                   void *arg)
{
    os_memset(ev, 0, sizeof(*ev));
    ev->fn = fn;
    ev->arg = arg;
}

bool ble_npl_event_is_queued(struct ble_npl_event *ev)
{
    return ev->queued;
}

void *
ble_npl_event_get_arg(struct ble_npl_event *ev)
{
    return ev->arg;
}

void
ble_npl_event_set_arg(struct ble_npl_event *ev, void *arg)
{
    ev->arg = arg;
}

/*
 * Mutexes
 */

ble_npl_error_t
ble_npl_mutex_init(struct ble_npl_mutex *mu)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    rc = osi_LockObjCreate(&(mu->lockObj));
    assert(OSI_OK == rc);

    return BLE_NPL_OK;
}

ble_npl_error_t
ble_npl_mutex_delete(struct ble_npl_mutex *mu)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    rc = osi_LockObjDelete(&(mu->lockObj));
    assert(OSI_OK == rc);

    return BLE_NPL_OK;
}

ble_npl_error_t
ble_npl_mutex_pend(struct ble_npl_mutex *mu, ble_npl_time_t timeout)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    rc = osi_LockObjLock(&(mu->lockObj), timeout);
    assert(OSI_OK == rc);

    return BLE_NPL_OK;
}

ble_npl_error_t
ble_npl_mutex_release(struct ble_npl_mutex *mu)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    rc = osi_LockObjUnlock(&(mu->lockObj));
    assert(OSI_OK == rc);

    return BLE_NPL_OK;
}

/*
 * Semaphores
 */

ble_npl_error_t
ble_npl_sem_init(struct ble_npl_sem *sem, uint16_t tokens)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    rc = osi_SemaphoreObjCreate(&(sem->semaphoreObj),"nimbleSemaphore",tokens,128);
    assert(OSI_OK == rc);

    return BLE_NPL_OK;
}

ble_npl_error_t
ble_npl_sem_delete(struct ble_npl_sem *sem)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    rc = osi_SemaphoreObjDelete(&(sem->semaphoreObj));
    assert(OSI_OK == rc);

    return BLE_NPL_OK;
}

ble_npl_error_t
ble_npl_sem_pend(struct ble_npl_sem *sem, ble_npl_time_t timeout)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    rc = osi_SemaphoreObjObtain(&(sem->semaphoreObj), timeout);

    return rc == OSI_OK ? BLE_NPL_OK : BLE_NPL_TIMEOUT;
}

ble_npl_error_t
ble_npl_sem_release(struct ble_npl_sem *sem)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    rc = osi_SemaphoreObjRelease(&(sem->semaphoreObj));
    assert(OSI_OK == rc);

    return BLE_NPL_OK;
}

uint16_t
ble_npl_sem_get_count(struct ble_npl_sem *sem)
{

    OsiReturnVal_e rc = OSI_OK;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    return osi_SemaphoreObjGetCount(&(sem->semaphoreObj));
}

/*
 * Callouts
 */

static void
os_callout_timer_cb(void *timer)
{
    struct ble_npl_callout *co = (struct ble_npl_callout*)timer;
    assert(co);

    if (co->evq) {
        ble_npl_eventq_put(co->evq, &co->ev);
    } else {
        co->ev.fn(&co->ev);
    }
}

void
ble_npl_callout_init(struct ble_npl_callout *co, struct ble_npl_eventq *evq,
                     ble_npl_event_fn *ev_cb, void *ev_arg)
{
    OsiReturnVal_e rc = OSI_OK;

    os_memset(co, 0, sizeof(*co));

    rc = osi_TimerCreate(&(co->timer), "nimbleTimer", os_callout_timer_cb, (void *)co);
    assert(OSI_OK == rc);

    co->evq = evq;
    ble_npl_event_init(&co->ev, ev_cb, ev_arg);
}

void
ble_npl_callout_delete(struct ble_npl_callout *co)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!co) {
        return;
    }

    rc = osi_TimerDelete(&(co->timer));
    assert(OSI_OK == rc);
}

ble_npl_error_t
ble_npl_callout_reset(struct ble_npl_callout *co, ble_npl_time_t ticks)
{
    OsiReturnVal_e rc = OSI_OK;

    if (!co) {
        return BLE_NPL_INVALID_PARAM;
    }

    if (ticks == 0) {
        ticks = 1;
    }

    rc = osi_TimerStop(&(co->timer));
    assert(OSI_OK == rc);
    rc = osi_TimerStart(&(co->timer), TICK_TO_mSEC(ticks), OSI_ONE_SHOT_TIMER);
    assert(OSI_OK == rc);

    return BLE_NPL_OK;
}

void
ble_npl_callout_stop(struct ble_npl_callout *co)
{
    OsiReturnVal_e rc = OSI_OK;
    assert(co);
    rc = osi_TimerStop(&(co->timer));
    assert(OSI_OK == rc);
}

bool
ble_npl_callout_is_active(struct ble_npl_callout *co)
{
    OsiReturnVal_e rc = OSI_OK;
    assert(co);
    rc = osi_TimerIsActive(&(co->timer));
    return (rc == OSI_OK);
}

ble_npl_time_t
ble_npl_callout_remaining_ticks(struct ble_npl_callout *co,
                                ble_npl_time_t time)
{
    OsiTime_t ms;
    ble_npl_time_t ticks;
    ble_npl_time_t rt;

    assert(co);

    ms = osi_TimerGetRemainingTime(&(co->timer));
    ticks = ble_npl_time_ms_to_ticks32(ms);

    if (ticks > time) {
        rt = ticks - time;
    } else {
        rt = 0;
    }

    return rt;
}

ble_npl_time_t
ble_npl_callout_get_ticks(struct ble_npl_callout *co)
{
    OsiTime_t ms;
    ble_npl_time_t ticks;

    assert(co);

    ms = osi_TimerGetRemainingTime(&(co->timer));
    ticks = ble_npl_time_ms_to_ticks32(ms);

    return ticks;
}

void
ble_npl_callout_set_arg(struct ble_npl_callout *co,
                             void *arg)
{
    assert(co);
    co->ev.arg = arg;
}

/*
 * Time functions
 */

uint32_t
ble_npl_time_get(void)
{
    return ble_npl_time_ms_to_ticks32(osi_GetTimeMS());
}

ble_npl_error_t
ble_npl_time_ms_to_ticks(uint32_t ms, ble_npl_time_t *out_ticks)
{
    *out_ticks = ble_npl_time_ms_to_ticks32(ms);
    return BLE_NPL_OK;
}

ble_npl_error_t
ble_npl_time_ticks_to_ms(ble_npl_time_t ticks, uint32_t *out_ms)
{
    *out_ms = ble_npl_time_ticks_to_ms32(ticks);
    return BLE_NPL_OK;
}

ble_npl_time_t
ble_npl_time_ms_to_ticks32(uint32_t ms)
{
    return mSEC_TO_TICK(ms);
}

uint32_t
ble_npl_time_ticks_to_ms32(ble_npl_time_t ticks)
{
    return TICK_TO_mSEC(ticks);
}

void
ble_npl_time_delay(ble_npl_time_t ticks)
{
    OsiTime_t usec;
    usec = ble_npl_time_ticks_to_ms32(ticks) * 1000;
    osi_uSleep(usec);
}

/*
 * Hardware-specific
 *
 */

uint32_t
ble_npl_hw_enter_critical(void)
{
    osi_EnterCriticalSection();
    return 0;
}

void
ble_npl_hw_exit_critical(uint32_t ctx)
{
    osi_ExitCriticalSection(ctx);
}
