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
#ifndef APP_ENTRY_WLAN_COMMANDS_H_
#define APP_ENTRY_WLAN_COMMANDS_H_


typedef enum
{
    UPPER_MAC_SET_MAC_ADDRESS,
    UPPER_MAC_SET_POWER_SAVE,
    UPPER_MAC_SET_CUSTOM_DOMAIN_ENTRY,
    UPPER_MAC_SET_LISTED_CHANNELS_FOR_SCAN

} UpperMacWlanSet_e;

typedef struct
{
    int retVal;
}wlan_start_complete_t;


typedef struct
{
    cmd_header_t hdr;
    uint32_t dummy;
}wlan_start_cmd_t;

typedef struct
{
    int retVal;
}wlan_stop_complete_t;


typedef struct
{
    cmd_header_t hdr;
    uint32_t dummy;
}wlan_stop_cmd_t;


typedef struct
{
    int retVal;
}wlan_roleup_complete_t;

typedef struct
{
    unsigned char pName[33]; //ssid up to 32 chars + null termination
    unsigned char hidden;
    unsigned char channel;
    unsigned char tx_power;
    unsigned char sta_limit;
    unsigned char type;
    char pPass[64]; //passphrase can be 8-63 characters long + Null termination
    char PassLen;
    unsigned char countryDomain[3];
    char padding[1];
}wlan_roleup_ap_param_t;

typedef struct
{
    cmd_header_t hdr;
    WlanRole_e roleType;
    unsigned long int timeout;
    wlan_roleup_ap_param_t ap_param;
}wlan_roleup_cmd_t;


typedef struct
{
    int retVal;
}wlan_roledown_complete_t;


typedef struct
{
    cmd_header_t hdr;
    WlanRole_e roleType;
    unsigned long int timeout;
}wlan_roledown_cmd_t;


typedef struct
{
    cmd_header_t hdr;
    signed char pName[33]; //ssid up to 32 chars + null termination
    int NameLen;
    unsigned char pMacAddr[6];
    char SecType;
    char pPass[64]; //passphrase can be 8-63 characters long + Null termination
    char PassLen;
    char padding[3];
}wlan_connect_cmd_t;

typedef struct
{
    int retVal;
}wlan_connect_complete_t;

typedef struct
{
    cmd_header_t hdr;
    unsigned char scanResultSize;
    char padding[3];
}wlan_scan_cmd_t;

typedef struct
{
    int retVal;
}wlan_scan_complete_t;


typedef struct
{
    cmd_header_t hdr;
    unsigned char isHostReq;
    char padding[3];
}wlan_disconnect_cmd_t;

typedef struct
{
    int retVal;
}wlan_disconnect_complete_t;

typedef struct
{
    uint8_t Type;
    uint32_t Len;
}wlanGetHeader_t;

typedef struct
{
    cmd_header_t hdr;
    wlanGetHeader_t WlanGetHdr;
    unsigned int roleType;
}wlan_get_mac_cmd_t;

typedef struct
{
    unsigned char macAddr[6];
    char padding[2];
}wlan_get_mac_complete_t;

typedef struct
{
    uint8_t Type;
    uint32_t Len;
}wlanSetHeader_t;


typedef struct
{
    cmd_header_t hdr;
    wlanSetHeader_t WlanSetHdr;
    unsigned int roleType;
    unsigned char macAddr[6];
    char padding[2];
}wlan_set_mac_cmd_t;

typedef struct
{
    int retVal;
}wlan_set_mac_complete_t;

typedef struct
{
    cmd_header_t hdr;
    wlanGetHeader_t WlanGetHdr;
    int val;
}wlan_get_ps_cmd_t;

typedef struct
{
    unsigned char psMode;
    char padding[3];
}wlan_get_ps_complete_t;

typedef struct
{
    cmd_header_t hdr;
    wlanSetHeader_t WlanSetHdr;
    unsigned char psMode;
    char padding[3];
}wlan_set_ps_cmd_t;


typedef struct
{
    int retVal;
}wlan_set_listed_channel_for_scan_complete_t;

typedef struct
{
    cmd_header_t hdr;
    wlanSetHeader_t WlanSetHdr;
    uint32_t listedChannelsBitmap[6];
    char padding[0];
}wlan_set_listed_channel_for_scan_cmd_t;



typedef struct
{
    int retVal;
}wlan_set_custom_domain_entry_complete_t;

typedef struct
{
    cmd_header_t hdr;
    wlanSetHeader_t WlanSetHdr;
    WlanSetRegDomainCustomEntry_t entry;
}wlan_set_custom_domain_entry_cmd_t;

typedef struct
{
    int val;
}wlan_set_ps_complete_t;



/**
 *
 * WLSendFW_CfgCommand - send a FW Command that has the struct of a cfg command (CMD_DEBUG/CMD_CONFIGURE)
 * @param opcode - opcode of command (CMD_DEBUG/CMD_CONFIGURE/CMD_DEBUG_READ)
 * @param id -  cfg command Id
 * @param cmd - pointer to command
 * @param len - command length
 * @param out - output buffer, could be NULL if no read from FW needed
 * @param outLen - output len
 * @return - 0 on success, CMD_ERR_XXX on fail
 *
 */
int  WLSendFW_CfgCommand(uint16_t opcode, uint32_t id, void *cmd, uint32_t len, void *out, uint32_t outLen);

/**
 *
 * WLSendFW_ConfigureCommand - send a FW Configure Command
 * @param id -   Cfg command Id
 * @param cmd - pointer to command
 * @param len - command length
 * @param out - output buffer, could be NULL if no read from FW needed
 * @param outLen - output len
 * @return - 0 on success, CMD_ERR_XXX on fail
 *
 */
int  WLSendFW_ConfigureCommand(uint32_t id, void *cmd, uint32_t len, void *out, uint32_t outLen);


/**
 *
 * WLSendFW_InterrogateCommand - send a FW Interrogate Command
 * @param id -  Interrogate command Id
 * @param cmd - pointer to command
 * @param len - command length
 * @param out - output buffer, could be NULL if no read from FW needed
 * @param outLen - output len
 * @return - 0 on success, CMD_ERR_XXX on fail
 *
 */
int  WLSendFW_InterrogateCommand(uint32_t id, void *cmd, uint32_t len, void *out, uint32_t outLen);


/**
 *
 * WLSendFW_DebugCommand - send a FW Debug Command
 * @param id -  Debug Cfg Id
 * @param cmd - pointer to command
 * @param len - command length
 * @param out - output buffer, could be NULL if no read from FW needed
 * @param outLen - output len
 * @return - 0 on success, CMD_ERR_XXX on fail
 *
 */
int  WLSendFW_DebugCommand(uint32_t id, void *cmd, uint32_t len, void *out, uint32_t outLen);


/**
 *
 * ctrlCmdFw_WLSendCommand - send Regular FW Command
 * @param opcode -  FW command opcode
 * @param cmd - pointer to command
 * @param len - command length
 * @param out - output buffer, could be NULL if no read from FW needed
 * @param outLen - output len
 * @return - 0 on success, CMD_ERR_XXX on fail
 *
 */
int  WLSendFW_Command(uint16_t opcode, void *cmd, uint32_t len, void *out, uint32_t outLen);



/**
 *
 * ctrlCmdFw_WLSendCommand - send Uppermac Command
 *
 * @param id -  uppermac command id
 * @param cmd - pointer to command
 * @param len - command length
 * @param out - output buffer, could be NULL if no read from FW needed
 * @param outLen - output len
 * @return - 0 on success, CMD_ERR_XXX on fail
 *
 */
int  ctrlCmdFw_WLSendCommand(uint32_t id, void *cmd, uint32_t len, void *out,uint32_t outLen);

#endif /* APP_ENTRY_WLAN_COMMANDS_H_ */
