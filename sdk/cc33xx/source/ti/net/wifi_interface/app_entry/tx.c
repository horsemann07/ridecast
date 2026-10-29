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

#include <fw_event_handle.h>
#include <stdint.h>
#include <txn_bus_drv.h>
#include "osi_kernel.h"
#include "w_queue.h"
#include "tx.h"
#include "macro_utils.h"
#include "nab.h"
#include <trnspt_if.h>
#include <trnspt_thread.h>
#include "ethernet.h"
#include "tx_defs.h"
#include "user_errno.h"
#include "wlan_if.h"
#include "host_event_if.h"
#include "errors.h"



typedef struct TxIfDescriptor_t
{
    uint16_t          length;         /* Length of packet in words, including descriptor+header+data */
    uint8_t           totalMemBlks;   /* Total number of memory blocks allocated by the host for this packet. */
                                      /* Must be equal or greater than the actual blocks number allocated by HW!! */
    uint8_t           extraMemBlks;   /* Number of extra memory blocks to allocate for this packet in addition */
    uint8_t           descID;         /* Packet identifier used also in the Tx-Result. */
    uint8_t           tid;            /* The packet TID value (as User-Priority) */
    uint8_t           hlid;           /* Identifier of link. each hlid (host lid) is mapped to a flid (fw lid) */
    uint8_t           ac;
    uint16_t          lifeTime;       /* Max delay in TUs until transmission. The last device time the
                                         packet can be transmitted is: startTime+(1024*LifeTime) */
    uint16_t          txAttr;         /* Bitwise fields - see TX_ATTR... definitions above. */
} TxIfDescriptor_t;


typedef struct
{
    TQueNodeHdr queueHeader;
    uint8_t *inBuff;
    uint32_t inLen;
}TxFrame_t;


typedef struct
{
    void *txQ;
    uint32_t clientId;
    /* Pending TX frames */
    unsigned long tx_frames_map[1];
    TxFrame_t *tx_frames[MAX_TX_DESCRIPTORS];
    int tx_frames_cnt;

    /* Accounting for allocated / available TX blocks on HW */
    uint32_t tx_blocks_freed;
    uint32_t tx_blocks_available;
    uint32_t tx_allocated_blocks;
    uint32_t tx_results_count;

    /* Accounting for allocated / available Tx packets in HW */
    uint32_t tx_pkts_freed[4];
    uint32_t tx_allocated_pkts[4];

    /* Transmitted TX packets counter for chipset interface */
    uint32_t tx_packets_count;

    uint8_t *aggr_buf;
    uint32_t aggr_buf_size;
    uint32_t num_tx_desc;
    uint32_t last_fw_rls_idx;
}TxCB_t;




TxCB_t *gTxCB = NULL;

void TxThread();


void free_tx_id(TxCB_t *wl, int id)
{
    if(IS_BIT_SET(wl->tx_frames_map[0],id))
    {
        CLEAR_BIT_BY_IDX(wl->tx_frames_map[0],id);
        wl->tx_frames[id] = NULL;
        wl->tx_frames_cnt--;
    }

    TX_PRINT("free desc ID. id - %d, frames count %d\n\r",id,wl->tx_frames_cnt);

}


void TxCompletePacket(TxCB_t *wl, uint8_t tx_stat_byte)
{

    TxFrame_t *frame;
    int id;
    #ifdef DEBUG_TX
    uint32_t tx_success;
    #endif
    TxIfDescriptor_t *tx_desc;
    WlanEventError_t  error_event;


    id = (tx_stat_byte & TX_STATUS_DESC_ID_MASK);
    /* a zero bit indicates Tx success */
    #ifdef DEBUG_TX
    tx_success = !(tx_stat_byte & TI_BIT(TX_STATUS_STAT_BIT_IDX));
    #endif


    /* check for id legality */
    if ((id >= (int)wl->num_tx_desc) || (wl->tx_frames[id] == NULL))
    {
        TX_PRINT("\n\rIllegal tx complete id %d desc num %d , tx_success :%d (if 1 success)",id,wl->num_tx_desc, tx_success);
        ASSERT_GENERAL(0);
        error_event.error_num = WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__TX_ILLEGAL_ID);
        error_event.module = WLAN_MODULE_TX_THREAD;
        error_event.severity = WLAN_SEVERITY_HIGH;
        Wlan_HostSendEvent(WLAN_EVENT_ERROR, (uint8_t*)&error_event, sizeof(WlanEventError_t));
        return;
    }


    frame = wl->tx_frames[id];


    tx_desc = (TxIfDescriptor_t *)(frame->inBuff);

    TX_PRINT("\n\rTx status id %u skb 0x%p success %d, tx_memblocks %d\n\r",id, (uint32_t)frame, tx_success,tx_desc->totalMemBlks);


    /* in order to update the memory management we should have total_blocks, ac, and hlid */
    /* update memory management variables */
    wl->tx_blocks_available += tx_desc->totalMemBlks;
    wl->tx_allocated_blocks -= tx_desc->totalMemBlks;

    /* prevent wrap-around in freed-packets counter */
    wl->tx_allocated_pkts[tx_desc->ac]--;


    free_tx_id(wl, id);

    if(NULL != frame)
    {
        if(NULL != frame->inBuff)
        {
            os_free(frame->inBuff);
        }
        os_free(frame);
    }

    /* call the scheduler to see if new TX need to be sent */
    trnspt_RequestSchedule(wl->clientId, FALSE);

}

void tx_ImmediateComplete(FwStatus_t *core_status)
{
    uint8_t txResultQueueIndex;
    uint8_t i;
    TxCB_t *wl = gTxCB;

    txResultQueueIndex = core_status->fwInfo.txResultQueueIndex;

    if(txResultQueueIndex != wl->last_fw_rls_idx)
    {

        TX_PRINT("\n\rlast released desc = %d, current idx = %d",wl->last_fw_rls_idx, txResultQueueIndex);

        /* freed Tx descriptors */
        if (txResultQueueIndex >= TX_RESULT_QUEUE_SIZE)
        {
            TX_PRINT_ERROR("\n\rinvalid desc release index %d", txResultQueueIndex);
            return;
        }

        TX_PRINT("\n\rTX command complete ,  last_fw_rls_idx:%d, txResultQueueIndex %d",
                 wl->last_fw_rls_idx,wl->last_fw_rls_idx, txResultQueueIndex);

        for (i = wl->last_fw_rls_idx; i != txResultQueueIndex; i = (i + 1) % TX_RESULT_QUEUE_SIZE)
        {
            TxCompletePacket(wl,core_status->fwInfo.txResultQueue[i]);
            wl->tx_results_count++;
        }

        wl->last_fw_rls_idx = txResultQueueIndex;
    }
}



int tx_Init()
{
    int ret = 0;

    gTxCB = os_zalloc(sizeof(TxCB_t));
    if(NULL == gTxCB)
    {
        ret = WlanError(WLAN_ERROR_SEVERITY__HIGH, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__MALLOC);
        goto return_error;
    }

    gTxCB->aggr_buf = os_malloc(TX_BUFFER_SIZE);

    if(NULL == gTxCB->aggr_buf)
    {
        ASSERT_GENERAL(0);
        ret = WlanError(WLAN_ERROR_SEVERITY__HIGH, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__MALLOC);
        goto return_error;
    }

    gTxCB->aggr_buf_size = TX_BUFFER_SIZE;

    // add plus one to the queue for requeue situation
    gTxCB->txQ = que_Create(MAX_TX_FRAMES + 1, 0);

    gTxCB->num_tx_desc = MAX_TX_DESCRIPTORS;
    gTxCB->tx_blocks_available = MAX_TX_BLOCK;

    if(NULL == gTxCB->txQ)
    {
        ret = WlanError(WLAN_ERROR_SEVERITY__HIGH, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__MSG_QUEUE_CREATE);
        goto return_error;
    }

    gTxCB->clientId = trnspt_RegisterClient((TTransportCbFunc)TxThread, TRUE);

    return ret;

return_error:

    if(NULL != gTxCB->aggr_buf)
    {
        os_free(gTxCB->aggr_buf);
    }

    if(NULL != gTxCB)
    {
        os_free(gTxCB);
    }
    gTxCB = NULL;

    return ret;


}

void tx_Deinit()
{
    TxFrame_t   *pDequeue = NULL;
    int i;
    if(NULL != gTxCB)
    {
        if(NULL != gTxCB->txQ)
        {
            /* Dequeue and free all queued packets */
            while (1)
            {
                trnspt_EnterCriticalSection ();
                pDequeue = (TxFrame_t   *)que_Dequeue (gTxCB->txQ);
                trnspt_LeaveCriticalSection();
                if (pDequeue == NULL)
                {
                    break;
                }
                else
                {

                    os_free(pDequeue->inBuff);
                    os_free(pDequeue);
                    pDequeue = NULL;
                }
            }
            /* Queue destroy */
            que_Destroy(gTxCB->txQ);
        }

        for(i = 0;i < MAX_TX_DESCRIPTORS;i++)
        {
            pDequeue = gTxCB->tx_frames[i];
            if(NULL != pDequeue)
            {
                os_free(pDequeue->inBuff);
                os_free(pDequeue);
                pDequeue = NULL;
            }
        }

        /* Free memory */
        if(NULL != gTxCB->aggr_buf)
        {
            os_free(gTxCB->aggr_buf);
        }

        if(NULL != gTxCB)
        {
            os_free(gTxCB);
        }
        gTxCB = NULL;
    }
}

uint32_t counter = 9;
int tx_SendEthrBuf(uint8_t *inBuff, uint32_t inLen)
{
    TxFrame_t *frame;
    uint32_t qNumOfItems;
    frame = os_zalloc(sizeof(TxFrame_t));
    if (NULL == frame)
    {
        return WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__MALLOC);
    }

    //allocate the buffer but insure Wlan header, snap header and rsn header
    // inbuff is                                              | Eth header 14B | Data |
    // make sure               | TxIfDesc 12B | WLAN Header qos 26B | SNAP 8B | HT header 4B | RSN header 8B  | Data |
    // need to allocated       |                   44B pad                    | Eth header 14B | Data |
    frame->inBuff = os_malloc(inLen + PADDING_COMPLETION_FOR_WLAN_HEADER);

    if (NULL == frame->inBuff)
    {
        os_free(frame);
        return WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__MALLOC);
    }

    os_memcpy((frame->inBuff + PADDING_COMPLETION_FOR_WLAN_HEADER),inBuff,inLen);

    frame->inLen = inLen + PADDING_COMPLETION_FOR_WLAN_HEADER;

    trnspt_EnterCriticalSection();
    counter++;
    qNumOfItems = que_Size(gTxCB->txQ);
    if(qNumOfItems < MAX_TX_FRAMES)
    {
        if(que_Enqueue(gTxCB->txQ, frame) < 0)
        {
            os_free(frame->inBuff);
            os_free(frame);

            TX_PRINT("tx enqueue failed!\r\n");
            trnspt_LeaveCriticalSection();
            return WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__ENQUEUE);
        }
    }
    else
    {
        os_free(frame->inBuff);
        os_free(frame);
        //TX_PRINT("tx queue is full!\r\n");
        trnspt_LeaveCriticalSection();
        return WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__QUEUE_FULL);
    }
    trnspt_LeaveCriticalSection();

    trnspt_RequestSchedule(gTxCB->clientId, FALSE);

    return inLen;
}

#ifdef __ICCARM__
int __builtin_ctz(uint32_t x)
{
    int count = 0;

    if (x == 0)
        return 32;

    while (!(x & 1)) 
    {
        count++;
        x >>= 1;
    }
    return count;
}
#endif

unsigned long FindFirstZeroBit(const unsigned long longVal)
{

    return __builtin_ctz(~longVal);
}



int AllocTxId(TxCB_t *wl, TxFrame_t *frame)
{
    int id;

    id = FindFirstZeroBit(wl->tx_frames_map[0]);

    if (id >= (int)wl->num_tx_desc)
    {
        return -EBUSY;
    }
    SET_BIT_BY_IDX(wl->tx_frames_map[0],id);

    wl->tx_frames[id] = frame;
    wl->tx_frames_cnt++;
    TX_PRINT("alloc desc ID. id - %d, frames count %d\n\r",id,wl->tx_frames_cnt);
    return id;
}

uint32_t CalcTxBlocks(uint32_t len, uint32_t spare_blks)
{
    uint32_t blk_size = OSPREY_TX_HW_BLOCK_SIZE;
    /* In CC3xxx the packet will be stored along with its internal descriptor.
     * the descriptor is not part of the host transaction, but should be considered as part of
     * the allocate memory blocks in the device
     */
    len = len + OSPREY_INTERNAL_DESC_SIZE;
    return (len + blk_size - 1) / blk_size + spare_blks;
}




int Allocate(TxCB_t *wl, TxFrame_t *frame, uint32_t buf_offset, NAB_tx_header_t *nab_cmd)
{

    TxIfDescriptor_t * desc;

    uint32_t total_blocks;
    int id, ret = -EBUSY, ac;
    uint32_t spare_blocks;
    uint32_t total_skb_len = frame->inLen;

    // Add NAB command required for CC3xxx architecture
    uint32_t total_len = total_skb_len + sizeof(NAB_tx_header_t);

    TX_PRINT("michal1 wl->tx_blocks_available %d total_len%d\n\r", wl->tx_blocks_available , total_len);

    if (buf_offset + ALIGN_TO_4_BYTE(total_len) > wl->aggr_buf_size)
    {
        return -EAGAIN;
    }
    spare_blocks = TX_HW_BLOCK_SPARE;

    /* allocate free identifier for the packet */
    id = AllocTxId(wl, frame);
    if (id < 0)
    {
        return id;
    }

    /* memblocks should not include nab descriptor */
    total_blocks = CalcTxBlocks(total_skb_len, spare_blocks);
    TX_PRINT("michal1 total blocks %d\n\r", total_blocks);

    if (total_blocks <= wl->tx_blocks_available)
    {

        // In CC3xxx the packet starts with NAB command, only then the descriptor.
        nab_cmd->sync = HOST_SYNC_PATTERN;
        nab_cmd->opcode = NAB_SEND_CMD;
        nab_cmd->len = total_len - sizeof(NAB_header_t); // length should include the following 4 bytes of the NAB command.
        nab_cmd->desc_length = total_len - sizeof(NAB_tx_header_t);
        nab_cmd->sd = 0;
        nab_cmd->flags = NAB_SEND_FLAGS;

        desc = (TxIfDescriptor_t *)(frame->inBuff);

        desc->totalMemBlks = total_blocks;

        desc->descID = id;

        TX_PRINT("tx alocate id %u skb 0x%p tx_memblocks %d\n\r",id, frame,desc->totalMemBlks);

        wl->tx_blocks_available -= total_blocks;
        wl->tx_allocated_blocks += total_blocks;

        // TODO: extend to ac
        ac = 0;
        desc->ac = ac;
        wl->tx_allocated_pkts[ac]++;
        ret = 0;
        TX_PRINT("tx_allocate: size: %d, blocks: %d, id: %d",total_len, total_blocks, id);
    }
    else
    {
        free_tx_id(wl, id);
    }

    return ret;
}



void FillHdr(TxFrame_t *frame)
{
    TxIfDescriptor_t *desc;

    desc = (TxIfDescriptor_t *) frame->inBuff;
    desc->lifeTime = TX_HW_MGMT_PKT_LIFETIME_TU;

    /* queue */
    //Access Category and WMM - we only support best effort - 0
    desc->tid = 0;
    desc->hlid = 0;
    desc->txAttr = 0;
    desc->length = frame->inLen - sizeof(TxIfDescriptor_t);
}


/* caller must hold wl->mutex */
int PrepareTxFrame(TxCB_t *wl,TxFrame_t *frame, uint32_t buf_offset)
{
    int ret = 0;
    uint32_t total_len;
    NAB_tx_header_t nab_cmd;

    ret = Allocate(wl, frame, buf_offset, &nab_cmd);
    TX_PRINT("\n\r PrepareTxFrame %d\n\r", ret);

    if (ret < 0)
        return ret;

    FillHdr(frame);

    total_len = ALIGN_TO_4_BYTE(frame->inLen);

    memcpy(wl->aggr_buf + buf_offset, &nab_cmd, sizeof(NAB_tx_header_t));
    memcpy(wl->aggr_buf + buf_offset + sizeof(NAB_tx_header_t), frame->inBuff, frame->inLen);
    memset(wl->aggr_buf + buf_offset + sizeof(NAB_tx_header_t) + frame->inLen, 0, total_len - frame->inLen);

    return (total_len + sizeof(NAB_tx_header_t));
}



int WriteDataMassage(uint8_t *inBuf, size_t inLen)
{
    TTxnStruct *pCmdTxn;
    ETxnStatus ret = TXN_STATUS_OK;

    pCmdTxn = os_zalloc(sizeof(TTxnStruct));
    if(!pCmdTxn)
    {
        ASSERT_GENERAL(0);
        return -1;
    }

    /* Build the command TxnStruct */
    TXN_PARAM_SET(pCmdTxn, TXN_LOW_PRIORITY, TXN_FUNC_ID_WLAN, TXN_DIRECTION_WRITE, TXN_FIXED_ADDR)
    BUILD_TTxnStruct(pCmdTxn, NAB_DATA_ADDR, inBuf, inLen, NULL, NULL)

    /* Send the command */
    ret = twIf_Transact(pCmdTxn);


    if(ret != TXN_STATUS_COMPLETE)
    {
        ASSERT_GENERAL(0);
        return -1;
    }
    os_free(pCmdTxn);

    return 0;

}



void TxThread()
{
    TxFrame_t *frame;
    uint8_t *scratchBuffer;
    uint32_t buf_offset = 0;
    uint32_t last_len = 0;
    uint32_t transfer_len = 0;
    uint32_t padding_size = 0;
    int ret;
    WlanEventError_t  error_event;

    scratchBuffer = fwEvent_GetRxTxBuffer();
    if(NULL == scratchBuffer)
    {
        return;
    }

    while(1)
    {
        trnspt_EnterCriticalSection();
        frame = que_Dequeue(gTxCB->txQ);
        trnspt_LeaveCriticalSection();
        if(frame == NULL)
        {
            break;
        }

        ret = PrepareTxFrame(gTxCB,frame, buf_offset);

        if (ret == -EAGAIN)
        {
            /*
             * Aggregation buffer is full.
             * Flush buffer and try again.
             */
            trnspt_EnterCriticalSection();
            if(que_Requeue(gTxCB->txQ,frame) < 0)
            {            	
                TX_PRINT("reque failed \r\n");
                ASSERT_GENERAL(0);
                error_event.error_num = WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__REQUEUE);
                error_event.module = WLAN_MODULE_TX_THREAD;
                error_event.severity = WLAN_SEVERITY_MID;
                Wlan_HostSendEvent(WLAN_EVENT_ERROR, (uint8_t*)&error_event, sizeof(WlanEventError_t));
            }

            trnspt_LeaveCriticalSection();

            transfer_len = ALIGN_TO_256_BYTE(buf_offset);

            padding_size = transfer_len - buf_offset;
            memset(gTxCB->aggr_buf + buf_offset, 0x33, padding_size);

            TX_PRINT("sdio transaction length: %d\n\r",transfer_len);

            ret = WriteDataMassage(gTxCB->aggr_buf,transfer_len);
            if (ret < 0)
            {
                TX_PRINT("WriteDataMassage failed \r\n");
                error_event.error_num = WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__TX_WRITE);
                error_event.module = WLAN_MODULE_TX_THREAD;
                error_event.severity = WLAN_SEVERITY_HIGH;
                Wlan_HostSendEvent(WLAN_EVENT_ERROR, (uint8_t*)&error_event, sizeof(WlanEventError_t));
            }

            buf_offset = 0;
            continue;
        }
        else if (ret == -EBUSY)
        {
            trnspt_EnterCriticalSection();
            if(que_Requeue(gTxCB->txQ,frame) < 0)
            {
                TX_PRINT("reque failed 1\r\n");
                ASSERT_GENERAL(0);
                error_event.error_num = WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__REQUEUE);
                error_event.module = WLAN_MODULE_TX_THREAD;
                error_event.severity = WLAN_SEVERITY_MID;
                Wlan_HostSendEvent(WLAN_EVENT_ERROR, (uint8_t*)&error_event, sizeof(WlanEventError_t));
            }
            trnspt_LeaveCriticalSection();
            goto out_ack;
        }
        else if(ret < 0)
        {
            TX_PRINT("Got minus value when trying to tx\n\r");
            ASSERT_GENERAL(0);
            error_event.error_num = WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__TX, WLAN_ERROR_TYPE__TX_WRITE);
            error_event.module = WLAN_MODULE_TX_THREAD;
            error_event.severity = WLAN_SEVERITY_HIGH;
            Wlan_HostSendEvent(WLAN_EVENT_ERROR, (uint8_t*)&error_event, sizeof(WlanEventError_t));

        }
        last_len = ret;
        buf_offset += last_len;
        gTxCB->tx_packets_count++;
    }

out_ack:
    if(buf_offset)
    {
        transfer_len = ALIGN_TO_256_BYTE(buf_offset);

        padding_size = transfer_len - buf_offset;
        memset(gTxCB->aggr_buf + buf_offset, 0x33, padding_size);

        TX_PRINT("sdio transaction out of the while length: %d\n\r",transfer_len);

        WriteDataMassage(gTxCB->aggr_buf,transfer_len);


    }
}


