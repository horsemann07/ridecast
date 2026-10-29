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

#include <bus_if.h>
#include <stdio.h>
#include <stdint.h>
#include "wlan_if.h"
#include "macro_utils.h"
#include "commands.h"
#include "timer.h"
#include <fw_event_handle.h>
#include <trnspt_if.h>
#include <trnspt_thread.h>
#include "fw_event_if.h"
#include <wlan_irq_adapt.h>
#include "errors.h"
#include "rx.h"
#include "tx.h"
#include "osi_kernel.h"
#include "init_host_if.h"
#include "control_cmd_fw.h"
#include "tx_defs.h"
#include "init_host.h"
#include "osprey_public_commands.h"
#include "init_device.h"
#include "host_event_if.h"

extern OsiLockObj_t driverLock;//TODO isri verify if this can be removed

/******************************************************************************

    NAME:           DeInitHostDriverInternal

    PROTOTYPE:      init

    DESCRIPTION:    First Init

    CALLER:         ChipWakeUp

    RETURNS:        NONE
******************************************************************************/
int InitHostDriver()
{
    uint32_t reserved = 0;
    int ret;
    const TPartition aPartition[] = {{0,0xF0000},{0,0},{0,0},{0,0}};
    uint32_t TrnsptThrdPrior;

    wlan_IRQDeinit();

    /* boot the device */
    osi_LockObjCreate(&driverLock);

    ret = ctrlCmdFw_CreateMsgQueue();

    if(ret != OSI_OK)
    {
        ASSERT_GENERAL(0);
        return ret;
    }

    /* Transport init */
    TrnsptThrdPrior = TRANSPORT_THRD_PRIORITY;
    ret = trnspt_Init(TrnsptThrdPrior);
    if(ret != OSI_OK)
    {
        ASSERT_GENERAL(0);
        return ret;
    }

    twIf_Init();
    twIf_SetPartition((TPartition *)aPartition);

    ret = tmr_Init();
    if(ret != OSI_OK)
    {
        ASSERT_GENERAL(0);
        return ret;
    }

    ret = ctrlCmdFw_CreateEventThread();
    if(ret != OSI_OK)
    {
        ASSERT_GENERAL(0);
        return ret;
    }

    ret = fwEvent_Init(RX_TX_BUFFER,CONTROL_BUFFER_READ_SIZE);
    if(ret != OSI_OK)
    {
        Report("fwEvent_Init FAILED");
        return ret;
    }

    wlan_IRQInitBeforeHwInit((void*) FwEvent_irq_handler);
    if(ret != OSI_OK)
    {
        Report("wlan_IRQInitBeforeHwInit FAILED");
        return ret;
    }
    /* For the following functions , the fail will be probably as a result of allocation failure */
    /* The return value of the failure is negative */
    ret |= cmd_Init();
    ret |= rx_Init();
    ret |= tx_Init();

    if(ret < 0)
    {
        ASSERT_GENERAL(0);
        return OSI_MEMORY_ALLOCATION_FAILURE;
    }

    /* Set HW init */
    ret = HwInit();
    
    if(ret != OSI_OK)
    {
        Report("\n\rHwInit failed \n\r");
        ASSERT_GENERAL(0);
        return OSI_OPERATION_FAILED;
    }

    Report("\n\rHardware init DONE! \n\r");

    /* send command to init bus */
    bus_sendInitCommand(reserved, 0);
    
    /* enable the IRQ interrupt */
    wlan_IRQEnableInt();

    ret = init_device(CONTROL_BUFFER_READ_SIZE,CONTROL_BUFFER_READ_SIZE);
    if(ret != OSI_OK)
    {
        Report("Init device FAILED");
        return ret;
    }

    return ret;
}


void DeInitHostDriverInternal()
{
    int32_t ret = 0;
    WlanEventError_t  error_event;

    ret |= tmr_Destroy();
    ret |= DeHwInit();
    ret |= fwEvent_Destroy();
    rx_Deinit();
    tx_Deinit();
    ret |= cmd_Deinit();

    /*If some deinit function is failed , send event */
    if(ret < 0)
    {
        error_event.error_num = WlanError(WLAN_ERROR_SEVERITY__HIGH, WLAN_ERROR_MODULE__DEVICE, WLAN_ERROR_TYPE__DRIVER_DEINIT);
        error_event.module = WLAN_MODULE_DEINIT;
        error_event.severity = WLAN_SEVERITY_MID;
        Wlan_HostSendEvent(WLAN_EVENT_ERROR, (uint8_t*)&error_event, sizeof(WlanEventError_t));
        Report("\n\rDeinit modules FAILED! \n\r");
        return;
    }

    Report("\n\rDeinit modules DONE! \n\r");

    return;

}

int DeInitHostDriver()
{
    uint32_t ret = 0;
    uint32_t uDestroyClientId;
    uDestroyClientId = trnspt_RegisterClient((TTransportCbFunc)DeInitHostDriverInternal, TRUE);

    if(0 == uDestroyClientId)
    {
        /* max number of clients is exceeded, report error and exit. */
        ASSERT_GENERAL(0);
        return WlanError(WLAN_ERROR_SEVERITY__HIGH, WLAN_ERROR_MODULE__DEVICE, WLAN_ERROR_TYPE__REACHED_MAX_CLIENTS);
    }

    /* Transport deinit */
    ret = trnspt_Destroy(uDestroyClientId);
    if(OSI_OK != ret)
    {
        ASSERT_GENERAL(0);
        return ret;
    }

    ret = ctrlCmdFw_DeleteEventThread();
    if(OSI_OK != ret)
    {
        ASSERT_GENERAL(0);
        return ret;
    }

    ret = ctrlCmdFw_DeleteMsgQueue();
    if(OSI_OK != ret)
    {
        ASSERT_GENERAL(0);
        return ret;
    }

    ret = twIf_Destroy();
    if(OSI_OK != ret)
    {
        ASSERT_GENERAL(0);
        return ret;
    }

    osi_LockObjDelete(&driverLock);




    return 0;
}



