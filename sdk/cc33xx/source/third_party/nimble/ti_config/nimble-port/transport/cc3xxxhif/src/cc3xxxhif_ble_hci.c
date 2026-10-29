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
#include <string.h>
#include <syscfg/syscfg.h>
#include <sysinit/sysinit.h>
#include <nimble/ble.h>
#include <nimble/transport.h>
#include <nimble/transport/hci_h4.h>

#include "cc3xxxhif_ble_hci.h"
#include "ble_if.h"

#define HCI_MAX_PDU_LEN (255) /*!< \brief Maximum HCI packet data length */
#define HCI_MAX_HDR_LEN (5)   /*!< \brief Maximum HCI packet header length */
#define HCI_CMD_HDR_LEN (4)   /*!< \brief Command packet header length */
#define HCI_ACL_HDR_LEN (5)   /*!< \brief ACL packet header length */

/* Structure for holding outgoing HCI packets. */
typedef struct {
    uint8_t data[HCI_MAX_PDU_LEN+HCI_MAX_HDR_LEN];
    uint16_t len;
    uint16_t padding;
} hci_tl_pkt_t;

static struct hci_h4_sm cc3xxxhif_hci_h4sm;

int
cc3xxxhif_ble_hci_trans_read_cb(uint8_t* data, uint16_t len)
{
    int rlen;

    rlen = hci_h4_sm_rx(&cc3xxxhif_hci_h4sm, data, len);
    assert(rlen >= 0);

    return rlen;
}

#if !MYNEWT_VAL(BLE_CONTROLLER)
static int
cc3xxxhif_ble_hci_frame_cb(uint8_t pkt_type, void *data)
{
    int rc;

    switch (pkt_type) {
    case HCI_H4_ACL:
        rc = ble_transport_to_hs_acl(data);
        break;
    case HCI_H4_EVT:
        rc = ble_transport_to_hs_evt(data);
        break;
    default:
        assert(0);
        break;
    }

    return rc;
}

static int
cc3xxxhif_ble_hci_cmd_tx(void *buf)
{
    int rc = 0;
    uint8_t *cmd = buf; //NOTE: buf is a pointer to HCI packet without the packet type byte
    hci_tl_pkt_t hci_tl_pkt;
    uint8_t *ptr = (uint8_t *)hci_tl_pkt.data;

    *ptr = HCI_H4_CMD; //Set the packet type
    ptr++; //Move the ptr to point to the start of packet data
    hci_tl_pkt.len = HCI_CMD_HDR_LEN + cmd[2];
    os_memcpy(ptr, cmd, hci_tl_pkt.len-1);

    rc = BleIf_SendCommand(hci_tl_pkt.data, hci_tl_pkt.len);

    ble_transport_free(buf);

    return (rc < 0) ? BLE_ERR_UNKNOWN_HCI_CMD : 0;
}

static int
cc3xxxhif_ble_hci_acl_tx(struct os_mbuf *om)
{
    int rc = 0;
    hci_tl_pkt_t hci_tl_pkt;
    uint8_t *ptr = (uint8_t *)hci_tl_pkt.data;

    *ptr = HCI_H4_ACL; //Set the packet type
    ptr++; //Move the ptr to point to the start of packet data
    hci_tl_pkt.len = 1;

    os_mbuf_copydata(om, 0, OS_MBUF_PKTLEN(om), ptr);
    hci_tl_pkt.len += OS_MBUF_PKTLEN(om);

    rc = BleIf_SendCommand(hci_tl_pkt.data, hci_tl_pkt.len);

    os_mbuf_free_chain(om);

    return (rc < 0) ? BLE_ERR_MEM_CAPACITY :  0;
}

int
ble_transport_to_ll_cmd_impl(void *buf)
{
    return cc3xxxhif_ble_hci_cmd_tx(buf);
}

int
ble_transport_to_ll_acl_impl(struct os_mbuf *om)
{
    return cc3xxxhif_ble_hci_acl_tx(om);
}

int
ble_transport_to_ll_iso_impl(struct os_mbuf *om)
{
    return 0; //Not Supported
}

void
ble_transport_ll_init(void)
{
    hci_h4_sm_init(&cc3xxxhif_hci_h4sm, &hci_h4_allocs_from_ll,
                   cc3xxxhif_ble_hci_frame_cb);

    BleIf_VendorSpecificEventFormat(VENDOR_SPECIFIC_FORMAT_NIMBLE);
    BleIf_EventCbRegister(cc3xxxhif_ble_hci_trans_read_cb);
}

#endif /* !BLE_CONTROLLER */
