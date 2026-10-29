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



#ifndef _FW_EVENT_HANDLER_
#define _FW_EVENT_HANDLER_

#include "osi_type.h"
#include "txn_bus_drv.h"
#include "wlan_if.h"
#include "fw_event_if.h"

#define RX_TX_BUFFER (4092)


/* Refer to define in osprey_public_types.h.*/
#define MAX_BLE_PKT_LEN     (300)

/* Refer to EventMBoxId_e in osprey_public_event.h.*/
#define BLE_EVENT_ID        0x08000000 // BIT_27

/* Refer to EventMailBox_t in osprey_public_event.h.*/
typedef struct
{
    uint8   data[MAX_BLE_PKT_LEN];
    uint16  dataLen;
    uint16  padding;
}bleEvent_t;

typedef struct EventMailBox_t
{
    /* Events Bit Mask */
    unsigned int eventsVector;

    /* Events Data */
    WlanEvent_t uppermacevent;
    bleEvent_t  bleEvent;
} EventMailBox_t;
#define EVENT_MBOX_SIZE (sizeof(EventMailBox_t))

#define NAB_EXTRA_BYTES 4

//the NAB adds the sync pattern + add 4 bytes for the status(NAB_EXTRA_BYTES)
#define EVENT_COMPLETE_MAX_LENGTH (sizeof(NAB_header_t) + NAB_EXTRA_BYTES + sizeof(EventCompleteMboxTLV_t) + sizeof( CommandComplete_t) + sizeof(EventCompleteMboxTLV_t) + EVENT_MBOX_SIZE)
#define CONTROL_BUFFER_READ_SIZE EVENT_COMPLETE_MAX_LENGTH




#endif /* _FW_EVENT_HANDLER_ */
