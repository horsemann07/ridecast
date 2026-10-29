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
/**********************************************************************************************************************

  FILENAME:       cc33_public_infoele.h

  DESCRIPTION:    Contains information element defines/structures used by the FW and host.



***********************************************************************************************************************/
#ifndef PUBLIC_INFOELE_H
#define PUBLIC_INFOELE_H


#include <inc_common_fw/osprey_public_types.h>
//#include "802_11defs.h"

//#include "osprey_public_commands.h"
//#include "public_radio.h"
#ifdef TNETW18xx
#define NUM_OF_RATES_INDEXES            30 // RateIndex_e::NUM_OF_RATE_INDEXES
#define NUM_OF_FIFOS                    11 //gen_HW_fifo.h:FIFO_e:NUM_OF_HW_FIFOS
#define NUM_OF_CALIBRATIONS_ERRORS      18 //public_mac_phy_api.h:CalibrationsList_e:NUM_RUN_TIME_CALIBRATIONS_E
#endif

#define NOISE_HIST_LEN 8





typedef struct
{
    uint16 id;
    uint16 len;
} EleHdrStruct;



#ifdef HOST_COMPILE
#define INFO_ELE_HDR    EleHdrStruct    EleHdr;
#else
#define INFO_ELE_HDR
#endif
/******************************************************************************

    Name:   ACX_WAKE_UP_CONDITIONS
    Type:   Configuration
    Access: Write Only
    Length: 2

******************************************************************************/
typedef enum
{
    WAKE_UP_EVENT_BEACON_BITMAP     = 0x01, /* Wake on every Beacon*/
    WAKE_UP_EVENT_DTIM_BITMAP       = 0x02, /* Wake on every DTIM*/
    WAKE_UP_EVENT_N_DTIM_BITMAP     = 0x04, /* Wake on every Nth DTIM (Listen interval)*/
    //not supported
    //WAKE_UP_EVENT_N_BEACONS_BITMAP  = 0x08, /* Wake on every Nth Beacon (Nx Beacon)*/
    WAKE_UP_EVENT_BITS_MASK         = 0x0F
} WakeUpEventBitMask_e;

typedef struct
{
                                    /* Offset Size */
    INFO_ELE_HDR                    /*      0    4 */
    CMD_PARAMS_HOST_ROLE_DEFINITION /*      4    1  - host RoleID */
    uint8  wakeUpConditionBitmap;   /*      5    1  - The host can set one bit only. */
                                    /*                WakeUpEventBitMask_e describes the Possible */
                                    /*                Wakeup configuration bits*/

    uint8  listenInterval;          /*      6    1  - 0 for Beacon and Dtim, */
                                    /*                xDtims (1-10) for Listen Interval and */
                                    /*                xBeacons (1-255) for NxBeacon*/
    uint8  padding[1];              /*      7    1  - alignment to 32bits boundry   */
}WakeUpCondition_t;


/******************************************************************************

    Name:   ACX_MEM_CFG
    Type:   Configuration
    Access: Write Only
    Length: 12

******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    uint8   rxMemblockNumber;           /* specifies the number of memory buffers that */
                                        /* is allocated to the Rx memory pool. The */
                                        /* actual number allocated may be less than*/
                                        /* this number if there are not enough memory */
                                        /* blocks left over for the Minimum Number of */
                                        /* Tx Blocks. Returns the actual number of RX */
                                        /* buffers allocated in the memory map*/

    uint8   txMinimumMemblockNumber;    /* specifies the minimum number of blocks that */
                                        /* must be allocated to the TX pool. Follows */
                                        /* this limit even if the Number of Rx Memory */
                                        /* Blocks parameter is ignored.*/

    uint8   numStations;                /* The number of STAs supported in IBSS mode. */
                                        /* The FW uses this field to allocate memory */
                                        /* for STA context data such as security keys*/

    uint8   numSsidProfiles;            /* The number of SSID profiles used in IBSS mode */
                                        /* Enables different profiles for different STAs */

    uint32  totalTxDescriptors;         /* Total TX Descriptors - in the past it was configured per AC */

    uint8    dynMemEnable;              /* 0 - not supported */

    uint8    txFreeReq;                 /*minimum required free memory blocks in TX pool*/

    uint8    rxFreeReq;                 /*minimum required free memory blocks in TX pool*/

    uint8    txMin;                     /*minimum required total TX memory blocks in TX pool*/
    uint8   loggerFreeReq;              /*minimum required free memory blocks in FW Logger pool, 0 for disable feature*/

    uint8   reserved[3];                /* DWORD alignment padding */
} ACXConfigMemory_t;


/******************************************************************************

    Name:   ACX_SLOT
    Type:   Configuration
    Access: Write Only
    Length: 8

******************************************************************************/



typedef struct
{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION     /* host RoleID */
    uint8      woneIndex;               /* reserved*/

    uint8_t/*SlotTime_e*/ slotTime;                /* The slot size to be used. refer to SlotTime_enum.    */
    uint8      reserved[5];
} ACXSlot_t;


/******************************************************************************

    Name:   ACX_MEM_MAP
    Type:   Configuration
    Access: Read Only
    Length: 72
    Note:   Except for the numTxMemBlks, numRxMemBlks fields, this is
            used in MASTER mode only!!!
    
******************************************************************************/
#define MEM_MAP_NUM_FIELDS  24

typedef struct 
{
    uint32 *controlBlock; /* array of two 32-bit entries in the following order:
                            1. Tx-Result entries counter written by the FW
                            2. Tx-Result entries counter read by the host */
    void   *txResultQueueStart; /* points t first descriptor in TRQ */
} TxResultPointers_t;


/******************************************************************************

    Name:   ACX_AID
    Type:   Configuration
    Access: Write Only
    Length: 2
    
******************************************************************************/

typedef struct
{
                                    /* Offset Size */
    INFO_ELE_HDR                    /*      0    4 */
    CMD_PARAMS_HOST_ROLE_DEFINITION /*      4    1 - host RoleID */
    uint8  reserved;                /*      5    1 - alignment to 32bits boundary   */
    uint16  Aid;                    /*      6    2 - The Association ID to the WiLink. The WiLink uses this */
                    /* field to determine when the STA's AID bit is set in a */
                    /* received beacon and when a PS Poll frame should be */
                    /* transmitted to the AP. The host configures this information */
                    /* element after it has associated with an AP. This information */
                    /* element does not need to be set in Ad Hoc mode.*/
} ACXAid_t;


/******************************************************************************

    Name:   ACX_ERROR_CNT
    Type:   Operation
    Access: Read Only
    Length: 12
    
******************************************************************************/
typedef struct
{
    INFO_ELE_HDR
    uint32 PLCPErrorCount;  /* The number of PLCP errors since the last time this */
                            /* information element was interrogated. This field is */
                            /* automatically cleared when it is interrogated.*/
    
    uint32 FCSErrorCount;   /* The number of FCS errors since the last time this */
                            /* information element was interrogated. This field is */
                            /* automatically cleared when it is interrogated.*/
    
    uint32 validFrameCount; /* The number of MPDUs without PLCP header errors received*/
                            /* since the last time this information element was interrogated. */
                            /* This field is automatically cleared when it is interrogated.*/

    uint32 seqNumMissCount; /* the number of missed sequence numbers in the squentially */
                            /* values of frames seq numbers */

} ACXErrorCounters_t;

/******************************************************************************

    Name:   ACX_MEDIUM_USAGE
    Type:   Configuration
    Access: Read Only
    Length: 8

******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    uint32 mediumUsage; /* report to the host the value of medium usage registers*/
    uint32 period;      /* report to the host the value of medium period registers*/
} ACXMediumUsage_t;


/******************************************************************************

    Name:   ACX_BEACON_FILTER_OPT
    Desc:   This information element enables the host to activate beacon filtering. 
            The filter can only be activated when the STA is in PS mode. 
            When activated, either the host is not notified about beacons whose 
            unicast TIM bit is not set, or these beacons are buffered first and 
            the host is notified only after the buffer reaches a predetermined size.
            The host should not activate the filter if it configures the firmware 
            to listen to broadcasts (see the VBM Options field in the 
            ACXPowerMgmtOptions information element). The filter only affects beacons, 
            and not other MSDUs - the firmware notifies the host immediately about 
            their arrival.
    Type:   Filtering Configuration
    Access: Write Only
    Length: 2
 
******************************************************************************/
typedef struct  
{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION/* host RoleID */
    uint8   enable;                /* Indicates whether the filter is enabled. */
                                   /* 1 - enabled, 0 - disabled. */
    uint8   maxNumOfBeaconsStored; /* The number of beacons without the unicast TIM */
                                   /* bit set that the firmware buffers before */
                                   /* signaling the host about ready frames. */
                                   /* When set to 0 and the filter is enabled, beacons */
                                   /* without the unicast TIM bit set are dropped.*/
    uint8  padding[1];             /* alignment to 32bits boundry   */
} ACXBeaconFilterOptions_t;


/******************************************************************************

    Name:   ACX_BEACON_FILTER_TABLE
    Desc:   This information element configures beacon filtering handling for the
            set of information elements. An information element in a beacon can be 
            set to be: ignored (never compared, and changes will not cause beacon 
            transfer), checked (compared, and transferred in case of a change), or 
            transferred (transferred to the host for each appearance or disappearance).
            The table contains all information elements that are subject to monitoring 
            for host transfer. 
            All information elements that are not in the table should be ignored for 
            monitoring.
            This functionality is only enabled when beacon filtering is enabled by 
            ACX_BEACON_FILTER_OPT.
    Type:   Filtering Configuration
    Access: Write Only
    Length: 101
    Notes:  the field measuring the value of received beacons for which the device 
            wakes up the host in ACX_BEACON_FILTER_OPT does not affect 
            this information element.
    
******************************************************************************/

/*
    ACXBeaconFilterEntry (not 221)
    Byte Offset     Size (Bytes)    Definition 
    ===========     ============    ==========
    0               1               IE identifier
    1               1               Treatment bit mask

    ACXBeaconFilterEntry (221)
    Byte Offset     Size (Bytes)    Definition 
    ===========     ============    ==========
    0               1               IE identifier
    1               1               Treatment bit mask
    2               3               OUI
    5               1               Type
    6               2               Version


    Treatment bit mask - The information element handling:
                         bit 0 - The information element is compared and transferred
                                 in case of change.
                         bit 1 - The information element is transferred to the host 
                                 with each appearance or disappearance.
                         Note that both bits can be set at the same time.
*/
#define BEACON_FILTER_TABLE_MAX_IE_NUM                      (32)
#define BEACON_FILTER_TABLE_MAX_VENDOR_SPECIFIC_IE_NUM      (6)
#define BEACON_FILTER_TABLE_IE_ENTRY_SIZE                   (2)
#define BEACON_FILTER_TABLE_EXTRA_VENDOR_SPECIFIC_IE_SIZE   (6)
#define BEACON_FILTER_TABLE_MAX_SIZE    ((BEACON_FILTER_TABLE_MAX_IE_NUM * BEACON_FILTER_TABLE_IE_ENTRY_SIZE) + \
                                         (BEACON_FILTER_TABLE_MAX_VENDOR_SPECIFIC_IE_NUM * BEACON_FILTER_TABLE_EXTRA_VENDOR_SPECIFIC_IE_SIZE))

typedef struct ACXBeaconFilterIETableStruct {
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION             /* host RoleID */
    uint8 NumberOfIEs;                          /* The number of IE's in the table*/
                                                /* 0 - clears the table.*/

    uint8 padding[2];  /* alignment to 32bits boundry   */
    uint8 IETable[BEACON_FILTER_TABLE_MAX_SIZE];
} ACXBeaconFilterIETable_t;

/******************************************************************************

    Name:   ACX_COEX_ACTIVITY_TABLE
    
******************************************************************************/

typedef enum
{
    COEX_IP_BT = 0,
    COEX_IP_WLAN, 
    COEX_IP_DUAL_MODE,   /* That define isn't valid value in DR&FW interface and use just in the FW */
    MAX_COEX_IP
} CoexIp_enum;

#ifdef HOST_COMPILE
typedef uint8 CoexIp_e;
#else
typedef CoexIp_enum CoexIp_e;
#endif

typedef struct ACXCoexActivityIEStruct {
    INFO_ELE_HDR
    CoexIp_e coexIp;         /* 0-BT, 1-WLAN (according to CoexIp_e in FW) */
    uint8  activityId;       /* According to BT/WLAN activity numbering in FW */ 
    uint8  defaultPriority;  /* 0-255, activity default priority */
    uint8  raisedPriority;   /* 0-255, activity raised priority */
    uint16 minService;       /* 0-65535, The minimum service requested either in
                                requests or in milliseconds depending on activity ID */
    uint16 maxService;       /* 0-65535, The maximum service allowed either in
                            requests or in milliseconds depending on activity ID */
} ACXCoexActivityIE_t;

/******************************************************************************

    Name:   ACX_ARP_IP_FILTER 
    Type:   Filtering Configuration
    Access: Write Only
    Length: 20

******************************************************************************/

#define ARP_FILTER_DISABLED                    (0)
#define ARP_FILTER_ENABLED                  (0x01)
#define ARP_FILTER_AUTO_ARP_ENABLED         (0x02)

#define ARP_FILTER_SUSPENDED_BY_FW          (0x80)      // Bit set/cleared by FW to indicate temporary suspention of Auto-ARP 
                                                        // (due to join, before template is configured)
                                                        // This is NEVER used by the Host itself

#define ARP_FILTER_HOST_ENABLE_MASK         (0x03)      // Take only two lower bits of host configuration

typedef struct  
{    
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION /* host RoleID */
    uint8     ipVersion;            /* The IP version of the IP address: 4 - IPv4, 6 - IPv6.*/
    uint8     arpFilterEnable;      /* BITMAP (!) field containing all "enabled" ARP features */
                                    /* 0x00 - No ARP features */
                                    /* 0x01 - Only ARP filtering */
                                    /* 0x02 - Same as 3: Both ARP filtering and Auto-ARP (kept for backwards compatibility) */
                                    /* 0x03 - Both ARP filtering and Auto-ARP */
    uint8     padding[1];           /* alignment to 32bits boundry   */
    uint8     address[16];          /* The IP address used to filter ARP packets. ARP packets */
                                    /* that do not match this address are dropped. */
                                    /* When the IP Version is 4, the last 12 bytes of */
                                    /* the address are ignored.*/
} ACXConfigureIP_t;


/******************************************************************************

  Name:     ACX_IBSS_FILTER
  Type:     Filtering Configuration
  Access:   Write Only
  Length:   1
  
******************************************************************************/
typedef struct  
{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION /* host RoleID */
    uint8   enable;                 /* if set (i.e. IBSS mode), forward beacons from the same SSID*/
                                    /* (also from different BSSID), with bigger TSF then the this of */
                                    /* the current BSS.*/
    uint8   padding[2];             /* alignment to 32bits boundry   */
} ACXIBSSFilterOptions_t;


/******************************************************************************

  Name:     ACX_SERVICE_PERIOD_TIMEOUT
  Type:     Configuration
  Access:   Write Only
  Length:   1
  
******************************************************************************/
typedef struct 
{    
                                    /* Offset Size */
    INFO_ELE_HDR                    /*      0    4 */
    CMD_PARAMS_HOST_ROLE_DEFINITION /*      4    1 - host RoleID */
    uint8  Reserved;                /*      5    1 */
    uint16 PsPollTimeout;           /*      6    2 - the maximum time that the device will wait to receive */
                                                    /* traffic from the AP after transmission of PS-poll.*/
    
    uint16 UpsdTimeout;             /*      8    2 - the maximum time that the device will wait to receive */
                                                    /* traffic from the AP after transmission from UPSD enabled*/
                                                    /* queue.*/
    uint8  Padding[2];              /*      10   2 - 32-bit align */
} ACXRxTimeout_t;

/******************************************************************************

    Name:   ACX_TX_QUEUE_CFG
    Type:   Configuration
    Access: Write Only
    Length: 8
    
******************************************************************************/
typedef struct
{
    INFO_ELE_HDR
    uint8   qID;                        /* The TX queue ID number.*/
    uint8   padding[3];                 /* alignment to 32bits boundry   */
    uint16  numberOfBlockHighThreshold; /* The maximum memory blocks allowed in the */
                                        /* queue.*/
    uint16  numberOfBlockLowThreshold;  /* The minimum memory blocks that are */
                                        /* guaranteed for this queue.*/
} ACXTxQueueCfg_t;


/******************************************************************************

    Name:   ACX_STATISTICS
    Type:   Statistics
    Access: Write Only
    Length: 
    Note:   Debug API

******************************************************************************/
typedef struct 
{
    uint32  debug1;
    uint32  debug2;
    uint32  debug3;
    uint32  debug4;
    uint32  debug5;
    uint32  debug6;
}DbgStatistics_t;

typedef struct
{
    uint32 IRQs;              /* irqisr() */
} IsrStatistics_t;

#define PWR_STAT_MAX_CONT_MISSED_BCNS_SPREAD 10
typedef struct PwrStatistics_t
{
    uint32 MissingBcnsCnt;      /* Count the amount of missing beacon interrupts to the host.*/
    uint32 RcvdBeaconsCnt;      /* Count the number of received beacons.*/
    uint32 ConnectionOutOfSync;         /* Count the number of times TSF Out Of Sync occures, meaning we lost more consecutive beacons that defined by the host's threshold.*/
    uint32 ContMissBcnsSpread[PWR_STAT_MAX_CONT_MISSED_BCNS_SPREAD];  /* Gives statistics about the spread continuous missed beacons.*/
                                    /* The 16 LSB are dedicated for the PS mode.*/
                                    /* The 16 MSB are dedicated for the PS mode.*/
                                    /* ContMissBcnsSpread[0] - single missed beacon.*/
                                    /* ContMissBcnsSpread[1] - two continuous missed beacons.*/
                                    /* ContMissBcnsSpread[2] - three continuous missed beacons.*/
                                    /* ...*/
                                    /* ContMissBcnsSpread[9] - ten and more continuous missed beacons.*/
    uint32 RcvdAwakeBeaconsCnt; /* Count the number of beacons in awake mode.*/
} PwrStatistics_t;

typedef struct PsPollUpsdStatistics_t
{
    uint32 psPollTimeOuts;
    uint32 upsdTimeOuts;
    uint32 upsdMaxAPturn;
    uint32 psPollMaxAPturn;
    uint32 psPollUtilization;
    uint32 upsdUtilization;
} PsPollUpsdStatistics_t;



#ifdef TNETW18xx

typedef struct
{
    uint32 numOfPreparedDescs;
    uint32 numOfTxCmplt;
    uint32 numOfTxTemplatePrepared;
    uint32 numOfTxDataPrepared;
    uint32 numOfTxTemplateProgrammed;
    uint32 numOfTxDataProgrammed;
    uint32 numOfTxBurstProgrammed;
    uint32 numOfTxStarts;
    uint32 numOfTxStop;
    uint32 numOfTxImmResp;
    uint32 numOfTxStartTempaltes;
    uint32 numOfTxStartIntTemplate;
    uint32 numOfTxStartFwGen;
    uint32 numOfTxStartData;
    uint32 numOfTxStartNullFrame;
    uint32 numOfTxExch;
    uint32 numOfTxRetryTemplate;
    uint32 numOfTxRetryData;
    uint32 numOfTxRetryPerRate[NUM_OF_RATES_INDEXES];
    uint32 numOfTxExchPending;
    uint32 numOfTxExchExpiry;
    uint32 numOfTxDoneTemplate;
    uint32 numOfTxDoneData;
    uint32 numOfTxDoneIntTemplate;
    uint32 numOfTxCFE_1;
    uint32 numOfTxCFE_2;
    uint32 fragCalled;
    uint32 fragMpduAllocFailed;
    uint32 fragInitCalled;
    uint32 fragInProcessCalled;
    uint32 fragTkipCalled;
    uint32 fragKeyNotFound;
    uint32 fragNeedFragmentation;
    uint32 fragBadMblkNum;
    uint32 fragFailed;
    uint32 fragCacheHit;
    uint32 fragCacheMiss;
} TxStatistics_t;

typedef struct
{
    uint32 RxBeaconEarlyTerm;
    uint32 RxOutOfMpduNodes;
    uint32 RxHdrOverflow;
    uint32 RxDroppedFrame;
    uint32 RxDoneStage;
    uint32 RxDone;
    uint32 RxDefrag;
    uint32 RxDefragEnd;
    uint32 RxCmplt;
    uint32 RxPreCmplt;
    uint32 RxCmpltTask;
    uint32 RxPhyHdr;
    uint32 RxTimeout;
    uint32 RxRtsTimeout;
    uint32 RxTimeoutWA;
    uint32 RxWADensityDroppedFrame;
    uint32 RxWABaNotExpected;
    uint32 defragCalled;
    uint32 defragInitCalled;
    uint32 defragInProcessCalled;
    uint32 defragTkipCalled;
    uint32 defragNeedDefrag;
    uint32 defragDecryptFailed;
    uint32 decryptKeyNotFound;
    uint32 defragNeedDecr;
    uint32 TKIPReplays;
    uint32 RxXfr;
} RxStatistics_t;

typedef struct RxFilterStatistics_t
{
    uint32 beaconFilter;
    uint32 arpFilter;
    uint32 MCFilter;
    uint32 dupFilter;
    uint32 dataFilter;
    uint32 ibssFilter;
    uint32 protectionFilter;
    uint32 accumArpPendRequests;
    uint32 maxArpQueueDep;
} RxFilterStatistics_t;

typedef struct RxRatesStatistics_t
{
	uint32 rxFramesPerRates[50]; // max number of rates
}RxRatesStatistics_t;

typedef struct AggregationSizeStat_t
{
    uint32 txAggRate[16];
    uint32 txAggLen[16];
    uint32 rxAggregationSize[16];
}AggregationSizeStat_t;

typedef struct NewPipelineStat_t
{
    uint32 HSTxStatFifoInt;
    uint32 HSRxStatFifoInt;
    uint32 TCPTxStatFifoInt;
    uint32 TCPRxStatFifoInt;
    uint32 EncTxStatFifoInt;
    uint32 EncRxStatFifoInt;
    uint32 RxCompleteStatFifoInt;
    uint32 preProcSwi;
    uint32 postProcSwi;
    uint32 secFragSwi;
    uint32 preToDefragSwi; //rx
    uint32 defragToCsumSwi;//rx
    uint32 CsumToRxXferSwi;//rx
    uint32 DecPacketIn;
    uint32 DecPacketInFifoFull;
    uint32 DecPacketOut;
    uint32 CsRxPacketIn;
    uint32 CsRxPacketOut;
    uint16 pipelineFifoFull[NUM_OF_FIFOS];
    uint16 padding[1];
}PipelineStat_t;

typedef struct memStatistics_t
{
    uint32 rxFreeMemBLks;
    uint32 txFreeMemBLks;
    uint32 fwLogFreeMemBLks;
    uint32 fwGenFreeMemBLks;
}memStatistics_t;
//TODO: numlFrameCtsNulFlid not used at all, may be removed
// numllFrameCtsStart probably not needed as well
typedef struct ErrorStatistics_t
{
//-------------------------------------------------------------
// separate error frames counter to error frames originated from 
// control frames vs. error frames originated from non ctrl frames
    uint32 errorFrameNonCtrl;
    uint32 errorFrameCtrl;
    uint32 errorFrameDuringProtection;
//-------------------------------------------------------------
    uint32 nullFrameTxStart;
    uint32 numllFrameCtsStart;
    uint32 barRetry;
	uint32 numlFrameCtsNulFlid;
	uint32 txAbortFailure;
	uint32 txResumeFailure;
	//uint32 receivedCorruptedBA;
    uint32 rxCmpltDbOverflowCount; 
    uint32 goint_to_elp_while_rx_exch_in_progress;
    uint32 goint_to_elp_while_tx_exch_in_progress;
    uint32 goint_to_elp_while_tx_in_progress;
    uint32 going_to_elp_while_nvic_is_pending;
	uint32 rxExcessiveFrameLen;
    uint32 errorBurstModeMismatch;
    uint32 errorTbcHwExchMismatch;
}ErrorStatistics_t;


typedef struct thermalStatistics_t
{
	uint16 numOfIrqThrLow;
	uint16 numOfIrqThrHigh;
    uint16 numOfTxStop;
    uint16 numOfTxResume;
	uint16 numOfFalseIrq;
    uint16 numOfAdcSourceUnexpected;
}thermalStatistics_t;


typedef struct
{
    uint16  calibFailCount[NUM_OF_CALIBRATIONS_ERRORS];
    uint32  calibrationCount;
}CalibFailStatistics_t;

typedef struct RoamingStatistics_t
{
    int32   rssiLevel;
}RoamingStatistics_t;

typedef struct
{
    uint32  numOfRadarDetections;
}DfsStatistics_t;


typedef struct ACXStatisticsStruct
{
    INFO_ELE_HDR
    ErrorStatistics_t           error;
    DbgStatistics_t             debug;
    TxStatistics_t              tx;
    RxStatistics_t              rx;
    IsrStatistics_t             isr;
    PwrStatistics_t             pwr;
    PsPollUpsdStatistics_t      ps;
    RxFilterStatistics_t        rxFilter;
    RxRatesStatistics_t         rxRates;
    AggregationSizeStat_t       aggSize;
    PipelineStat_t              pipeLine;
    memStatistics_t             mem;
    thermalStatistics_t			thermal;
    CalibFailStatistics_t       calibFStArr;
    RoamingStatistics_t         roam;
    DfsStatistics_t             dfs;
} ACXStatistics_t;

#else //end of R8 start R5

typedef struct 
{
    uint32  numOfTxProcs;
    uint32  numOfPreparedDescs;
    uint32  numOfTxXfr;
    uint32  numOfTxDma;    
    uint32  numOfTxCmplt;
    uint32  numOfRxProcs;
    uint32  numOfRxData;
}RingStatistics_t;

typedef struct
{
    uint32 numOfTxTemplatePrepared;
    uint32 numOfTxDataPrepared;
    uint32 numOfTxTemplateProgrammed;
    uint32 numOfTxDataProgrammed;
    uint32 numOfTxBurstProgrammed;
    uint32 numOfTxStarts;
    uint32 numOfTxImmResp;
    uint32 numOfTxStartTempaltes;
    uint32 numOfTxStartIntTemplate;
    uint32 numOfTxStartFwGen;
    uint32 numOfTxStartData;
    uint32 numOfTxStartNullFrame;
    uint32 numOfTxExch;
    uint32 numOfTxRetryTemplate;
    uint32 numOfTxRetryData;
    uint32 numOfTxExchPending;
    uint32 numOfTxExchExpiry;
    uint32 numOfTxExchMismatch;
    uint32 numOfTxDoneTemplate;
    uint32 numOfTxDoneData;
    uint32 numOfTxDoneIntTemplate;
    uint32 numOfTxPreXfr;
    uint32 numOfTxXfr;
    uint32 numOfTxXfrOutOfMem;
    uint32 numOfTxDmaProgrammed;
    uint32 numOfTxDmaDone;
} TxStatistics_t;


typedef struct
{
    uint32 RxOutOfMem;
    uint32 RxHdrOverflow;
    uint32 RxHWStuck;
    uint32 RxDroppedFrame;
    uint32 RxCompleteDroppedFrame;
    uint32 RxAllocFrame;
    uint32 RxDoneQueue;
    uint32 RxDone;
    uint32 RxDefrag;
    uint32 RxDefragEnd;
    uint32 RxMic;
    uint32 RxMicEnd;
    uint32 RxXfr;
    uint32 RxXfrEnd;
    uint32 RxCmplt;
    uint32 RxPreCmplt;
    uint32 RxCmpltTask;
    uint32 RxPhyHdr;
    uint32 RxTimeout;
} RxStatistics_t;


typedef struct
{
    uint32 RxDMAErrors;
    uint32 TxDMAErrors;
} DMAStatistics_t;

typedef struct WepStatistics_t
{
    uint32 WepAddrKeyCount;      /* Count of WEP address keys configured*/
    uint32 WepDefaultKeyCount;   /* Count of default keys configured*/
    uint32 WepKeyNotFound;       /* count of number of times that WEP key not found on lookup*/
    uint32 WepDecryptFail;       /* count of number of times that WEP key decryption failed*/
    uint32 WepEncryptFail;       /* count of number of times that WEP key encryption failed*/
    uint32 WepDecPackets;        /* WEP Packets Decrypted*/
    uint32 WepDecInterrupt;      /* WEP Decrypt Interrupts*/
    uint32 WepEnPackets;         /* WEP Packets Encrypted*/
    uint32 WepEnInterrupt;       /* WEP Encrypt Interrupts*/
} WepStatistics_t;

typedef struct MicStatistics_t
{
    uint32 MicRxPkts;
    uint32 MicCalcFailure;
} MicStatistics_t;


typedef struct AesStatisticsStruct
{
    uint32 AesEncryptFail;
    uint32 AesDecryptFail;
    uint32 AesEncryptPackets;
    uint32 AesDecryptPackets;
    uint32 AesEncryptInterrupt;
    uint32 AesDecryptInterrupt;
} AesStatistics_t;

typedef struct GemStatisticsStruct
{
    uint32 GemEncryptFail;
    uint32 GemDecryptFail;
    uint32 GemEncryptPackets;
    uint32 GemDecryptPackets;
    uint32 GemEncryptInterrupt;
    uint32 GemDecryptInterrupt;
} GemStatistics_t;

typedef struct EventStatistics_t
{
    uint32 calibration;
    uint32 rxMismatch;
    uint32 rxMemEmpty;
} EventStatistics_t;

typedef struct RxFilterStatistics_t
{
    uint32 beaconFilter;
    uint32 arpFilter;
    uint32 MCFilter;
    uint32 dupFilter;
    uint32 dataFilter;
    uint32 ibssFilter;
    uint32 protectionFilter;
} RxFilterStatistics_t;

typedef struct ClaibrationFailStatistics_t
{
    uint32 initCalTotal;
    uint32 initRadioBandsFail;
    uint32 initSetParams;
    uint32 initTxClpcFail;
    uint32 initRxIqMmFail;
    uint32 tuneCalTotal;
    uint32 tuneDrpwRTrimFail;
    uint32 tuneDrpwPdBufFail;
    uint32 tuneDrpwTxMixFreqFail;
    uint32 tuneDrpwTaCal;
    uint32 tuneDrpwRxIf2Gain;
    uint32 tuneDrpwRxDac;
    uint32 tuneDrpwChanTune;
    uint32 tuneDrpwRxTxLpf;
    uint32 tuneDrpwLnaTank;
    uint32 tuneTxLOLeakFail;
    uint32 tuneTxIqMmFail;
    uint32 tuneTxPdetFail;
    uint32 tuneTxPPAFail;
    uint32 tuneTxClpcFail;
    uint32 tuneRxAnaDcFail;
#ifdef TNETW1283
    uint32 tuneRxDigDcFail;
#endif
    uint32 tuneRxIqMmFail;
    uint32 calStateFail;
}ClaibrationFailStatistics_t;

typedef struct ACXStatisticsStruct
{
    INFO_ELE_HDR
    RingStatistics_t ringStat;
    DbgStatistics_t  debug;
    TxStatistics_t   tx;
    RxStatistics_t   rx;
    DMAStatistics_t  dma;
    IsrStatistics_t  isr;
    WepStatistics_t  wep;
    PwrStatistics_t  pwr;
    AesStatistics_t  aes;
    MicStatistics_t  mic;
    EventStatistics_t event;
    PsPollUpsdStatistics_t ps;
    RxFilterStatistics_t rxFilter;
    ClaibrationFailStatistics_t radioCal;
    GemStatistics_t  gem;
} ACXStatistics_t;

#endif //end of R5

/******************************************************************************

    Name:   ACX_ROAMING_STATISTICS_TBL
    Desc:   This information element reads the current roaming triggers 
            counters/metrics. 
    Type:   Statistics
    Access: Read Only
    Length: 6

******************************************************************************/
typedef struct 
{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION
    uint8  padding[3];
    uint32 MissedBeacons; /* The current number of consecutive lost beacons*/
    uint8  snrData;       /* The current average SNR in db - For Data Packets*/
    uint8  snrBeacon;     /* The current average SNR in db - For Beacon Packets*/
    int8   rssiData;      /* The current average RSSI  - For Data Packets*/
    int8   rssiBeacon;    /* The current average RSSI - For Beacon Packets*/
}ACXRoamingStatisticsTable_t;


/******************************************************************************

    Name:   ACX_FEATURE_CFG
    Desc:   Provides expandability for future features
    Type:   Configuration
    Access: Write Only
    Length: 8
    
******************************************************************************/

/* bit defines for Option: */
#define FEAT_PCI_CLK_RUN_ENABLE     0x00000002  /* Enable CLK_RUN on PCI bus */

/* bit defines for dataflowOptions: */
#define DF_ENCRYPTION_DISABLE       0x00000001  /* When set, enable encription in FW.*/
                                                /* when clear, disable encription. */
#define DF_SNIFF_MODE_ENABLE        0x00000080  /* When set, enable decryption in FW.*/
                                                /* when clear, disable decription. */
typedef struct
{
                                    /* Offset Size */
    INFO_ELE_HDR                    /*      0    4 */
    CMD_PARAMS_HOST_ROLE_DEFINITION /*      4    1 - host RoleID */
    uint8  padding[3];              /*      5    3 - for 32-bit align */
    uint32 Options;                 /*      8    4 - Data flow options - refer to above definitions*/
    uint32 dataflowOptions;         /*     12    4 - Data flow options - refer to above definitions*/
} ACXFeatureConfig_t;




/******************************************************************************

    Name:	ACX_PS_RX_STREAMING
    Type:   Configuration
    Access: Write Only
	Length: 64
    
******************************************************************************/
typedef struct
{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION /* host RoleID */
    uint8     TID;                  /* The TID index*/
    Bool_e     rxPSDEnabled;        /* indicates if this traffic stream requires */
                                    /* employing an RX Streaming delivery mechanism for the TID*/

    uint16   streamPeriod;           /* the time period for which a trigger needs to be transmitted*/
                                    /* in case no data TX triggers are sent by host*/
    uint16   txTimeout;              /* the timeout from last TX trigger after which FW*/
                                    /* starts generating triggers by itself*/
    uint8   padding[1];             /* for 32-bit align */
}ACXPsRxStreaming_t;

#ifdef TNETW18xx

/******************************************************************************

    Name:	ACX_AUTO_RX_STREAMING
	Type:	Configuration
	Access:	Write Only
	Length: 32

******************************************************************************/
typedef struct
{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION 	/* host RoleID */
    Bool_e     	rxAutoEnable;        	/* indicates if the RX Streaming use HOST or FW configuration*/
    uint8   	padding[2];             /* for 32-bit align */
}ACXAutoRxStreaming_t;

#endif

/************************************************************
*      MULTIPLE RSSI AND SNR                                *
*************************************************************/

typedef enum
{
    RX_QUALITY_EVENT_LEVEL = 0,  /* The event is a "Level" indication which keeps */
                               /* triggering as long as the average RSSI is below*/
                               /* the threshold.*/

    RX_QUALITY_EVENT_EDGE = 1    /* The event is an "Edge" indication which triggers*/
                               /* only when the RSSI threshold is crossed from above.*/
}rxQualityEventType_enum;

/* The direction in which the trigger is active */
typedef enum
{
    RSSI_EVENT_DIR_LOW = 0,
    RSSI_EVENT_DIR_HIGH = 1,
    RSSI_EVENT_DIR_BIDIR = 2
}RssiEventDir_e;

/******************************************************************************

    RSSI/SNR trigger configuration:

    ACX_RSSI_SNR_TRIGGER
    ACX_RSSI_SNR_WIGHTS

******************************************************************************/
#define NUM_OF_RSSI_SNR_TRIGGERS 8
typedef struct
{
                                    /* Offset Size */
    CMD_PARAMS_HOST_ROLE_DEFINITION /*      4    1 - host RoleID */
    uint8  metric;                  /*      5    1 - RSSI Beacon, RSSI Packet, SNR Beacon, SNR Packet */
    uint8  type;                    /*      6    1 - Level / Edge */
    uint8  direction;               /*      7    1 - Low, High, Bidirectional */
    int16  threshold;               /*      8    2 */
    uint16 pacing;                  /*     10    2 - Minimum delay between consecutive triggers in milliseconds (0 - 60000) */
    uint8  hystersis;               /*     12    1 - Hysteresis range in dB around the threshold value (0 - 255) */
    uint8  index;                   /*     13    1 - Index of Event. Values 0 - 7 */
    uint8  enable;                  /*     14    1 - 1 - Configured, 2 - Not Configured;  (for recovery using) */
    uint8  padding[1];              /*     15    1 - for 32-bit alignment */
}RssiSnrTriggerCfg_t;

typedef struct
{
                                /* Offset Size */
    INFO_ELE_HDR                /*      0    4 */
    RssiSnrTriggerCfg_t param;  /*      4   12 */
}ACXRssiSnrTriggerCfg_t;

/* Filter Weight for every one of 4 RSSI /SNR Trigger Metrics  */
typedef struct
{
                                    /* Offset Size */
    uint8 rssiBeaconAverageWeight;  /*      8    1 */
    uint8 rssiPacketAverageWeight;  /*      9    1 */
    uint8 snrBeaconAverageWeight;   /*     10    1 */
    uint8 snrPacketAverageWeight;   /*     11    1 */
}RssiSnrAverageWeights_t;

typedef struct
{
                                    /* Offset Size */
    INFO_ELE_HDR                    /*      0    4 */
    CMD_PARAMS_HOST_ROLE_DEFINITION /*      4    1 - host RoleID*/
    uint8 padding[3];               /*      5    3 - for 32-bit alignment */
    RssiSnrAverageWeights_t param;  /*      8    4 */
}ACXRssiSnrAverageWeights_t;

typedef enum
{
    METRIC_EVENT_RSSI_BEACON = 0,
    METRIC_EVENT_RSSI_DATA   = 1,
    METRIC_EVENT_SNR_BEACON  = 2,
    METRIC_EVENT_SNR_DATA     = 3, 
    METRIC_EVENT_TRIGGER_SIZE = 4
}MetricEvent_e;

/******************************************************************************

    Name:   ACX_NOISE_HIST
    Desc:   Noise Histogram activation is done by special command from host which
            is responsible to read the results using this IE.
    Type:   Configuration
    Access: Read Only
    Length: 48 (NOISE_HIST_LEN=8)
    
******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    uint32 counters[NOISE_HIST_LEN]; /* This array of eight 32 bit counters describes */
                                     /* the histogram created by the FW noise */
                                     /* histogram engine.*/

    uint32 numOfLostCycles;          /* This field indicates the number of measurement */
                                     /* cycles with failure because Tx was active.*/

    uint32 numOfTxHwGenLostCycles;   /* This field indicates the number of measurement */
                                     /* cycles with failure because Tx (FW Generated)*/
                                     /* was active.*/

    uint32 numOfRxLostCycles;        /* This field indicates the number of measurement */
                                     /* cycles because the Rx CCA was active. */
} NoiseHistResult_t;

/******************************************************************************

    Name:   ACX_PD_THRESHOLD
    Type:   Configuration
    Access: Write Only
    Length: 4

******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    uint32 pdThreshold; /* The packet detection threshold in the PHY.*/
} ACXPacketDetection_t;


/******************************************************************************

    Name:   ACX_RATE_POLICY
    Type:   Configuration
    Access: Write Only
    Length: 132

******************************************************************************/

#define HOST_MAX_RATE_POLICIES       (16)

typedef struct 
{
    INFO_ELE_HDR
    uint32        ratePolicyIndex;                      /* The index of the rate policy */
    txAttrClass_t ratePolicy;                           /* Rate Policies table*/
} ACXTxAttrRatePolicy_t;


/******************************************************************************

    Name:   ACX_CTS_PROTECTION
    Type:   Configuration
    Access: Write Only
    Length: 1
    
******************************************************************************/

typedef struct 
{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION /* host RoleID */
    uint8   ctsProtectMode;         /* This field is a flag enabling or disabling the*/
                                    /* CTS-to-self protection mechanism:*/
                                    /* 0 - disable, 1 - enable*/
    uint8  padding[2];              /* alignment to 32bits boundry   */
}ACXCtsProtection_t;

/******************************************************************************

    ACX_FRAG_CFG

******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    uint16  fragThreshold;
    uint8   padding[2];          /* alignment toIE_RTS_CTS_CFG 32bits boundry   */
   
} ACXFRAGThreshold_t;


/******************************************************************************

    ACX_RX_CONFIG_OPT

******************************************************************************/
typedef enum  
{
    RX_QUEUE_TYPE_RX_LOW_PRIORITY,    /* All except the high priority */
    RX_QUEUE_TYPE_RX_HIGH_PRIORITY,   /* Management and voice packets */
    RX_QUEUE_TYPE_NUM,
    RX_QUEUE_TYPE_MAX = MAX_POSITIVE8
} RxQueueType_enum;


#ifdef HOST_COMPILE
    typedef uint8 RxQueueType_e;
#else
    typedef RxQueueType_enum RxQueueType_e;
#endif


typedef struct 
{
    INFO_ELE_HDR
    uint16         rxMblkThreshold;   /* Occupied Rx mem-blocks number which requires interrupting the host (0 = no buffering) */
    uint16         rxPktThreshold;    /* Rx packets number which requires interrupting the host  (0 = no buffering) */ 
    uint16         rxCompleteTimeout; /* Max time in msec the FW may delay Rx-Complete interrupt */
    RxQueueType_e  rxQueueType;       /* see above */   
    uint8          reserved;
} ACXRxBufferingConfig_t;


/******************************************************************************

    FW_GEN_CMD

******************************************************************************/

#define GEN_FW_CMD_SIZE    16

typedef struct 
{
    INFO_ELE_HDR
    uint8         genFwCmdBytes[GEN_FW_CMD_SIZE];   
} ACXGenFwCmd_t;

/******************************************************************************

    HOST_IF_CFG_BITMAP

******************************************************************************/

#define HOST_IF_CFG_BITMAP_NO_CHANGE          0x00000000
#define HOST_IF_CFG_BITMAP_RX_FIFO_ENABLE     0x00000001 //BIT_0
#define HOST_IF_CFG_BITMAP_TX_EXTRA_BLKS_SWAP 0x00000002 //BIT_1
#define HOST_IF_CFG_BITMAP_RX_AGGR_WA_ENABLE  0x00000004 //BIT_2
#define HOST_IF_CFG_BITMAP_TX_PAD_TO_SDIO_BLK 0x00000008 //BIT_3
#define HOST_IF_CFG_BITMAP_RX_PAD_TO_SDIO_BLK 0x00000010 //BIT_4   
#define HOST_IF_CFG_BITMAP_USE_EOT            0x00000020 //BIT_5   // using EOT interrupts for FW status reading, command & events mailbox
#define HOST_IF_CFG_BITMAP_ADD_RX_ALIGNMENT   0x00000040 //BIT_6   // for QOS frames HW will add 2 extra bytes between rx info and mac header.



typedef struct 
{
    INFO_ELE_HDR
    uint32         hostIfCfgBitmap;   
    uint32         hostSdioBlockSize;
    uint32         extraMemBlocks;      // default 2 - extra mem blocks per frame in TX.
    uint32         lengthFieldSize;     // number of bits of the length field in the first TX word (up to 15 - for using the entire 16 bits).
} ACXHostIfCfgBitmap_t;

/******************************************************************************

    Name:   ACX_SLEEP_AUTH
    Desc:   configuration of sleep authorization level
    Type:   System Configuration
    Access: Write Only
    Length: 1

******************************************************************************/

typedef struct 
{
    INFO_ELE_HDR
    uint8   sleepAuth; /* The sleep level authorization of the device. */
                       /* 0 - Always active*/
                       /* 1 - Power down mode: light / fast sleep*/
                       /* 2 - ELP mode: Deep / Max sleep*/
        
    uint8  padding[3]; /* alignment to 32bits boundry   */
}ACXSleepAuth_t;

/******************************************************************************

    Name:   ACX_PM_CONFIG
    Desc:   configuration of power management
    Type:   System Configuration
    Access: Write Only
    Length: 1

******************************************************************************/

typedef struct 
{
    INFO_ELE_HDR
    uint32  hostClkSettlingTime;    /* Host CLK settling time (in uSec units) */
    uint8   hostFastWakeupSupport;  /* 0 - not supported */
                                    /* 1 - supported */
    uint8  padding[3];              /* alignment to 32bits boundry   */
}ACXPMConfig_t;

#ifdef TNETW18xx

/******************************************************************************

    Name:   ACX_CSUM_CONFIG
    Type:   Configuration
    Access: Write Only
    Length: 1
    
******************************************************************************/

typedef enum
{
    CHECKSUM_OFFLOAD_DISABLED = 0,
    CHECKSUM_OFFLOAD_ENABLED  = 1,
    CHECKSUM_OFFLOAD_FAKE_RX  = 2,
    CHECKSUM_OFFLOAD_INVALID  = 0xFF
} cSumState_enum;


#ifdef HOST_COMPILE
typedef uint8 cSumState_e;
#else
typedef cSumState_enum cSumState_e;
#endif

typedef struct
{
	INFO_ELE_HDR
	cSumState_e csumState;  		
	uint8 padding[3];
} ACXCSumCfg_t;

#endif

/******************************************************************************

    Name:   ACX_PREAMBLE_TYPE
    Type:   Configuration
    Access: Write Only
    Length: 1
    
******************************************************************************/


typedef struct
{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION     /* host RoleID */
    PreambleType_e preamble;                /* When set, the WiLink transmits beacon, probe response, */
                                        /* RTS and PS Poll frames with a short preamble. */
                                        /* When clear, the WiLink transmits the frame with a long */
                                        /* preamble.*/
    uint8  padding[2];                  /* alignment to 32bits boundry   */
} ACXPreamble_t;


/******************************************************************************

    Name:   ACX_CCA_THRESHOLD
    Type:   Configuration
    Access: Write Only
    Length: 2
    
******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    uint16 rxCCAThreshold; /* The Rx Clear Channel Assessment threshold in the PHY*/
                           /* (the energy threshold).*/
    Bool_e txEnergyDetection;  /* The Tx ED value for TELEC Enable/Disable*/
    uint8  padding;
} ACXEnergyDetection_t;
      

/******************************************************************************

    Name:   ACX_EVENT_MBOX_MASK
    Type:   Operation
    Access: Write Only
    Length: 8
    
******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    uint32 lowEventMask;   /* Indicates which events are masked and which are not*/
                           /* Refer to EventMBoxId_enum in public_event_mbox.h.*/
    
    uint32 highEventMask;  /* Not in use (should always be set to 0xFFFFFFFF).*/
} ACXEventMboxMask_t;


/******************************************************************************

    Name:   ACX_CONN_MONIT_PARAMS
    Desc:   This information element configures the SYNCHRONIZATION_TIMEOUT 
            interrupt indicator. It configures the number of missed Beacons 
            before issuing the SYNCHRONIZATION_TIMEOUT event.
    Type:   Configuration
    Access: Write Only
    Length: 8

******************************************************************************/

//typedef struct
//{
//                                    /* Offset Size */
//    INFO_ELE_HDR                    /*      0    4 */
//    CMD_PARAMS_HOST_ROLE_DEFINITION /*      4    1 - host RoleID */
//    uint8  padding[3];              /*      5    3 - 32-bit align */
//    uint32 TSFMissedThreshold;      /*      8    4 - The number of consecutive beacons that can be */
//                                                    /* lost before the WiLink raises the */
//                                                    /* SYNCHRONIZATION_TIMEOUT event.*/
//
//    uint32 BSSLossTimeout;          /*     12    4 - The delay (in time units) between the time at */
//                                                    /* which the device issues the SYNCHRONIZATION_TIMEOUT*/
//                                                    /* event until, if no probe response or beacon is */
//                                                    /* received a BSS_LOSS event is issued.*/
//} AcxConnectionMonitorOptions;


/******************************************************************************

    Name:   MAX_TX_FAILURE_THRESHOLD_CFG
    Desc:   This information element configures the number of frames transmission
            failures before issuing the "Max Tx Retry" event. The counter is 
            incremented only for unicast frames or frames that require Ack.
            The host will act according to the corresponding role: roam on STA and
            mark station as inactive on AP/GO.
    Type:   Configuration
    Access: Write Only
    Length: 1
    
******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION        /* host RoleID */
    uint8 reserved;
    uint16 maxTxRetry; /* the number of frames transmission failures before */
                      /* issuing the aging event*/
    
} ACXTxFailureThresholdParameters_t;


 
/******************************************************************************

    Name:   ACX_BCN_DTIM_OPTIONS
    Type:   Configuration
    Access: Write Only
    Length: 5
    
******************************************************************************/

//typedef struct
//{
//                                                /* Offset Size */
//    INFO_ELE_HDR                                /*      0    4 */
//    CMD_PARAMS_HOST_ROLE_DEFINITION             /*      4    1 - host RoleID */
//    uint8  rxBroadcastInPS;                     /*      5    1 - if set, enables receive of broadcast packets */
//    uint16 beaconRxTimeOut;                     /*      6    2 */
//    uint16 broadcastTimeOut;                    /*      8    2 */
//                             /* in Power-Save mode.*/
//    uint8  consecutivePsPollDeliveryFailureThr; /*     10    1 - Consecutive PS Poll Fail before updating the Driver (NO longer relevant) */
//                                                                /*
//                                                                 * Kaplan [May/2012]: in the process of moving the traffic monitor
//                                                                 * to the FW, the Driver had stopped to check this event, so it is NO longer relevant.
//                                                                 * If this IOP issue will re-appear, there are two options:
//                                                                 * 1. Driver can check this event and Force PS (change Auto mode to PS mode)
//                                                                 * 2. Traffic Monitor can handle this event and Force PS
//                                                                 * Note: for SG it is still needed to count the number of consecutive PsPoll/QosNull
//                                                                 * delivery failures.
//                                                                 */
//    uint8  padding[1];                          /*     11    1 - alignment to 32bits boundary   */
//} ACXBeaconAndBroadcastOptions_t;


/******************************************************************************

    Name:   ACX_SG_ENABLE
    Desc:   This command instructs the WiLink to set the Soft Gemini (BT co-existence)
            state to either enable/disable or sense mode. 
    Type:   Configuration
    Access: Write Only
    Length: 1
    
******************************************************************************/
typedef struct
{
    INFO_ELE_HDR
    uint8   coexOperationMode; /* 0- Co-ex operation is Disabled
                                  1- Co-ex operation is Disabled
                                  
                                  Default Value: 0- Co-ex operation is Disabled
                                */

    uint8  padding[3];  /* alignment to 32bits boundry   */

} ACXBluetoothWlanCoEnableStruct;



/** \struct TSoftGeminiParams
 * \brief Soft Gemini Parameters
 * 
 * \par Description
 * Used for Setting/Printing Soft Gemini Parameters
 * 
 * \sa
 */ 
#ifdef TNETW18xx
typedef enum
{
    /*0*/  SOFT_GEMINI_PARAM_0 = 0,
    /*1*/  SOFT_GEMINI_PARAM_1,
    /*2*/  SOFT_GEMINI_PARAM_2,
    /*3*/  SOFT_GEMINI_PARAM_3,

    /*4*/  SOFT_GEMINI_PARAM_4,
    /*5*/  SOFT_GEMINI_PARAM_5,
    /*6*/  SOFT_GEMINI_PARAM_6,
    /*7*/  SOFT_GEMINI_PARAM_7,

    /*8*/  SOFT_GEMINI_PARAM_8,
    /*9*/  SOFT_GEMINI_PARAM_9,
    /*10*/ SOFT_GEMINI_PARAM_10,
    /*11*/ SOFT_GEMINI_PARAM_11,

    /*12*/ SOFT_GEMINI_PARAM_12,
    /*13*/ SOFT_GEMINI_PARAM_13,
    /*14*/ SOFT_GEMINI_PARAM_14,
    /*15*/ SOFT_GEMINI_PARAM_15,

    /*16*/ SOFT_GEMINI_PARAM_16,
    /*17*/ SOFT_GEMINI_PARAM_17,
    /*18*/ SOFT_GEMINI_PARAM_18,
    /*19*/ SOFT_GEMINI_PARAM_19,

    /*20*/ SOFT_GEMINI_PARAM_20,
    /*21*/ SOFT_GEMINI_PARAM_21,

    /*22*/ SOFT_GEMINI_PARAM_22,
    /*23*/ SOFT_GEMINI_PARAM_23,
    /*24*/ SOFT_GEMINI_PARAM_24,
    /*25*/ SOFT_GEMINI_PARAM_25,

    /**************** Start of Active Scan Parameters *******************/
    /*26*/ SOFT_GEMINI_AUTO_SCAN_PROBE_REQ,
    /*27*/ SOFT_GEMINI_ACTIVE_SCAN_DURATION_FACTOR_HV3,

    /*28*/ SOFT_GEMINI_PARAM_28,
    /**************** End of Active Scan Parameters *******************/

    /**************** Start of Passive Scan Parameters *******************/

    /*29*/ SOFT_GEMINI_PARAM_29,
    /*30*/ SOFT_GEMINI_PARAM_30,
    /*31*/ SOFT_GEMINI_PASSIVE_SCAN_DURATION_FACTOR_HV3,
    /**************** End of Passive Scan Parameters *******************/

    /**************** Start of Passive Scan in Dual Antenna Parameters *******************/
    /*32*/ SOFT_GEMINI_CONSECUTIVE_HV3_IN_PASSIVE_SCAN,
    /*33*/ SOFT_GEMINI_BEACON_HV3_COLLISION_THRESHOLD_IN_PASSIVE_SCAN,
    /*34*/ SOFT_TX_RX_PROTECTION_BANDWITH_IN_PASSIVE_SCAN,
    /**************** End of Passive Scan in Dual Antenna Parameters *******************/

    /**************** Start of General Parameters *******************/
    /*35*/ SOFT_GEMINI_STA_FORCE_PS_IN_BT_SCO,
    /*36*/ SOFT_GEMINI_ANTENNA_CONFIGURATION,
    /*37*/ SOFT_GEMINI_BEACON_MISS_PERCENT,
    /*38*/ SOFT_GEMINI_PARAM_38,
    /*39*/ SOFT_GEMINI_RXT,
    /*40*/ SOFT_GEMINI_UNUSED, //Redundant
    /*41*/ SOFT_GEMINI_ADAPTIVE_RXT_TXT,
    /*42*/ SOFT_GEMINI_GENERAL_USAGE_BIT_MAP,
    /*43*/ SOFT_GEMINI_HV3_MAX_SERVED,
    /*44*/ SOFT_GEMINI_PARAM_44,
    /*45*/ SOFT_GEMINI_PARAM_45,
    /*46*/ SOFT_GEMINI_CONSECUTIVE_CTS_THRESHOLD,
    /*47*/ SOFT_GEMINI_PARAM_47,
    /*48*/ SOFT_GEMINI_STA_CONNECTION_PROTECTION_TIME,
    /**************** End of General Parameters *******************/

    /**************** Start of AP Parameters *******************/
    /* 49 */   SOFT_GEMINI_AP_BEACON_MISS_TX,
    /*50*/ SOFT_GEMINI_PARAM_50,
    /*51*/ SOFT_GEMINI_AP_BEACON_WINDOW_INTERVAL_DURING_PAGE_INQUIRY,
    /*52*/ SOFT_GEMINI_AP_CONNECTION_PROTECTION_TIME,
    /*53*/ SOFT_GEMINI_PARAM_53,
    /*54*/ SOFT_GEMINI_PARAM_54,
/**************** End of AP Parameters *******************/

    /**************** Start of CTS Diluting Parameters *******************/
    /*55*/ SOFT_GEMINI_CTS_DILUTED_BAD_RX_PACKETS_TH,
    /*56*/ SOFT_GEMINI_CTS_CHOP_IN_DUAL_ANT_SCO_MASTER,
    /**************** End of CTS Diluting Parameters *******************/

    /*57*/ SOFT_GEMINI_TEMP_PARAM_1,
    /*58*/ SOFT_GEMINI_TEMP_PARAM_2,
    /*59*/ SOFT_GEMINI_TEMP_PARAM_3,
    /*60*/ SOFT_GEMINI_TEMP_PARAM_4,
    /*61*/ SOFT_GEMINI_TEMP_PARAM_5,
    /*62*/ SOFT_GEMINI_TEMP_PARAM_6,
    /*63*/ SOFT_GEMINI_TEMP_PARAM_7,
    /*64*/ SOFT_GEMINI_TEMP_PARAM_8,
    /*65*/ SOFT_GEMINI_TEMP_PARAM_9,
    /*66*/ SOFT_GEMINI_TEMP_PARAM_10,

    /*67*/ SOFT_GEMINI_PARAMS_MAX
} softGeminiParams;

#else
typedef enum
{   
    /**************** Start of DPMS Parameters *******************/
    /*0*/  SOFT_GEMINI_BT_MASTER_BR_WLAN_ACTIVE_PS_SCAN_BT_MIN = 0,                    
    /*1*/  SOFT_GEMINI_BT_MASTER_BR_WLAN_ACTIVE_PS_SCAN_BT_MAX,           
    /*2*/  SOFT_GEMINI_BT_SLAVE_BR_WLAN_ACTIVE_PS_SCAN_BT_MIN,                         
    /*3*/  SOFT_GEMINI_BT_SLAVE_BR_WLAN_ACTIVE_PS_SCAN_BT_MAX,           
    
    /*4*/  SOFT_GEMINI_BT_MASTER_EDR_WLAN_ACTIVE_PS_SCAN_BT_MIN,      
    /*5*/  SOFT_GEMINI_BT_MASTER_EDR_WLAN_ACTIVE_PS_SCAN_BT_MAX,
    /*6*/  SOFT_GEMINI_BT_SLAVE_EDR_WLAN_ACTIVE_PS_SCAN_BT_MIN,
    /*7*/  SOFT_GEMINI_BT_SLAVE_EDR_WLAN_ACTIVE_PS_SCAN_BT_MAX,
    
    /*8*/  SOFT_GEMINI_BT_MASTER_BR_WL_PS_WL_SERV_TIME,
    /*9*/  SOFT_GEMINI_BT_SLAVE_BR_WL_PS_WL_SERV_TIME,
    /*10*/ SOFT_GEMINI_BT_MASTER_EDR_WL_PS_WL_SERV_TIME,                                  
    /*11*/ SOFT_GEMINI_BT_SLAVE_EDR_WL_PS_WL_SERV_TIME,                           
    
    /*12*/ SOFT_GEMINI_BT_MASTER_BR_WL_ACTIVE_WL_MIN,                           
    /*13*/ SOFT_GEMINI_BT_MASTER_BR_WL_ACTIVE_WL_MAX,                              
    /*14*/ SOFT_GEMINI_BT_SLAVE_BR_WL_ACTIVE_WL_MIN,                             
    /*15*/ SOFT_GEMINI_BT_SLAVE_BR_WL_ACTIVE_WL_MAX,  
    
    /*16*/ SOFT_GEMINI_BT_MASTER_EDR_WL_ACTIVE_WL_MIN,                           
    /*17*/ SOFT_GEMINI_BT_MASTER_EDR_WL_ACTIVE_WL_MAX,                            
    /*18*/ SOFT_GEMINI_BT_SLAVE_EDR_WL_ACTIVE_WL_MIN,                            
    /*19*/ SOFT_GEMINI_BT_SLAVE_EDR_WL_ACTIVE_WL_MAX,                           
    
    /*20*/ SOFT_GEMINI_BT_BR_WLAN_ACTIVE_SCAN_WLAN_SERVE_TIME,                          
    /*21*/ SOFT_GEMINI_BT_EDR_WLAN_ACTIVE_SCAN_WLAN_SERVE_TIME,                            
    
    /*22*/ SOFT_GEMINI_BT_BR_WLAN_PASSIVE_SCAN_BT_SERVE_TIME,                          
    /*23*/ SOFT_GEMINI_BT_BR_WLAN_PASSIVE_SCAN_WLAN_SERVE_TIME,
    /*24*/ SOFT_GEMINI_BT_EDR_WLAN_PASSIVE_SCAN_BT_SERVE_TIME,
    /*25*/ SOFT_GEMINI_BT_EDR_WLAN_PASSIVE_SCAN_WLAN_SERVE_TIME,
    /**************** End of DPMS Parameters *******************/
    
    /**************** Start of Active Scan Parameters *******************/
    /*26*/ SOFT_GEMINI_AUTO_SCAN_PROBE_REQ,
    /*27*/ SOFT_GEMINI_ACTIVE_SCAN_DURATION_FACTOR_HV3,
    /*28*/ SOFT_GEMINI_ACTIVE_SCAN_DURATION_FACTOR_A2DP,                   
    /**************** End of Active Scan Parameters *******************/
    
    /**************** Start of Passive Scan Parameters *******************/
    /*29*/ SOFT_GEMINI_PASSIVE_SCAN_DURATION_FACTOR_A2DP_BR,                  
    /*30*/ SOFT_GEMINI_PASSIVE_SCAN_DURATION_FACTOR_A2DP_EDR,                     
    /*31*/ SOFT_GEMINI_PASSIVE_SCAN_DURATION_FACTOR_HV3,                    
    /**************** End of Passive Scan Parameters *******************/
    
    /**************** Start of Passive Scan in Dual Antenna Parameters *******************/
    /*32*/ SOFT_GEMINI_CONSECUTIVE_HV3_IN_PASSIVE_SCAN,                  
    /*33*/ SOFT_GEMINI_BEACON_HV3_COLLISION_THRESHOLD_IN_PASSIVE_SCAN,                  
    /*34*/ SOFT_TX_RX_PROTECTION_BANDWITH_IN_PASSIVE_SCAN,                    
    /**************** End of Passive Scan in Dual Antenna Parameters *******************/
    
    /**************** Start of General Parameters *******************/
    /*35*/ SOFT_GEMINI_STA_FORCE_PS_IN_BT_SCO,                   
    /*36*/ SOFT_GEMINI_ANTENNA_CONFIGURATION,            
    /*37*/ SOFT_GEMINI_BEACON_MISS_PERCENT,
    /*38*/ SOFT_GEMINI_DHCP_TIME,            
    /*39*/ SOFT_GEMINI_RXT,                          
    /*40*/ SOFT_GEMINI_TXT,                        
    /*41*/ SOFT_GEMINI_ADAPTIVE_RXT_TXT, 
    /*42*/ SOFT_GEMINI_GENERAL_USAGE_BIT_MAP,
    /*43*/ SOFT_GEMINI_HV3_MAX_SERVED,
    /*44*/ SOFT_GEMINI_PS_POLL_TIMEOUT,
    /*45*/ SOFT_GEMINI_UPSD_TIMEOUT,
    /*46*/ SOFT_GEMINI_CONSECUTIVE_CTS_THRESHOLD,
    /*47*/ SOFT_GEMINI_STA_RX_WINDOW_AFTER_DTIM_DURING_PAGE_INQUIRY,
    /*48*/ SOFT_GEMINI_STA_CONNECTION_PROTECTION_TIME,
    /**************** End of General Parameters *******************/
    
    /**************** Start of AP Parameters *******************/
    /* 49 */   SOFT_GEMINI_AP_BEACON_MISS_TX,
    /*50*/ SOFT_GEMINI_AP_RX_WINDOW_AFTER_BEACON_DURING_PAGE_INQUIRY,
    /*51*/ SOFT_GEMINI_AP_BEACON_WINDOW_INTERVAL_DURING_PAGE_INQUIRY,
    /*52*/ SOFT_GEMINI_AP_CONNECTION_PROTECTION_TIME,
    /*53*/ SOFT_GEMINI_AP_BT_ACL_VAL_BT_SERVE_TIME,                 
    /*54*/ SOFT_GEMINI_AP_BT_ACL_VAL_WL_SERVE_TIME,  
/**************** End of AP Parameters *******************/

	/**************** Start of CTS Diluting Parameters *******************/
    /*55*/ SOFT_GEMINI_CTS_DILUTED_BAD_RX_PACKETS_TH,                 
    /*56*/ SOFT_GEMINI_CTS_CHOP_IN_DUAL_ANT_SCO_MASTER,  	
	/**************** End of CTS Diluting Parameters *******************/

    /*57*/ SOFT_GEMINI_TEMP_PARAM_1,                 
    /*58*/ SOFT_GEMINI_TEMP_PARAM_2,  
    /*59*/ SOFT_GEMINI_TEMP_PARAM_3,             
    /*60*/ SOFT_GEMINI_TEMP_PARAM_4,                         
    /*61*/ SOFT_GEMINI_TEMP_PARAM_5,                       
    /*62*/ SOFT_GEMINI_TEMP_PARAM_6,                       
    /*63*/ SOFT_GEMINI_TEMP_PARAM_7,
    /*64*/ SOFT_GEMINI_TEMP_PARAM_8,
    /*65*/ SOFT_GEMINI_TEMP_PARAM_9,
    /*66*/ SOFT_GEMINI_TEMP_PARAM_10,

    /*67*/ SOFT_GEMINI_PARAMS_MAX
} softGeminiParams;
#endif
typedef struct
{   
  uint32   coexParams[SOFT_GEMINI_PARAMS_MAX];
  uint8    paramIdx;       /* the param index which the FW should update, if it equals to 0xFF - update all */ 
  uint8    padding[3];
} TSoftGeminiParams;


/******************************************************************************

    Name:   ACX_SG_CFG
    Desc:   This command instructs the WiLink to set the Soft Gemini (BT co-existence) 
            parameters to the desired values. 
    Type:   Configuration
    Access: Write (Read For GWSI - disable for now)
    Length: 1
    
******************************************************************************/
typedef struct

{
    INFO_ELE_HDR
    
    TSoftGeminiParams softGeminiParams;
} ACXBluetoothWlanCoParamsStruct;
  
/******************************************************************************

    Name:   ACX_FM_COEX_CFG
    Desc:   This command instructs the WiLink to set the FM co-existence
            parameters to the desired values. 
    Type:   Configuration
    Access: Write
    Length: 
    
******************************************************************************/
typedef struct

{
    INFO_ELE_HDR
    
    uint8   enable;                     /* enable(1) / disable(0) the FM Coex feature */

    uint8   swallowPeriod;              /* Swallow period used in COEX PLL swallowing mechanism,
                                           Range: 0-0xFF,  0xFF = use FW default
                                        */

    uint8   nDividerFrefSet1;           /* The N divider used in COEX PLL swallowing mechanism for Fref of 38.4/19.2 Mhz.  
                                           Range: 0-0xFF,  0xFF = use FW default
                                        */

    uint8   nDividerFrefSet2;           /* The N divider used in COEX PLL swallowing mechanism for Fref of 26/52 Mhz.
                                           Range: 0-0xFF,  0xFF = use FW default
                                        */

    uint16  mDividerFrefSet1;           /* The M divider used in COEX PLL swallowing mechanism for Fref of 38.4/19.2 Mhz.
                                           Range: 0-0x1FF,  0xFFFF = use FW default
                                        */

    uint16  mDividerFrefSet2;           /* The M divider used in COEX PLL swallowing mechanism for Fref of 26/52 Mhz.
                                           Range: 0-0x1FF,  0xFFFF = use FW default
                                        */

    uint32  coexPllStabilizationTime;   /* The time duration in uSec required for COEX PLL to stabilize.
                                           0xFFFFFFFF = use FW default
                                        */

    uint16  ldoStabilizationTime;       /* The time duration in uSec required for LDO to stabilize.
                                           0xFFFFFFFF = use FW default
                                        */

    uint8   fmDisturbedBandMargin;      /* The disturbed frequency band margin around the disturbed 
                                             frequency center (single sided). 
                                           For example, if 2 is configured, the following channels 
                                             will be considered disturbed channel: 
                                             80 +- 0.1 MHz, 91 +- 0.1 MHz, 98 +- 0.1 MHz, 102 +- 0.1 MHz
                                           0xFF = use FW default
                                        */

    uint8   swallowClkDif;              /* The swallow clock difference of the swallowing mechanism.
                                           0xFF = use FW default
                                        */

} ACXWlanFmCoexStruct;
  


/******************************************************************************

    Name:   ACX_TSF_INFO
    Type:   Operation
    Access: Read Only
    Length: 20

******************************************************************************/
typedef struct ACX_fwTSFInformation
{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION
    uint8  paddind1[3];  /* alignment to 32bits boundry   */   
    uint32 CurrentTSFHigh;
    uint32 CurrentTSFLow;
    uint32 lastTBTTHigh;
    uint32 lastTBTTLow;
    uint8 LastDTIMCount;
    uint8  padding2[3];  /* alignment to 32bits boundry   */
}ACX_fwTSFInformation_t;

 
/******************************************************************************

Name:   ACX_BET_ENABLE
Desc:   Enable or Disable the Beacon Early Termination module. In addition initialized the
        Max Dropped beacons parameter
Type:   Configuration
Access: Write 
Length: 6
Note:  
******************************************************************************/
typedef struct

{
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION         /* host RoleID */
    uint8           Enable;                 /* specifies if beacon early termination procedure is enabled or disabled: 0 - disabled, 1 - enabled */
    uint8           MaximumConsecutiveET;   /* specifies the maximum number of consecutive beacons that may be early terminated. After this number is reached 
                                                at least one full beacon must be correctly received in FW before beacon ET resumes.  Legal range: 0 - 255 */
    uint8           padding[1];
}ACXBet_Enable_t;


/******************************************************************************

    Name:   DOT11_RX_MSDU_LIFE_TIME
    Type:   Operation
    Access: Write Only
    Length: 4
    
******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    uint32 RxMsduLifeTime; /* The maximum amount of time, in TU, that the WiLink */
                           /* should attempt to collect fragments of an MSDU before */
                           /* discarding them. */
                           /* The default value for this field is 512.*/
} dot11RxMsduLifeTime_t;


/******************************************************************************

    Name:   DOT11_CUR_TX_PWR
    Desc:   This IE indicates the maximum TxPower in Dbm/10 currently being used to transmit data.
    Type:   Operation
    Access: Write Only
    Length: 1
    
******************************************************************************/

typedef struct
{ 
    INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION /* host RoleID */
    uint8 dot11CurrentTxPower;      /* the max Power in Dbm/10 to be used to transmit data.*/
    uint8  padding[2];              /* alignment to 32bits boundry   */
} dot11CurrentTxPowerStruct ;


/******************************************************************************

    Name:   DOT11_RX_DOT11_MODE
    Desc:   This IE indicates the current Rx Mode used by DSSS PHY.
    Type:   Configuration
    Access: Write Only
    Length: 4
    
******************************************************************************/
/*
Possible values for Rx DOT11 Mode are the following:
Value   Description
=====   ===========
3       11g - processing of both a and b packet formats is enabled
2       11b - processing of b packet format is enabled
1       11a - processing of a packet format is enabled
0       undefined
*/

typedef struct
{
    INFO_ELE_HDR
    uint32 dot11RxDot11Mode; /* refer to above table*/
} dot11RxDot11ModeStruct;


/******************************************************************************

    Name:   DOT11_RTS_THRESHOLD 
    Type:   Configuration
    Access: Write Only
    Length: 2

******************************************************************************/

typedef struct 
{
                                    /* Offset Size */
    INFO_ELE_HDR                    /*      0    4 */
    CMD_PARAMS_HOST_ROLE_DEFINITION /*      4    1 - host RoleID */
    uint8  reserved;                /*      5    1 - alignment to 32bits boundary   */
    uint16  RTSThreshold;           /*      6    2 - The number of octets in an MPDU, below which an */
                                                /* RTS/CTS handshake is not performed.*/
}dot11RTSThreshold_t;


/******************************************************************************

   ACX_CONFIG_PS_WMM (Patch for Wi-Fi Bug)

******************************************************************************/

typedef struct 
{    
    INFO_ELE_HDR
    uint32      ConfigPsOnWmmMode;  /* TRUE  - Configure PS to work on WMM mode - do not send the NULL/PS_POLL 
                                               packets even if TIM is set.
                                       FALSE - Configure PS to work on Non-WMM mode - work according to the 
                                               standard. */
} ACXConfigPsWmm_t;

/******************************************************************************

      
    Name:   ACX_SET_RX_DATA_FILTER
    Desc:   This IE configure one filter in the data filter module. can be used 
            for add / remove / modify filter.
    Type:   Filtering Configuration
    Access: Write Only
    Length: 4 + size of the fields of the filter (can vary between filters)

******************************************************************************/
/* data filter action */

#ifdef HOST_COMPILE

#define FILTER_DROP  0          /* Packet will be dropped by the FW and wont be delivered to the driver. */
#define FILTER_SIGNAL  1        /* Packet will be delivered to the driver. */
#define FILTER_FW_HANDLE  2     /* Packet will be handled by the FW and wont be delivered to the driver. */

#else

typedef enum {
    FILTER_DROP = 0,
    FILTER_NOT_DROP,
    FILTER_FW_HANDLE, 
    FILTER_MAX  = 0xFF
}filter_enum;

#endif

#ifdef HOST_COMPILE
typedef uint8 filter_e;
#else
typedef filter_enum filter_e;
#endif

/* data filter command */
#define REMOVE_FILTER   0       /* Remove filter */
#define ADD_FILTER      1       /* Add filter */

/* limitation */
#ifdef TNETW18xx
#define MAX_DATA_FILTERS 5
#else
#define MAX_DATA_FILTERS 7
#endif
#define MAX_DATA_FILTER_SIZE 98

typedef struct 
{
    INFO_ELE_HDR
    uint8                command;   /* 0-remove, 1-add */
    uint8                index;     /* range 0-MAX_DATA_FILTERS */
    filter_e             action;    /* action: FILTER_DROP, FILTER_SIGNAL, FILTER_FW_HANDLE */  
    uint8                numOfFields; /* number of field in specific filter */
    uint8                FPTable;   /* filter fields starts here. variable size. */
} DataFilterConfig_t;

/******************************************************************************
      
    Name:   ACX_ENABLE_RX_DATA_FILTER
    Desc:   This IE disable / enable the data filtering feature. in case the
            featue is enabled - default action should be set as well.
    Type:   Filtering Configuration
    Access: Write Only
    Length: 2

******************************************************************************/

typedef struct  
{
                                    /* Offset Size */
    INFO_ELE_HDR                    /*      0    4 */
    uint8       enable;             /*      4    1 - 1 - enable, 0 - disable the data data filtering feature */
    filter_e    action;             /*      5    1 - default action that should be implemented for packets that wont
                                                    match any of the filters, or in case no filter is configured */
    uint8   padding[2];             /*      6    2 - alignment to 32bits boundary   */        
} DataFilterDefault_t;


/******************************************************************************
      
    Name:   ACX_GET_DATA_FILTER_STATISTICS
    Desc:   get statistics of the data filtering module.
    Type:   Statistics
    Access: Read Only
    Length: 20

******************************************************************************/

typedef struct 
{
    INFO_ELE_HDR
    uint32  unmatchedPacketsCount;                  /* number of packets didn't match any filter (when the feature was enabled). */
    uint32  matchedPacketsCount[MAX_DATA_FILTERS];  /* number of packets matching each of the filters */
} ACXDataFilteringStatistics_t;


#ifdef RADIO_SCOPE
/******************************************************************************

******************************************************************************

    Name:   ACX_RS_ENABLE
    Desc:   This command instructs the WiLink to set the Radio Scope functionality
            state to either enable/disable. 
    Type:   Configuration
    Access: Write Only
    Length: 1
    
******************************************************************************/
typedef struct
{
    INFO_ELE_HDR
    uint8   Enable; /* RadioScope feature will be enabled (1) or disabled(0) */
    uint8  padding[3];  /* alignment to 32 bits  */
} ACXRadioScopeEnableStruct;

/******************************************************************************

    Name:   ACX_RS_RX
    Desc:   This command instructs the WiLink to set the Radio Scope 
            parameters to the desired values. 
    Type:   Configuration
    Access: Read/Write 
    Length: 1

    We have the following available memory area:
        
            Information Element ID -        2 bytes
            Information Element Length -    2 bytes         
            
                Now the rest is MAX_CMD_PARAMS
                but 4 bytes must be subtracted
                because of the IE in Buffer.
            
    
******************************************************************************/
typedef struct
{       
    uint16  service;
    uint16  length;
    uint8   channel;
    uint8   band;
    uint8   status;
    uint8   padding[1]; /*32 bit padding */
}RxPacketStruct;

typedef struct 
{       
    /*  We have the following available memory area:        */
    /*                                                      */
    /*  Information Element ID -        2 bytes             */
    /*  Information Element Length -    2 bytes             */
    /*  Number Of Packets in Buffer -    2 bytes            */
    /*                                                      */
    /*        Now the rest is MAX_CMD_PARAMS                */
    /*        but 2 bytes must be subtracted                */
    /*        because of the Number Of Packets in Buffer.   */
    RxPacketStruct packet[(MAX_CMD_PARAMS-2)/sizeof(RxPacketStruct)];
}RxCyclicBufferStruct;

typedef struct

{
    INFO_ELE_HDR
    /*uint8   padding[MAX_CMD_PARAMS-4]; */
    RxCyclicBufferStruct buf;
} ACXRadioScopeRxParamsStruct;

#endif /* RADIO_SCOPE */
/******************************************************************************
    Name:   ACX_KEEP_ALIVE_MODE
    Desc:   Set/Get the Keep Alive feature mode.
    Type:   Configuration
    Access: Write
    Length: 4 - 1 for the mode + 3 for padding.

******************************************************************************/

typedef struct
{
INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION        /* host RoleID */
    Bool_e  modeEnabled;
    uint8 padding [2];
}AcxKeepAliveMode;


/******************************************************************************

    Name:   ACX_SET_KEEP_ALIVE_CONFIG
    Desc:   Configure a KLV template parameters.
    Type:   Configuration
    Access: Write Only
    Length: 8

******************************************************************************/

typedef enum
{
    NO_TX = 0,
    PERIOD_ONLY
} KeepAliveTrigger_enum;

#ifdef HOST_COMPILE
typedef uint8 KeepAliveTrigger_e;
#else
typedef KeepAliveTrigger_enum KeepAliveTrigger_e;
#endif

typedef enum
{
    KLV_TEMPLATE_INVALID = 0,
    KLV_TEMPLATE_VALID,
} KeepAliveTemplateValidation_enum;

#ifdef HOST_COMPILE
typedef uint8 KeepAliveTemplateValidation_e;
#else
typedef KeepAliveTemplateValidation_enum KeepAliveTemplateValidation_e;
#endif

typedef struct
{
                                            /* Offset Size */
    INFO_ELE_HDR                            /*      0    4 */
    CMD_PARAMS_HOST_ROLE_DEFINITION         /*      4    1 - host RoleID */
    uint8 index;                            /*      5    1 */
    KeepAliveTemplateValidation_e   valid;  /*      6    1 */
    KeepAliveTrigger_e  trigger;            /*      7    1 */
    uint32 period;                          /*      8    4 - at range 1000-3600000 (msec). (To allow better range for debugging)*/
} AcxSetKeepAliveConfig_t;

/*
 * BA session interface structure
 */

typedef struct
{
    INFO_ELE_HDR
    uint8 hLid;                     /* Specifies Link Id, Range 0-31*/
    uint8 Tid;                     /* TID */
    uint8 state;                    /* SET/CLEAR */
    uint8 WinSize;                /* windows size in num of packet */
    uint16 SSN;                    /* BA session starting sequence number.  RANGE 0-FFF */
    uint8 Padding[2];
    
} TAxcBASessionRxSetupConfigure;



typedef struct
{
    INFO_ELE_HDR
    uint8 roleId;                   /* Specifies role Id, Range 0-7, FF means ANY role    */
    uint8 PolicyBitmap;                  /* per TID bitmap either BA is allowed or not allowed */
    uint8 WinSize;                 /* windows size of the BA session */
    uint8 padding_1[1];
    uint16 InactivityTimeout;       /* as initiator inactivity timeout in time units(TU) of 1024us /
                                       as receiver reserved */
    uint8 padding_2[2];
} TAxcBaSessionInitiatorPolicy;



/******************************************************************************

    Name:   ACX_PEER_HT_CAP
    Desc:   Configure HT capabilities - declare the capabilities of the peer
            we are connected to.
    Type:   Configuration
    Access: Write Only
    Length:

******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    uint32 uHtCapabilites;      /*
                                 * bit 16  -  Allow HT Operation.
                                 * bit 0   -  Indicates support for receiving LDPC coded packets.
                                 * bit 1   -  Set to 0 if only 20 MHz operation is supported.
                                              Set to 1 if both 20 MHz and 40 MHz operation is supported.
                                 * bit 2|3 -  Indicates the spatial multiplexing power save mode.
                                              Set to 0 for static SM power save mode.
                                              Set to 1 for dynamic SM power save mode.
                                              Set to 3 for SM Power Save disabled.
                                 * bit 4   -  Indicates support for the reception of PPDUs with HT-greenfield format.
                                 * bit 5   -  Indicates short GI support for the reception of packets with BW of 20Mhz.
                                 * bit 6   -  Indicates short GI support for the reception of packets with BW of 40Mhz.
                                 * bit 7   -  Indicates support for the transmission of PPDUs using STBC.
                                 * bit 8|9 -  Indicates support for the reception of PPDUs using STBC
                                              Set to 0 for no support.
                                              Set to 1 for support of one spatial stream.
                                              Set to 2 for support of one and two spatial streams.
                                              Set to 3 for support of one, two and three spatial streams.
                                 * bit 10  -  Indicates support for HTdelayed Block Ack operation.
                                 * bit 11  -  Indicates maximum AMSDU length.
                                              Set to 0 for 3839 octets
                                              Set to 1 for 7935 octets
                                 * bit 12  -  Indicates use of DSSS/CCK mode in a 20/40 MHz BSS.
                                              In Beacon and Probe Response frames:
                                                Set to 0 if the BSS does not allow use of DSSS/CCK in 40 MHz.
                                                Set to 1 if the BSS does allow use of DSSS/CCK in 40 MHz.
                                              Otherwise:
                                                Set to 0 if the STA does not use DSSS/CCK in 40 MHz.
                                                Set to 1 if the STA uses DSSS/CCK in 40 MHz.
                                 * bit 13  - Indicates support for PSMP mechanism.
                                 * bit 14  - Indicates whether APs receiving this information or reports of this
                                             information are required to prohibit 40 MHz transmissions.
                                 * bit 15  - Indicates support for the LSIG TXOP protection mechanism.
                                 */


     uint8  hlid;               /*
                                 * Indicates to which peer these capabilities are relevant.
                                 * Note, currently this value will be set to FFFFFFFFFFFF to indicate it is
                                 * relevant for all peers since we only support HT in infrastructure mode.
                                 * Later on this field will be relevant to IBSS/DLS operation
                                 */

     uint8  uAmpduMaxLength;    /*
                                 * This the maximum a-mpdu length supported by the AP. The FW may not
                                 * exceed this length when sending A-MPDUs
                                 */

     uint8  uAmpduMinSpacing;   /* This is the minimal spacing required when sending A-MPDUs to the AP. */

} TAxcHtCapabilitiesIeFwInterface;

/******************************************************************************

    Name:   ACX_PEER_CAP
    Desc:   Configure HT capabilities - declare the capabilities of the peer
            we are connected to.
    Type:   Configuration
    Access: Write Only
    Length:

******************************************************************************/



/******************************************************************************
 FwStaticData_t - information stored in command mailbox area after the Init 
                  process is complete

 Note:  This structure is passed to the host via the mailbox at Init-Complete
        without host request!!
        The host reads this structure before sending any configuration to the FW.
******************************************************************************/
#define SIZE_OF_PHY_VERSION_STRING 20
typedef struct
{
    /* dot11StationIDStruct */
    uint8 dot11StationID[6]; /* The MAC address for the STA.*/
    uint8 padding[2];       /* alignment to 32bits boundry   */
    /* ACXRevision_t */
    char FWVersion[20];     /* The WiLink firmware version, an ASCII string x.x.x.x.x */
                            /* that uniquely identifies the current firmware. */
                            /* The left most digit is incremented each time a */
                            /* significant change is made to the firmware, such as */
                            /* WLAN new project.*/
                            /* The second and third digit is incremented when major enhancements*/
                            /* are added or major fixes are made.*/
                            /* The fourth digit is incremented for each SP release */
                            /* and it indicants the costumer private branch */
                            /* The fifth digit is incremented for each build.*/
        
    uint32 HardWareVersion; /* This 4 byte field specifies the WiLink hardware version. */
                            /* bits 0  - 15: Reserved.*/
                            /* bits 16 - 23: Version ID - The WiLink version ID  */
                            /*              (1 = first spin, 2 = second spin, and so on).*/
                            /* bits 24 - 31: Chip ID - The WiLink chip ID. */
      //  uint8 txPowerTable[NUMBER_OF_SUB_BANDS_E][NUM_OF_POWER_LEVEL]; /* Maximun Dbm in Dbm/10 units */
        char FwPhyVersion[SIZE_OF_PHY_VERSION_STRING];
} FwStaticData_t;

/******************************************************************************



    ACX_TX_CONFIG_OPT

 

******************************************************************************/

typedef struct 
{
    INFO_ELE_HDR
    uint16  txCompleteTimeout;   /* Max time in msec the FW may delay frame Tx-Complete interrupt */
    uint16  txCompleteThreshold; /* Tx-Complete packets number which requires interrupting the host (0 = no buffering) */
} ACXTxConfigOptions_t;


/******************************************************************************

Name:   ACX_PWR_CONSUMPTION_STATISTICS
Desc:   Retrieve time statistics of the different power states.
Type:   Configuration
Access: Read Only
Length: 20 

******************************************************************************/

// Power Statistics
typedef struct
{
    INFO_ELE_HDR
    uint32 awakeTimeCnt_Low;
    uint32 awakeTimeCnt_Hi;
    uint32 powerDownTimeCnt_Low;
    uint32 powerDownTimeCnt_Hi;
    uint32 elpTimeCnt_Low;
    uint32 elpTimeCnt_Hi;
    uint32 ListenMode11BTimeCnt_Low;
    uint32 ListenMode11BTimeCnt_Hi;
    uint32 ListenModeOFDMTimeCnt_Low;
    uint32 ListenModeOFDMTimeCnt_Hi;
}ACXPowerConsumptionTimeStat_t;


/******************************************************************************
    Name:   ACX_BURST_MODE
    Desc:   enable/disable burst mode in case TxOp limit != 0.
    Type:   Configuration
    Access:    Write
    Length: 1 - 2 for the mode + 3 for padding.

******************************************************************************/

typedef struct
{
INFO_ELE_HDR
    CMD_PARAMS_HOST_ROLE_DEFINITION        /* host RoleID */
    Bool_e  enable;
    uint8 padding [2];
}AcxBurstMode;


/******************************************************************************
    Name:   ACX_SET_RATE_MAMAGEMENT_PARAMS
    Desc:   configure one of the configurable parameters in rate management module.
    Type:   Configuration
    Access:    Write
    Length: 8 bytes

******************************************************************************/
typedef enum
{
    RATE_MGMT_RETRY_SCORE_PARAM,
    RATE_MGMT_PER_ADD_PARAM,
    RATE_MGMT_PER_TH1_PARAM,
    RATE_MGMT_PER_TH2_PARAM,
    RATE_MGMT_MAX_PER_PARAM,
    RATE_MGMT_INVERSE_CURIOSITY_FACTOR_PARAM,
    RATE_MGMT_TX_FAIL_LOW_TH_PARAM,
    RATE_MGMT_TX_FAIL_HIGH_TH_PARAM,
    RATE_MGMT_PER_ALPHA_SHIFT_PARAM,
    RATE_MGMT_PER_ADD_SHIFT_PARAM,
    RATE_MGMT_PER_BETA1_SHIFT_PARAM,
    RATE_MGMT_PER_BETA2_SHIFT_PARAM,
    RATE_MGMT_RATE_CHECK_UP_PARAM,
    RATE_MGMT_RATE_CHECK_DOWN_PARAM,
    RATE_MGMT_RATE_RETRY_POLICY_PARAM,
    RATE_MGMT_PER_WEIGHT_SHIFT_PARAM,
    RATE_MGMT_TP_WEIGHT_SHIFT_PARAM,
    RATE_MGMT_ALL_PARAMS = 0xff
} rateAdaptParam_enum;

#ifdef HOST_COMPILE
typedef uint8 rateAdaptParam_e;
#else
typedef rateAdaptParam_enum rateAdaptParam_e;
#endif

#define RATE_MGMT_NUM_OF_UC     (2)
#define RATE_MGMT_NUM_OF_RATES  (13)
#ifdef TNETW18xx

/* NEW STRUCTURE */
typedef struct
{
    INFO_ELE_HDR
    rateAdaptParam_e paramIndex;
    uint16 PerAdd;
    uint16 PerTh1;
    uint16 PerTh2;
    uint16 MaxPer;
    uint8 InverseCuriosityFactor;
    uint8 PerAlphaShift;
    uint8 PerAddShift;
    uint8 RateCheckUp;
    uint8 RateCheckDown;
    uint8 PerWeightShift; 
    uint8 TpWeightShift;
	uint8 PerDecayFactor;
	uint8 PerAlphaShiftTxLevel;
	uint32 thermalSensorEnabledRates;
}AcxRateAdaptParams;

#else
typedef struct
{
    INFO_ELE_HDR
    rateAdaptParam_e paramIndex;
    uint16 RateRetryScore;
    uint16 PerAdd;
    uint16 PerTh1;
    uint16 PerTh2;
    uint16 MaxPer;
    uint8 InverseCuriosityFactor;
    uint8 TxFailLowTh;

    uint8 TxFailHighTh;
    uint8 PerAlphaShift;
    uint8 PerAddShift;
    uint8 PerBeta1Shift;
    uint8 PerBeta2Shift;
    uint8 RateCheckUp;
    uint8 RateCheckDown;
    uint8 RateRetryPolicy[RATE_MGMT_NUM_OF_RATES]; 
}AcxRateMangeParams;


/* NEW STRUCTURE */
typedef struct
{
    INFO_ELE_HDR
    rateAdaptParam_e paramIndex;
    uint16 RateRetryScore;
    uint16 PerAdd;
    uint16 PerTh1;
    uint16 PerTh2;
    uint16 MaxPer;
    uint8 InverseCuriosityFactor[RATE_MGMT_NUM_OF_UC];
    uint8 TxFailLowTh;
    uint8 TxFailHighTh;
    uint8 PerAlphaShift;
    uint8 PerAddShift;
    uint8 PerBeta1Shift;
    uint8 PerBeta2Shift;
    uint8 RateCheckUp;
    uint8 RateCheckDown;
    uint8 RateRetryPolicy[RATE_MGMT_NUM_OF_RATES]; 
    uint8 PerWeightShift[RATE_MGMT_NUM_OF_UC];          // 2 weights, 1 per UC
    uint8 TpWeightShift[RATE_MGMT_NUM_OF_UC];               // 2 weights, 1 per UC
}AcxRateAdaptParams;

#endif
/******************************************************************************
    Name:   ACX_GET_RATE_MAMAGEMENT_PARAMS
    Desc:   read the configurable parameters of rate management module.
    Type:   
    Access: read
    Length: 8 bytes

******************************************************************************/
typedef struct
{
    INFO_ELE_HDR
    int32  SNRCorrectionHighLimit;
    int32  SNRCorrectionLowLimit;
    int32  PERErrorTH;
    int32  attemptEvaluateTH;
    int32  goodAttemptTH;
    int32  curveCorrectionStep;
}AcxRateMangeReadParams;


/******************************************************************************

    Name:   ACX_SET_DCO_ITRIM_PARAMS    
    Desc:   Configure DCO Itrim operational parameters:               
            1. Enable/disable of the entire feature.                                     
            2. Moderation timeout (usec) - how much time to wait from last TX
            until DCO Itrim can be set low.
    Type:   Configuration
    Access: Write Only
    Length: 

******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    Bool_e enable;
    uint32 moderation_timeout_usec;
}ACXDCOItrimParams_t ;


/******************************************************************************

    Name:   ACX_UPDATE_INCONNECTION_STA_LIST    
    Desc:   Configure In Connection STA List.
            Add/Remove STA from In Connection List according to mac address.
    Type:   Configuration
    Access: Write Only
    Length: 

******************************************************************************/

typedef struct  
{
    INFO_ELE_HDR                    
    uint8   mac_address[6];         /* STA mac address  */
    CMD_PARAMS_HOST_ROLE_DEFINITION /* host RoleID */
    uint8   padding[1];
} ACXInConnectionSTA_t;

///******************************************************************************
//
//    Name:   ACX_DISABLE_BROADCASTS
//    Type:   Configuration
//    Access: Write Only
//    Length:
//
//******************************************************************************/
//typedef enum
//{
//     RECEIVE_ALL_BROADCAST_WHILE_IN_SUSPEND = 0,
//     FILTER_OUT_BROADCAST_IN_FW_LEVEL       = BIT_0,
//     FILTER_OUT_BROADCAST_IN_HW_LEVEL       = BIT_1,
//     FILTER_OUT_BROADCAST_MAX_SIZE          = MAX_POSITIVE8
//}BroadcastFilterInSuspend_enum;
//
//#ifdef HOST_COMPILE
//typedef uint8 BroadcastFilterInSuspend_e;
//#else
//typedef BroadcastFilterInSuspend_enum BroadcastFilterInSuspend_e;
//#endif
//
//
//typedef struct
//{
//    INFO_ELE_HDR
//
//    BroadcastFilterInSuspend_e broadcastDisable;  // defines whether to enable Broadcasts and multicast RX
//	    				                          // when host is in suspend
//    uint8   padding[3];
//
//} ACXDisableBroadcasts_t;


/******************************************************************************
    Name:   ACX_CONFIG_PS    
    Desc:   Configure PS operational parameters:               
            numOfEnterPsRetries        -  Number of retries attempts when enter to PS.
            numOfExitPsRetries         -  Number of retries attempts when exit from PS.
            RequestedRateForNullDataTx - Rates requested for sending the Null Data packet.
    Type:   Configuration
    Access: Write Only
    Length: 

******************************************************************************/

typedef struct
{
    INFO_ELE_HDR
    uint8   numOfExitPsRetries;
    uint8   numOfEnterPsRetries;
    uint8   temp1;
    uint8   temp2;
    uint32  RequestedRateForNullDataTx;
}ACXConfigPsParams_t ;

/******************************************************************************

    Name:   dot11ERPProtectionMode    
    Desc:   Configure PS operational parameters:               
            numOfEnterPsRetries        -  Number of retries attempts when enter to PS.
            numOfExitPsRetries         -  Number of retries attempts when exit from PS.
            RequestedRateForNullDataTx - Rates requested for sending the Null Data packet.
    Type:   Configuration
    Access: Write Only
    Length: 

******************************************************************************/
#define PROTECTION_MODE_DISABLE     (0)
#define PROTECTION_MODE_FW          (1)
#define PROTECTION_MODE_DRIVER      (2)


typedef struct
{                                   /* Offset Size */
    INFO_ELE_HDR                    /*      0    4 */
    CMD_PARAMS_HOST_ROLE_DEFINITION /*      4    1 - host RoleID */
    uint8    ProtectionMode;        /*      5    1 - operation mode for beacon erp filtering */
    uint8    reserved[2];           /*      6    2 - alignment to 32bits boundary   */
}dot11ProtectionModeParams_t;

/******************************************************************************

    Name:   ACX_CONFIG_HANGOVER    
    Desc:   Configure Hangover operational parameters:               
            
            bDynamicHangoverMode-          Enable/Disable dynamic hangover algorithm.
            maxHangOverPeriod-             max time that hangover can be.
            minHangOverPeriod-             min time that hangover can be.
                                           hangover is decreased to the next time: max hangover inside sliding
                                           window + this parameter
            increaseDelatTimeFromHangover- if the time stamp of the last RX packet is bigger 
                                           than (hangover - IncreaseDelatTimeFromHangover) means
                                           the hangover is increased. 
            decreaseDelatTimeFromHangover- if the time stamp of the last RX packet is smaller than 
                                           (hangover - DecreaseDelatTimeFromHangover) during all sliding window
                                           means the hangover is decreased. 
            recoverTime-                   how often to return to the original hangover.If time is zero means
                                           recover hangover feature is disabled
            bEarlyTerminationMode-         Enable/Disable early termination in dynamic hangover algorithm.
            quietTimeForEarlyTermination-  Duration that should be "quite" in the air in order to do early termination.
            IncreaseHangoverTime-          hangover is increased by this time
            SlidingWindowSize-             the size of sliding window the decision is based on sliding window
            hangOverPeriod-                original hangover - hangover time if bDynamicHangoverMode is disable,
                                           hangover every restart of the algorithm(every join command)
                                           hangover every recovery time
                                           

            NOTE -PAY ATTENTION  
            all the times and periods are in units of half MSEC.
            1  = 0.5 MSEC
            2  = 1   MSEC
            3  = 1.5 MSEC
                    
            for example:
            if IncreaseDelatTimeFromHangover = 1 (value of host command) that means that the hangover will be
            increase with 0.5 MSEC each time.
            Because the module parameters are with USEC units 
            there is a multiply by 500 to each host parameters which describes time
            so in the the module IncreaseDelatTimeFromHangover will be 500 (500 USEC means 0.5 MSEC
            means 1 in half MSEC units).

  
    Type:   Configuration
    Access: Write Only
    Length: 

******************************************************************************/
typedef struct
{
  INFO_ELE_HDR
  uint32 recoverTime;
  uint8  hangOverPeriod;
  Bool_e bDynamicHangoverMode;
  Bool_e bEarlyTerminationMode;
  uint8  maxHangOverPeriod;
  uint8  minHangOverPeriod;
  uint8  increaseDelatTimeFromHangover;
  uint8  decreaseDelatTimeFromHangover;
  uint8  quietTimeForEarlyTermination;
  uint8  increaseHangoverTime;
  uint8  slidingWindowSize;
  uint8  spare1;
  uint8  spare2;

}ACXConfigHangOverParams_t ;

#ifdef TNETW18xx

/******************************************************************************

    Name:	ACXsimulationConfigKick_e
    Desc: Type of kicks for Tx/Rx Simulation

******************************************************************************/

typedef enum
{
    TX_SIM_KICK_NONE   = 0x00, 
    TX_SIM_KICK_RX     = 0x01, 
    TX_SIM_KICK_TX     = 0x02
} ACXsimulationConfigKick_e;

typedef enum
{
    GI_LONG   = 0x00, 
    GI_SHORT  = 0x01
} ACXsimulationConfigGuardInterval_e;

typedef enum
{
    BW_20MHZ   = 0x00, 
    BW_40MHZ   = 0x01
} ACXsimulationConfigBW4TX_e;

typedef enum
{
    SIM_RATE_1MBPS   =  0,
    SIM_RATE_2MBPS   =  1,
    SIM_RATE_5_5MBPS =  2,
	SIM_RATE_11MBPS  =  3,
    SIM_RATE_6MBPS   =  4,
    SIM_RATE_9MBPS   =  5,    
    SIM_RATE_12MBPS  =  6,
    SIM_RATE_18MBPS  =  7,
    SIM_RATE_24MBPS  =  8,
    SIM_RATE_36MBPS  =  9,
    SIM_RATE_48MBPS  =  10,
    SIM_RATE_54MBPS  =  11,
	SIM_RATE_MCS0	 =  12,//6.5Mbps
	SIM_RATE_MCS1	 =  13,//13Mbps
	SIM_RATE_MCS2	 =  14,//19.5Mbps
	SIM_RATE_MCS3    =  15,//26Mbps
	SIM_RATE_MCS4    =  16,//39Mbps
	SIM_RATE_MCS5    =  17,//52Mbps
	SIM_RATE_MCS6    =  18,//58.5Mbps
	SIM_RATE_MCS7    =  19 //65Mbps
} ACXsimulationConfigRate_e;

/******************************************************************************

    Name:	ACX_SIM_CONFIG    
    Desc:   Configure parameters for simulation.
    Type:   Configuration
    Access: Write Only
    Length: 

******************************************************************************/

typedef struct ACXsimulationConfig_t{

    INFO_ELE_HDR

    uint32	numOfPackets; // 0 - means continue running
    uint32	packetSize;

    uint8	sourceAddr[6];
    uint8	destAddr[6];
    uint8	bssidAddr[6];

    uint32	expiryValue; // 0 means no expiry;
    
	uint8					            rate            ; //RateIndex_e
    ACXsimulationConfigKick_e           kick            ; // ACXsimulationConfigKick_e
    ACXsimulationConfigBW4TX_e          bandwidth4TX    ; // 27-JUN-2011 Boaz: selection of rate bandwidth after a 40MHz channel tune, 
                                                          //                   still only 20MHz of the bandwidth may be used upon selection   
    ACXsimulationConfigGuardInterval_e  guardInterval   ; // 0 = long gi (default) , 1 = short gi.
    uint8_t/*BandWidthSettings_e */     bandwidthTune   ; // 27-JUN-2011 Boaz: 40MHz bandwidth channel tune SIMULATION
    uint32	                            aggregationSize ;
    uint32	                            density         ; // See MPDUDensity_e for values.
    uint32	                            channel         ;
    uint32	                            tid             ;
    uint32	                            content         ; // 0 - regular, 1 - iperf
	uint32                              Delay           ; // Delay between packets, usec
    uint8                               shortGI         ; // 40MHz bandwidth - 0 = long gi (default) , 1 = short gi.
    uint8_t/*RadioBand_e*/              BandTune        ; // See RadioBand_e for values.

    uint8                               RxFilterEnabled ;
    uint8                               RxFilterSrcAddr[6] ;
    uint8                               RxFilterDestAddr[6] ;
} ACXsimulationConfig_t;

#ifndef HOST_COMPILE
extern uint32 dataFlowOptions;
inline uint32 GetDataFlowOptions()
{
	return dataFlowOptions;
}
#endif


//////////////////////////////////////////////////////////////////////////////////
//                  BA Activity Suspend/Resume Enhancements                      //
//////////////////////////////////////////////////////////////////////////////////

/******************************************************************************

    Name:   ACX_FAST_LINK_NOTIFY
    Desc:   Configure Fast Link notification in case of suspend/resume
    Type:   Configuration
    Access: Write Only
    Length:

******************************************************************************/
typedef struct
{
    INFO_ELE_HDR
    uint32 enable;
}ACXFastLinkNotify_t ;

/******************************************************************************

    Name:   ACX_RX_BA_FILTER
    Desc:   Configure RX BA Activities Filter that can arrive from the other device
    Type:   Configuration
    Access: Write Only
    Length:

******************************************************************************/
typedef struct
{
    INFO_ELE_HDR
    uint32 enable;
}ACXRxBAActivity_t ;



#endif
#endif /* PUBLIC_INFOELE_H */

