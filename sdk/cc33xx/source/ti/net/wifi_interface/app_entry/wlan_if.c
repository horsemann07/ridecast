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
#ifndef USER_INTEFACE_WLAN_IF_C_
#define USER_INTEFACE_WLAN_IF_C_

#include <stdint.h>
#include "wlan_if.h"
#include "errors.h"
#include "osi_kernel.h"
#include <string.h>
#include "tx.h"
#include "rx.h"
#include "macro_utils.h"
#include <wlan_irq_adapt.h>
#include "control_cmd_fw.h"

#include "init_host_if.h"
#include "nab.h"
#include "wlan_commands.h"
#include "tx_defs.h"
#include "osprey_public_commands.h"
#include "osprey_public_types.h"
#include "public_types.h"
#include "host_event_if.h"


//#define EVENT_TEST
#ifdef EVENT_TEST
void testEventHandler(uint16_t opcode);
#endif /* EVENT_TEST */




WlanNetworkDataRecvCB_t RecvCallBack[2] = {0};
WlanEventHandlerCB_t userEventHandlerCallBack = NULL;
uint32_t gWlanState = FALSE;

uint32_t role_bitmap = 0;

device_info_t gDevice_info = {0};

TMacAddr     g_fuse_mac_addr[6]= {
                                 {0},//STA mac
                                 {0},//BLE role
                                 {0},//P2P Devicr
                                 {0},//AP/P2PGO mac
                                 {0},
                                 {0}};

uint32_t gIsRecovery;
uint32_t gIsCustomDomainSet = FALSE;
// ============================================================================
//      Data path Functions
// ============================================================================
int Wlan_EtherPacketSend(WlanRole_e role, uint8_t *inbuf, uint32_t inbuf_len,uint32_t flags)
{

    // marking the parameters as unused to bypass 'unused parameter' warning
    (void)role;
    (void)flags;

    return tx_SendEthrBuf(inbuf, inbuf_len);
}

void _recvNetworkPacket(uint32_t netifid,uint8_t *inbuf, uint32_t inbuf_len)
{
    WlanRole_e roleType;
    if(netifid == 0)
    {
        roleType = WLAN_ROLE_STA;
    }
    else
    {
        roleType = WLAN_ROLE_AP;
    }

    if(RecvCallBack[netifid] != NULL)
    {
        RecvCallBack[netifid](roleType,inbuf,inbuf_len);
    }
}

int Wlan_EtherPacketRecvRegisterCallback(WlanRole_e roleType, WlanNetworkDataRecvCB_t cb)
{
    int retVal = -1;

    if(roleType == WLAN_ROLE_STA)
    {
        RecvCallBack[0] = cb;
        retVal = 0;
    }
    else if(roleType == WLAN_ROLE_AP)
    {
        RecvCallBack[1] = cb;
        retVal = 0;
    }
    else
    {
        retVal = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_ROLE);
    }

    rx_RegisterNextLayerRecvFunction(_recvNetworkPacket, NULL, RX_NEXT_LAYER_ETHERNET);//call to rx_callback or _recvNetworkPacket


    return retVal;

}


void WlanEventHandler(void *internalEvent)
{
    WlanEvent_t *event = (WlanEvent_t *)((uint32_t)internalEvent+HOST_EVENT_RESERVED_EVENT_HEADER);
    if(NULL != userEventHandlerCallBack)
    {
        userEventHandlerCallBack(event);
    }
}

// ============================================================================
//      USER API Function
// ============================================================================
int Wlan_Start(WlanEventHandlerCB_t eventHandlerCB)
{
    int ret = 0;
    int ret_status = 0;
    wlan_start_cmd_t cmd;
    wlan_start_complete_t complete;

    //Check if wlan_start is already active
    if(TRUE == gWlanState)
    {
        Report("\n\rWlan already started!!");
        return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__WLAN_ALREADY_STARTED);
    }

    WlanMacAddress_t macAddressParams;
    memset(&macAddressParams, 0, sizeof(WlanMacAddress_t));

    userEventHandlerCallBack = eventHandlerCB;

    ret = InitHostDriver();


    if(ret != 0)
    {
        return ret;
    }

    ctrlCmdFw_LockHostDriver();

    ret = ctrlCmdFw_WLSendCommand(WLAN_START, &cmd, sizeof(wlan_start_cmd_t),&complete,sizeof(wlan_start_complete_t));

    ctrlCmdFw_UnlockHostDriver();


    if (ret == WLAN_RET_CODE_OK)
    {
        if (complete.retVal != WLAN_RET_CODE_OK)
        {
            ret = WlanError(WLAN_ERROR_SEVERITY__HIGH, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.retVal));
            Report("\n\r Wlan_Start: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
            return ret;
        }
    }
    else
    {
        Report("\n\r Wlan_Start: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
        return ret;
    }

//If the Mac Address from the FW is Zeros. Generate a random number with 12 hex digits and
//Turn on the 41 bit to represent the number is locally administerrd.

    macAddressParams.roleType = WLAN_ROLE_STA;

    ret_status = Wlan_Get(WLAN_GET_MACADDRESS,(void *)&macAddressParams);
    if(ret_status < 0)
    {
        return ret_status;
    }

    WlanPowerSave_e PsMode = WLAN_STATION_AUTO_PS_MODE;
    ret_status = Wlan_Set(WLAN_SET_POWER_SAVE, (void *)&PsMode);

    if(ret_status < 0)
    {
        return ret_status;
    }

    WlanPowerManagement_e PmMode = POWER_MANAGEMENT_ELP_MODE;
    ret_status = Wlan_Set(WLAN_SET_POWER_MANAGEMENT, (void *)&PmMode);

    if(ret_status < 0)
    {
        return ret_status;
    }

    gWlanState = TRUE;
    return ret;
}

int  Wlan_Stop(uint32_t flags)
{
    int ret = 0;

    gIsRecovery = flags;

    if(!gIsRecovery)
    {
        Report("\n\rWlan_Stop NON recovery");

        wlan_stop_cmd_t cmd;
        wlan_stop_complete_t complete;

        ctrlCmdFw_LockHostDriver();

        ret = ctrlCmdFw_WLSendCommand(WLAN_STOP, &cmd, sizeof(wlan_stop_cmd_t),&complete,sizeof(wlan_stop_complete_t));

        ctrlCmdFw_UnlockHostDriver();

        if (ret == WLAN_RET_CODE_OK)
        {
            if (complete.retVal != WLAN_RET_CODE_OK)
            {
                ret = WlanError(WLAN_ERROR_SEVERITY__HIGH, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.retVal));
                Report("\n\r Wlan_Stop: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
                return ret;
            }
        }
        else
        {
            Report("\n\r Wlan_Stop: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
            return ret;
        }

        os_sleep(1, 0);

        ret = DeInitHostDriver();


    #ifndef    OSPREY_BUILD_MX
        wlan_TurnOffWlan();
    #endif
    }
    else
    {
        Report("\n\rWlan_Stop recovery");
        //If recovery wlan_stop
        wlan_TurnOffWlan();

        ret = DeInitHostDriver();
    }

    role_bitmap = 0;

    os_memset(&gDevice_info, 0, sizeof(gDevice_info));

    os_memset(&g_fuse_mac_addr, 0, sizeof(g_fuse_mac_addr));

    userEventHandlerCallBack = NULL;

    gWlanState = FALSE;

    gIsRecovery = FALSE;

    return ret;

}


int Wlan_RoleUp(WlanRole_e roleType, void *params, unsigned long int timeout)
{
    int ret;
    wlan_roleup_cmd_t cmd;
    wlan_roleup_complete_t complete;

    cmd.roleType = roleType;
    cmd.timeout = timeout;

     /* If transceiver role is active, don't allow starting any other role */
    if ((role_bitmap & (uint32_t)BIT_x(ROLE_TRANSCEIVER)) > 0)
    {
        if(ROLE_IS_TYPE_TRANSCEIVER(roleType))
            Report("\n\rcalibrator already up!\r\n");
        else
            Report("\n\rCan't enable Role when any calibrator is up!\r\n");

        return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__CALIBRATOR_ALREADY_ON);
    }

    if (ROLE_IS_TYPE_STA(roleType))
    {


        role_bitmap |= (uint32_t)BIT_x(ROLE_STA);
    }
    else if (ROLE_IS_TYPE_AP(roleType))
    {

        role_bitmap |= (uint32_t)BIT_x(ROLE_AP);

    }
    else if (ROLE_IS_TYPE_TRANSCEIVER(roleType))
    {
        /* If any other role is active, don't allow using PLT */
        if (role_bitmap != 0)
        {
            GTRACE(GRP_DRIVER_CC33,"Can't enable calibrator when any other role is up!\r\n");
            Report("\n\rCan't enable calibrator when any other role is up!\r\n");
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__TRANSCEIVER_ALREADY_ON);
        }

        ret = ctrlCmdFw_pltInit();
        if (ret < 0)
        {
            return ret;
        }
        role_bitmap |= (uint32_t)BIT_x(ROLE_TRANSCEIVER);


        return ret;
    }
    else if (ROLE_IS_TYPE_DEVICE(roleType))
    {
        role_bitmap |= (uint32_t)BIT_x(ROLE_DEVICE);
    }
    else
    {
        Report("\n\rERROR! role not supported roleType:%d" , roleType);
        GTRACE(GRP_DRIVER_CC33,"ERROR! role not supported roleType:%d" , roleType);
        return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__ROLE_NOT_SUPPORTED);
    }

    if(params)
    {

        wlan_roleup_ap_param_t *copyParam = &cmd.ap_param;

        if(WLAN_ROLE_AP == roleType)
        {
            RoleUpApCmd_t *apParam = (RoleUpApCmd_t *)params;

            memset(copyParam->pName, 0,WLAN_SSID_MAX_LENGTH);
            int ssid_len = strlen((const char *)apParam->ssid) + 1;
            memcpy(copyParam->pName, apParam->ssid, ssid_len );

            copyParam->channel      = apParam->channel;
            copyParam->hidden       = apParam->hidden;
            copyParam->tx_power     = apParam->tx_pow;
            copyParam->sta_limit    = apParam->sta_limit;

            copyParam->type         = apParam->secParams.Type;
            memset(copyParam->pPass, 0, sizeof(copyParam->pPass));
            memcpy(copyParam->pPass, apParam->secParams.Key, apParam->secParams.KeyLen);

            copyParam->PassLen      = apParam->secParams.KeyLen;

            memcpy(copyParam->countryDomain, apParam->countryDomain, 3);

            if ((!strncmp((char *)copyParam->countryDomain, "CS", 2)))
            {
                Report("\n\r Wlan_RoleUp: Custom Domain mode is not available for AP role");
                ret = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__HOST_RESPONSE_INVALID_PARAM);
                return ret;
            }
        }

        if(WLAN_ROLE_STA == roleType)
        {
            RoleUpStaCmd_t *staParam = (RoleUpStaCmd_t *)params;

            memcpy(copyParam->countryDomain, staParam->countryDomain, 3);

            if ((!strncmp((char *)copyParam->countryDomain, "CS", 2)) && (FALSE == gIsCustomDomainSet))
            {
                Report("\n\r Wlan_RoleUp: Set Custom domain before set custom entry is not allowed");
                ret = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__SET_CUSTOM_DOMAIN_BEFORE_SET_ENTRY);
                return ret;
            }
        }

    }

    ctrlCmdFw_LockHostDriver();

    ret = ctrlCmdFw_WLSendCommand(WLAN_ROLE_UP, &cmd, sizeof(wlan_roleup_cmd_t), &complete, sizeof(wlan_roleup_complete_t));

    ctrlCmdFw_UnlockHostDriver();

    if(ret == WLAN_RET_CODE_OK)
    {
        if (complete.retVal != WLAN_RET_CODE_OK)
        {
            ret = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.retVal));
            Report("\n\r Wlan_RoleUp: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
            return ret;
        }
    }
    else
    {
        Report("\n\r Wlan_RoleUp: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
        return ret;
    }

    return ret;
}

int Wlan_RoleDown(WlanRole_e roleType, unsigned long int timeout)
{
    int ret;
    wlan_roledown_cmd_t cmd;
    wlan_roledown_complete_t complete;

    cmd.roleType = roleType;
    cmd.timeout = timeout;


    if (ROLE_IS_TYPE_STA(roleType))
    {
        role_bitmap &= ~(uint32_t)BIT_x(ROLE_STA);

    }
    else if (ROLE_IS_TYPE_AP(roleType))
    {
        role_bitmap &= ~(uint32_t)BIT_x(ROLE_AP);
    }
    else if (ROLE_IS_TYPE_TRANSCEIVER(roleType))
    {
        if (0 == (role_bitmap & BIT_x(ROLE_TRANSCEIVER)))
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__TRANSCEIVER_NOT_SET);
        }

        ret = ctrlCmdFw_pltStop();
        if (ret < 0)
        {
            return ret;
        }

        role_bitmap &= ~(uint32_t)BIT_x(ROLE_TRANSCEIVER);

    }
    else if (ROLE_IS_TYPE_DEVICE(roleType))
    {
        role_bitmap &= ~(uint32_t)BIT_x(ROLE_DEVICE);
    }


    ctrlCmdFw_LockHostDriver();

    ret = ctrlCmdFw_WLSendCommand(WLAN_ROLE_DOWN, &cmd, sizeof(wlan_roleup_cmd_t), &complete, sizeof(wlan_roleup_complete_t));

    if(ret < 0)
    {
        Report("\n\r Wlan_RoleDown: failed to ctrlCmdFw_WLSendCommand, error code %d",ret);
        return ret;
    }

    ctrlCmdFw_UnlockHostDriver();

    os_sleep(1, 0);

    if (ret == WLAN_RET_CODE_OK)
    {
        ret = complete.retVal;
        if (ret != WLAN_RET_CODE_OK)
        {
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.retVal));
        }
    }

    return ret;
}


int Wlan_Connect(const signed char *pName, const int NameLen, const unsigned char *pMacAddr,
                 char SecType, const char *pPass, const char PassLen, unsigned int flags)
{
    // marking the parameter as unused
    (void)NameLen;

    wlan_connect_cmd_t *cmd;
    wlan_connect_complete_t complete;
    int ret;
    cmd = os_zalloc(sizeof(wlan_connect_cmd_t));

    if(NULL == cmd){
        return WlanError(WLAN_ERROR_SEVERITY__MID, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__MALLOC);
    }

    if(NULL == pName)
    {
        ret = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_PARAM_NAME);
        goto finish;
    }

    cmd->NameLen = strlen((const char *)pName);

    if(cmd->NameLen > 32 || cmd->NameLen <= 0)
    {
        ret =  WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_PARAM_NAME);
        goto finish;
    }

    memcpy(cmd->pName,pName,cmd->NameLen);

    if(NULL != pMacAddr)
    {
        memcpy(cmd->pMacAddr,pMacAddr,6);
    }

    if( (SecType != WLAN_SEC_TYPE_OPEN)     &&
        (SecType != WLAN_SEC_TYPE_WPA_WPA2) &&
        (SecType != WLAN_SEC_TYPE_WPA3)     &&
        (SecType != WLAN_SEC_TYPE_WPA2_PLUS)
      )
    {
        ret = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_PARAM_SEC_TYPE);
        goto finish;
    }

    cmd->SecType = SecType;

    if(SecType != WLAN_SEC_TYPE_OPEN)
    {
        if((NULL == pPass) || ((PassLen < 8) || (PassLen > 64)))
        {
            ret = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_PARAM_PASSWRD);
            goto finish;
        }
        cmd->PassLen = PassLen;
        memcpy(cmd->pPass,pPass,PassLen);
    }

    ctrlCmdFw_LockHostDriver();

    ret = ctrlCmdFw_WLSendCommand(WLAN_CONNECT, cmd, sizeof(wlan_connect_cmd_t), &complete, sizeof(wlan_connect_complete_t));

    ctrlCmdFw_UnlockHostDriver();

    if(ret == WLAN_RET_CODE_OK)
    {
        if (complete.retVal != WLAN_RET_CODE_OK)
        {
            ret = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.retVal));
            Report("\n\r Wlan_Connect: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
            return ret;
        }
    }
    else
    {
        Report("\n\r Wlan_Connect: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
        return ret;
    }

finish:
    os_free(cmd);
    return ret;
}

int Wlan_Disconnect(WlanRole_e roleType,void* params)
{
    wlan_disconnect_cmd_t       cmd;
    wlan_disconnect_complete_t  complete;
    int ret = -1;

    cmd.isHostReq = TRUE;

    ctrlCmdFw_LockHostDriver();

    ret = ctrlCmdFw_WLSendCommand(WLAN_DISCONNECT, &cmd, sizeof(wlan_disconnect_cmd_t), &complete, sizeof(wlan_disconnect_complete_t));

    ctrlCmdFw_UnlockHostDriver();

    if (ret == WLAN_RET_CODE_OK)
    {
        if (complete.retVal != WLAN_RET_CODE_OK)
        {
            ret = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.retVal));
            Report("\n\r Wlan_Disconnect: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
            return ret;
        }
    }
    else
    {
        Report("\n\r Wlan_Disconnect: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
        return ret;
    }

    return ret;
}

int Wlan_Scan(WlanRole_e role, void *pEntries,
              const unsigned char ScanResultsSize)
{
    // marking the parameters as unused to bypass 'unused parameter' warning
    (void)role;
    (void)pEntries;

    int ret = -1;
    wlan_scan_cmd_t      cmd;
    wlan_scan_complete_t complete;

    cmd.scanResultSize = ScanResultsSize;

    ctrlCmdFw_LockHostDriver();

    ret = ctrlCmdFw_WLSendCommand(WLAN_SCAN, &cmd, sizeof(wlan_scan_cmd_t), &complete, sizeof(wlan_scan_complete_t));

    ctrlCmdFw_UnlockHostDriver();

    if(ret == WLAN_RET_CODE_OK)
    {
        if (complete.retVal != WLAN_RET_CODE_OK)
        {
            ret = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.retVal));
            Report("\n\r Wlan_Scan: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
            return ret;
        }
    }
    else
    {
        Report("\n\r Wlan_Scan: failed to ctrlCmdFw_WLSendCommand, error code %d", ret);
        return ret;
    }

    return ret;
}

int Wlan_Get(WlanGet_e wlanGetType, void *params)
{
    int ret = 0;

    switch (wlanGetType)
    {
        case WLAN_GET_MACADDRESS:
        {
            WlanMacAddress_t *pMacParams = (WlanMacAddress_t *)params;

            wlan_get_mac_cmd_t cmd;
            wlan_get_mac_complete_t complete;
            cmd.WlanGetHdr.Type = wlanGetType;
            cmd.roleType = pMacParams->roleType;

            ctrlCmdFw_LockHostDriver();

            ret = ctrlCmdFw_WLSendCommand(WLAN_GET, &cmd, sizeof(wlan_get_mac_cmd_t), &complete, sizeof(wlan_get_mac_complete_t));

            if(ret < 0)
            {
                Report("\n\r Wlan_Get, WLAN_GET_MACADDRESS: failed to ctrlCmdFw_WLSendCommand, error code %d",ret);
                return ret;
            }

            ctrlCmdFw_UnlockHostDriver();

            memcpy(pMacParams->pMacAddress, complete.macAddr, 6);
        }
        break;

        case WLAN_GET_POWER_SAVE:
        {

            WlanPowerSave_e *pPsMode = (WlanPowerSave_e *)params;

            wlan_get_ps_cmd_t cmd;
            wlan_get_ps_complete_t complete;
            cmd.WlanGetHdr.Type = wlanGetType;

            ctrlCmdFw_LockHostDriver();

            ret = ctrlCmdFw_WLSendCommand(WLAN_GET, &cmd, sizeof(wlan_get_ps_cmd_t), &complete, sizeof(wlan_get_ps_complete_t));
            if(ret < 0)
            {
                Report("\n\r Wlan_Get, WLAN_GET_POWER_SAVE: failed to ctrlCmdFw_WLSendCommand, error code %d",ret);
                return ret;
            }

            ctrlCmdFw_UnlockHostDriver();

            *pPsMode = (WlanPowerSave_e)complete.psMode;
        }
        break;


        case WLAN_GET_CALIBRATOR_RX_STATS:
        case WLAN_GET_CALIBRATOR_TX_PARAMS:
        case WLAN_GET_CALIBRATOR_BEACON_RSSI:
        {
            CalibratorCommandWrapper_t *calibratorCommand = (CalibratorCommandWrapper_t *)params;
            return ctrlCmdFw_CalibratorCommand((CalibratorAction_e) calibratorCommand->calibratorAction, calibratorCommand->calibratorCommandParams);
        }
        break;
        case WLAN_GET_RSSI:
        {
            WlanBeaconRssi_t *rssi = (WlanBeaconRssi_t *) params;
            return ctrlCmdFw_GetRssi(rssi);
        }
        break;
        case WLAN_GET_FWVERSION:
        {
            return ctrlCmdFw_GetFwVersion((FWVersions_t*)params);
        }
        break;

        default:
            return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_PARAM_GET_TYPE);
    }

    return ret;
}

int Wlan_Set(WlanSet_e wlanSetType, void *params)
{
    int16_t ret = 0;

    ctrlCmdFw_LockHostDriver();

    switch (wlanSetType)
    {
        case WLAN_SET_MACADDRESS:
        {
            WlanMacAddress_t *pMacParams = (WlanMacAddress_t *)params;
            
            wlan_set_mac_cmd_t cmd;
            wlan_set_mac_complete_t complete;

            cmd.WlanSetHdr.Type = UPPER_MAC_SET_MAC_ADDRESS;
            cmd.roleType = pMacParams->roleType;
            
            os_memcpy(g_fuse_mac_addr, pMacParams->pMacAddress, MAC_ADDRESS_LEN);
            os_memcpy(cmd.macAddr, pMacParams->pMacAddress, sizeof(cmd.macAddr));

            ret = ctrlCmdFw_WLSendCommand(WLAN_SET, &cmd, sizeof(wlan_set_mac_cmd_t), &complete, sizeof(wlan_set_mac_complete_t));

            if (ret == WLAN_RET_CODE_OK)
            {
                ret = complete.retVal;
                if (ret != WLAN_RET_CODE_OK)
                {
                    return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.retVal));
                }
            }
            else
            {
                Report("\n\r Wlan_Set, WLAN_SET_MACADDRESS: failed to ctrlCmdFw_WLSendCommand, error code %d",ret);
                return ret;
            }

        }
        break;
    
        case WLAN_SET_POWER_SAVE:
        {
            wlan_set_ps_cmd_t cmd;
            wlan_set_ps_complete_t complete;

            cmd.WlanSetHdr.Type = UPPER_MAC_SET_POWER_SAVE;
            cmd.psMode = *(unsigned char *)params;

            ret = ctrlCmdFw_WLSendCommand(WLAN_SET, &cmd, sizeof(wlan_set_ps_cmd_t), &complete, sizeof(wlan_set_ps_complete_t));

            if (ret == WLAN_RET_CODE_OK)
            {
                ret = complete.val;
                if(ret != WLAN_RET_CODE_OK)
                {
                    return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.val));
                }
            }
            else
            {
                Report("\n\r Wlan_Set, WLAN_SET_POWER_SAVE: failed to ctrlCmdFw_WLSendCommand, error code %d",ret);
                return ret;
            }

        }
        break;
        case WLAN_SET_CUSTOM_DOMAIN_ENTRY:
        {
            wlan_set_custom_domain_entry_cmd_t cmd;
            wlan_set_custom_domain_entry_complete_t complete;

            cmd.WlanSetHdr.Type = UPPER_MAC_SET_CUSTOM_DOMAIN_ENTRY;
            cmd.entry = *(WlanSetRegDomainCustomEntry_t *)params;

            //Band protection
            if(cmd.entry.band < BAND_24GHZ || cmd.entry.band > JAPAN_49GHZ )
            {
                Report("\n\r Invalid BAND");
                return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_PARAM_SET_TYPE);
            }

            //Channel protection
            switch (cmd.entry.band)
            {
                case BAND_24GHZ:
                {
                    if(cmd.entry.minChannel < CHANNEL_1 || cmd.entry.minChannel > CHANNEL_14 ||
                            cmd.entry.maxChannel < CHANNEL_1 || cmd.entry.maxChannel > CHANNEL_14)
                    {
                        Report("\n\r Invalid minChannel/maxChannel 2.4G. minChannel: %d, maxChannel: %d"
                                ,cmd.entry.minChannel, cmd.entry.maxChannel);
                        return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_PARAM_SET_TYPE);
                    }
                }
                break;
                case BAND_5GHZ:
                {
                    if(cmd.entry.minChannel < CHANNEL_36 || cmd.entry.minChannel > CHANNEL_169 ||
                            cmd.entry.maxChannel < CHANNEL_36 || cmd.entry.maxChannel > CHANNEL_169)
                    {
                        Report("\n\r Invalid minChannel/maxChannel 5G. minChannel: %d, maxChannel: %d"
                                ,cmd.entry.minChannel, cmd.entry.maxChannel);
                        return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_PARAM_SET_TYPE);
                    }
                }
                break;
                case JAPAN_49GHZ:
                {
                    if(cmd.entry.minChannel < CHANNEL_184 || cmd.entry.minChannel > CHANNEL_196 ||
                            cmd.entry.maxChannel < CHANNEL_184 || cmd.entry.maxChannel > CHANNEL_196)
                    {
                        Report("\n\r Invalid minChannel/maxChannel 4.9G. minChannel: %d, maxChannel: %d"
                                 ,cmd.entry.minChannel, cmd.entry.maxChannel);
                        return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_PARAM_SET_TYPE);
                    }
                }
                break;
            }

            ret = ctrlCmdFw_WLSendCommand(WLAN_SET, &cmd, sizeof(wlan_set_custom_domain_entry_cmd_t), &complete, sizeof(wlan_set_custom_domain_entry_complete_t));

            if (ret == WLAN_RET_CODE_OK)
            {
                ret = complete.retVal;
                if(ret != WLAN_RET_CODE_OK)
                {
                    return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.retVal));
                }
            }
            else
            {
                Report("\n\r Wlan_Set, WLAN_SET_CUSTOM_DOMAIN_ENTRY: failed to ctrlCmdFw_WLSendCommand, error code %d",ret);
                return ret;
            }
            gIsCustomDomainSet = TRUE;
        }
        break;
        case WLAN_SET_LISTED_CHANNELS_FOR_SCAN:
        {

            WlanSelectedChannelsArray_t* channelArray = (WlanSelectedChannelsArray_t *)params;
            wlan_set_listed_channel_for_scan_cmd_t cmd;
            wlan_set_listed_channel_for_scan_complete_t complete;

             uint32_t listedChannelsBitmap[6] = {0,0,0,0,0,0};


            os_memset(cmd.listedChannelsBitmap, 0, (CTRL_CHANNELS_BITMAP_SIZE_24GHZ_5GHZ) * sizeof(uint32_t));

            if(params == NULL)
            {
                ctrlCmdFw_SetChannelInBitmap(cmd.listedChannelsBitmap, CTRL_SET_ALL_CHANNELS);
            }
            else
            {
                if(ctrlCmdFw_CreateSelectedChannelsBitmap(channelArray, listedChannelsBitmap) < 0 )
                {
                    Report("\n\r Wlan_Set, WLAN_SET_LISTED_CHANNELS_FOR_SCAN: invalid Channels Sent");
                    return ret;  
                }
                os_memcpy(cmd.listedChannelsBitmap, listedChannelsBitmap, sizeof(uint32_t) * CTRL_CHANNELS_BITMAP_SIZE_24GHZ_5GHZ);
            }

            cmd.WlanSetHdr.Type = UPPER_MAC_SET_LISTED_CHANNELS_FOR_SCAN;
            ret = ctrlCmdFw_WLSendCommand(WLAN_SET, &cmd, sizeof(wlan_set_listed_channel_for_scan_cmd_t), &complete, sizeof(wlan_set_listed_channel_for_scan_complete_t));  
            if (ret == WLAN_RET_CODE_OK)
            {
                ret = complete.retVal;
                if(ret != WLAN_RET_CODE_OK)
                {
                    return WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__FW_RESPONSE, WlanError_ConvertWsocError(complete.retVal));
                }
            }
            else
            {
                Report("\n\r Wlan_Set, WLAN_SET_LISTED_CHANNELS_FOR_SCAN: failed to ctrlCmdFw_WLSendCommand, error code %d",ret);
                return ret;
            }
        }
        break;
        case WLAN_SET_POWER_MANAGEMENT:
        {

            WlanPowerManagement_e *sleep_auth = (WlanPowerManagement_e *)params;

            // Send cmd to FW
            enable_sleep_mode_cmd_t cmd;

            cmd.sleep_auth = (uint8_t)*sleep_auth;

            ret = WLSendFW_ConfigureCommand(ACX_SLEEP_AUTH, &cmd, sizeof(enable_sleep_mode_cmd_t), NULL, 0);

        }
        break;

        case WLAN_SET_LSI:
        {

            // Send cmd to FW
            WlanLongSleepInterval *LsiParams = (WlanLongSleepInterval *)params;
            
            cmd_set_lsi_t cmd;

            cmd.LsiParams.ListenInterval = LsiParams->ListenInterval;
            cmd.LsiParams.WakeUpEvent = LsiParams->WakeUpEvent;

            ret = WLSendFW_ConfigureCommand(WAKE_UP_CONDITIONS_CFG, &cmd, sizeof(cmd_set_lsi_t), NULL, 0);

        }
        break;

        case WLAN_SET_CALIBRATOR_CHANNEL_TUNE:
        case WLAN_SET_CALIBRATOR_TX_TONE_START:
        case WLAN_SET_CALIBRATOR_SWITCH_ANT:
        case WLAN_SET_CALIBRATOR_TX_START:
        case WLAN_SET_CALIBRATOR_TX_STOP:
        case WLAN_SET_CALIBRATOR_START_RX:
        case WLAN_SET_CALIBRATOR_STOP_RX:
        case WLAN_SET_CALIBRATOR_TX_PARAMS:
        case WLAN_SET_CALIBRATOR_MANUAL_CALIBRATION:
        case WLAN_SET_CALIBRATOR_SET_IO_CFG:
        case WLAN_SET_CALIBRATOR_BLE_ENABLE:
        case WLAN_SET_CALIBRATOR_TB_TX_PARAMS:
        case WLAN_SET_CALIBRATOR_RATE_OVERRIDE:
        case WLAN_SET_CALIBRATOR_GI_LTF:
        case WLAN_SET_CALIBRATOR_UPLINK_MU:
        case WLAN_SET_CALIBRATOR_OPERATION_MODE_CONTROL:
        case WLAN_SET_CALIBRATOR_MCS_RATE:
        case WLAN_SET_CALIBRATOR_UPLINK_MU_DATA:
        case WLAN_SET_CALIBRATOR_PSM:
        case WLAN_SET_CALIBRATOR_UPLINK_POWER_HEADER:
        case WLAN_SET_CALIBRATOR_TRANSMIT_OSI: 
        {
            CalibratorCommandWrapper_t *calibratorCommand = (CalibratorCommandWrapper_t *)params;
            ret = ctrlCmdFw_CalibratorCommand(calibratorCommand->calibratorAction, calibratorCommand->calibratorCommandParams);
        }
        break;

        case WLAN_SET_TX_CTRL:
        {
            //Return success code to avoid default case with wlan error
            ret = 0;
        }
        break;

        default:
            ret = WlanError(WLAN_ERROR_SEVERITY__LOW, WLAN_ERROR_MODULE__COMMANDS, WLAN_ERROR_TYPE__INVALID_PARAM_SET_TYPE);
    } 

    ctrlCmdFw_UnlockHostDriver();

    return ret;
}

// ============================================================================
//      Event Function
// ============================================================================
int wlanDispatcherSendEvent(uint16_t opcode, uint8_t *args, uint16_t argsLen)
{
    // Temporarily marking the parameters as unused to bypass 'unused parameter' warning
    (void)opcode;
    (void)args;
    (void)argsLen;


#if EVENT_TEST
    WlanEvent_t *pWlanEvent;
    pWlanEvent = os_zalloc(sizeof(WlanEvent_t));

    if (!args)
    {
        Report("\n\r[WLAN EVENT] NULL buffer\n\r");
        return WLAN_ERROR_NULL_BUF;
    }
    pWlanEvent->Id = opcode;
    switch (opcode)
    {
    case WLAN_EVENT_CONNECT:
    {
        os_memcpy((void *)&pWlanEvent->Data.Connect, args, sizeof(WlanEventConnect_t));
    }
    break;
    case WLAN_EVENT_DISCONNECT:
    {
        os_memcpy((void *)&pWlanEvent->Data.Disconnect, args, sizeof(WlanEventDisconnect_t));
    }
    break;
    case WLAN_EVENT_SCAN_RESULT:
    {
        if (argsLen > WLAN_MAX_SCAN_COUNT)
        {
            argsLen = WLAN_MAX_SCAN_COUNT;
        }
        pWlanEvent->Data.ScanResult.NetworkListResultLen = argsLen;
        os_memcpy((void *)&pWlanEvent->Data.ScanResult.NetworkListResult, args, sizeof(WlanNetworkEntry_t) * argsLen);
    }
    break;

    default:
    {
        Report("\n\r[WLAN EVENT] Unexpected event [0x%x]\n\r", opcode);
    }
    break;
    }
    // call to evenet handler
    gRoleAppsParams.eventHandlerCB(pWlanEvent);
    os_free(pWlanEvent);
#endif
    return 0;
}

// ============================================================================
//      Unit testing
// ============================================================================
#ifdef EVENT_TEST
// test event handler
void testEventHandler(uint16_t opcode)
{
    int i;
    switch (opcode)
    {
    case WLAN_EVENT_CONNECT:
    {
        WlanEventConnect_t *args;
        args = (WlanEventConnect_t *)os_zalloc(sizeof(WlanEventConnect_t));
        args->Bssid[0] = '\x8c';
        args->Bssid[1] = 'S';
        args->Bssid[2] = '\xc3';
        args->Bssid[3] = '\xee';
        args->Bssid[4] = '\xd9';
        args->Bssid[5] = '\xea';
        args->Channel = '\x01';
        args->SsidLen = '\x05';
        args->SsidName[0] = 't';
        args->SsidName[1] = 'e';
        args->SsidName[2] = 's';
        args->SsidName[3] = 't';
        args->SsidName[4] = '0';

        wlanDispatcherSendEvent(WLAN_EVENT_CONNECT, (uint8_t *)args, sizeof(WlanEventConnect_t));
        os_free(args);
    }
    break;
    case WLAN_EVENT_DISCONNECT:
    {
        WlanEventDisconnect_t *args;
        args = (WlanEventDisconnect_t *)os_zalloc(sizeof(WlanEventDisconnect_t));
        args->Bssid[0] = '\x8c';
        args->Bssid[1] = 'S';
        args->Bssid[2] = '\xc3';
        args->Bssid[3] = '\xee';
        args->Bssid[4] = '\xd9';
        args->Bssid[5] = '\xea';
        args->ReasonCode = 200;
        args->SsidLen = '\x05';
        args->SsidName[0] = 't';
        args->SsidName[1] = 'e';
        args->SsidName[2] = 's';
        args->SsidName[3] = 't';
        args->SsidName[4] = '0';

        wlanDispatcherSendEvent(WLAN_EVENT_DISCONNECT, (uint8_t *)args, sizeof(WlanEventDisconnect_t));
        os_free(args);
    }
    break;

    case WLAN_EVENT_SCAN_RESULT:
    {
        WlanEventScanResult_t args;
        args.NetworkListResultLen = 2;
        args.NetworkListResult = os_malloc(args.NetworkListResultLen * sizeof(WlanNetworkEntry_t));
        WlanNetworkEntry_t netEntry;
        for (i = 0; i < args.NetworkListResultLen; ++i)
        {
            args.NetworkListResult[i].Bssid[0] = '\x8c';
            args.NetworkListResult[i].Bssid[1] = 'S';
            args.NetworkListResult[i].Bssid[2] = '\xc3';
            args.NetworkListResult[i].Bssid[3] = '\xee';
            args.NetworkListResult[i].Bssid[4] = '\xd9';
            args.NetworkListResult[i].Bssid[5] = '\xea';

            args.NetworkListResult[i].Ssid[0] = 't';
            args.NetworkListResult[i].Ssid[1] = 'e';
            args.NetworkListResult[i].Ssid[2] = 's';
            args.NetworkListResult[i].Ssid[3] = 't';
            args.NetworkListResult[i].Ssid[4] = i + '0';

            args.NetworkListResult[i].SsidLen = '\x05';
            args.NetworkListResult[i].Rssi = '\xde';
            args.NetworkListResult[i].SecurityInfo = 5828;
            args.NetworkListResult[i].Channel = '\0x01';
            args.NetworkListResult[i].Reserved[0] = '\0x00';
        }

        wlanDispatcherSendEvent(WLAN_EVENT_SCAN_RESULT, (uint8_t *)&args, sizeof(WlanEventScanResult_t));
        os_free(args.NetworkListResult);
    }
    break;

    default:
        break;
    }
}
#endif /* EVENT_TEST */

#endif /* USER_INTEFACE_WLAN_IF_C_ */

