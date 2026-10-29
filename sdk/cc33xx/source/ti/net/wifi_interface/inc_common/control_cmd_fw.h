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
#ifndef CONTROL_CMD_FW_H_
#define CONTROL_CMD_FW_H_

#include "wlan_if.h"
#include "nab.h"
#include "osi_type.h"
#include "fw_event_handle.h"
#include "macro_utils.h"
#include "conf.h"
#include "osprey_public_commands.h"


#define MSG_QUEUE_SIZE (4)
#define EVENT_STACK_SIZE (1024)
#define EVENT_TRANSPORT_THRD_PRIORITY (5)

#define LIST_SIZE_OFFSET (52U) //parameter of manifest container
#define RECORD_SUFFIX_SIZE (82U) //parameter of manifest container

#define OSPREY_INTERNAL_DESC_SIZE   200
#define NAB_EXTRA_BYTES 4

#define WAKE_UP_CONDITIONS_CFG (12)

#define BLE_ENABLE (12)
#define CMD_BLE_COMMANDS (39)

#define MAC_ADDR_SIZE              6

#define CMD_INI_DOWNLOAD_COMMAND_SIZE (sizeof(cmd_ini_params_download_t))
#define CMD_INI_FILE_SIZE (ALIGNN_TO_4(CC33X_CONF_SIZE + CMD_HEADER_SIZE + CMD_HEADER_SIZE + CMD_INI_DOWNLOAD_COMMAND_SIZE))

#define CTRL_CHANNELS_BITMAP_SIZE_24GHZ_5GHZ  (6)
#define CTRL_GET_CHANNEL_BITMAP_INDEX(channelNumber)  ((channelNumber) / 32) 
#define CTRL_GET_CHANNEL_BIT_IN_BITMAP(channelNumber)   ((channelNumber) % 32) 
#define CTRL_SET_ALL_CHANNELS (-1)
#define CTRL_MAX_CHANNEL_NUMBER (180)

typedef struct
{
    NAB_header_t NAB_header;
    uint16_t id;
    uint16_t status;
    uint32_t uppermacCommandID;
    /* payload */
    uint8_t data[0];
}cmd_header_t;

typedef struct
{
    NAB_header_t NAB_header;
    uint16_t id;
    uint16_t status;
    uint8_t data[0];
}cmd_header_generic_t;

typedef struct
{
    cmd_header_generic_t cmdHeader;
    BleCommandParams_t   bleCmdParams;
}cmd_ble_hci_command_t;

typedef struct
{
    cmd_header_generic_t cmdHeader;
    uint16_t id;
    uint16_t len;
}cmd_generic_cfg_t;

typedef struct
{
    NAB_header_t NAB_header;
    uint16_t id;
    uint16_t status;
    uint32_t length;
    uint8_t payload[0];
}cmd_download_t;

typedef struct
{
	cmd_header_generic_t header;
	uint32_t length;
	uint8_t payload[0];
}cmd_ini_params_download_t;

typedef struct
{
    cmd_header_t hdr;
}EnableBLE_cmd_t;

typedef struct
{
    cmd_header_generic_t hdr;
    SetBdAddrParam_t setBdAddrCmd;
}cmd_set_bd_addr_t;

typedef struct
{
    cmd_generic_cfg_t       cmdGeneric;
    uint8_t                 seed[MAX_SEED_SIZE];
}cmd_cfg_seed_t;

typedef struct
{
    cmd_generic_cfg_t    cmdGeneric;
    uint8_t              sleep_auth;
    uint8_t              padding[3];
}enable_sleep_mode_cmd_t;

typedef struct
{
    cmd_generic_cfg_t       cmdGeneric;
    WlanLongSleepInterval   LsiParams;
    uint8_t                 padding[2];
}cmd_set_lsi_t;

typedef struct __attribute__((packed))
{
    uint8_t cmdTestId;
    uint8_t padding[3];
}cmd_test_header_t;
typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;

    TransceiverRxStatCounters_t rxStats;
}cmd_test_get_rx_stats_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;

    uint32_t dummy;
}cmd_test_stop_rx_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;

    TTestCmd_StartRX_t startRxParams;
}cmd_test_start_rx_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;

    uint32_t dummy;
}cmd_test_stop_tx_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;

    TTestCmd_StartTX_t setTxParams;
}cmd_test_set_tx_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;

    TTestCmd_StartTX_t startTxParams;
}cmd_test_start_tx_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;

    TTestCmd_channelTune_t tuneParams;
}cmd_test_channel_tune_t;

typedef struct __attribute__((packed))  {
	uint8_t mode;
	int8_t offset;
	int8_t output_power;
}start_tone_tx;

typedef struct __attribute__((packed))  {
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;
    start_tone_tx tx_params;
	uint8_t padding[1];
}cmd_test_start_tone_tx_t;

typedef struct __attribute__((packed)) {
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;
	uint8_t ant;
	uint8_t padding[3];
}cmd_test_switch_ant_t;

typedef struct __attribute__((packed)){
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;
	uint16_t calibration_bitmap;
	uint8_t padding[2];
}cmd_test_set_manual_calib_t;

typedef struct  __attribute__((packed)) {
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;
	uint16_t io_id;
	uint8_t  direction;
	uint8_t  state;
}cc33xx_cmd_set_io_cfg_t;

typedef struct  __attribute__((packed)) {
    cmd_header_generic_t cmdHeader;
    cmd_test_header_t cmdTestHeader;
	uint16_t io_id;
	uint8_t  direction;
	uint8_t  value;
}cc33xx_cmd_get_io_cfg_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    RoleID_t roleId;
    uint8_t padding[3];
}cmd_role_disable_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    RoleEnableParameters_t roleEnableParams;
    uint8_t padding;
}cmd_role_enable_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    RoleID_t roleId;
    uint8_t padding[3];
}cmd_role_stop_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    RoleStartParameters_t roleStartParams;
}cmd_role_start_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    uint32_t dummy;
}cmd_plt_disable_t;

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    uint32_t dummy;
}cmd_plt_enable_t;

typedef uint8_t                    TMacAddr[MAC_ADDR_SIZE];

typedef struct __attribute__((packed))
{
    cmd_header_generic_t cmdHeader;
    uint16_t id;
    uint16_t len;
}cmd_debug_header_t;

typedef struct __attribute__((packed))
 {
    cmd_header_generic_t cmdHeader;
    uint16_t id;
    uint16_t len;
}cmd_interrogate_header_t;

typedef struct __attribute__((packed))
{
    cmd_debug_header_t cmdDebugHeader;
    TransmitOMICfg_t transmitOmiParams;
}cmd_debug_transmit_omi_dbg_t;

typedef struct __attribute__((packed))
{
    cmd_debug_header_t cmdDebugHeader;
    UplinkPWRHDRCfg_t powerHeaderParams;
}cmd_debug_uplink_power_header_dbg_t;

typedef struct __attribute__((packed))
{
    cmd_debug_header_t cmdDebugHeader;
    ForcePsCfg_t forcePsmParams;
}cmd_debug_psm_dbg_t;

typedef struct __attribute__((packed))
{
    cmd_debug_header_t cmdDebugHeader;
    UplinkMUDataCfg_t uplinkMuDataParams;
}cmd_debug_uplink_mu_data_dbg_t;

typedef struct __attribute__((packed))
{
    cmd_debug_header_t cmdDebugHeader;
    MSCRateCfg_t mcsRateParams;
}cmd_debug_mcs_rate_dbg_t;

typedef struct __attribute__((packed))
{
    cmd_debug_header_t cmdDebugHeader;
    OperModeCTRLCfg_t operationModeControlParams;
}cmd_debug_operation_mode_control_dbg_t;

typedef struct __attribute__((packed))
{
    cmd_debug_header_t cmdDebugHeader;
    UplinkMUCfg_t uplinkMuParams;
}cmd_debug_uplink_mu_dbg_t;

typedef struct __attribute__((packed))
{
    cmd_debug_header_t cmdDebugHeader;
    GiLtfCfg_t giLtfParams;
}cmd_debug_gi_ltf_dbg_t;

typedef struct __attribute__((packed))
{
    cmd_debug_header_t cmdDebugInterrogateHeader;
    BeaconRssi_t beaconRssiParams;
}cmd_debug_beacon_rssi_dbg_t;

typedef struct __attribute__((packed))
{
    cmd_debug_header_t cmdDebugHeader;
    PmacHwRateMngOverrideCfg_t rateOverrideParams;
}cmd_debug_rate_override_dbg_t;

typedef struct __attribute__((packed))
{
    cmd_interrogate_header_t    cmdInterrogateHeader;
    FWVersions_t                FWVersions;
}cmd_interrogate_get_fwversion_t;

typedef struct __attribute__((packed))
{
    cmd_interrogate_header_t    cmdInterrogateHeader;
    BeaconRssi_t                BeaconRssi;
}cmd_interrogate_get_rssi_t;

int  ctrlCmdFw_DownloadIniParams();
void ctrlCmdFw_LockHostDriver();
void ctrlCmdFw_UnlockHostDriver();
int  ctrlCmdFw_CreateMsgQueue();
int  ctrlCmdFw_DeleteMsgQueue();
int  ctrlCmdFw_CreateEventThread();
int  ctrlCmdFw_DeleteEventThread();
int  ctrlCmdFw_ContainerDownload(char *containerName);
int  ctrlCmdFw_GetCmdSize();
int  ctrlCmdFw_GetFwVersion(FWVersions_t* FwVersion);
int  ctrlCmdFw_GetRssi(WlanBeaconRssi_t* rssi);

int  ctrlCmdFw_CalibratorCommand(CalibratorAction_e calibratorAction, void *calibratorCmdParams);
int  ctrlCmdFw_pltInit();
int  ctrlCmdFw_pltStop();

int  ctrlCmdFw_EnableBLECmd(void);
int  ctrlCmdFw_SetBdAddrCmd(const unsigned char *pBdAddr);
int  ctrlCmdFw_SetSeedCmd(uint8_t *pSeed, uint8_t size);

int8_t ctrlCmdFw_SetChannelInBitmap(uint32_t *channelBitmapArr, uint32_t channel);
int8_t ctrlCmdFw_CreateSelectedChannelsBitmap(WlanSelectedChannelsArray_t* channelArray ,uint32_t* listedChannelsBitmap);

#endif /* HOST_DRIVER_INIT_DEVICE_H_ */
