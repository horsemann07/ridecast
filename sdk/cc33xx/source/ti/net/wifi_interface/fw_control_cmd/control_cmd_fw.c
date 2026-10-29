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

#include <stdio.h>
#include <stdint.h>
#include "wlan_if.h"
#include "macro_utils.h"
#include "trnspt_thread.h"
#include "trnspt_if.h"
#include "commands.h"
#include "timer.h"
#include "bus_if.h"
#include <control_cmd_fw.h>
#include "fw_event_if.h"
#include <wlan_irq_adapt.h>
#include "errors.h"
#include "rx.h"
#include "tx.h"
#include "osi_kernel.h"
#include "fw_event_handle.h"
#include "conf.h"
#include "osprey_public_commands.h"


/*this is the NAB address for MX cmd APP2NAB_ADDR, it is not used on MX, because the SDIO wrapper is not there
 * so for passing the validation on twIf_Transact function, it is set the same as in CC3xxx */
uint32_t g_cmd_box_addr = NAB_DATA_ADDR ;
OsiLockObj_t driverLock = NULL;
OsiMsgQ_t eventMsgQueue = NULL;
OsiThread_t eventThread=NULL;
WlanEventHandlerCB_t userHandler = NULL;
OsiSyncObj_t gEventThrdRunStopSyncObject = NULL;
OsiSyncObj_t gEventThrdDummySleep = NULL;

Bool_e gPlt = FALSE;
CalibratorPowerModes_e gPltState = PLT_OFF;
RoleID_t gTransceiverRoleId = 0xFF;

void eventEntryThread();


#define IS_WLAN_EVENT_MESSAGE_STOP_RUN(eventMessage)    ( (*((uint8_t *)(eventMessage)) == 0xFF) && (*(((uint8_t *)(eventMessage))+1) == 0xFF))

#define CMD_UPPER_MAC_COMMANDS 35 //this can be any number, only rx and tx goes to specific location
#define CMD_CONTAINER_COMMANDS 36 //this can be any number, only rx and tx goes to specific location

#define INVALID_ROLE_ID            (0xFF)
#define CHUNK_SIZE (600)
#define RECORD_MAGIC_NUM (0xB007C0DE)
#define CMD_BM_READ_DEVICE_INFO (CMD_UPPER_MAC_COMMANDS)

typedef struct
{
    uint32_t magicNum;
    uint16_t type;
    uint8_t  sn;
    uint8_t  digestType;
    uint32_t len;
}recordHeader_t;


int WLSendFW_CfgCommand(uint16_t opcode, uint32_t id, void *cmd, uint32_t len, void *out, uint32_t outLen)
{
    int ret = 0;

    cmd_generic_cfg_t *cmdHdr = (cmd_generic_cfg_t *)cmd;
    cmdHdr->cmdHeader.NAB_header.len = len;
    cmdHdr->cmdHeader.NAB_header.opcode = opcode;
    cmdHdr->cmdHeader.NAB_header.sync_pattern = HOST_SYNC_PATTERN;
    cmdHdr->cmdHeader.id = opcode;
    cmdHdr->cmdHeader.status = 0;
    cmdHdr->id = id;
    cmdHdr->len = len - sizeof(cmd_generic_cfg_t);


    ret = cmd_Send(cmd, len, out, outLen);

    return ret;
}

int WLSendFW_ConfigureCommand(uint32_t id, void *cmd, uint32_t len, void *out, uint32_t outLen)
{
    return WLSendFW_CfgCommand(CMD_CONFIGURE, id, cmd, len, out, outLen);
}

int WLSendFWGetDeviceInfoCommand( void *out,uint32_t outLen)
{
    cmd_download_t cmd_dnld;
    cmd_download_t* cmd = &cmd_dnld;
    int len = sizeof(cmd_dnld);

    int ret = 0;
    cmd_download_t *cmdHdr = (cmd_download_t *)cmd;
    cmdHdr->NAB_header.len = len + sizeof(cmd_download_t);
    cmdHdr->NAB_header.opcode = CMD_BM_READ_DEVICE_INFO;
    cmdHdr->NAB_header.sync_pattern = HOST_SYNC_PATTERN;
    cmdHdr->id = CMD_BM_READ_DEVICE_INFO;
    cmdHdr->status = 0;
    cmdHdr->length = len;

    ret = cmd_Send((char *)cmd, len + sizeof(cmd_download_t), out, outLen);

    return ret;
}

int WLSendFW_DebugCommand(uint32_t id, void *cmd, uint32_t len, void *out, uint32_t outLen)
{
    return WLSendFW_CfgCommand(CMD_DEBUG, id, cmd, len, out, outLen);
}

int WLSendFW_Command(uint16_t opcode, void *cmd, uint32_t len, void *out, uint32_t outLen)
{
    int ret = 0;

    cmd_header_generic_t *cmdHdr = (cmd_header_generic_t *)cmd;
    cmdHdr->NAB_header.len = len;
    cmdHdr->NAB_header.opcode = opcode;
    cmdHdr->NAB_header.sync_pattern = HOST_SYNC_PATTERN;
    cmdHdr->id = opcode;
    cmdHdr->status = 0;
    ret = cmd_Send(cmd, len, out, outLen);

    return ret;
}


int WLSendFWDownloadCommand(void *cmd, uint32_t len, void *out,uint32_t outLen)
{
    int ret = 0;
    cmd_download_t *cmdHdr = (cmd_download_t *)cmd;
    cmdHdr->NAB_header.len = len + sizeof(cmd_download_t);
    cmdHdr->NAB_header.opcode = CMD_CONTAINER_COMMANDS;
    cmdHdr->NAB_header.sync_pattern = HOST_SYNC_PATTERN;
    cmdHdr->id = CMD_CONTAINER_DOWNLOADS;
    cmdHdr->status = 0;
    cmdHdr->length = len;

    ret = cmd_Send(cmd, len + sizeof(cmd_download_t), out, outLen);

    return ret;
}

int ctrlCmdFw_WLSendCommand(uint32_t id, void *cmd, uint32_t len, void *out,uint32_t outLen)
{
    int ret = 0;
    cmd_header_t *cmdHdr = (cmd_header_t *)cmd;
    cmdHdr->NAB_header.len = len;
    cmdHdr->NAB_header.opcode = CMD_UPPER_MAC_COMMANDS;
    cmdHdr->NAB_header.sync_pattern = HOST_SYNC_PATTERN;
    cmdHdr->id = CMD_UPPER_MAC_COMMANDS;
    cmdHdr->status = 0;
    cmdHdr->uppermacCommandID = id;

    ret = cmd_Send(cmd, len, out, outLen);

    return ret;
}

int ctrlCmdFw_GetCmdSize()
{
    return sizeof(cmd_download_t);
}



int ctrlCmdFw_DownloadIniParams()
{
    // Send cmd to FW
    cmd_ini_params_download_t *cmd = NULL;
    uint32_t cmdSize;
    uint32_t cmdSizeForIni = CMD_MAX_SIZE;
    int ret = 0;

    if(CMD_INI_FILE_SIZE > CMD_MAX_SIZE)
    {
        cmdSizeForIni = CMD_INI_FILE_SIZE;
    }

    cmd_SetBufferSize(cmdSizeForIni);

    cmdSize = sizeof(cmd_ini_params_download_t) + CC33X_CONF_SIZE;
    
    cmdSize = ALIGNN_TO_4(cmdSize);

    cmd = os_malloc(cmdSize);

    if (NULL == cmd)
    {
        Report("\n\rCouldn't allocate memory for calibration command\n\r");
        ASSERT_GENERAL(0);
        return OSI_MEMORY_ALLOCATION_FAILURE;
    }

    FILE *IniFileHandle = osi_fopen("cc33xx-conf", "rb");

    if(!IniFileHandle)
    {
        Report("\n\rERROR: Cant Open/Find cc33xx-conf.bin file\n\r");
        ASSERT_GENERAL(0);
        return OSI_FILESYS_FAILURE;
    }
    else
    {
        osi_fread((cmd->payload), CC33X_CONF_SIZE, 0, IniFileHandle);
        cmd->length = CC33X_CONF_SIZE;
        osi_fclose(IniFileHandle);
    }


    ctrlCmdFw_LockHostDriver();

    WLSendFW_Command(CMD_DOWNLOAD_INI_PARAMS, cmd, cmdSize, NULL, 0);

    ctrlCmdFw_UnlockHostDriver();

    os_free(cmd);

    //return cmd buffer to default buffer with buffer size = CMD_MAX_SIZE
    ret = cmd_SetBufferSize(0);
    if(ret < 0)
    {
        Report("Failed to Allocate Memory for cmd Buffer");
        ASSERT_GENERAL(0);
        return OSI_MEMORY_ALLOCATION_FAILURE;
    }

    return ret;
}

int ctrlCmdFw_cmdRoleDisable(RoleID_t *roleId)
{
    int ret = 0;
    cmd_role_disable_t cmdRoleDisable;

    if (*roleId == INVALID_ROLE_ID)
    {
        Report("\r\nCan't disable an invalid role");
    }

    cmdRoleDisable.roleId = *roleId;

    ret = WLSendFW_Command(CMD_ROLE_DISABLE,
                           (void *)&cmdRoleDisable,
                           sizeof(cmd_role_disable_t),
                           NULL,
                           0);

    *roleId = INVALID_ROLE_ID;

    return ret;
}

int ctrlCmdFw_cmdRoleEnable(RoleID_t *roleId, uint8_t *macAddress, RoleType_e roleType)
{
    int8_t rc, retVal;
    cmd_role_enable_t roleEnableCmd;
    CommandComplete_t cmdComplete;

    RoleEnableReturn_t *cmdReturn = (RoleEnableReturn_t*)&cmdComplete.info.param;
    RoleEnableParameters_t *cmdEnableParams = (RoleEnableParameters_t *)&roleEnableCmd.roleEnableParams;

    cmdEnableParams->roleType = roleType;
    cmdEnableParams->staRoleIdx = 0;
    os_memcpy(cmdEnableParams->macAddress, macAddress, MAC_ADDR_LEN);

    retVal = WLSendFW_Command(CMD_ROLE_ENABLE,
                              (void *)&roleEnableCmd,
                              sizeof(cmd_role_enable_t),
                              (char *)&(cmdComplete.info.param),
                              sizeof(RoleEnableReturn_t));

    cmdComplete.info.status = retVal;
    if (retVal >= 0)
    {
        rc = cmdComplete.info.status;
        *roleId = cmdReturn->roleID; // Save on global context and not in ptr?
        // Mark role wasn't started yet ?
    }
    else
    {
        rc =  retVal;
    }

    return rc;
}

int ctrlCmdFw_cmdRoleStop(RoleID_t roleId)
{
    int ret = 0;
    cmd_role_stop_t cmdRoleStop;

    cmdRoleStop.roleId = roleId;

    ret = WLSendFW_Command(CMD_ROLE_STOP,
                           (void *)&cmdRoleStop,
                           sizeof(cmd_role_stop_t),
                           NULL,
                           0);
    return ret;
}

int ctrlCmdFw_transceiverRoleStop(RoleID_t roleId)
{
    int ret = 0;

    if (!gPlt)
    {
        return WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__PLT_NOT_STARTED);
    }

    ret = ctrlCmdFw_cmdRoleStop(roleId);

    return ret;
}

int ctrlCmdFw_cmdRoleStart(RoleID_t roleId, RoleType_e roleType, uint8_t band, uint8_t channel)
{
    int ret = 0;
    cmd_role_start_t cmdRoleStart;

    cmdRoleStart.roleStartParams.roleType = roleType;
    cmdRoleStart.roleStartParams.roleID = roleId;
    cmdRoleStart.roleStartParams.band = band;
    cmdRoleStart.roleStartParams.channelNumber = channel;

    ret = WLSendFW_Command(CMD_ROLE_START,
                           (void *)&cmdRoleStart,
                           sizeof(cmd_role_start_t),
                           NULL,
                           0);
    return ret;
}

int ctrlCmdFw_cmdPltDisable()
{
    int ret = 0;
    cmd_plt_disable_t cmdPltDisable;

    ret = WLSendFW_Command(CMD_PLT_DISABLE,
                           (void *)&cmdPltDisable,
                           sizeof(cmd_plt_disable_t),
                           NULL,
                           0);
    return ret;
}

int ctrlCmdFw_cmdPltEnable()
{
    int ret = 0;
    cmd_plt_enable_t cmdPltEnable;

    ret = WLSendFW_Command(CMD_PLT_ENABLE,
                           (void *)&cmdPltEnable,
                           sizeof(cmd_plt_enable_t),
                           NULL,
                           0);
    return ret;
}

int ctrlCmdFw_pltInit()
{
    int ret = 0;
    RoleID_t roleId;
    uint8_t broadcastAddress[MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    if (gPltState == PLT_ON)
    {
        Report("\r\nPLT already on");
        return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__PLT_ALREADY_ON);
    }

    ret = ctrlCmdFw_cmdRoleEnable(&roleId, broadcastAddress, ROLE_TRANSCEIVER);
    gTransceiverRoleId = roleId;

    if (ret < 0)
    {
        Report("\r\nPLT role enable failed");
        return ret;
    }

    /* Default values are 2.4GHz and channel 6 */
    ret = ctrlCmdFw_cmdRoleStart(roleId, ROLE_TRANSCEIVER, 0, 6);

    if (ret < 0)
    {
        Report("\r\nPLT role start failed");
        ctrlCmdFw_cmdRoleDisable(&roleId);
        return ret;
    }

    ret = ctrlCmdFw_cmdPltEnable();
    if (ret < 0)
    {
        Report("\r\nPLT enable failed");

        ret = ctrlCmdFw_cmdRoleDisable(&roleId);
        if (ret < 0)
        {
            Report("\r\nPLT role disable failed");
        }

        ret = ctrlCmdFw_transceiverRoleStop(gTransceiverRoleId);
        return ret;
    }

    gPlt = TRUE;
    gPltState = PLT_ON;

    Report("\r\nPLT enabled!");

    return ret;
}

int ctrlCmdFw_pltStop()
{
    int ret = 0;

    ret = ctrlCmdFw_transceiverRoleStop(gTransceiverRoleId);
    if (ret < 0)
    {
        return ret;
    }

    ret = ctrlCmdFw_cmdRoleDisable(&gTransceiverRoleId);
    if (ret < 0)
    {
        return ret;
    }

    ret = ctrlCmdFw_cmdPltDisable();
    if (ret < 0)
    {
        return ret;
    }

    gPlt = FALSE;
    gPltState = PLT_OFF;

    Report("\r\nPLT Disabled");
    
    return ret;
}

Bool_e isCalibratorPowerNeeded(CalibratorAction_e calibratorAction)
{
    Bool_e needed = FALSE;

    switch (calibratorAction)
    {
        case (CALIBRATOR_ACTION_RATE_OVERRIDE):
        case (CALIBRATOR_ACTION_GET_BEACON_RSSI):
        case (CALIBRATOR_ACTION_SET_GI_LTF):
        case (CALIBRATOR_ACTION_SET_UPLINK_MU):
        case (CALIBRATOR_ACTION_SET_OPERATION_MODE_CONTROL):
        case (CALIBRATOR_ACTION_SET_MCS_RATE):
        case (CALIBRATOR_ACTION_SET_UPLINK_MU_DATA):
        case (CALIBRATOR_ACTION_SET_PSM):
        case (CALIBRATOR_ACTION_SET_UPLINK_POWER_HEADER):
        case (CALIBRATOR_ACTION_SET_TRANSMIT_OMI):
            needed = FALSE;
            break;
        default:
            needed = TRUE;
            break;
    }

    return needed;
}

int WLSendFW_CfgOrDebugCommand(uint16_t opcode, uint32_t id, void *cmd, uint32_t len, void *out, uint32_t outLen)
{
    int ret = 0;

    /* cmd cfg header is the same size as cmd debug header */
    cmd_generic_cfg_t *cmdHdr = (cmd_generic_cfg_t *)cmd;
    cmdHdr->cmdHeader.NAB_header.len = len;
    cmdHdr->cmdHeader.NAB_header.opcode = opcode;
    cmdHdr->cmdHeader.NAB_header.sync_pattern = HOST_SYNC_PATTERN;
    cmdHdr->cmdHeader.id = opcode;
    cmdHdr->cmdHeader.status = 0;
    cmdHdr->id = id;
    cmdHdr->len = len - sizeof(cmd_generic_cfg_t);


    ret = cmd_Send(cmd, len, out, outLen);

    return ret;
}

int WLSendFW_InterrogateCommand(uint32_t id, void *cmd, uint32_t len, void *out, uint32_t outLen)
{
    return WLSendFW_CfgOrDebugCommand(CMD_INTERROGATE, id, cmd, len, out, outLen);
}

int WLSendFW_DebugInterrogateCommand(uint32_t id, void *cmd, uint32_t len, void *out, uint32_t outLen)
{
    return WLSendFW_CfgOrDebugCommand(CMD_DEBUG_READ, id, cmd, len, out, outLen);
}

int ctrlCmdFw_GetFwVersion(FWVersions_t* FwVersion)
{
    cmd_interrogate_get_fwversion_t cmdGetFwVersion;
    CommandComplete_t cmdComplete;
    int ret;

    cmdGetFwVersion.cmdInterrogateHeader.id = GET_FW_VERSIONS_INTR;

    ret = WLSendFW_InterrogateCommand(GET_FW_VERSIONS_INTR,
                                      &cmdGetFwVersion,
                                      sizeof(cmdGetFwVersion),
                                      &cmdComplete,
                                      sizeof(uint16_t)/*sizeof(Command_e)*/ + \
                                      sizeof(uint16_t)/*sizeof(CommandStatus_e)*/ + \
                                      sizeof(FWVersions_t));


    os_memcpy(FwVersion, cmdComplete.info.param, sizeof(FWVersions_t));
    return ret;
}

int ctrlCmdFw_GetRssi(WlanBeaconRssi_t* rssi)
{
    cmd_interrogate_get_rssi_t cmdGetRssi;
    CommandComplete_t cmdComplete;
    int ret;

    cmdGetRssi.BeaconRssi.role_id = WLAN_ROLE_STA;
    cmdGetRssi.cmdInterrogateHeader.id = RSSI_INTR;

    ret = WLSendFW_InterrogateCommand(RSSI_INTR,
                                      &cmdGetRssi,
                                      sizeof(cmdGetRssi),
                                      &cmdComplete,
                                      sizeof(uint16_t)/*sizeof(Command_e)*/ + \
                                      sizeof(uint16_t)/*sizeof(CommandStatus_e)*/ + \
                                      sizeof(BeaconRssi_t));

    os_memcpy(rssi, cmdComplete.info.param, sizeof(BeaconRssi_t));
    return ret;
}

int ctrlCmdFw_CalibratorCommand(CalibratorAction_e calibratorAction, void *calibratorCmdParams)
{
    int ret = 0;
    CommandComplete_t cmdComplete;

    if ((isCalibratorPowerNeeded(calibratorAction)) &&
        (FALSE == gPlt))
    {
        Report("\r\nError! Turn on calibrator first!");
        return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__CALIBRATOR_OFF);
    }

    switch (calibratorAction)
    {
    case (CALIBRATOR_ACTION_CHANNEL_TUNE):
    {
        CalibratorChannelTuneParams_t *tuneParams;
        cmd_test_channel_tune_t cmdChannelTune;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        tuneParams = (CalibratorChannelTuneParams_t *)calibratorCmdParams;

        os_memcpy(&cmdChannelTune.tuneParams, tuneParams, sizeof(CalibratorChannelTuneParams_t));

        cmdChannelTune.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_CHANNEL_TUNE;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdChannelTune,
                               sizeof(cmd_test_channel_tune_t),
                               NULL,
                               0);
    }
        break;
    case (CALIBRATOR_ACTION_TX_START):
    {
        cmd_test_start_tx_t cmdStartTx;

        os_memset(&cmdStartTx.startTxParams, 0, sizeof(TTestCmd_StartTX_t));
        cmdStartTx.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_TX_START;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdStartTx,
                               sizeof(cmd_test_start_tx_t),
                               NULL,
                               0);
    }
        break;

    case(CALIBRATOR_ACTION_MANUAL_CALIBRATION):
    {
        uint16_t* pCalibration_bitmap;
        cmd_test_set_manual_calib_t cmdManualCalib;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        pCalibration_bitmap = (uint16_t*)(calibratorCmdParams);

        os_memcpy(&cmdManualCalib.calibration_bitmap, pCalibration_bitmap, sizeof(uint16_t));
        cmdManualCalib.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_MANUAL_CALIBRATION;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdManualCalib,
                               sizeof(cmd_test_set_manual_calib_t),
                               NULL,
                               0);

    }
        break;    

    case(CALIBRATOR_ACTION_TX_TONE_START):
    {
        cmd_test_start_tone_tx_t cmdTxTone;
        //tx_tone_params
        CalibratorStartTxToneParams_t* txTone;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }

        txTone = (CalibratorStartTxToneParams_t*)calibratorCmdParams;

        os_memcpy(&(cmdTxTone.tx_params), txTone, sizeof(CalibratorStartTxToneParams_t));


        cmdTxTone.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_TX_TONE_START;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdTxTone,
                               sizeof(cmd_test_start_tone_tx_t),
                               NULL,
                               0);
    }
        break;  
    case(CALIBRATOR_ACTION_SWITCH_ANT):
    {
        cmd_test_switch_ant_t cmdSwitchAnt;

        uint8_t* pAnt;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }

        pAnt = (uint8_t *)calibratorCmdParams;

        os_memcpy(&cmdSwitchAnt.ant, pAnt, sizeof(uint8_t));


        cmdSwitchAnt.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_SWITCH_ANT;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdSwitchAnt,
                               sizeof(cmd_test_switch_ant_t),
                               NULL,
                               0);
    }
        break;    
    case(CALIBRATOR_ACTION_SET_IO_CFG):
    {
        cc33xx_cmd_set_io_cfg_t cmdSetIoCfg;

        CalibratorSetIoCfgParams_t* pSetIoCfg;


        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }

        pSetIoCfg = (CalibratorSetIoCfgParams_t *)calibratorCmdParams;

        cmdSetIoCfg.io_id = pSetIoCfg->io_id;
        cmdSetIoCfg.direction = pSetIoCfg->direction;
        cmdSetIoCfg.state = pSetIoCfg->state;

        cmdSetIoCfg.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_SET_IO_CFG;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdSetIoCfg,
                               sizeof(cc33xx_cmd_set_io_cfg_t),
                               NULL,
                               0);
    }
        break; 
    case(CALIBRATOR_ACTION_GET_IO_CFG):
    {
        cc33xx_cmd_get_io_cfg_t cmdGetIoCfg;
        CalibratorGetIoCfgParams_t* pGetIoCfgParams;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }

        pGetIoCfgParams = (CalibratorGetIoCfgParams_t *)calibratorCmdParams;

        cmdGetIoCfg.io_id = pGetIoCfgParams->io_id;

        cmdGetIoCfg.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_GET_IO_CFG;


        ret = WLSendFW_Command(CMD_TEST,
                               &cmdGetIoCfg,
                               sizeof(cc33xx_cmd_get_io_cfg_t),
                               &cmdComplete,
                               sizeof(uint16_t)/*sizeof(Command_e)*/ + \
                               sizeof(uint16_t)/*sizeof(CommandStatus_e)*/ + \
                               sizeof(CalibratorGetIoCfgParams_t));

        os_memcpy(pGetIoCfgParams, cmdComplete.info.param, sizeof(CalibratorGetIoCfgParams_t));
    }
        break; 
    case(CALIBRATOR_ACTION__BLE_ENABLE):
    {
        cc33xx_cmd_set_io_cfg_t cmdSetIoCfg;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }

        cmdSetIoCfg.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_BLE_ENABLE;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdSetIoCfg,
                               sizeof(cc33xx_cmd_set_io_cfg_t),
                               NULL,
                               0);
    }
        break; 
    case (CALIBRATOR_ACTION_TX_STOP):
    {
        cmd_test_stop_tx_t cmdStopTx;

        cmdStopTx.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_TX_STOP;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdStopTx,
                               sizeof(cmd_test_stop_tx_t),
                               NULL,
                               0);
    }
        break;
    case (CALIBRATOR_ACTION_SET_TX_PARAMS):
    case (CALIBRATOR_ACTION_SET_TB_TX_PARAMS):
    {
        CalibratorStartTxParams_t *setTxParams;
        cmd_test_set_tx_t cmdSetTx;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        setTxParams = (CalibratorStartTxParams_t *)calibratorCmdParams;

        os_memcpy(&cmdSetTx.setTxParams, setTxParams, sizeof(TTestCmd_StartTX_t));
        cmdSetTx.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_SET_PARAMS;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdSetTx,
                               sizeof(cmd_test_set_tx_t),
                               NULL,
                               0);
    }
        break;
    case (CALIBRATOR_ACTION_GET_TX_PARAMS):
    {
        CalibratorStartTxParams_t *getTxParams;
        cmd_test_start_tx_t cmdGetTx;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        getTxParams = (CalibratorStartTxParams_t *)calibratorCmdParams;

        cmdGetTx.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_GET_PARAMS;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdGetTx,
                               sizeof(cmd_test_start_tx_t),
                               &cmdComplete,
                               sizeof(uint16_t)/*sizeof(Command_e)*/ + \
                               sizeof(uint16_t)/*sizeof(CommandStatus_e)*/ + \
                               sizeof(TTestCmd_StartTX_t));

        os_memcpy(getTxParams, cmdComplete.info.param, sizeof(TTestCmd_StartTX_t));
    }
        break;
    case (CALIBRATOR_ACTION_START_RX):
    {
        CalibratorStartRxParams_t *startRxParams;
        cmd_test_start_rx_t cmdStartRx;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        startRxParams = (CalibratorStartRxParams_t *)calibratorCmdParams;

        os_memcpy(&cmdStartRx.startRxParams, startRxParams, sizeof(CalibratorStartRxParams_t));
        cmdStartRx.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_START_RX_SIMULATION;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdStartRx,
                               sizeof(cmd_test_start_rx_t),
                               NULL,
                               0);
    }
    break;
    case (CALIBRATOR_ACTION_STOP_RX):
    {
        cmd_test_stop_rx_t cmdStopRx;
        cmdStopRx.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_STOP_RX_STATS;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdStopRx,
                               sizeof(cmd_test_stop_rx_t),
                               NULL,
                               0);
    }
        break;
    case (CALIBRATOR_ACTION_GET_RX_STATS):
    {
        CalibratorGetRxParams_t *getRxParams;
        cmd_test_get_rx_stats_t cmdGetRxStats;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        getRxParams = (CalibratorGetRxParams_t *)calibratorCmdParams;

        cmdGetRxStats.cmdTestHeader.cmdTestId = CC33XX_TEST_CMD_GET_RX_STATS;

        ret = WLSendFW_Command(CMD_TEST,
                               &cmdGetRxStats,
                               sizeof(cmd_test_get_rx_stats_t),
                               &cmdComplete,
                               sizeof(uint16)/*sizeof(Command_e)*/ + \
                               sizeof(uint16)/*sizeof(CommandStatus_e)*/ + \
                               sizeof(TransceiverRxStatCounters_t));

        os_memcpy(getRxParams, cmdComplete.info.param, sizeof(TransceiverRxStatCounters_t));
    }
        break;
    case (CALIBRATOR_ACTION_RATE_OVERRIDE):
    {
        CalibratorRateOverrideParams_t *rateOverrideParams;
        cmd_debug_rate_override_dbg_t cmdRateOverride;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        rateOverrideParams = (CalibratorRateOverrideParams_t *)calibratorCmdParams;

        os_memcpy(&cmdRateOverride.rateOverrideParams, rateOverrideParams, sizeof(CalibratorRateOverrideParams_t));

        ret = WLSendFW_DebugCommand(RATE_OVERRRIDE_CFG,
                                    &cmdRateOverride,
                                    sizeof(cmd_debug_rate_override_dbg_t),
                                    NULL,
                                    0);
    }
        break;
    case (CALIBRATOR_ACTION_GET_BEACON_RSSI):
    {
        CalibratorBeaconRssiParams_t *beaconRssiParams;
        cmd_debug_beacon_rssi_dbg_t cmdGetBeaconRssi;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        beaconRssiParams = (CalibratorBeaconRssiParams_t *)calibratorCmdParams;

        ret = WLSendFW_DebugInterrogateCommand(BEACON_RSSI_INTR,
                                               &cmdGetBeaconRssi,
                                               sizeof(cmd_debug_beacon_rssi_dbg_t),
                                               &cmdComplete,
                                               sizeof(Command_e) + \
                                               sizeof(CommandStatus_e) + \
                                               sizeof(BeaconRssi_t));

        os_memcpy(beaconRssiParams, cmdComplete.info.param, sizeof(BeaconRssi_t));
    }
        break;
    case (CALIBRATOR_ACTION_SET_GI_LTF):
    {
        CalibratorGiLtfModeParams_t *giLtfParams;
        cmd_debug_gi_ltf_dbg_t cmdGiLtfCfg;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        giLtfParams = (CalibratorGiLtfModeParams_t *)calibratorCmdParams;

        os_memcpy(&cmdGiLtfCfg.giLtfParams, giLtfParams, sizeof(CalibratorGiLtfModeParams_t));

        ret = WLSendFW_DebugCommand(GI_LTF_CFG,
                                    &cmdGiLtfCfg,
                                    sizeof(cmd_debug_gi_ltf_dbg_t),
                                    NULL,
                                    0);
    }
        break;
    case (CALIBRATOR_ACTION_SET_UPLINK_MU):
    {
        CalibratorUplinkMuParams_t *uplinkMuParams;
        cmd_debug_uplink_mu_dbg_t cmdUplinkMuCfg;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        uplinkMuParams = (CalibratorUplinkMuParams_t *)calibratorCmdParams;

        os_memcpy(&cmdUplinkMuCfg.uplinkMuParams, uplinkMuParams, sizeof(CalibratorUplinkMuParams_t));

        ret = WLSendFW_DebugCommand(UPLINK_MULTI_USER_CFG,
                                    &cmdUplinkMuCfg,
                                    sizeof(cmd_debug_uplink_mu_dbg_t),
                                    NULL,
                                    0);
    }
        break;
    case (CALIBRATOR_ACTION_SET_OPERATION_MODE_CONTROL):
    {
        CalibratorOperationModeControlParams_t *operationModeControlParams;
        cmd_debug_operation_mode_control_dbg_t cmdOperationModeCfg;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        operationModeControlParams = (CalibratorOperationModeControlParams_t *)calibratorCmdParams;

        os_memcpy(&cmdOperationModeCfg.operationModeControlParams, operationModeControlParams, sizeof(CalibratorOperationModeControlParams_t));

        ret = WLSendFW_DebugCommand(OPERATION_MODE_CTRL_CFG,
                                    &cmdOperationModeCfg,
                                    sizeof(cmd_debug_operation_mode_control_dbg_t),
                                    NULL,
                                    0);
    }
        break;
    case (CALIBRATOR_ACTION_SET_MCS_RATE):
    {
        CalibratorSetMcsRateParams_t *mcsRateParams;
        cmd_debug_mcs_rate_dbg_t cmdMcsRateCfg;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        mcsRateParams = (CalibratorSetMcsRateParams_t *)calibratorCmdParams;

        os_memcpy(&cmdMcsRateCfg.mcsRateParams, mcsRateParams, sizeof(CalibratorSetMcsRateParams_t));

        ret = WLSendFW_DebugCommand(MCS_FIXED_RATE_CFG,
                                    &cmdMcsRateCfg,
                                    sizeof(cmd_debug_mcs_rate_dbg_t),
                                    NULL,
                                    0);
    }
        break;
    case (CALIBRATOR_ACTION_SET_UPLINK_MU_DATA):
    {
        CalibratorSetUplinkMuDataParams_t *uplinkMuDataParams;
        cmd_debug_uplink_mu_data_dbg_t cmdUplinkMuDataCfg;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        uplinkMuDataParams = (CalibratorSetUplinkMuDataParams_t *)calibratorCmdParams;

        os_memcpy(&cmdUplinkMuDataCfg.uplinkMuDataParams, uplinkMuDataParams, sizeof(CalibratorSetUplinkMuDataParams_t));

        ret = WLSendFW_DebugCommand(UPLINK_MULTI_USER_DATA_CFG,
                                    &cmdUplinkMuDataCfg,
                                    sizeof(cmd_debug_uplink_mu_data_dbg_t),
                                    NULL,
                                    0);
    }
        break;
    case (CALIBRATOR_ACTION_SET_PSM):
    {
        CalibratorSetPsmParams_t *setPsmParams;
        cmd_debug_psm_dbg_t cmdPsmCfg;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        setPsmParams = (CalibratorSetPsmParams_t *)calibratorCmdParams;

        os_memcpy(&cmdPsmCfg.forcePsmParams, setPsmParams, sizeof(CalibratorSetPsmParams_t));

        ret = WLSendFW_DebugCommand(FORCE_PS_CFG,
                                    &cmdPsmCfg,
                                    sizeof(cmd_debug_psm_dbg_t),
                                    NULL,
                                    0);
    }
        break;
    case (CALIBRATOR_ACTION_SET_UPLINK_POWER_HEADER):
    {
        CalibratorSetPowerHeaderParams_t *setPowerHeaderParams;
        cmd_debug_uplink_power_header_dbg_t cmdPowerHeaderCfg;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        setPowerHeaderParams = (CalibratorSetPowerHeaderParams_t *)calibratorCmdParams;

        os_memcpy(&cmdPowerHeaderCfg.powerHeaderParams, setPowerHeaderParams, sizeof(CalibratorSetPowerHeaderParams_t));

        ret = WLSendFW_DebugCommand(UPLINK_POWER_HEADER_CFG,
                                    &cmdPowerHeaderCfg,
                                    sizeof(cmd_debug_uplink_power_header_dbg_t),
                                    NULL,
                                    0);
    }
        break;
    case (CALIBRATOR_ACTION_SET_TRANSMIT_OMI):
    {
        CalibratorSetTransmitOmiParams_t *setTransmitOmiParams;
        cmd_debug_transmit_omi_dbg_t cmdTransmitOmiCfg;

        if (NULL == calibratorCmdParams)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__INVALID_PARAM_CALIBRATOR);
        }
        setTransmitOmiParams = (CalibratorSetTransmitOmiParams_t *)calibratorCmdParams;

        os_memcpy(&cmdTransmitOmiCfg.transmitOmiParams, setTransmitOmiParams, sizeof(CalibratorSetTransmitOmiParams_t));

        ret = WLSendFW_DebugCommand(TRANSMIT_OMI_CFG,
                                    &cmdTransmitOmiCfg,
                                    sizeof(cmd_debug_transmit_omi_dbg_t),
                                    NULL,
                                    0);
    }
        break;
    default:
        Report("\r\nUnknown calibrator action requested.");
        break;
    }
    return ret;
}

int ctrlCmdFw_EnableBLECmd(void)
{
    // Send cmd to FW
    EnableBLE_cmd_t *cmd = NULL;
    uint32_t cmdSize = sizeof(EnableBLE_cmd_t);
    int ret = 0;

    cmd = os_malloc(cmdSize);

    if (NULL == cmd)
    {
        Report("\n\rCouldn't allocate memory for BLE enable command\n\r");
        ASSERT_GENERAL(0);
        return OSI_MEMORY_ALLOCATION_FAILURE;
    }

    ctrlCmdFw_LockHostDriver();

    ret = WLSendFW_DebugCommand(BLE_ENABLE, cmd, cmdSize, NULL, 0);

    ctrlCmdFw_UnlockHostDriver();

    os_free(cmd);

    return ret;
}

int ctrlCmdFw_SetBdAddrCmd(const unsigned char *pBdAddr)
{
    int ret = 0;
    cmd_set_bd_addr_t cmd;

    os_memcpy(cmd.setBdAddrCmd.BdAddr, pBdAddr, MAC_ADDR_LEN);

    ret = WLSendFW_Command(CMD_SET_BD_ADDR,
                           (void *)&cmd,
                           sizeof(cmd_set_bd_addr_t),
                           NULL,
                           0);

    return ret;
}

int ctrlCmdFw_SetSeedCmd(uint8_t *pSeed, uint8_t size)
{
    cmd_cfg_seed_t cmd;

    os_memcpy(cmd.seed, pSeed, size);

    return WLSendFW_ConfigureCommand(SET_SEED_CFG,
                                     &cmd,
                                     sizeof(cmd_cfg_seed_t),
                                     NULL,
                                     0);
}

void ctrlCmdFw_LockHostDriver()
{
   // osi_LockObjLock(&driverLock, OSI_WAIT_FOREVER);
}

void ctrlCmdFw_UnlockHostDriver()
{
   // osi_LockObjUnlock(&driverLock);
}

int8_t ctrlCmdFw_SetChannelInBitmap(uint32_t *channelBitmapArr, uint32_t channel)
{
    if(channel == CTRL_SET_ALL_CHANNELS)
    {
        SET_BIT_IN_BITMAP( (channelBitmapArr[0]), (0) ) ;
        return CTRL_SET_ALL_CHANNELS;
    }
    
    else if((channel <= 0) || (channel > CTRL_MAX_CHANNEL_NUMBER))
    {
        Report("\n\r[Set Channel] Invalid Channel! \n\r");
        return FALSE;
    }
    else
    {
        SET_BIT_IN_BITMAP((channelBitmapArr [CTRL_GET_CHANNEL_BITMAP_INDEX(channel)]), (CTRL_GET_CHANNEL_BIT_IN_BITMAP(channel))) ;
        return TRUE;
    } 
}


int8_t ctrlCmdFw_CreateSelectedChannelsBitmap(WlanSelectedChannelsArray_t* channelArray ,uint32_t* listedChannelsBitmap)
{
    int index = 0;
    int8_t result = 0;
    uint32_t length = 0;

    length = channelArray->length;
    for(index = 0 ; index < length ; index++)
    {

        result = ctrlCmdFw_SetChannelInBitmap(listedChannelsBitmap, channelArray->channelArray[index]);

        if(result == CTRL_SET_ALL_CHANNELS)
        {
            break;
        }
        else if (result == FALSE)
        {
            return -1;
        }
    }

    return 0;
}


int ctrlCmdFw_CreateMsgQueue()
{
    int32_t ret;
    ret = osi_MsgQCreate(&eventMsgQueue, "eventQueue", sizeof(EventMessage_t), MSG_QUEUE_SIZE);
    if(ret != OSI_OK)
    {
        ASSERT_GENERAL(0);
        return WlanError(WLAN_ERROR_SEVERITY__HIGH, WLAN_ERROR_MODULE__CMD_CTRL, WLAN_ERROR_TYPE__MSG_QUEUE_CREATE);
    }
    return ret;
}

int ctrlCmdFw_DeleteMsgQueue()
{
    int32_t ret;
    ret = osi_MsgQDelete(&eventMsgQueue);
    if(ret != OSI_OK)
    {
        Report("\n\rerror!!! failed to delete eventMsgQueue");
        ASSERT_GENERAL(0);
    }
    return ret;
}

int ctrlCmdFw_CreateEventThread()
{
    int ret = 0;

    ret = osi_SyncObjCreate(&gEventThrdRunStopSyncObject);
    if(ret != OSI_OK)
    {
        Report("\n\rerror!!! failed to create syn object gEventThrdRunStopSyncObject");
        return ret;
    }

    ret = osi_SyncObjCreate(&gEventThrdDummySleep);
    if(ret != OSI_OK)
    {
        Report("\n\rerror!!! failed to create syn object gEventThrdDummySleep");
        return ret;
    }

    ret = osi_ThreadCreate(&eventThread, "eventThread", EVENT_STACK_SIZE, EVENT_TRANSPORT_THRD_PRIORITY, eventEntryThread, NULL);
    if(ret != OSI_OK)
    {
        Report("\n\rerror!!! failed to create thread eventThread");
        ASSERT_GENERAL(0);
    }
    return ret;
}

int ctrlCmdFw_DeleteEventThread()
{
    int ret;
    EventMessage_t event;

    //send message to stop the thread
    event.len = 2;
    event.message = os_malloc(event.len);
    {
        uint8_t* pMsg = event.message;
        *pMsg = 0xFF;
        *(pMsg+1) = 0xFF;
    }

    ret = osi_MsgQWrite(&eventMsgQueue, &event, OSI_WAIT_FOREVER,OSI_FLAG_NOT_FROM_INTR);
    if(ret != OSI_OK)
    {
        Report("\n\rerror!!! ctrlCmdFw_DeleteEventThread, failed to osi_MsgQWrite");
        return ret;
    }

    //wait for the thread to be out of the main loop
    ret = osi_SyncObjWait(&gEventThrdRunStopSyncObject,OSI_WAIT_FOREVER);
    if(ret != OSI_OK)
    {
        Report("\n\rerror!!! failed to wait  syn object gEventThrdRunStopSyncObject");
        return ret;
    }

    os_free(event.message);

    //delete event thread
    ret = osi_ThreadDelete(&eventThread);
    if(ret != OSI_OK)
    {
        Report("\n\rerror!!! failed to delete thread eventThread");
        return ret;
    }

    ret = osi_SyncObjDelete(&gEventThrdRunStopSyncObject);
    if(ret != OSI_OK)
    {
        Report("\n\rerror!!! failed to delete syn object gEventThrdRunStopSyncObject");
        return ret;
    }

    ret = osi_SyncObjDelete(&gEventThrdDummySleep);
    if(ret != OSI_OK)
    {
        Report("\n\rerror!!! failed to delete syn object gEventThrdDummySleep");
    }

    return ret;
}

extern void WlanEventHandler(void *pWlanEvent);
void eventEntryThread()
{
    EventMessage_t *event = os_malloc(sizeof(EventMessage_t));
    uint8_t eventThrdRunning = 1;
    int ret;

    while(eventThrdRunning)
    {
        if(OSI_OK == osi_MsgQRead(&eventMsgQueue, event, OSI_WAIT_FOREVER))
        {
            if ((event->len == 2) && (IS_WLAN_EVENT_MESSAGE_STOP_RUN(event->message)))
            {
                Report("\n\rreceive event to deinit event thread");
                //this message means that the thread is requested to stop run
                eventThrdRunning = 0;
            }
            else//handle the event
            {
                WlanEventHandler(event->message);
                os_free(event->message);
            }
        }
        else
        {
            ASSERT_GENERAL(0);
            Report("\n\reventEntryThread: Failed to read event message");
        }
    }
    os_free(event);

    //signal to the "delete thread" function that we are out of main loop
    ret = osi_SyncObjSignal(&gEventThrdRunStopSyncObject);
    if(ret != 0)
    {
        Report("\n\rerror!!! failed to signal gEventThrdRunStopSyncObject");
    }

    //wait forever until the thread is deleted
    //or  when thread implementation is disconnect and not delete, this will be signaled
    //Report("\n\r event thread , thread on wait loop to be deleted or disconnected !!!");
    ret = osi_SyncObjWait(&gEventThrdDummySleep ,OSI_WAIT_FOREVER);
    if(ret != 0)
    {
        Report("\n\rerror!!! failed to signal gEventThrdDummySleep");
    }

}
