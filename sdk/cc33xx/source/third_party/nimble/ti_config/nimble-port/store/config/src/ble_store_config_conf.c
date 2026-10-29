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

#include "syscfg/syscfg.h"

#if MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST)

#include <inttypes.h>
#include <string.h>

#include "sysinit/sysinit.h"
#include "host/ble_hs.h"
#include "base64/base64.h"
#include "store/config/ble_store_config.h"
#include "../src/ble_store_config_priv.h"

#include <nimble/nimble_osi_filesystem.h>

#define BLE_STORE_CONFIG_SEC_ENCODE_SZ      \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_sec))

#define BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ  \
    (MYNEWT_VAL(BLE_STORE_MAX_BONDS) * BLE_STORE_CONFIG_SEC_ENCODE_SZ + 1)

#define BLE_STORE_CONFIG_CCCD_ENCODE_SZ     \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_cccd))

#define BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ \
    (MYNEWT_VAL(BLE_STORE_MAX_CCCDS) * BLE_STORE_CONFIG_CCCD_ENCODE_SZ + 1)

static void
ble_store_config_serialize_arr(const void *arr, int obj_sz, int num_objs,
                               char *out_buf, int buf_sz)
{
    int arr_size;

    arr_size = obj_sz * num_objs;
    assert(arr_size <= buf_sz);

    nimble_base64_encode(arr, arr_size, out_buf, 1);
}

static int
ble_store_config_deserialize_arr(const char *enc,
                                 void *out_arr,
                                 int obj_sz,
                                 int *out_num_objs)
{
    int len;

    len = nimble_base64_decode(enc, out_arr);
    if (len < 0) {
        return OS_EINVAL;
    }

    *out_num_objs = len / obj_sz;
    return 0;
}

static int
ble_store_config_conf_set(const char *setting_name, char *val)
{
    int rc;

    if (strcmp(setting_name, "our_sec") == 0) {
        rc = ble_store_config_deserialize_arr(
                val,
                ble_store_config_our_secs,
                sizeof *ble_store_config_our_secs,
                &ble_store_config_num_our_secs);
        return rc;
    } else if (strcmp(setting_name, "peer_sec") == 0) {
        rc = ble_store_config_deserialize_arr(
                val,
                ble_store_config_peer_secs,
                sizeof *ble_store_config_peer_secs,
                &ble_store_config_num_peer_secs);
        return rc;
    } else if (strcmp(setting_name, "cccd") == 0) {
        rc = ble_store_config_deserialize_arr(
                val,
                ble_store_config_cccds,
                sizeof *ble_store_config_cccds,
                &ble_store_config_num_cccds);
        return rc;
    }
    return OS_ENOENT;
}

static int
ble_store_config_persist_sec_set(const char *setting_name,
                                 const struct ble_store_value_sec *secs,
                                 int num_secs)
{
    char *buf;
    int rc;

    buf = os_malloc(BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ);
    if (buf == NULL)
    {
        Report("\n\rCouldn't allocate memory for persisting\n\r");
        return BLE_HS_ESTORE_FAIL;
    }

    ble_store_config_serialize_arr(secs, sizeof *secs, num_secs,
                                   buf, BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ);
    rc = conf_save_one(setting_name, buf, BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ);

    os_free(buf);

    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }

    return 0;
}

int
ble_store_config_persist_our_secs(void)
{
    int rc;

    rc = ble_store_config_persist_sec_set("our_sec",
                                          ble_store_config_our_secs,
                                          ble_store_config_num_our_secs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
ble_store_config_persist_peer_secs(void)
{
    int rc;

    rc = ble_store_config_persist_sec_set("peer_sec",
                                          ble_store_config_peer_secs,
                                          ble_store_config_num_peer_secs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
ble_store_config_persist_cccds(void)
{
    char buf[BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ];
    int rc;

    ble_store_config_serialize_arr(ble_store_config_cccds,
                                   sizeof *ble_store_config_cccds,
                                   ble_store_config_num_cccds,
                                   buf,
                                   sizeof buf);
    rc = conf_save_one("cccd", buf, sizeof buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }

    return 0;
}

void
ble_store_config_conf_init(void)
{
    char *our_buf;
    char *peer_buf;
    char *cccd_buf;

    our_buf = os_malloc(BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ);
    if (our_buf == NULL)
    {
        Report("\n\rCouldn't allocate memory for our persisting\n\r");
        return;
    }
    if (conf_read_one("our_sec", our_buf, BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ) == 0)
    {
        ble_store_config_conf_set("our_sec", our_buf);
    }
    os_free(our_buf);

    peer_buf = os_malloc(BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ);
    if (peer_buf == NULL)
    {
        Report("\n\rCouldn't allocate memory for peer persisting\n\r");
        return;
    }
    if (conf_read_one("peer_sec", peer_buf, BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ) == 0)
    {
        ble_store_config_conf_set("peer_sec", peer_buf);
    }
    os_free(peer_buf);

    cccd_buf = os_malloc(BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ);
    if (cccd_buf == NULL)
    {
        Report("\n\rCouldn't allocate memory for cccd\n\r");
        return;
    }
    if (conf_read_one("cccd", ble_store_config_cccds, BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ) == 0)
    {
        ble_store_config_conf_set("cccd", cccd_buf);
    }
    os_free(cccd_buf);
}

#endif /* MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST) */
