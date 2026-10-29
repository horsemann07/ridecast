/*******************************************************************************
   FILENAME:       versions.h

   DESCRIPTION:

   Copyright (c) 2002 Texas Instruments Inc. All Rights Reserved This
   program is the confidential and proprietary product of Texas
   Instruments Inc.  Any Unauthorized use, reproduction or transfer of
   this program is strictly prohibited.  
*******************************************************************************/

#ifndef VERSIONS_H
#define VERSIONS_H


// All max_len of versions are defined in host_shared.h file. If you change the name make sure it doesn't exceeding the max_len
//Main Software revision
#define SW_MAJOR_VERSION        1
#define SW_MINOR_VERSION        7
#define SW_API_VERSION          0
#define SW_BUILD_VERSION        331



//Components revision
#define version_wifi_lower_mac  "wifi_lmac_rev_1.0.0.287"
#define version_wsoc            "wsoc_rev_1.0.0.13"
#define version_bootloader_rom  "bootman_rom_rev_1.0.0.31"
#define version_bootloader_ram  "bootman_ram_rev_1.0.0.30"

#define version_wphy            "wphy_rev_x.x.x.47.6.13.38.69"

#define version_ble             "ble_rev_1.0.0.49"

#define version_upper_mac       "upper_mac_rev_2.0.0.11"

#endif /*VERSIONS_H*/


/******************************************************************************
* Notes for Revision number:
* =========================
  Version numbering
  The following convention will be used to set the version number:

  qq        ww          zz       yy
   |         |           |        |
   |         |           |        |
   |         |           |        |----- Build numbering
   |         |           |-------------- Compatibility version
   |         |-------------------------- Minor version
   |------------------------------------ Major version


*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
********************



***********************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
GuyM:             1.7.0.331 - 14/11/2025
                  - OSPREY_LDB-3148: [TM][BLE] Add BLE controller init done indication to host

Michal:           1.7.0.330 - 5/10/2025
                  - Align to RTM release patch (1.7.6.323):
                  - Fix incorrect value in the power limit array for channel 64 in cc3351mod-conf_FCC.ini
                  - Update INI for FCC LDB MOB
                  - OSPREY_LDB-3100 - fix back. the product type indicates both LDB (vs. LSB) and TI module.
                  - Update PHY version x.x.x.47.6.13.38.69
                  - OSPREY_LDB-3115 - New INI files and PHY for TI module support


BasharB:          1.7.0.329 - 29/09/2025
                  - OSPREY_TM-128: No data with Arris AP on 5GHz
                  - OSPREY_TM-121: TCP and UDP (when connecting to NTP) is not stable [Arris NVG468Q]

GuyM:             1.7.0.328 - 14/09/2025
                  - Update BLE submodules to 1.0.0.49 (Bug fixes)

BasharB:          1.7.0.327 - 19/08/2025
                  - Implement TKIP for ThickMac


Dana:             1.7.0.326 - 19/08/2025
                  - Remove disable of BLE for container type programming 

BasharB:          1.7.0.325 - 6/8/2025
                  - Align Thickmac Changes:
                  Includes:
                            - OmniVision Crash After Scan [Pointer incremnt by 0x1248 bytes instead of 0x10]
                            - OSPREY_TM-103: [Thick MAC] Cannot connect to Arris AP in 5GHz
                            - AMSDU parsing implemented for ThickMac
                            - OSPREY_TM-91:[ThickMac] wifi FW packet drop (iperf3)

                            - OSPREY_TM-101: [Thick MAC] second connect to tplink Archer C50 crash the FW:
                              block ack DELBA wrong initiator in Block ack RX (DELBA_PARAMETER_RECIPIENT should be sent for Block ack RX)
                              block ack RX assert, NULL param sent to BA_RX_StateMachine and we attempt to gtrace a field of the param causing to an assert
                              ReAssociation Request is sent as re association response because we don’t assign the packet time when getting the packet ready for retransmission

                            - OSPREY_TM-101: [Thick MAC] second connect to tplink Archer C50 crash the FW
                            - OSPREY_TM-100: [Thick MAC] cannot get scan list after tplink Archer C50 disconnect the station on inactivity

GuyM:             1.7.0.324 - 4/8/2025
                  - Do not reset WDT when disabled to avoid triggering it
                  - Update BLE submodules to 1.0.0.48

MichaelN:         1.7.0.323 - 31/07/2025
                  - OSPREY_LDB-3042: Drop frames with incorrect key ID in AES/DMA

GuyM:             1.7.0.322 - 24/07/2025
                  - Update BLE Submodule to ble_rev_1.0.0.47 (re-fix OSPREY_LDB-2430)

MichaelN:         1.7.0.321 - 24/07/2025
                  -   OSPREY_LDB-3024: FW: Remove implicit configuration of beacon filtering params
                  -   OSPREY_LDB-1708: FW: Remove code for legacy wlconf, wl_logproxy
                  -   Lx Driver: Increase post-irq delay on boot to 50 mSec
                  -   FW: Implement efficient general buffer dump in FW
                  -   FW: Align VScode tasks with new CMake flags
                  -   FW: Remove VScode problem matchers as they often stall the client on build errors

Dana:             1.7.0.320 - 16/07/2025
                  - [RAM Bootloader] version:
                  - OSPREY_LDB-3013: Programming WIFI 6 Disabled Fuse makes the host load unprogrammed OTP MAC address fuse
                    instead of the TI Fuse
                  - Update fuse write parameters (in-field)
                  - Disable random delays and HW CRC by bypassing fuse row 34 value

BasharB:          1.7.0.319 - 13/07/2025
                  -  OSPREY_LDB-2974:DHCP fail when AP configure to 11b 11bg
                  -  conf.key set to 0x0000000 by default

BasharB:          1.7.0.318 - 09/07/2025
                  -  New PHY version 1.2.0.47.6.13.37.69
                  
Michal:           1.7.0.317 - 08/07/2025
                  -  OSPREY_LDB-3006 - ap aging - avoid updating the last tx timestamp upon probe response (fix is in hte scripter)
                  -  OSPREY_LDB-3004 - fix timestamp field in probe response frame
                  -  OSPREY_LDB-3007 - p2p rates fix

BasharB:           1.7.0.316 - 06/07/2025
                  - TCXO ELP Boot Issue Fix
                  - new PHY_1.2.0.47.6.12.37.69
                  - new Ini PHY Param (Product_type default to 0)  

Michal:           1.7.0.315 - 26/6/2025
                  - OSPREY_LDB-2917 avoid using AX ER PPDU format due to iop issues
                  - OSPREY_LSB-3001 add sgi support for 11n connection

Katya:            1.7.0.314 - 24/06/2025
                  -OSPREY_LDB-2887: Disable WiFi6 in FW, according to eFuse bit Value
                  
Dana:             1.7.0.313 - 23/06/2025
                  - OSPREY_LDB-2970: calibration collision causing FW assert in sched state 
                    upon "calibration start" fix.
                  - [RAM Bootloader] Update BLE random seed generation method

BasharB:          1.7.0.312 - 29/05/2025
                  -  OSPREY_LDB-2820: [LX] Thermal protection - False temperature in print
                  -  Bad GPADC monitor Handling State removed (timeout state)


Dana:             1.7.0.311 - 29/05/2025
                  - New PHY version PHY_1.2.0.45.6.0.37.69

BasharB:          1.7.0.310 - 28/05/2025
                  - OSPREY_LDB-2820: [LX] Thermal protection - False temperature in print

Dana:             1.7.0.309 - 28/05/2025
                  - Disabling OCP access to the EFUSE ROM banks before boot done in BL when container type isn't programming

Dana:             1.7.0.308 - 27/05/2025
                  - OSPREY_LDB-2968: eFuse - All rows are locked on fresh device
                  - Noam: OSPREY_LDB-2926: Tune command fail in PHY causes a FW assert

GalM:             1.7.0.307 - 27/05/2025
                  - OSPREY_LDB-2960: Certification - WPA3 Ent. - SuiteB 19.2.1,2- STA - Assert during basic connection test.
                  
Dana:             1.7.0.306 - 25/05/2025
                  - OSPREY_LDB-2957: eFuse - Writing to vendor key eFuse fails
                  - OSPREY_LDB-2958: eFuse - Writing 1 to Vendor Key Usage locks the eFuse without writing
                  - OSPREY_LDB-2941: eFuse - Assert when attempting to read eFuse after writing
                  - OSPREY_LDB-2881: Customer OTP: Custom MAC always uses last nibble of "0" for base address
                  - OSPREY_LDB-2882: Customer OTP: "Worldwide" field should be renamed to "County Code Enable" for set/get
                  - OSPREY_LDB-2880: Customer OTP: Country code not included in userChecksum
                  - OSPREY_LDB-2879: Customer OTP: Conf key is readable in programming version
                  - Michal: OSPREY_LDB-2953 - fix power table in link adaptation module

BasharB:          1.7.0.305 - 25/05/2025
                  - OSPREY_LDB-2911: conf and m3m0 ini struct are not aligned
                  - Update rflib submodule to 1.00.00.46_blerev

Noam:             1.7.0.304 - 25/05/2025
                  - OSPREY_LDB-2933: LX - FW assert in stability TCP bidir after 9 hours (sched state post calibration assert)
                  - Update BLE Submodule to ble_rev_1.0.0.46 (fixing OSPREY_LDB-2947 and OSPREY_LDB-2948)
                  - New PHY FW versions (fixing OSPREY_LDB-2938):
                    Lx1 PG2 Silicon PHY_1.2.0.45.6.0.36.69
                    Mx1 PG1 Silicon (for XTAL 52MHz) PHY_2.1.0.45.6.0.5.71
                    Mx1 PG1 Silicon (for XTAL 40MHz) PHY_2.1.0.45.6.0.5.71
                    Mx1 PG2 FPGA PHY_2.102.0.45.6.0.5.71
                    Mx1 PG1 FPGA PHY_2.101.0.45.6.0.5.71

Noam:             1.7.0.303 - 20/05/2025
                  - OSPREY_LDB-2893: Connect / Disconnect in a loop with channel switch - causes FW assert
                  - OSPREY_LDB-2895: FW Crash when using wlan1 (AP) interface
                  - OSPREY_LDB-2904: AP Aging is not supported
                  - OSPREY_LDB-2913: Channel switch on AP role - FW assert
                  - Shay: Disable Thermal protection for 85c device

BasharB:          1.7.0.302 - 18/05/2025
                  - Update PHY version to x.x_0.45.6.0.34.69
                  - Add TI MODULE CONF INI files

Shaked,Amir Asher: 1.7.0.301 - 15/05/2025
                  - Update uppermac submodule

BasharB:          1.7.0.300 - 14/05/2025
                  - OSPREY_LDB-2909: FW assert -  SCHED_PlanGenerator_AllocTimeForRequest() - ***ERROR: req 0x80e9 got invalid serving time 30000 with latest schedule point 0x00000000
                  - OSPREY_LDB-2900: FW assert - in Timers_mng.c - ASSERT ERROR aDeltaTsf <= (((0xFFFFFFFF))/2)


Dana:             1.7.0.299 - 12/05/2025
                  - Bug fix - Change worldwide country code (default) to 0x3030
                  - Country code added to user checksum, calculation over ini file.
                  - New toolbox version.
                  - Fuse programing is enabled for container type "programmer". 
                  - Fuse programming ang reading is enabled for the following additional fuses:
                    Wifi 6 disable, rollback protection enable, vendor key and vendor key usage.
                  - New PHY: 1.2.0.45.6.0.32.69 (toward Lx 105C RTM release)

GuyM:              1.7.0.298 - 09/05/2025
                  - [MX][BLE][HIF] Increase BLE HCI total packet length

MichaelN:          1.7.0.297 - 27/04/2025
                  - ** Actually ** enable dynamic trace for MX PG2

BasharB:           1.7.0.296 - 24/04/2025
                  - OSPREY_LDB-2819 - set keys while RX fragmentation.
                  - OSPREY_LDB-2874: 4-way handshake failure with D-Link DIR-X1560. Increase mgmt sequence sched priority.
                  - OSPREY_LDB-2875: increase mgmt pool size by 4 memblocks.
                  - Update MX pool allocation configurations
                  - [RT]: Reg Tx Limit Set Based on Minumum between INI File and Selected Value
                  - Adding Hooks to some Link adapation functions (hooks missing)
                  
                  
BasharB:           1.7.0.295 - 23/04/2025
                  - Splitting conf.h to conf_cc33xx.h and conf_cc35xx including dictionary and default conf 
                  - MX-Only related params removed LX Conf (conf_cc33xx.h) 
                  - enable fw_defrag in MX (set in mx conf file)
                  - [OSPREY_MX-734] GPADC measuring is always disabled in MX_PG2 (upper_mac_3.0.1.12)
                  - linker file: changing the size of DATA_RAM_NO_NINT and DATA_RAM_INIT_ROM
                  - Break hook dispatcher (ptc_bypass_engine) into 4 sub-functions (MichaelN)

Michal:           1.7.0.294 - 21/04/2025
                  - OSPREY_LDB-2857 - remove the WD assert in PLT mode (Bashar)
                  - Stuck overflow mechanism fix (Michael)
                  - [BLE] Move important defines to globals (Guy)
                  - CSI - prepare code for extended data length

Katya:            1.7.0.293 - 20/04/2025
                  - OSPREY_MX-684: Assert at the end of the test 10158. 
                  - Decrease WSOC_ROM for MX PG2 to 0x6B7FF [DanH]

AviramD:          1.7.0.292 - 17/04/2025
                  - Added MX1 PG2 hooks
                  - Removed LX FPGA build configuration

BasharB:          1.7.0.291 - 16/04/2025
                  - Fix corrupt RFCIO Sensor measurement after PMAC HW transition to RF_OFF
                  - New Lx PHY_1.2.0.44.6.0.27.69

BasharB:          1.7.0.290 - 15/04/2025
                  - Reduce GPADC Interface Driver Code by ~2KB
                  - Fix gpadc issues in release 1.7.0.287 and refactor the code
                  - Fix conf file file corruption
                  - Add Hooks to system_info.c
                  - Remove Dead Vbat Code (GPADC)
                  - Fix Ini File Complete Signaling (Katya)
                  
Amir Asher:       1.7.0.289 - 15/04/2025
                  - moving ptc_bypass from stub to orig address
                  - HOOK planting
                  - Remove HOOKS from inline

Katya:            1.7.0.288 - 14/04/2025
                  - OSPREY_MX-719: Add first command
                  - OSPREY_LDB-2849: Initiating Scan during BSS Transition causes FW stuck - Assert in Scheduler
                  - Increase BLE Stack to 1024 [GuyM]
                  - THICK_MAC: Fixing hook redefinition in upper mac [DanH]

ShayS:            1.7.0.287 - 10/04/2025
                  - GPADC: add HOOKS, change defines to globals, add the option to measure in Mx
                  - Thermal Protection BLE placeholders
                  - OSPREY_LDB-2808: EAPOL identify in FW fix
                  - Xtal correction in conf
                  - New PHY x.x.0.44.6.0.26.69
                  - Added PA_LDO_IN_MONITORto shared ROM [EladZ]


DanH:             1.7.0.286 - 09/04/2025
                  - Move event mailbox to be allocated from heap and not globaly. increase the heap acordingly (3k for LX, 2k for thick MAC).
                  - Stop using "union EventMBoxData_u" to allow each caller to consume only its own required message size.
                  - [HOOKS] Add hooks

GuyM:             1.7.0.285 - 07/04/2025
                  - [TIMERS] Do not print BLE_TIMER_EVENT_ID to reduce excessive prints
                  - [SDIO] Reduce SDIO traces to reduce packet handling time
                  - [BLE][HIF] Reduce SDIO NPI traces to reduce packet handling time
                  - Katya: move defines to globals
                  - Noam: OSPREY_LDB-2855: Fix assert in indigo p2p test 10137 (GO Start mgmt sequence was not stopped properly)
                  - MichalA:
                      - OSPREY_LDB-2849: Initiating Scan during BSS Transition causes FW stuck - Assert in Scheduler
                      - Increasing scheduler array to 20 requests
                      - Clear scheduler handle in traffic module, when scheduler is up, so traffic module will generate a new request.

GuyM:             1.7.0.284 - 07/04/2025
                  - Update BLE submodules to 1.0.0.44 (Add hooks in each function for osprey MX ROM)
                  - [HOOKS] Add hooks to BLE submodules

BasharB:          1.7.0.283 - 06/04/2025
                  - PRCM values Update For MX (Enter/Exit Sleep Sequence)

Shay, Michael:
                  1.7.0.282 - 02/04/2025
                   - Thermal Protection limit high rates
                   - New Phy x.x.25.69 (CSI)
                   - Transciever half dB with HOOKS

Dan, Aviram, Guy, Elad:
                   1.7.0.281 - 31/03/2025
                    - Adding interrogate - GET_SP_VERSIONS_INTR
                    - Add fuse structure read for Mx PG1
                    - [NIMBLE] Increase BLE_MAX_CONNECTIONS from 8 to 16
                    - change the test_sleep command to closing the UART instead
                      of releasing the power constraint of UART. Then, exit
                      LPDS by reopenning the UART. This is the recommended
                      way by Nikolaj.
                    - Hooks

Katya, AsherA:     1.7.0.280 - 30/03/2025
                    - Move defines to globals
                    - Hooks

Michal:            1.7.0.279 - 27/3/2025
                    - Add FCC cahnnels support to link adaptation module
                    - New Phy version  x.x.0.43.6.1.24.69
                    - [BLE][SDIO] Discard packet in case SDIO header was not identified @GuyM

AviramD :          1.7.0.278 - 26/3/2025
                    - Remove APL_CALIB code.
                    - Remove redundant LX PG1 specific code.
                    - OSPREY_MX-651: Remove All WSOC FW Code Muxing GPIOs Select.
                    - OSPREY_MX-650: Verify OSPREY_MX/OSPREY_LX "#ifdef" Pre-processor Conditions in WSOC FW.
                    - MichaelN: Add dynamic trace infrastructure.
                    - MichaelN: Split Gtrace groups into two headers, one for RAM and one for ROM.
                    - MichaelN: Make ASU, MR, RPU, LTXP and LRXP use the dynamic trace infrastructure.
                    - MichaelN: Generate version string via preprocessor.
                    - Katya: Open ldo and settling ini params for Mx.
                    - Noam: OSPREY_LDB-2852: Channel switch on role AP causes an assert, instead of ignoring the channel switch request.
                    - Guy: OSPREY_LDB-2806: FW stuck observed during SDIO packet reception interrupt.

MichaelN:          1.7.0.277 - 26/3/2025
                    - OSPREY_LDB-2803: Implement FW sleep stats

Michal:            1.7.0.276 - 24/3/2025
                    - CSI feature - Demo level

Dana:              1.7.0.275 - 24/3/2025
                    - Disable ECO for fuse program & read.
                    - Fuse programing is disabled. Fuse reading is enabled for the following fuses:
                      MAC address, crc conf, worldwide restriction and country code.
                    - OSPREY_LDB-1943 : Read MAC address from fuse bits.
	                  - OSPREY_LDB-1944 : Customer eFuse programing MAC address, Conf_key, worldwide restriction and country code.
	                  - Noam - OSPREY_LDB-2834 - Config dynamic RTS/CTS on/off via conf file

AviramD:            1.7.0.274 - 23/3/2025
                    - OSPREY_MX-647: Integrate 52MHz External Clock PHY FW Version to MX PG1.
                    - New PHY versions x.x.0.43.6.0.23.69
                    - Initialize BLE Regulatory Power Limit Register

eladso:             1.7.0.273 - 19/3/2025
                    - OSPREY_LDB-2838: [RADIO_TOOL][BLE] Add support in regulatory TX power limit for BLE
                    - Update BLE submodules to 1.0.0.43 (Remove static globals and functions definition)
                    - Remove global static variables and static functions

BasharB:            1.7.0.272 - 19/3/2025
                    - PLT Support for 6GHZ
                    - PLT new Set TX params regulatory tx power limit added [WIFI+BLE]

ShayS:              1.7.0.271 - 18/3/2025
                    - Thermal Protection

AviramD:            1.7.0.270 - 18/3/2025
                    - OSPREY_MX-641: [INI] Support FW Build Versions Larger Than 255 (overflow).
                    - OSPREY_MX-642: [INI] Support SP Versions Validation in Conf Binary.
                    - OSPREY_MX-643: [INI] Support Extending INI Conf Binary When WSOC is in ROM.
                    - OSPREY_MX-576: MX-PG2 - Coex modification before WSOC FW ROM Closer.
                    - OSPREY_MX-589: Coex and other external configuration modification towards WSOC ROM.
                    - DanH: Adding creation of hex file, and hex file for wsoc_rom.

MichaelN:           1.7.0.269 - 17/3/2025
                    - Modernize host interface to use stdint.h types
                    - Align host interface headers with upper-mac host
                    - Automatically derive cc33xx_SoftRevision from number macros

GuyM, Amir Asher:   1.7.0.268 - 17/3/2025
                    - Add HOOKS to BLE source files in Osprey repo
                    - Add HOOKS to WLAN modules

Katya:              1.7.0.267 - 16/3/2025
                    - OSPREY_MX-557 - add support for LPDS in MX PG1
                    - OSPREY_MX-592 - move core status address to HIF shared memory. Currently hardcoded

DanH, EladZ, Shaked:1.7.0.266 - 12/3/2025
                    - Update MX PG2 shared RAM (to be initialized by 1st patch)
                    - New PG2 PHY_1.2.0.42.6.0.20.69 (BLE_-20dBm__PhyVerExtended)
                    - PG2: update header file to FPGA 9103
                    - Update linker file of PG2
                    - Fixed AP security recognition for thickmac
                    - Configure dwell time by the INI file for thickmac

GuyM:               1.7.0.265 - 12/3/2025
                    - OSPREY_LDB-2828: [BLE] Add negative TX power levels to TX table
                    - New BLE version ble_rev_1.0.0.42 (Negative TX power levels)
                    - Update BLE CPE Radio versions (0xF210 / 0xE107 / 0xE207)
                    - New PHY_1.2.0.42.6.0.20.69 (BLE_-20dBm__PhyVerExtended)
                    - OSPREY_LDB-2810: Assign correct OSC_GAIN and CLK_LDO values
                    - Extend PHY version length to 8 bytes

Michal:             1.7.0.264 - 12/3/2025
                    - Update link adaptation module for rom preparation.
                    - Update client based modules for rom (MR sched, tx scheduler, timers mng).
                    - Remove clearing the shared memory for MX PG1

DanH:               1.7.0.263 - 12/3/2025
                    - Adding MX PG2 support - more info can be found in First_Patch_version.c
                    - Adding defines in order to separate code between MX PG1 and PG2

GuyM:               1.7.0.262 - 6/3/2025
                    - New BLE version ble_rev_1.0.0.41 (Add debug logs + Bug fixes)

BasharB:            1.7.0.261 - 6/3/2025
                    - Post-Mortem FW Trace Retrival Added

Michal:             1.7.0.260 - 3/3/2025
                    - Link adaptation update for MX

GuyM:               1.7.0.259 - 3/3/2025
                    - OSPREY_LDB-2761: [BLE][MX] Set initial seed from host

Amir Asher          1.7.0.258 - 2/3/2025
                    - OSPREY_LDB-2740 - Fix bug in defragmentation
                    - Thickmac - Update uppermac submodule
                    - Thickmac - Enable regulatory domain
                    - Thickmac - Enable using DFS cahnnels
                    - Thickmac - Add custom regulatory domain for thickmac
                    - Thickmac - Change ZeroInitLength

GuyM:               1.7.0.257 - 2/3/2025
                    - OSPREY_LDB-2811: [BLE] Unify the BLE Host Interface for LX and MX
                    - New BLE version ble_rev_1.0.0.40 (Move NPI implementation for Osprey)

BasharB             1.7.0.256 - 25/2/2025
                    - New PHY_1.2.41.5.14.69 for LX PG2
                    - New PHY_2.1.41.5.14.69 for MX PG1 (XTAL 40)
                    - New PHY_2.101.41.5.14.69 for MX FPGA PG1

BasharB             1.7.0.255 - 24/2/2025
                    - Dynamic AGC Configuation Added
                    - New PHY_1.2.41.5.97.69__Lx_Dynamic_AGC__m3-m0-api_AGC-struct
                    - OSPREY_LDB-2779: [LX] association optimization didn't apply at version 1.7.0.253

GuyM:               1.7.0.254 - 24/2/2025
                    - OSPREY_LDB-2361: [BLE] Add option to configure the BLE default TX power level via INI file
                    - New BLE version ble_rev_1.0.0.39 (Update default BLE TX Power)

ShayS:              1.7.210.253  27/2/2025
                    Thermal Protection solution
BasharB:            1.7.0.253 - 16/2/2025
                    - Merge Fixes From HP To LX
                    - New PHY_1.2.40.5.10.69 
                    Dana:
                    - Michal: OSPREY_LDB-2757 - fix for HP Assert in Printer - RX_BB_LAST_2_IDLE, field issue
                    - Victor: OSPREY_LDB-2759 - fix for HP PHY API Timeout on Printer, field issue
                    Michal:
                    - Using lower rates for probe request, auth & association request packets.
                    Katya:
                    - OSPREY_LDB-2688: The FW should also not assert in case we get frames from the host when the role is not enabled / started
                    - OSPREY_LDB-2708: HP - Channel Switch. FW Assert when sta role stop before switching channel
                    - OSPREY_LDB-2710: CTS duration WA -assert - priority bug
                    Aviram:
      		          - Disabling TX BA session establishment for all frames except QOS data frames which are not EAPOLs.
                    Bashar:
                    - LrxpFrags_checkSecAttack() NULL Link pointer Assert Fix
                    - Container Signing restored to makefile
                    - Set RX/TX Active Timout back to diffrent values (200ms TX, 200ms RX) for STA Role and 40 for AP role
                    - remove  unwanted best effort traffic requests (AP_ROLE)
                    - lastSchedReqWinStartTsf nested into trafficEntry_t struct (now its per traffic entry)
                    - update no RX/TX traffic period for ap role as well at  TrafficModule NotifyRoleSuspend
                    - notify NAV protection module on stop role for role device (OSPREY_LDB-2727)
                    - Set Diffrent Timeouts for STA vs NON STA roles in MR Scheduler
                    - Set Limit on needed overhead to remove PS constraint when  ASU STA role resume (max of 5000, anything over is enough
                      for exiting PS) 

GalM:               1.7.0.246 - 19/1/2025
                    - OSPREY_LDB-2634: SUT/APUT tests certification frag attack fail.
                    
GalM:               1.7.0.245 - 16/1/2025
                    - fix in fragmentation attack.
                    - OSPREY_LDB-2667(Michal): avoid accessing un-initialized role in the CTS duration WA.
                    - OSPREY_LDB-2680(Michael): Return error if Vendor IE is set when AP is not running.

BasharB:            1.7.0.244 - 16/1/2025     
                    - OSPREY_LDB-2616: [HP] 5.64.1 STAUT MU EDCA Parameter Set test.
                    - Align the AIFS counter when MU EDCA is used
                    - Set Ac Params (PMAC) at every TX_PARAMS_CFG (if STA is running)


Shir:               1.7.0.243 - 14/1/2025
                    - OSPREY_LDB-2650: Add a protection in case the host sends two start scan commands.
                    - Consider the size needed to run OCLA in the calculation of the total amount of memory blocks.
                    - OSPREY_LDB-2664: Try to remove the scan timer when scan module thread is not running.

GalM:               1.7.0.242 - 12/1/2025
                    - OSPREY_LDB-2634: SUT/APUT tests certification frag attack fail.
					          - OSPREY_LDB-2625(Katya): Do not enable coex if ble disabled.
                    - OSPREY_LDB-2579(Dana): ROC interferes with scan causing scan to timeout and FW assert. Fix includes changing scheduler priority for ROC on device role.
					
BasharB:               1.7.0.241 - 6/1/2025
                    - OSPREY_LDB-2616: [HP] 5.64.1 STAUT MU EDCA Parameter Set test
                    - Pmac TX Scripter After TB Data Transmission Sets Wrong Timestamp required in order for SW to set expiry timer for MU-EDCA AC parameters,
                      Timestamp set is right after Trigger and not after TB Data Transmittion (should be after TB Data TX)
                    - Dana: 11AX_TB frequency error violation         

Shir:               1.7.0.240 - 6/1/2025
                    - OSPREY_LDB-2474: Continue with the start role flow if a periodic calibration request exists in the command queue.
                    - OSPREY_LDB-2645: Send a PERIODIC_SCAN_COMPLETE_EVENT_ID event after stop scan cmd appears without a preceding start scan command.

Shir:               1.7.0.239 - 30/12/2024
                    - OSPREY_LDB-2589: Recover from rx ppdu overflow.
                    - OSPREY_LDB-2284: fix bug in stop channel switch implementation (Dana & Amir Asher).
                    - OSPREY_LDB-2073: for thick mac - keep security header for management frames(Dana & Amir Asher).

GuyM:               1.7.0.238 - 29/12/2024
                    - OSPREY_LDB-2334: HP : BLE FW transmit low RSSI  as compare to the WI-FI
                    - Update BLE CPE Radio versions (0xF20F / 0xE106)

Dana:               1.7.0.237 - 18/12/2024
                    - OSPREY_LDB-2459: FW Assert During UDP Rx in TKIP (AP's with TKIP only mode)
                    fix: Add sleep for 10 ms (which is the smallest os_sleep duration and will cause context switch) during TKIP, 
                    when watchdog timer is almost reached.
                    - OSPREY_LDB-2432: Coex in 5ghz band, sometimes TP decrease to nearly 0 after BLE scan
                    New PHY versions:
                    - LDB PG2 Silicon                  PHY_1.2.40.5.23.68
                    - MDB PG1 Silicon                  PHY_2.1.40.5.23.68
                    - MDB PG1 FPGA                     PHY_2.101.40.5.23.68

Dan:                1.7.0.236 - 4/12/2024
                    - OSPREY_LDB-2598: [COEX]: BLE enable command during WiFi connection creates beacon loss and may cause disconnection

BasharB:            1.7.0.235 - 15/12/2024
                    New PHY versions:
                    - LDB PG2 Silicon                  PHY_1.2.40.5.22.68
                    - MDB PG1 Silicon (for XTAL 40MHz) PHY_2.1.40.5.22.68
                    - MDB PG1 FPGA                     PHY_2.101.40.5.22.68

Michal:             1.7.0.234 - 9/12/2024
                    - OSPREY_LDB-2481: RTS stuck at 6 OFDM cause Stop TX (low RSSI)
                    fix: after m5 failures in RTS we transmit the packet without rts, to allow notification to link adaptation.
                    - Call GPADC measurement upon tune command from calibrator
                    - Updating PHY versions LDB PG2 Silicon PHY_1.2.40.5.20.68

Dan:                1.7.0.233 - 4/12/2024
                    - OSPREY_LDB-2559: [LX] Increase Scan Dwell Time for non CoEX operation to 60ms

Shir:               1.7.0.232 - 3/12/2024
                    - Modify the FRAME_RX_DESC.xml file to match the one of pg2.1.

Shir:               1.7.0.231 - 28/11/2024
                    - OSPREY_LDB-2152: Do not wait for a callback from the RPU_TX_ENGINE if a suspend role timeout occurs.
                    - OSPREY_LDB-2524: PMAC error: RX_BB_FIFO_OVERFLOW. Receiving too long frame.
                    - Updating PHY versions LDB PG2 Silicon PHY_1.2.40.5.19.68

Noam:               1.7.0.230 - 26/11/2024
                    - OSPREY_LDB-2437: Fixes in set beacon info commands

Dan:                1.7.0.229 - 26/11/2024
                    - OSPREY_LDB-2430: [COEX] - Incomplete WiFi scan results while using BLE connection with interval lower than 7.5ms
                    - OSPREY_LDB-2482: [LX] - During scan, not all AP in the area are published in the scan results
                    - New BLE version ble_rev_1.0.0.38
                       - Reject extended indication fix
                       - Dense connection events indication
                       - Bug Fixes

Shir:               1.7.0.228 - 26/11/2024
                    - OSPREY_LDB-2484 & OSPREY_LDB-2345: Drop an aborted beacon (when the abortion is not part of beacon early termination).
                    - Increase an assigned PMAC memory size
                    - Cleanup debug messages (Bashar)

Dana:               1.7.0.227 - 25/11/2024
                    - OSPREY_LDB-2433: Remove ROC timer from FW by renewing request to sched once it is expired.
                    - Remove traffic_module.c trace from idle task which prints a warning when using device role.

BasharB:            1.7.0.226 - 21/11/2024
                    - Spamming of Broadcast RX enters MR Scheduler into a scheduling loop every 1ms, many Trace Dropped Fix
                    - Not enough memory for allocation Probe Responses with Vendor IE included workaround
                    - OSPREY_LDB-2493: WLAN high priority thread got stuck on 'MMU alloc failed' after getting AP_NEW_PARAMS message - Field issue
                    - OSPREY_LDB-2496: WLAN Low priority thread got stuck on 'MMU alloc failed' after getting Set vendor elements command - Field issue 
                    - OSPREY_LDB-2409: HP Printer hang - FW is stuck with no logs - Field issue
                    - OSPREY_LDB-2494: CMD_ERR_TIMEOUT after some hours - no commands send to the FW. Lots of trace drop errors - Field issue
                    - ILTOOLS-424:     [TB][INI COMPOSER] Remove 'Reserve' parameters from GUI


Noam:               1.7.0.225 - 19/11/2024
                    - OSPREY_LDB-2405: fix noise floor calculation.
                    - OSPREY_LDB-2375: modify the TX and RX pool configuration to enable using larger RX aggregations.
                    - New INI params: MaxRxAmpduLen, TxPsatCompensation_2_4GHz, TxPsatCompensation_5GHz, Reserved2.
                    - New PHY version PHY_1.2.40.5.16.68 with noise floor support and PSAT compensation.

Michal:               1.7.0.224 - 13/11/2024
                     - Add support for Dynamic RTS usage:
                         1. RTS protection will be used for every retransmission of data packet.
                         2. RTS will be used for data packet with sub-optimal rates (not mcs6-7, not OFDM 48-54).

Michal:               1.7.0.223 - 4/11/2024
                     - OSPREY_LDB-2441: Fix pmac rx scripter for memory override in BET case for beacons with damaged address1 (pretending to unicast address).
                       completing the fix from version 1.7.0.220.

Shir:               1.7.0.222 - 31/10/2024
                    - OSPREY_LDB-2412 & OSPREY_LDB-2414: send periodic calibration command via command queue
                    - Updating PHY versions LDB PG2 Silicon PHY_1.2.39.5.13.68, MDB PG1 Silicon PHY_2.1.39.5.13.68, MDB PG1 FPGA PHY_2.101.39.5.13.68

Dana:               1.7.0.221 - 30/10/2024
                    - OSPREY_LDB-2431: Possible fix for Bus fault in ScanModuleSM_ActiveChannelScan
                     due to memcpy

GuyM:               1.7.0.220 - 29/10/2024
                    - [BLE] Update files name according to naming convention
                    - OSPREY_LDB-2394: [BLE] Add API to compensate for the slow clock drifts
                    - OSPREY_LDB-2413: [BLE] Add PM module client for BLE central role
                    - New BLE version ble_rev_1.0.0.37 (Peripheral slow clock drift compensation + Central connected active)
                    - OSPREY_LDB-2441: Fix pmac rx scripter for memory override (unicast key) in BET case for beacons with damaged address2

Michael:            1.7.0.219 - 28/10/2024
                    - Fix BLE IRQ lock to disable only low and medium IRQs
                    - Separate Medium and Low priority IRQs, simplify IRQ prioritization macros
                    - Refactor DRBG continuous check logic and add statistics printing (Bashar)
                    - WDT handler to print interrupted context
                    - Fix Makefile bug that was deleting the wrong container_mod.json artifact
                    - Minor cleanup

Shir:               1.7.0.218 - 15/10/2024
                    - OSPREY_LDB-2399: No new calibration requests are allowed until calibration end call back is sent.
                    - Updating PHY versions LDB PG2 Silicon PHY_1.2.39.5.10.68, MDB PG1 Silicon PHY_2.1.39.5.10.68, MDB PG1 FPGA PHY_2.101.39.5.10.68.

GuyM:               1.7.0.217 - 08/10/2024
                    - OSPREY_LDB-2367: [DPL] Implement ClockP with Time manager
                    - OSPREY_LDB-2377: [TIMEMNG] Replace time manager protection from mutex to disable of IRQs
                    - OSPREY_LDB-1693, OSPREY_LDB-1700: add set PS mode command,
                    - Add new INI parameter StaPowerSave to enable or disable the power save mechanism.

Shir:               1.7.0.216 - 30/09/2024
                    - OSPREY_LDB-2234: Access to pRoleCtx without checking if it's NULL (due to the role is not being enable).
                    - OSPREY_LDB-2342: Michal's fix - beacon RX urgent.
                    - Updating PHY versions LDB PG2 Silicon PHY_1.2.39.5.6.68, MDB PG1 Silicon PHY_2.1.39.5.6.68, MDB PG1 FPGA PHY_2.101.39.5.6.68.
                    
GuyM:               1.7.0.215 - 26/09/2024
                    - OSPREY_LDB-2299: [COEX] BLE connection drop after a WIFI connection attempt with wrong PSK
                    - OSPREY_LDB-2299: [COEX] Updating WLAN channel without tuning when BLE has higher priority (requires PHY update)
                    - [COEX] Excluding skipping higher priority for PHY Channel tune while calibration in progress
                    - New BLE version ble_rev_1.0.0.36 (RFLIB abort protection, RTM bug fixes, general bug fixes, CENTRAL_GUARD_TIME_ENABLE by default)
                    - Updating PHY versions LDB PG2 Silicon PHY_1.2.39.5.5.68, MDB PG1 Silicon PHY_2.1.39.5.5.68, MDB PG1 FPGA PHY_2.101.39.5.5.68.

Michal:             1.7.0.214 - 12/09/2024
                    - Force RTS-CTS protection for every data \ qos data packet.
                    - Fix SDIO BLE bug (Elad Sorani)
                    - Update ini file to 'no ps poll' mode - move to active for every tx \ rx
                    - Link adaptation - move to new data base structure - dial table ( fail \ success)

Shir:               1.7.0.213 - 11/09/2024
                    - OSPREY_LDB-2149: send scan complete event after stop scan command appears without a preceding scan command.

Dana                1.7.0.212 - 09/09/2024
                    -OSPREY_LDB-2231, OSPREY_LDB-2232: channel switch fix in ChannelSwitch_StopHandler.

Katya               1.7.0.211 - 03/09/2024
                    -OSPREY_LDB-2201: P2P Client: complete missing parts - NOA

Katya:              1.7.0.210 - 02/09/2024
                    -automatic signature fix: missing real certificate file

AviramD:            1.7.0.209 - 01/09/2024
                    - Aligning to RTM patch release (merging versions 1.7.0.182 - 1.7.0.185):
                        - Updating PHY versions LDB PG2 Silicon PHY_1.2.39.5.42.67, MDB PG1 Silicon PHY_2.1.39.5.42.67, MDB PG1 FPGA PHY_2.101.39.5.42.67. 
                        - Aligning COEX_GRANT_pull_val INI variable to its correct default value.
                        - OSPREY_LDB-2160 - handling beacon timeout in MR
                        - Changed "IsDiplexerPresent" INI parameter default to 0x1.

Roey:               1.7.0.208 - 22/08/2024
                    - Defragmentation implementation by the FW.
                    - update the conf.h and .ini files that the implementation should be done by the FW or the host.
                    
GuyM:               1.7.0.207 - 21/08/2024
                    - OSPREY_LDB-2159: [AutoHIF] Cancel event is wrongly triggered for a non sleep like power mode
                    - Update BLE CPE Radio versions (0xF20E / 0xE105)

Shir:               1.7.0.206 - 19/08/2024
                    - OSPREY_LDB-2163: Remove assert when CROC command appear while FW is not in ROC
                    - OSPREY_LDB-2175: Remove assert when STOP-SCAN command appear while FW is not in scan

Shir:               1.7.0.205 - 15/08/2024
                    - OSPREY_LDB-1904: Bug fix for entering PS for traffic module too early

Michael:            1.7.0.204 - 13/08/2024
                    - Fix memory/OCP settings reset after PLL-init ELP
                    - Remove NMI handler code
                    - Rewrite hardfault handler to support full debug dump on bus-fault

BasharB:            1.7.0.203 - 12/08/2024
                    - GET_ROLE_CHANNEL_NUMBER command added

BasharB:            1.7.0.202 - 08/08/2024
                    - Channel Utilization Feature Added
                    - Decrypt Packets Counters Added (Success/Fail Counters)

Dana, MichaelN:     1.7.0.201 - 07/08/2024
                    - OSPREY_LDB-2003 - Bug fix for assert when Assert before channel switch ooccurs. (Dana)
                    - Legacy tools use python2 explicitly during build
                    - gServer script cleanup 
                    - Remove unused legacy tools
                    - Remove headers for old non-existing HW

TomerZ:             1.7.0.200 - 30/07/2024
                    - added configAP system info, and ROM BOOT DONE (MX build).
                    - Fuse data use from bootloader instead of reading registers/fuses (MX build).

AviramD:            1.7.0.185 - 26/08/2024
                    - Updating PHY versions LDB PG2 Silicon PHY_1.2.39.5.42.67, MDB PG1 Silicon PHY_2.1.39.5.42.67, MDB PG1 FPGA PHY_2.101.39.5.42.67. 
                    - Aligning COEX_GRANT_pull_val INI variable to its correct default value.

AviramD:            1.7.0.184 - 22/08/2024
                    - Updating PHY versions LDB PG2 Silicon PHY_1.2.39.5.41.67, MDB PG1 Silicon PHY_2.1.39.5.41.67, MDB PG1 FPGA PHY_2.101.39.5.41.67. 
                    - Changed "IsDiplexerPresent" INI parameter default to 0x1.

AviramD:            1.7.0.183 - 22/08/2024
                    - Updating PHY versions LDB PG2 Silicon PHY_1.2.39.5.40.67, MDB PG1 Silicon PHY_2.1.39.5.40.67, MDB PG1 FPGA PHY_2.101.39.5.40.67.
                    - OSPREY_LDB-2160 - handling beacon timeout in MR
                    - OSPREY_LDB-2159: [AutoHIF] Cancel event is wrongly triggered for a non sleep like power mode
                    - Update BLE CPE Radio versions (0xF20E / 0xE105)

GuyM:               1.7.0.182 - 30/07/2024
                    - New BLE version ble_rev_1.0.0.35 (ADV start/stop stress fix)

DanH:               1.7.0.181 - 29/07/2024
                    - New  PHY_1.2.39.5.32.67

DanH:               1.7.0.180 - 26/07/2024
                    - OSPREY_LDB-2100: [COEX]: Switching to WIFI after BLE advertise with logger disabled does not work 

AviramD:            1.7.0.179 - 25/07/2024
                    - Updating PHY FW versions: LDB PG2 Silicon PHY_1.2.39.5.31.67, MDB PG1 Silicon PHY_2.1.39.5.31.67, MDB PG1 FPGA PHY_2.101.39.5.31.67.
                    - OSPREY_LDB-2058: ThickMAC - Assert after BSS loss for maxTxRetries.
                    - OSPREY_LDB-2045: ThickMac: On 5G , STA send PEER_CAP_CFG with wrong rates of 2.4 instead of 5, field issue.

GalM:               1.7.0.178 - 24/07/2024
                    - OSPREY_LDB-2099: setting Iomux from IN file does not work (IOMUX_INIParam).
                    - Updating wifi-tool-box to version 3.0.4
                    - Michal: OSPREY_LDB-2077: drop broadcast encrypted management frame 

AviramD:            1.7.0.177 - 23/07/2024
                    - OSPREY_LDB-2043: ThickMAC: wlanLinks_AddLink() Function Called with Undefined MAC address.
                    - OSPREY_LDB-2046: ThickMac: in AP_ROLE beacon configuration for extended rates is corrupted.
                    - OSPREY_LDB-2051: [ThickMAC] Scan results show wrong rssi.
                    - OSPREY_LDB-1977: Thick MAC: Misalignment in Role Start and Channel Switch CMDs Between Osprey Dispatcher to Upper Mac Driver.
                    - OSPREY_LDB-2057: [ThickMAC]: SUT - high number of retries in RX. This is a possible fix for OSPREY_LDB-2057.
                    - OSPREY_LDB-2087: [ThickMAC]: ThickMAC: Connect to non-existent SSID - causes assert.
                    - OSPREY_LDB-1095: Thick MAC PG2 MR + 2 UDP - watch point occured !!! PC = 0x24570C.
                    - OSPREY_LDB-1561: Thick MAC PG2 MR + 2 UDP - watch point occured !!! PC = 0x218A88.
                    - Updating PHY FW versions: LDB PG2 Silicon PHY_1.2.39.5.30.67, MDB PG1 Silicon PHY_2.1.39.5.30.67, MDB PG1 FPGA PHY_2.101.39.5.30.67.

BasharB:            1.7.0.176 - 21/07/2024
                    - skip proccess PHY Logs and Just Flush if Logs are Disabled
                    - skip print attemps in idletask if logs are disabled

Noam:               1.7.0.175 - 21/07/2024
                    - OSPREY_LDB-1907: add diversity read commands

Dana                1.7.0.174 - 18/07/2024
                    - OSPREY_LDB-2025, OSPREY_LDB-2033, OSPREY_LDB-2034, OSPREY_LDB-2036 - Channel switch fix in ChannelSwitch_StopHandler parameters 
                    sent to WlanLpThread_InsertCommand were wrong.
                    - OSPREY_LDB-2035 - Channel switch fix in ChannelSwitch_StopHandler removing malloc.
                    Michal-
                    - OSPREY_LDB-2065, OSPREY_LDB-2066 – traffic module better handle stop role 

GalM:               1.7.0.173 - 14/07/2024
                    - New PHY ver_x.x.39.5.29.67
                    
GalM:               1.7.0.172 - 07/07/2024
                    - New PHY 
                    - Iomux configuration in radio-tool fix
                    - Primary sub-channel parameter added to phy start RF set.
                    - OSPREY_LDB-1999 - certification issue - clear the 'avoid edca' indication in PMAC as part of stop role. (Michal)
                    - OSPREY_LDB-2037 - recover PMAC error for 'data underflow' and 'length not ready' (Michal)
                    - OSPREY_LDB-1780 - fix null pointer read from linkSwEntry in set_key command
                    
GuyM:               1.7.0.171 - 07/07/2024
                    - [BLE] Remove the support in Bluetopia Host
                    - New BLE version ble_rev_1.0.0.34 (Remove Bluetopia support)

Noam:               1.7.0.170 - 04/07/2024
                    - OSPREY_LDB-2016: Add support in channel 169

Katya:              1.7.0.169 - 03/07/2024
                     - add container_to_h file - fix
                     - MDB: WA for PLL SOC, until Sleep is supported

BasharB:            1.7.0.168 - 02/07/2024
                    - Thickmac Device Handling
                    - upper_mac_service_utils.h Added for wrapping FW functions that we want to use in hooks

AviramD:            1.7.0.167 - 02/07/2024
                    - ThickMac patches & hooks delivery - mainly 5GHz and channel switch support
                    - OSPREY_LDB-2022: Patchtrap: [Assert] Patch Trap Status Register Shows Incorrect Value
                    - OSPREY_LDB-1981: ThickMac: patchtrap.c is Compiled Within FW Context
                    - OSPREY_LDB-1967: ThickMac: mcu_hooks.c is Compiled Within FW Context
                    - OSPREY_LDB-1960: Patch Mechanism Doesn't Function
                    - OSPREY_LDB-1974: ThickMAC: Channel Switch Support
                    - OSPREY_LDB-1978: Thick MAC: Reg Domain Causing Some 5GHz Channels Going Missing
                    - OSPREY_LDB-1977: Thick MAC: Misalignment in Role Start and Channel Switch CMDs Between Osprey Dispatcher to Upper Mac Driver
                    - OSPREY_LDB-1920: 5Ghz Basic for ThickMac
                    - OSPREY_LDB-1949: Thick MAC: Misalignment in Scan CMD Between Osprey Dispatcher to Upper Mac Driver
                    - OSPREY_LDB-1962: ThickMac: Regulatory Domain Channels Settings Algorithm Issue
                    - OSPREY_LDB-1948: Hooks Mechanism Doesn't Function


Katya:              1.7.0.166 - 30/06/2024
                    - New PHY MDB x.x.38.5.26.67
                    - OSPREY_MX-352 - Read MAc Address from fuse
                    - OSPREY_LDB-2020: [MX] Data path is not functional causing pairing failures

GalM:               1.7.0.165 - 30/06/2024
                  - New LX PHY ver_1.2.38.5.26.67
                   
GalM:               1.7.0.164 - 28/06/2024
                  - [OSPREY_LDB-1913] Calibrator & PLT additions
                    1) Auto trigger based support.
                    2) Device IO control.
                    3) BLE test mode via calibrator.
                    4) Disable PLT mode when BLE enable(in normal mode - not in PLT mode).
                    5) Clean-up in calibrator readme.
                    
Shir:               1.7.0.163
                  -  [OSPREY_LDB-1910] verify that data is pending for the specific role before submitted traffic request

BasharB:            1.7.0.162 - 26/06/2024
                  - Update codesize.py script to reflect  size of Extra Data Ram Used (ThickMAC Only)
                  - Hardcoded Upper MAC global variables objects placed manually in fixed addresses in DATA RAM (placed as expected by ROM)
                  - Split 16KB Code RAM For:
                   (These 16KB were already taken from code ram and set as Data Ram NO INIT)
                    - 13KB for EXTRA DATA RAM NO INIT
                    - 3KB  for EXTRA DATA RAM WITH INIT
                  - [OSPREY_LDB-1963] Thickmac FW Compile: 99% of Data Ram used



Noam:               1.7.0.161 - 24/06/2024
                  - OSPREY_LDB-1917: Unicast key Rotation - Handling RX PN correctly

Dana:               1.7.0.160 - 20/06/2024
                  - OSPREY_LDB-1930: Calibration after Channel switch - need to postpone calibration till we receive a beacon.
                  - If station is in the process of channel switch, it must wait the minimum CAC 
                    (Channel Availability Check) time for receiving a beacon the new channel.
                  - In the future we will reset AP jitter and beacon dtim procedures upon channel switch.
                  - OSPREY_LDB-1985 - PHY transition from LISTEN mode to RX ONLY is too long, and we may miss broadcast \ 
                    multicast after DTIM. Change PMAC configuration to terminate beacon in such case.

BasharB:            1.7.0.159 - 19/06/2024
                  - OSPREY_LDB-1919: Update temperature formula for PG2.1 devices

GalM:               1.7.0.158 - 13/06/2024
                  - New PHY: lx pg2  1.2.36.5.16.67
                             mx pg1  2.1.36.5.16.67
                             mx fpga 2.101.36.5.16.67

Noam:               1.7.0.157 - 10/06/2024
                  - OSPREY_LDB-1251: OBSS Narrow Bandwidth RU In OFDMA Tolerance Support
                  - Add isDfsChannel parameter to role start and channel switch commands.
                  - OSPREY_LDB-1812, OSPREY_LDB-1839: Antenna diversity bug: antenna 0 scan in 2.4Ghz antenna 1 scan in 5Ghz.

BasharB:            1.7.0.156 - 06/06/2024
                  - Hook added (for HOOK_DRV_TI_MLME)
                  - Assoc Descriptor Workaround (not relased by thickmac before assoc response received)
                  - OSPREY_LDB-1095: Thick MAC PG2 MR + 2 UDP - watch point occured !!! PC = 0x24570C
                  - OSPREY_LDB-1511: Thick MAC : SUT Connection fail 800 of 1000 connection successes
                  - OSPREY_LDB-1629: [Thickmac] MAC address derivation is incorrect
                  - OSPREY_LDB-1744: Thickmac (FW): Fuse Rom structure bug
                  - OSPREY_LDB-1955: AP Role - too much probe responses causing MANAGEMENT POOL to get full

BasharB:            1.7.0.155 - 06/06/2024
                  - OSPREY_LDB-1953: No BLE or WIFI Coex Grant after ./ble_enable (Coex Issue)

Katya:              1.7.0.154 - 04/06/2024
                  - Add chip conteiner and opertational phy fw for MX build
                  - Fix in device probe req frame drops
                  - OSPREY_LDB-1952: [BLE][AUTONOMOUS_UART] Set the FSM state before disabling autonomous UART

Dana:               1.7.0.153 - 03/06/2024
                  - OSPREY_LDB-1871: Calibration timeout is reached due to BET getting stuck because of missing a beacon.

GuyM:               1.7.0.152 - 03/06/2024
                  - OSPREY_LDB-1939: [BLE] Device get stuck with RECEIVER_LINE_STATUS when running ADV start/stop

Shir:               1.7.0.151 - 30/05/2024
                  - GPADC muxing disabling
                  - PhyApiDispatcher: PhyApiComplete CallBack Moved to After Put Semaphore
                  - OSP_SIVAL-263: restore some GPADC muxing after reading temprature
                  - OSPREY_LDB-1940: BET is not functional since version 118
                  
Shir:               1.7.0.150 - 29/05/2024
                  - OSPREY_LDB-1677: Going to Active for Tracking Awake mode, and not returning back to PS for Tracking LP.
                  - Add role id prints to the traces of changing beacon tracking mode.
                  - Add marker print & fix requested vector in NESTED Suspend trace.

GalM:             - 1.7.0.129 - 27/05/2024
                    New PHY version 1.2.36.5.7.67

GuyM:               1.7.0.128 - 23/05/2024
                  - OSPREY_LDB-1918: [CON DMA] DMA might not write a byte to UART FIFO
                  - OSPREY_LDB-1926: [BLE][AUTONOMOUS_UART] Device might get stuck when running ADV start/stop

Amitai:             1.7.0.127 - 23/05/2024
                  - OSPREY_LDB-1933: Remove dangling pointer in HP thread

Amitai:             1.7.0.126 - 22/05/2024
                  - Remove unused items in conf.h

GalM:               1.7.0.125 - 16/05/2024
                  - OSPREY_LDB-1885: SOC PLL locking impact TX EVM after sleep/reboot cycles
                  
Amitai:             1.7.0.124 - 12/05/2024
                  - Change vendor IE cmd to take role type instead of roleID
                  - Clear IEs on role down

Katya:             1.7.0.123 - 05/05/2024
                  - Rx Filter for probe req. for device role
                 
Dana:              1.7.0.122 - 02/05/2024
                  - Support unicast and broadcast TKIP for STA role only, without fragmentation.

GuyM:              1.7.0.121 - 01/05/2024
                  - OSPREY_MX-305: [MX][BLE_DISPATCHER] Fix ASSERT after connection
                  - [BLE_THERMAL] Add new APIs to BLE thermal module
                  - New BLE version ble_rev_1.0.0.33

GalM:              1.7.0.120 - 30/04/2024
                  - OSPREY_LDB-1883: Documentation mistake in INI file in grant polarity
                  
Michal:            1.7.0.119 - 25/04/2024
                  - Short term policy configuration - short term policy 3 & 4, that should be updated according to erp protection
                    configuration, are not configured correctly, which may cause sending ps-poll and other frames in high rates.
                  - OSPREY_LDB-1843: Link adaptation seems stuck on mcs7, despite many retransmissions.
                                     Problem caused as a combination of the wrong configuration of short term policy3, and
                                     the scripter WA avoid updating the link adaptation when short term policy is used.
                  - WOW - in case WOW is enabled each packet sent to host will trigger an interrupt (no pacing). Related to bugs
                          OSPREY_LDB-1783 & OSPREY_LDB-1782. (Gal)


BasharB:           1.7.0.118 - 23/04/2024
                  - OSPREY_LDB-1818: MR STA/STA, AP/STA - assert after reloading both interfaces ( After~ 100 times)
                  - OSPREY_LDB-1838: TWT - DUT stop sending TWT request after 3 times
                  - OSPREY_LDB-1825: TWT - No abort in logger when doing TWT teardown (with duration: 8192 interval: 65536 )

Noam:           1.7.0.117 - 21/04/2024
                  - OSPREY_LDB-1806: [Antenna Diversity] Beacons that are early terminated don't update the role DB
                  - Add PMAC ENC abort at TX error handling

Aviram:         1.7.0.116 - 18/04/2024
                  - OSPREY_LDB-1859: [Calibrator] RX Stats are 0 when we do manual calibration in plt
                  - New PHY version 1.2.36.5.22.66

BasharB:        1.7.0.115 - 17/04/2024
                  - New PHY ver PHY_x.x.36.5.20.66
                  - New GPADC probe Equations (PMCIO & RFCIO) for PG2.0
                  - OSPREY_LDB-1849: XTAL_SettlingTime ini param not taken into account when calculating wakeup time
                  - OSPREY_LDB-1852: TWT - certification HE-5.60 TWT requester set to 0 in extended cap in association request
                  - OSPREY_LDB-1836: RSSI INTR CMD Assert While TWT Suggest
                  - OSPREY_LDB-1835: RSSI INTR CMD Assert While TWT Demand
                  - OSPREY_LDB-1829: TWT - No "sp start" "sp end" print in logger while traffic in TWT demand with Cisco

Michal:         1.7.0.114 - 17/04/2024
                  - OSPREY_LDB-1673, OSPREY_LDB-1714 - suspend & resume role set request for power management.
                                                       (solve MR STA-STA issues).
                  - OSPREY_LDB-1771 - sched state module - handle 'process end' messages in NULL and PARTIAL state.                               
                  - OSPREY_LDB-1775 - TB PLT auto mode - add support for UL BW 0,1,2
                  - OSPREY_LDB-1858 - assert during TB PLT auto mode (related to PMAC TX ERROR mask modification).
                  - OSPREY_LDB-1861 - TB PLT auto mode - add gtrace of parameters.

Dan:            1.7.0.113 - 16/04/2024
                  - OSPREY_LDB-1845: [COEX]: EXT BLE - In some cases Wi-Fi connection fails with Assert
                  - OSPREY_LDB-1855: [COEX]: enabling all entities together may create assert

Noam:           1.7.0.112 - 16/04/2024
                  - Antenna diversity algorithm improvements
                  - Add the diversity module to the RPU suspend/resume mechanism
                  - OSPREY_LDB-1842 - suspend TX queues during channel tune
                  - OSPREY_LDB-1831, OSPREY_LDB-1833 bugs fix - curiosity range handling
                  - OSPREY_LDB-1830: Add support in gcc-11.4 in gccxml file
                  - New PHY version PHY_x.x.36.5.19.66

GalM            1.7.0.111 - 11/04/2024
                  - Patch for GPADC measurments in PG2.1
                  
GalM            1.7.0.110 - 09/04/2024
                  - Remove IOMUX pull configuration for GPIO0 and GPIO2 in ini
                  - Change 6GHz params to reserved in ini

Michal:         1.7.0.109 - 08/04/2024
                  - mask pmac error exception in radio tool calibration

GalM:           1.7.0.108 - 08/04/2024
                  - new IOMUX pull value param added to ini
                  - Noam: antenna diversity compensation default value change
                  - Bashar: 3 variables we didint intizliaze in the driver
                  
BasharB:        1.7.0.107 - 03/04/2024
                  - new PRCM - 2ms settling time
                  - Xtal settling time param added to ini
                  - Gal: regdomain phy param added to ini

Dan:            1.7.0.106 - 03/04/2024
                  - New BLE Agama version: ble_rev_1.0.0.32
                  - OSPREY_LDB-1412: [COEX]: FW crash when 5Ghz is enabled and using both BLE and WiFi entities
                  - OSPREY_LDB-1765: [COEX]: BLE fails to connect while WiFi is connected
                  - OSPREY_LDB-1774: [COEX]: Scan Coex priority is not removed in all scan flows
                  - OSPREY_LDB-1778: [COEX]: WiFi connection/data during BLE data fails with Assert


BasharB:        1.7.0.105 - 02/04/2024
                  - Change conf.h file types format (u8,u16,u32, __packed --> uint8_t uint16_t uint32_t __attribute__((__packed__)))
                  - Check if Trigger & Implicit are enabled in TWT Setup Response Recieved by the AP
                  - Add mgmt Protection for TWT Setup
                  - New PHY_1.2.36.5.18.66
                  - OSPREY_LDB-1789: GPADC Temperature Equation Issue PG2.1
                  TWT:
                  - OSPREY_LDB-1792: TWT - assert after demand request (AP response with alternate)
                  - OSPREY_LDB-1793: TWT - assert after suggest request (AP change params then we sent teardown)
                  - OSPREY_LDB-1794: TWT - assert in stability bidir UDP traffic after ~ 5 min
                  - OSPREY_LDB-1798: TWT: No Timeout on wait for TWT Setup Response
                  - OSPREY_LDB-1799: [TWT]:TWT Teardown Flow identifier is always 0
                  - OSPREY_LDB-1800: TWT requester support is disabled in he_cap
                  - OSPREY_LDB-1801: [TWT]: Qos Null sent by AP with EOSP =1 But last Trigger frame before that had moreTF=1
                  - OSPREY_LDB-1537: TWT - param validation
                  - OSPREY_LDB-1566: TWT - assert after teardown and traffic
                  - OSPREY_LDB-1791: [TWT]: Calibration Timeout Assert when TWT is enabled
                  - OSPREY_LDB-1800: TWT requester support is disabled in he_cap
                  - OSPREY_LDB-1822: TWT - assert when doing TWT terminate while notin TWT
                  - OSPREY_LDB-1825: TWT - No abort in logger when doing TWT teardown (with duration: 8192 interval: 65536 )

Michal:           1.7.0.104 - 24/03/2024
                  - In calibeartion agent - avoid CTS protection if the CTS queue is not enabled.
                  - PMAC TX error indication - error indication were blocked since RTP. now opened for monitoring and debug.
                  - GCC update
                  - BA management - score board module (scb) will now be used in mode 0 (memory storage) for all roles.
                    this is due to the fact the multi TID aggregation in ax were not used. should solve STA-STA RX TP issues.
                  - OSPREY_LDB-1714 - ASU callback timer - mark the ASU callback timer as wakeup event, to make sure we get a wake on time in case of STA-STA MR.

DaniellaS:        1.7.0.103 - 16/03/2024
                  Channel switch bugs fixes OSPREY_LDB-1768 OSPREY_LDB-1761
                  - Update sched_request to be SCHED_REQ_PULLBACK_UNACCOUNTED
                  - Update calibration not to send CTS when we 
                    are in channel switch is in progress
                  - Fix T.O Driver command - signal syncobj, check if BLE is up
                  - Fix MR - AP-STA with channel switch for STA 

Noam:             1.7.0.102 - 17/03/2024
                  - OSPREY_LDB-1735: Antenna Diversity
                  - New PHY version 1.2.36.5.14.66
                      
BasharB:          1.7.0.101 - 12/03/2024
                  - TWT Inital Added (Target Wake Time) - Individual TWT Only

Dana:             1.7.0.100 - 03/03/2024
                  - CTS to self added to calibration flow
                  - WA for OSPREY_SILICON-8272 (follow PMAC_CTS_DURATION_WA).
                  FW fix the CTS duration field under RT IRQ.
                  - New PHY Ver_1.2.35.5.9.66__5GHz_DFE_clk

Aviram:           1.8.0.26 - 01/03/2024
                  - Radiotool INI command download failed fix

GalM:             1.8.0.25 - 29/02/2024
                  - Enable WoW feture with data filtering
                  - Beacon filter ramp-up for enabling WoW

Daniella:         1.8.0.24 - 27/02/2024
                  - OSPREY_LDB-1584: STA Channel switch

eladso:           1.8.0.23 - 28/02/2024
                  - Added Support for 5G BAND + scan bug fixes
                  - OSPREY_LDB-1747 - [COEX] Add support for 5Ghz - Alt Band

Aviram:           1.8.0.22 - 26/02/2024
                  - OSPREY_LDB-1617: Regulatory Domain: Adjust 5GHz Channel Mapping to PHY INI Table Entries

Dan:              1.8.0.21 - 25/02/2024
                  - OSPREY_LDB-1598: [COEX] Vendor log reporting
                  - OSPREY_LDB-1573: [COEX]: Add WiFi specific activities priorities

GuyM              1.8.0.20 - 25/02/2024
                  - New BLE version ble_rev_1.0.0.30 (bug fixes)

BasharB:          1.8.0.19 - 22/02/2024
                  - New PHY_1.2.33.5.5.66 with new Phy Logger Prints
                  - Pass XTAL Presence to PHY

BasharB:          1.8.0.18 - 21/02/2024
                  - OSPREY_LDB-1720: preamble duration is calculated twice on precede time

GuyM:             1.8.0.17 - 19/02/2024
                  - OSPREY_LDB-1752: [MX][BLE] Add support in host interface

Michal:           1.8.0.16 - 18/02/2024
                  - Add PLT AUTO TB mode using PHY SA command

Katya:            1.8.0.15 - 18/02/2024
                  - Add missing interrogate cmd

Shir:             1.8.0.14 - 15/02/2024
                  - OSPREY_LDB-1498: Fix tx bitrate value in "iw" Linux-shell tool

Amitai:           1.8.0.13 - 15/02/2024
                  - Fixed some issues with the vendor info elements

Katya:            1.8.0.12 - 13/02/2024
                  - Remove INI WA in MX
                  - Fix mak file
                  - New PHY MX 2.101.33.5.3.66
                  
Aviram:           1.8.0.11 - 11/02/2024
                  - New INI parameters in PHY INI DB, especially for 5GHz support
                  - New LX PHY (5GHz support): 1.2.33.5.2.66

Katya:            1.8.0.10 - 08/02/2024
                  - OSPREY_LDB-1754: MX PHY requests calibartion is not supported in FPGA.
                  - New MX PHY: 2.101.32.5.41.65
                  
Shir:             1.8.0.9 - 08/02/2024
                  - OSPREY_LDB-1570: Added support for scanning by specific BSSID

Katya:            1.8.0.8 - 30/01/2024
                  - New MX PHY 2.101.32.5.40.65
                  - Disable GPADC in MX (FPGA) - currenly unsupported
                  - Create default ini params for MX

BasharB:          1.8.0.7 - 28/01/2024
                  - Fallout Issue Fix
                  - Add new SRAM LDO Voltage Trim Param To .conf
                  - Amitai: Vendor Elements Added

BasharB:          1.8.0.6 - 22/01/2024
                  - New PHY Ver_1.2.32.5.39.65 (Tx Stuck Fix)
                  - Updated limited power FCC conf ini file


GuyM:             1.8.0.5 - 21/01/2024
                  - [BLE] Update the connection handle offset value

Dana              1.8.0.4 - 21/01/2024
                  - 'Mixed mode Support’ enable/disable added to ini file 
                  - changes in arc4.c
                  - New PHY_1.2.32.5.36.65
                  - fix in make file for MX build

Katya:            1.8.0.3 - 09/01/2024
                  - OSPREY MX integration into osprey_int

Dan:              1.8.0.2 - 07/01/2023
                  - OSPREY_LDB-1702: [COEX][PWRMGT][BLE] When changing power modes, in specific settings, BLE doesn't receive the coex grant
                  - moving the Coex init for BLE to BLE thread so it will finish initialization before requesting grant
                  - fixing the wifi priority table
                  - fixing the activityID in the phyapidispatcher

BasharB:          1.8.0.1 - 05/01/2023
                  - PRCM Values Update (Lower FREF to 4ms, Set VDDSRAM to 0.65V) (-90uA IDLE Connection Current DTIM1)
                  - Reduce RX Beacon Timer PrecedeTime By 200us (-110uA IDLE Connection Current DTIM1)
                  - PHY FIX Reduce RX Current  By 2.5-3mA (-20uA IDLE Connection Current DTIM1)
                  - Reduce LPDS Restore Time When Logs are Disabled By -350us (-50uA IDLE Connection Current DTIM1)
                  - New CONF Param (in .conf file) for disable Beamforming_FFTP (Relevant to PG2.0)
                  - Michal BeamForming Fix
                  - PG Version with Metal Version Sent to PHY at FW Init
                  - Michal: PLT RSSI FIX
                  - New PHY_1.2.32.5.33.65

Dana:             1.8.0.0 - 03/01/2024
                  - Adding support for TKIP Mixed mode (STA role Rx broadcast only)

GuyM:             1.7.0.44 - 19/12/2023
                  - OSPREY_LDB-1701: [COEX] BLE running with external slow clock is not stable

MichaelN:         1.7.0.43 - 17/12/2023
                  - Update RAM BootMan binaires to 1.0.0.23
                  
MichaelN:         1.7.0.42 - 12/12/2023
                  - Update RAM BootMan binaires to 1.0.0.22
                  - Restore IR_TABLE_FW_CFG_AX_PARAM_OUT after ELP

GuyM:             1.7.0.41 - 10/12/2023
                  - [BLE][PWRMGT] Fix build warning due to missing include file

GuyM:             1.7.0.40 - 10/12/2023
                  - OSPREY_LDB-1686: [BLE][PWRMGT] Wait for PHY to be available after wakeup

BasharB:          1.7.0.39 - 06/12/2023
                  - Add New Ini Param Disable/Enable Log GTRACES and Prints
                  - Reduce Beacon Rx Precede Time based on disable_logger ini param
                  - New PHY Version: PHY_1.2.31.5.26.65__Claibration_Tx_Iqmm_stuck
                  - OSPREY_LDB-1680: - WDT fix - Reset the WDT counter only when the WDT should be enabled, as this action activates the counter (operational bug)

Dan:              1.7.0.38 - 05/12/2023
                  - Aviram: Added base offset for beacon FCS error threshold
                  - [COEX] Wi-Fi minimum priority is 1, even if none of the roles is active - OSPREY_LDB-1660
                  - [BLE_TEST_MODE] Force one time calibration when running BLE standalone tests - OSPREY_LDB-1681

Dana:             1.7.0.37 - 04/12/2023
                  - Calib fix in calibAgent_NotifyCalibrationStart and calibAgent_NotifyCalibrationEnd due to bug causing a command inserted into
                  WLanLpThread to overwrite another command already sent to WLanLpThread (both were sharing the same globals). 
                  - PMAC_TX__TX_INTRF_ERR_CFG register is set to disable all PMAC error interrupts and enable recovery. 
                  - PMAC_TX__TX_INTRF_ERR_CFG register is no longer masked by PHY.
                  - New PHY version: PHY_1.2.31.5.25.65
                  - PG version pass to PHY

Katya:            1.7.0.36 - 03/12/2023
                  - New PHY version 1.2.31.23.65
                  - Fix in traffic module on RX requests
                  - First Beacon Sync issue - OSPREY_LDB-1657

AviramD           1.7.0.35 - 30/11/2023
                  - New PHY version 1.2.30.5.21.65
                  - Adding 3 different INI files: unlimited, FCC, ETSI
                  - New BLE vector in INI regulatory domain PHY parameters
                  - OSPREY_LDB-1650: [Calibrator][Assert] ASSERT ERROR at address 2412d6 when doing PLT power_mode off
                  - OSPREY_LDB-1664: Beacon with FCS error due to BET is by mistake identified as 'miss beacon'
                  - GuyM: [BLE] New BLE CPE version 0xF20D (Update advBehConfig and pRpaCfg)
                  - Dana: Fix in calibration function: calibAgent_CalReqEvent allowing calibration for BLE which is not part of the start role flow

GuyM:             1.7.0.34 - 29/11/2023
                  - [BLE] Request for calibration after reading the CPE and MCE versions
                  - [RadioTool] Add new BLE_ENABLE command

Dana:             1.7.0.33 - 28/11/2023
                  - Fix in calibration function: calibAgent_CalReqEvent
                  - OSPREY_LDB-1622: W/A in UpdateSlowClockFreq and timeout optimization
                  
Noam:             1.7.0.32 - 27/11/2023
                  - OSPREY_LDB-1651 - When recovery is disabled, WDT should be disabled as well.
                  - OSPREY_LDB-1652 - Recovery mechanism should be disabled in PLT mode.

Dana:             1.7.0.31 - 26/11/2023
                  - OSPREY_LDB-1632 - Bug causing calibrations to be missed.
                  - OSPREY_LDB-1659 - fix for wlan lp commands queue
                  - Fix in calibration flow for calibrations that are part of Start Role sequence and calibrations not part 
                    of Start Role sequence.

GuyM:             1.7.0.30 - 26/11/2023
                  - OSPREY_LDB-1647: [COEX][CALIB][BLE] First BLE command (RADIO_SETUP) always not granted due to BLE calibration
                  - OSPREY_LDB-1639: [BLE][AutoUart] Fix wrong UART threshold when RX is Completed
                  - New BLE version ble_rev_1.0.0.29 (BLE bug fixes)

MichaelN:         1.7.0.29 - 22/11/2023
                  - OSPREY_LDB-1520: Relocate BLE SDIO PM activity
                  - SW workaround for missing GNS BLE SDIO indication
                  - [BLE] Ignore Thermal limit change while in DTM
                  - New BLE version ble_rev_1.0.0.28 (Shared SDIO sleep + BLE bug fixes)

Noam:             1.7.0.28 - 21/11/2023
                  - New debug command: Disable/enable burst for video AC

Bashar:           1.7.0.27 - 16/11/2023
                  - OSPREY_LDB-1628: External slow clock - Wake up params support
                  - OSPREY_LDB-1631: Remove currTsf>expectedTSF Assert in pwrmgt_lpds, add print in statistics instead
                  - Move gpadc call Before Tune For calibration to PhyApiDispatcher
                  - PRCM Update Values (SRAM_KA_TRIM_NO_RTA And Dig Ldo) For LPDS Reduce (~250uA lpds for NOM Chip)
                  - Keep Dig LDO at 1.1V Only For internal SlowClock
                  - InI File SlowClock Params Split to Internal/External
                  - LFOSC Values Update
                  - Rx Beacon PrecedeTime Reduced By 300, SlowClock WakeupEarlier  added to PrecedeTime instead (~60us For External)
                  - Beacon Profile Current Lowered By over 700uA (External SlowClk, NOMINAL CHIP)
                  - Michal: Fix SNR handling code for ThickMac Crash


Noam:             1.7.0.26 - 14/11/2023
                  - Add antenna selection commands to Radio Tool
                  - Add new debug command: triggering firmware assert
                  - Michael: Fix missing logger file in output artifacts
                  - Rivit: RSSI_INTR: implment interogate command for rssi and noise floor

Dan:              1.7.0.25 - 13/11/2023
                  - Adding External coex support
                  - OSPREY_LDB-1590: [COEX]: WiFi sends power save request after long BLE traffic (40ms +)
                  - OSPREY_LDB-1508: [RT][PLT] PLT disable/off before PLT Start Causes an assert


MichaelN:         1.7.0.24 - 13/11/2023
                 - Drop all PG1.0 artifacts from build
                 - Remove static calibration binary
                 - Better Makefile parallelization

GuyM             1.7.0.23 - 12/11/2023
                 - New BLE version ble_rev_1.0.0.27 (RF-Driver + BLE bug fixes)
                 - BLE Thermal DTM support
                 - Revert back to simplelink-wifi-toolbox compiled for Ubuntu18

BasharN:           1.7.0.22 - 09/11/2023
                 - OSPREY_LDB-1630:[Functionality]ASSERT ERROR at address 2185a4 when connecting with AX AP
                 - Wakeup Tsf lowered by 200us (wakes 200us earlier)
                 - New Toolbox and INI Composer release integrated to FW Project
                 - Aviram: tx_control #ifdef Instead of #ifndef Fix (for 5GHZ)
                 - Nab MailBox Size Aligned to 4 Bytes
                 - LRXP fix: global link id is reported to host as any other link id
                 - BET TH is reasied from 10 ro 20 - after 20 terminated beacons one is fully received and delivered to host.
                 - STA protection - beacon parsing bug fix
                 - OSPREY_LDB-1623 - RPU fix for nested suspend request (correct update of teh undone bitmap)
                 - Fix typo/unidentfied Symnol in .ini Files


Dana:           1.7.0.21 - 08/11/2023
                 - Fix calibration for use with Radio Tool and calibrator (set pltModeIndication)
                 - New PHY_x.x.29.5.13.65__ble_tx_power__no_calibs_in_plt
                 - Katya:  set a basic rates for immediate response for P2P device role



BasharB:         1.7.0.20 - 06/11/2023
                 - OSPREY_LDB-1604: Add Enable/Disable periodic calibration to Ini File
                 - New PHY Ver_x.x.29.5.12.65__use_temp_from_MAC_by_GPADC
                 - GPADC Measuring Call added Before Tune For Calibration
                 - GPADC Measuring Call added Before Tune For Radio Tool
                 - GPADC Temperature Equation Updated Based on System Team Results
                 - GPADC Temperature Equation Result Passed to PHY AfterEvery  Measurement
                 
MichaelN:         1.7.0.18 - 06/11/2023
                 - New API to set BD address from host
                 - Bugfix in internal BD address generation
                 - New BLE version ble_rev_1.0.0.26 (Update BLE BD address retrieval)

Dana:             1.7.0.17 - 06/11/2023
                 - OSPREY_LDB-1558: Calibration Disturbs Connection Flow - RF Set and calibration (when needed) is now part of the 
                    start role flow.
                 - OSPREY_LDB-1346: Adding callback for a request that is not served by the scheduler.
                 - OSPREY_LDB-1524: Bug causing calibration requests to be left behind in the scheduler. 
                    The Fix is by ignoring calibration resume commands that come from the scheduler, when they are not expected.
                 - OSPREY_LDB-1614: RTM Calibration: MR + BLE ASSERT- Getting event before command complete from PHY
                 - OSPREY_LDB-1613: RTM Calibration: Assert in ap_sta_scan_loop.sh - Getting event before command complete from PHY
                 - New PHY: PHY_1.2.28.5.11.65

BasharB:          1.7.0.16 - 02/11/2023
                 - Add Slow Clock Params To INI File: wakeup earlier, Open window longer (units of PPM)
                 - Internal/External Slow Clock Calibration Based on Slow Clock INI file params (default values for Internal Set)
                 - Beacon Profile: beacon pre-processing duration Lowered by 2.3 msec
                 - PWRMGT Min Sleep Time Set to at Least SLEEP_OVERHEAD(ENABLE + DISABLE SLOWCLK)
                 - OSPREY_LDB-1616: Add Check after lpds restore that currTSf is same as expected TSF
                 - OSPREY_LDB-1612: Add cc33xxConf files to Compilation Out files
                 - OSPREY_LDB-1621: GPADC Get latest measurments RT Crash
                 - Manual Calib: Rx SPUR CANCELER bit set to 1

AviramD          1.7.0.15 - 02/11/2023
                 - OSPREY_LDB-1508 - [RT][PLT] PLT disable/off before PLT Start Causes an asset
                 - Workaround for disabling Tx power control for thickmac

Guy M            1.7.0.14 - 01/11/2023
                 - OSPREY_LDB-1603: [BLE] Add FW support for Host enter into sleep
                 - New BLE version ble_rev_1.0.0.25 (Host sleep FW support)
                 - [BLE] Generate new RSA every System Reset
                 - Add option to disable the watchdog timer (WDT)

AviramD          1.7.0.13 - 30/10/2023
                 - Tx output control (regulatory domain, thermal power control)
                 - New BLE version ble_rev_1.0.0.24 (BLE Thermal Protection)

Noam             1.7.0.12 - 29/10/2023
                 - OSPREY_LDB-1592 - Recovery Upon Watchdog Expiration
                 - Conf files update - changed recovery settings
                 - Decrease ADD BA Wait to response timeout 

Guy M            1.7.0.11 - 26/10/2023
                 - New BLE CPE version to 0xF10C/0xF20C (Update advBehConfig)
                 - New BLE version ble_rev_1.0.0.23 (Qualification Completed)

BasharB          1.7.0.10 - 24/10/2023
                 - GPADC Controller Interface Added
                 - GPADC Monitor Module Added
                 - New PHY_x.x.27.5.8.65
                 - New Bootloader ver 1.0.0.21

GuyM             1.7.0.9 - 23/10/2023
                 - New BLE version ble_rev_1.0.0.22

Amitai           1.7.0.8 - 18/10/2023
                 - [OSPREY_LDB-1571] Disable BLE if not supported by device flavour

Shir             1.7.0.7 - 15/10/2023
                 - OSPREY_LDB-1492 - Michal's fix - TB
                 - update the files wphy_bin_operational.out and wphy_bin_operational.map for PG1

Katya            1.7.0.6 - 13/10/2023
                 - OSPREY_LDB-1517 - TGN 4.2.21 - AC prioritization 
                 
GuyM             1.7.0.5 - 10/10/2023
                 - Increase Heap size to 35K for Linux only
                 - [BLE] Increase the maximum supported connections to 16 for Linux only
                 - New BLE version ble_rev_1.0.0.21

AviramD:         1.7.0.4 - 04/10/2023
                 - Reg domain updated params
                 - New PHY Version  x.x.27.5.3.65 
                 - New M3_M0 API
                 - INI File update - reg domain params added to PHY section

EladZ:           1.7.0.3 - 04/10/2023
                 - OSPREY_LDB-1586 (RAM_BM & WSoC_FW): Save eFuse calibration factors for GPADC monitor

Noam:            1.7.0.2 - 03/10/2023
                 - add get antenna select and set antenna select commands

GuyM:            1.7.0.1 - 19/09/2023
                 - OSPREY_LDB-1528: [RadioTool] Device get stuck after repetitive start/stop TX commands

Dan:             1.7.0.0 - 18/09/2023
                 - Fixing linux compile compilation - disabling coex when using linux compile

GuyM:            1.6.0.15 - 12/09/2023
                 - OSPREY_LDB-1518: [COEX]: Provisioning is causing assert when trying to increase the wifi priority to max
                 - OSPREY_LDB-1519: [PWRMGT] Device fail to go to sleep upon SDIO wakeup interrupt while BLE uses UART
                 - wa for thickmac - add delay in role start command

Dan:             1.6.0.14 - 12/09/2023
                 - New PHY FW version x.x.26.5.25.64
                 - New BLE version ble_rev_1.0.0.18
                 - [OSPREY_LDB-1468] - [COEX]: SUT stuck during data-data
                 - [OSPREY_LDB-1439] - [COEX] When power management enabled, BLE or WiFi might stop transmitting
                 - [OSPREY_LDB-1472] - [COEX]: SUT fails to connect to AP during BLE data - Assert

AviramD          1.6.0.13 - 07/09/2023
                 - [OSPREY_LDB-1500] LSI: Setup INI Parameters to Truly Affect the LSI Configurations
                 - [OSPREY_LDB-1502] INI file: setting ps_scheme parameter to 0x01 doesn't set to UPSD

Noam:            1.6.0.12 - 06/09/2023
                 - New PHY FW version x.x.26.5.24.64
                 - We now don't update g_lastChannelInfo_ull upon BLE calibration.

Shir             1.6.0.11 - 31/08/2023
                 - [OSPREY_LDB-1491] avoid listen mode in MR STA-STA due to lack of bet HW service
                 - [OSPREY_LDB-1467] Working in DMA also for short packets, instead of stand-alone
                 - Fix FWVersions_t size
                 - Limit get rssi for station only.

Shir             1.6.0.10 - 30/08/2023
                 - [OSPREY_LDB-1339] Add a new command "get_vertions" that print you the driver and firmware versions.
                 - [OSPREY_LDB-1332] Fix return value of beacon rssi

BasharB          1.6.0.9 - 29/08/2023
                 - [OSPREY_LDB-1397] Beacon Tracking: Station doesn't disconnect at all if beacon tracking didn't star
                 - [OSPREY_LDB-1476] Add new Mode to beacon Tracking "Waiting First Beacon"


AviramD          1.6.0.8 - 29/08/2023
                 - [OSPREY_LDB-1391] LSI: Feature doesn't work as expected
                 - [OSPREY_LDB-1460] LSI: Beacon loss might occur with LSI values 4 and higher
                 - [OSPREY_LDB-1360] LSI: When configuring lsi =>4 Osprey missing beacons and wakeup twice
                 - [OSPREY_LDB-987] No ping reply when beacon interval is 1500 or higher

Michal           1.6.0.7 - 28/08/2023
                 - Fix packet length for Thick MAC (remove 14 extra bytes).
                 - [OSPREY_LDB-1420] STA-STA role packet sent on wrong channel - fix change mode command to use the last tuned channel.
                 - Enable CDM feature in link adaptation.
                 - [OSPREY_LDB-1444] MU_DISABLE flags for ax certificatio - fix polarity. Note - there is s still issue in certification with related tests. 
                 - Increase max number of packets in TB aggregation to 18 (for certification scenario).

GuyM             1.6.0.6 - 28/08/2023
                 - OSPREY_LDB-1481: [RadioTool] Update AdvAux flow to support lower packet intervals
                 - New BLE version ble_rev_1.0.0.19

BasharB          1.6.0.5 - 27/08/2023
                 - [OSPREY_LDB-1458] WLAN Rx Listen Mode - not operational from 40 MAC Ver
                 - [OSPREY_LDB-1462] [PLT][RT] Transmitting 11 AX SU ER with MCS0 and SU ER BW 106 tone RU result in FCS Error packets
                 - PLT Issue: Transciever Still Responds To External Trigger After Stop TX

GuyM             1.6.0.4 - 24/08/2023
                 - New BLE version ble_rev_1.0.0.18

Katya            1.6.0.3 - 16/08/2023
                - [OSPREY_LDB-1224] - [MR] [Driver]STA-STA: differentiation of two station interfaces are based on interface name statically 

GalM             1.6.0.2 - 16/08/2023
                - [OSPREY_LDB-1323] and [OSPREY_LDB-1324] CCMP protection for MIC error
                - [OSPREY_LDB-1363] DMA is not protected from SOC going into low power mode
                - [OSPREY_LDB-1219] Thick-MAC- SUT ignores AP supported rates {owner: Roey}

BasharB          1.6.0.1 - 10/08/2023
                - [OSPREY_LDB-1443] BBB Calibrator Get TX Parameters Results Not Returned Correctly 
                - [OSPREY_LDB-1461] Calibrator: get_tx_params might return incorrect values 
                - [OSPREY_LDB-1464] a. Maintaining 2 INI files, One for Thickmac and one for LX, with different values. 
                - [OSPREY_LDB-568] PLT- incorrect delay 
                - [OSPREY_LDB-1457] changing booster pack dts sdio max-frequency 
                - [OSPREY_LDB-1463] [COEX] Coex IRQ is disabled 
                -	PLT HW Delay packet payload memcpy: copy only 100 bytes of previous packet only in case of aggregation
                -	Remove IOMUX writes at FW init
                - Upgrade simplelink tool box for compilation in FW
                - Remove Transceiver_Stop in Transceiver init
                - PLT: Configure the PMAC back to EDCA only in Transceiver_Stop
                - New PHY: PHY_x.x.26.5.20.64

BasharB          1.6.0.0 - 10/08/2023
                - New PhY__1.2.26.5.19.64

Michal          1.5.0.2 - 09/08/2023
                - Fix in link adaptation - causing stuck in 1M - per weight table overflow.

EladZ           1.5.0.1 - 09/08/2023
                - (RAM_BM):  Update Customer OTP fields, mainly CRC check enable

Michal          1.5.0.0 - 08/08/2023
                - Link adaptation modifications (Matan Ben Shahar)
                - Change the 11b CCA TH value from -79 to -86

GuyM            1.4.0.43 - 06/08/2023
                - New BLE CPE version to 0xF10B/0xF20B (Update RSSI offset)

Dan             1.4.0.42 - 01/08/2023
                - Adding flush command to the rflib which will delete the
                  ongoing commands in the cpe when BLE degrant
                - Fixing Code review comments that related to the first rflib drop
                - BLE_OSPREY-124 - Adding flush function for the CoEX driver


BasharB         1.4.0.41 - 01/08/2023
                - New PHY_1.2.26.5.18.64
                - add in build output folders phy out and cpe out files.
                - fix in PhyApiDispatcher CalibrationExecuteCB

Dan             1.4.0.40 - 31/07/2023
                - Adding interrupt capability which will cancel the pta request
                  for the BLE when higher priority will receive the grant (Like phy)
                - OSPREY_LDB-1436 - Check degrant to BLE

GuyM            1.4.0.39 - 31/07/2023
                - [BLE] Add a check if BLE was enabled in INI file

BasharB         1.4.0.38 - 29/07/2023
                - INI Composer added to project
                - Update simplelink toolbox
                - Change PRCM Values
                - Lower SW_OVERHEAD for sleep_overhead_usec
                - IOMUX configuration Fix, high current consumption fix 

Katya:           1.4.0.37 - 27/07/2023
                - OSPREY_LDB-1115: support rf set to configuration rx filtering

GuyM            1.4.0.36 - 27/07/2023
                - Disable COEX for BLE_TEST_MODE
                - Increase BLE controller internal TX queues to 10 entries
                - New BLE version ble_rev_1.0.0.16

BasharB:        1.4.0.35 - 26/07/2023
                - OSPREY_LDB-1448: TX throughput degradation after allocating the FW_core_status at the top of db ram (g_Sock_DB_VecTable 
                - New PHY FW version x.x.26.5.17.64 
                - pmac rx scripter workaround removed:
                    override of PHY scrambler init value removed

GalM:           1.4.0.34 - 25/07/2023
                - OSPREY_LDB-1269: Remove channel switching flag in assoc req, beacuse we do NOT support it
                - OSPREY_LDB-1400: Add BLE baudrate and flow control to INI file
                - OSPREY_LDB-1211: Enable ocv paramter in hostapd.conf

Dana:           1.4.0.33 - 23/07/2023
                - OSPREY_LDB-1442: Probe requests are sent with minimum power - set power in 
                LtxpSend_setRateOverride to 0x1E
                - Workaround for systemInfo_IsFPGA() - always return 0
                - Don't perform calibration when OSPREY_MX is defined

BasharB:        1.4.0.32 - 20/07/2023
                - Katya: OSPREY_LDB-1420: Multirole STA-STA: STA1 transmit packets in STA2's channel
                - New PHY_x.x.26.5.16.64__Rx_deafness__Tx_CFG
                - AutoTB Changed Temporary, now needs External Trigger to TX
                
GuyM            1.4.0.31 - 19/07/2023
                - New BLE CPE version to 0xF10A/0xF20A (Update RSSI offset + Revert PG1.0 MCE version)

Dan             1.4.0.30 - 18/07/2023
                - Adding Coex feature, first drop
                - OSPREY_LDB-1328: [COEX] Add support for COEX entities (Internal BLE, WiFi, External SOC)
                - New BLE version ble_rev_1.0.0.15

Michal          1.4.0.29 - 18/07/2023
                - Replace link adaptation tables (from Matan BS)
                - Add two WA in PMAC TX scripter - 
                  - After several retransmision we drop to low rate for current packet
                  - Enable using LA tables smaller than 32 entries (empty lines marked by 0x3F)

Dana            1.4.0.28 - 16/07/2023
                - Remove phyapi_ChangeMode from the resume calibration function.
                - Don't perform calibration when using FPGA or PG1

GuyM:           1.4.0.27 - 06/07/2023
                - BLE: Remove UART clock gating settings
                - BLE makefile: Update ThickMac CFLAGS
                - OSPREY_LDB-1364: [BLE] Re-enable SDIO power management support
                - New BLE version ble_rev_1.0.0.14

BasharB:        1.4.0.26 - 04/07/2023
                - OSPREY_LDB-1325: Enable/disable 5Ghz according to device type (Driver and FW)
                - OSPREY_LDB-1317: add correct fw version numbering of INI file
                - OSPREY_LDB-1403: cannot transition to sleep/lpds in radio tool, Host interface is NOT closed
                - OSPREY_LDB-1404:CHANGE_PHY_MODE causes unexpected DSSM behaviour after sleep
                - Device Flavour handling add:
                  - new Parameter added in conf.h and will be part of the .conf file called enable_at_test_debug which will 
                    developers using give AT-TEST and NON TRIMMMED CHIPS to bypass flavour handling
                  - PG1/PG2 Handling
                  - Validating that Device Architecture is used with Valid FW (from Efuses)
                  - Protection Against illegal Band Usage (Based on Current Device Flavour)
                   
Dana:           1.4.0.25 - 04/07/2023
                - Bug fix in calibration for BLE
                - Bug fix in LtxpQueues_initRoleClientsTraffic - closing all RPU queues after init

Dana:           1.4.0.24 - 03/07/2023
                - Adding calibration for BLE upon init

Katya:          1.4.0.23 - 02/07/2023
                - Add section for core status in Linker

Dana:           1.4.0.22 - 25/06/2023
                - Ofir: New container build tool SWT-2.0.10

Katya:          1.4.0.21 - 22/06/2023
                - STA/STA Pwrmgt support

Noam:           1.4.0.20 - 21/06/2023
                - OSPREY_LDB-1302: Multicast Filtering
                - wrap PmacHw_SetRxFilter with disable/enable interrupts
                - wrap PmacHw_configureRoleResources with ASU_SetState(ASU_ROLE_STATE_RUNNING) in disable/enable interrupts

Dana:           1.4.0.19 - 20/06/2023
              - OSPREY_LDB-1292: APL Calibration - Joining RF set from aviram_rf_set_dev and calibration. 
              - Adding osprey debug command to FW for setting the calibration bitmap.
              - New PHY version PHY_1.2.26.5.9.64__listen_mode

BasharB:          1.4.0.18 - 07/06/2023
                -	Boot irq timeout when gtraces/code is added in fw.bin (issue turned out to be at driver)
                -	[OSPREY_LDB-1278] PLT assert - Texas Instruments JIRA (ti.com)
                -	[OSPREY_LDB-1281] [PLT} Limit RSSI / SNR value displayed in calibrator - Texas Instruments JIRA (ti.com)
                -	[OSPREY_LDB-1280] [RT] gtrace in wrong location,always prints error/warning message - Texas Instruments JIRA


GuyM:          1.4.0.17 - 07/06/2023
               - New BLE CPE version to 0xF109/0xF209 (PG2.0 bug fixes)

GuyM:          1.4.0.16 - 06/06/2023
               - Disable BLE power management for shared SDIO and ThickMac
               - OSPREY_LDB-1334: [RadtiTool] Init failed when trying to read CPE and MCE versions

Daniella:     1.4.0.15 - 05/06/2023
              -  OSPREY_LDB-1197 Max TX retries enhancements

BasharB          1.4.0.14 - 04/06/2023
              -  Tx transmit issue during PLT RX Issue Fix
              -  PHY partial modes allowed (RXLISTEN,RXONLY)
              -  New PHY Mode RFOFF when going to IDLE1
              -  New PHY version PHY_x.x.26.5.5.64__80MHz_replica__Tx_Calib__LoLeak-Iters__Rf-Off-mode
              -  Added PG2 Compilation
              -  New Partial mode command added

GuyM:         1.4.0.13 - 30/05/2023
              - OSPREY_LDB-1231: [BLE] Add support for BLE power management
              - BLE_OSPREY-107: [Osprey] Add support for Osprey Power Management
              - BLE_OSPREY-113: [RF-Driver] Add back power management support
              - New BLE version ble_rev_1.0.0.13

Daniella:     1.4.0.12 - 22/05/2023
              -  OSPREY_LDB-1309 Add PSD no PS POLL mode

GuyM          1.4.0.11 - 15/05/2023
              - New BLE version ble_rev_1.0.0.12

Michael:      1.4.0.10 - 15/05/2023
              - Init HCI UART only after BLE is enabled
              - Cleanup and consolidate BLE interface init code
              - OSPREY_LDB-1320: Fix Radio Tool ini data copy bug (Bashar)
              - PLT fix (Michal)

BasharB:      1.4.0.9 - 14/05/2023
              - OSPREY_LDB-1276	Add INI Download to Radio Tool
              - OSPREY_LDB-1277	change FW init flow to new Flow with INI Param

Noam:         1.4.0.8 - 09/05/2023
              - OSPREY_LDB-1249, OSPREY_LDB-960: DSSM/Radio State - FW code cleanup

GalM:         1.4.0.7 - 07/05/2023
              - cleanup for long key(save key on LinkHwEntryContainer for unicast or
                                     pRoleHwEntryContainer for broadcast instead of global struct).

Katya:        1.4.0.6 - 04/05/2023
              - OSPREY_LDB-1296: MR STA-STA: assert while robustness STA1 UP STA2 add connect remove

Noam:         1.4.0.5 - 04/05/2023
              - OSPREY_LDB-1247 - Add command for setting the TSF clock

Michal:       1.4.0.4 - 03/05/2023
              - OSPREY_LDB-1210 - Link adaptation - Fix DCM and ER upper support (considering peer capabilities and additional debug configuration).
              - Add dynamic capability to enable link adaptation debug traces.
              - IOP - Change default AX nominal padding to 2 to better support iop issues.
              - Add PLT support for changing PHY power mode (Aviram)

GalM:         1.4.0.3 - 27/04/2023
              - OSPREY_LDB-1235: AES_DMA - encrypt and decrypt long keys.

Katya:        1.4.0.2 - 23/04/2023
              - Receive first beacon before moving to AP Jitter learning state
              - Send probe request instead of null frame if Out Of Sync

Michal:       1.4.0.1 - 19/04/2023
              - PSD & APS bug fixes

GuyM:         1.4.0.0 - 10/04/2023
              - OSPREY_LDB-1272: Align the definitions for the bitmap described in the GNS status register
              - Replace deprecated AESCTRDRBG_getBytes with AESCTRDRBG_generateKey
              - New BLE version ble_rev_1.0.0.11

BasharB:      1.3.0.45 - 21/03/2023
              - 	OSPREY_LDB-1226 [RT][Calibrator] problem with Tx packet length in legacy preambles

Katya:        1.3.0.44 - 22/03/2023
              - OSPREY_LDB-1239: MR STA-STA: Assert while STA1 UP STA2 Connected to OPEN AP
              - OSPREY_LDB-1243: Scan: while STA connected we aren't scan all channels

BasharB:      1.3.0.43 - 21/03/2023
              - 	OSPREY_LDB-1266 Manual Calibration - REVERSE2BYTES Issue
              -	  OSPREY_LDB-1267 after starting tx tone, we cant stop TX


BasharB:      1.3.0.42 - 20/03/2023
              - 	New PHY_1.1.24.5.39.62

BasharB:      1.3.0.41 - 15/03/2023
              - 	OSPREY_LDB-1248	- APUT: crashes few seconds after WiLink8 connected
              - 	OSPREY_LDB-1252	- Auto PS: SUT doesn't exit PS for TX traffic
              - 	OSPREY_LDB-1228	- PLT - add TX tone command
              -   OSPREY_LDB-1245	- [FW] mark important GTRACES that is used by RT AUTO

BasharB:      1.3.0.40 - 12/03/2023
              - 	OSPREY_LDB-1228	PLT - add TX tone command
              - 	OSPREY_LDB-1226	- [RT][Calibrator] problem with Tx packet length in legacy preambles
              - 	OSPREY_LDB-1227	- [RT][Calibrator]logger prints only packet length end range even when packet length is const


GalM:         1.3.0.39 -
              - OSPREY_LDB-1235: AES_DMA - encrypt and decrypt long keys.

Katya:        1.3.0.38 - 09/03/2023
              - OSPREY_LDB-1230: Send Probe request on role start
              - OSPREY_LDB-1191: [MR][Sanity] Fails to receive any beacon right after STA role starts

RoeyB:        1.3.0.37 - 08/03/2023
              - Update Osprey_Dispatcher for Wlan_get / Wlan_set commands to unified commands.
              - DRBG service: Reverse the stack protection address for the seed.

AviramD:      1.3.0.36 - 07/03/2023
              - New PHY version 1.1.24.5.37.62 for CHIP/FPGA (1.1.24.5.137.62 for FCS)

Michal:       1.3.0.35 - 06/03/2023
              - OSPREY_LDB-757 - Link adaptation wa - Fix for HW counter wrap around (ldb pg1 only). Fixed in PMAC TX scripter.
              - OSPREY_LDB-1161 - Avoid 26 tones RU tb (under FW configuration). Fixed in PMAC RX scripter.
              - WA for hw BUG OSPREY_SILICON-1081 - In TB packet, after MU RTS trigger frame, TX scrambler init value should be taken from RX.
                Fixed in PMAC RX scripter.
              - LA short term policy  - packets transmitted in low rates due to short term policy (qos null, ps poll, etc) mislead the LA. Fix in the PMAC TX scripter
                (avoid updating link adaptation upon short term policy different than zero).
              - OSPREY_LDB-1223 -  TX exchange asserts for frames with Invalid link. THICK MAC tests crash upon connect - disconnect (invalidation of link).
              - OSPREY_LDB-1215 - OM fields (disable MU UL & disable MU UL DATA) were in wrong polarity  - AX certification test.

GuyM:         1.3.0.34 - 03/03/2023
              - Update file names to align to BLE inclusion naming convention
              - New BLE version ble_rev_1.0.0.10

Aviram Dick:  1.3.0.33 - 02/03/2023
              - OSPREY_LDB-1229: LSI (long sleep interval support)

GalM:         1.3.0.32 - 02/03/2023
              - OSPREY_LDB-1233 : LRXP and LTXP change for external encrypt/decrypt
              
Katya:        1.3.0.31 - 27/02/2023
              - ThickMac role enable fix

Katya:        1.3.0.30 - 22/02/2023
              - STA - STA support
              - HW resource management

BasharB:      1.3.0.29 - 20/02/2023
              - New FPGA version va22 , h files updated

BasharB:      1.3.0.28 - 19/02/2023
              - New FPGA version va21, h files updated
              - New PHY for FPGA/CHIP (PHY_1.1.23.5.31.62__Tx_Calibrations_fixes (1.1.23.5.131.62 for FCS))

GuyM:         1.3.0.27 - 19/2/2023
              - Add build flags to disable features for BLE compilation
              - Allow mixing of legacy/extended command status between ADV and Scan
              - New BLE version ble_rev_1.0.0.9

BasharB:      1.3.0.26 - 15/02/2023
              - AC VHT Support added
              - 5GHZ integration
              - OSPREY_LDB-1221: [Calibrator][RT] PG doesnt transmit Tx packets with AGG with rates MCS5 and above

BasharB:      1.3.0.25 - 03/2/2023
              - Aggregation Added to PLT Transciever ( const length up to 16000 bytes) (hardware delay only)
              - PLT transciever  can now do internal loop (send in a range of lengths) in continous mode

GuyM:      1.3.0.24 - 03/2/2023
              - Set eFuse BD address when executing system reset
              - New BLE version ble_rev_1.0.0.8

Aviram:    1.3.0.23 - 2/2/2023
              - Added parameters for PLT “rx_start” command
              - OSPREY_LDB-1213: ssidType field in ConnScanSSID_T has a wrong type
              -	Katya: Assert on phy tune failed command
              -	Dana: OSPREY_LDB-1167: Tx Fragmentation was disabled in FW

BasharB:   1.3.0.22 - 30/1/2023
              - New PHY version  1.1.23.5.21.62
              - New FPGA Version ver_a13, Registers updated

GuyM:      1.3.0.21 - 19/1/2023
              - Add Osprey System Reset Command to BLE Utils
              - New BLE version ble_rev_1.0.0.7

Katya:     1.3.0.20 - 16/1/2023
              - OSPREY_LDB-1203: WA for HW bug handling 11ax + gi 0.8 + ltfx4 - prevent some HW suspend
              - few comments

GuyM:      1.3.0.19 - 10/1/2023
              - OSPREY_LDB-1176: [BLE] Add support for secured BLE link
              - Add option to disable BLE Secure Connection
              - Update ClockP_tickPeriod value to Osprey system ticks value
              - Add new BLE define QUAL_TEST
              - New BLE version ble_rev_1.0.0.6

BasharB:   1.3.0.18 - 08/1/2023
              - Add ifdef for thick mac compile

BasharB:   1.3.0.17 - 08/1/2023
              - Choose no rta mode trim value
              - OSPREY_LDB-1198: BA TX Session not opened after immediate disconnect and connect

BasharB:   1.3.0.16 - 04/1/2023
              - IO drive strength (DS) setting Set in  Special Ram for DLL1/Non FCS Container (for RF team)
              - Aid parameter added to WIFI RX in radio tool and calibrator
              - AccomulatedDataRssi Now is calculated based on good packets only
              - OSPREY_LDB-1182: PLT - Packet Len > 4000 doesn't work
              - OSPREY_LDB-1041: PG1 - plt Rx_GET_STATS invalid results + warning message
              - OSPREY_LDB-1181: PLT - Max Tx power doesn't work
              - OSPREY_LDB-1195: [RT][PLT] 11ax TB doesn't work, and when configuring 11ax tb Transceiver stops working

Michael:   1.3.0.15 - 04/1/2023
              - Automatic slow clock source selection (internal LFOSC / external)
              - Periodic calculation of slow clock frequency
              - Force Dig LDO to remain at 1.1V in ELP to prevent LFOSC drift
              - Resurrect osi_uDelay based on TSF counter 
              - Move Upper-MAC submodule to PG2.0 (da8a9d8d675...)

EladZ:     1.3.0.14 - 02/01/2023
              - OSPREY_LDB-1187 (RAM_BM): Set RAM BM to run from main context at startup

BasharB:   1.3.0.13 - 27/12/2022
              - OSPREY_LDB-127: Problem with RATE POLICY in connection between Osprey-SUT to AP and APUT to STA
              - Rx Packet Ram: descriptors for management frames lowered to 1.5k

Michael:   1.3.0.12 - 27/12/2022
              - OSPREY_LDB-1185: Fix missing padding in DSSM Rx messages              

GuyM:      1.3.0.11 - 26/12/2022
              - OSPREY_LDB-1174: [RadioTool] BLE RX get stuck when running with store payload mode
              - New BLE CPE version to 0xF108/0xF208 (Osprey memory range + bug fixes)
              - New MCE version 0x0115/0x0125 (PER floor at 2M fix)
              - Increase MAX_PDU_SIZE to 255 by default in opt file

EladZ:     1.3.0.10 - 22/12/2022
              - OSPREY_LDB-1113 (RAM_BM): Detect if running from FPGA at runtime for seed generation

Michael:   1.3.0.9 - 19/12/2022
              - Fix regression in Osprey.ccxml
              - OSPREY_LDB-1162: SW workaround for DSSM reset HW bug

EladZ:     1.3.0.8 - 18/12/2022
              - OSPREY_LDB-1113 (RAM_BM):  Generate random seed for BLE secured link
              - OSPREY_LDB-1142 (RAM BM):  Added system info command support via DSSM (transport layer)
              - OSPREY_LDB-1142 (WSOC FW): Added system info command support via radio state

Aviram:    1.3.0.7 - 15/12/2022
              - OSPREY_LDB-1133: Static calibration - When the device is not in the file, we get 'problem with sync pattern' error
              - Katya: OSPREY_LDB-1160: Assert:Error type: PMAC TX interface error. Error subtype: Length not ready
              - Katya: OSPREY_LDB-1157: Assert:Error type: PMAC TX interface error. Error subtype: Too long full fifo.
              - Katya: OSPREY_LDB-1145: Thick MAC PG2 MR + SUT TCP RX - SUT disconnect
              - Michal: OSPREY_LDB-1173: UPSD: Ping RX - SUT often doesn't send trigger packets for long periods of time

GuyM:      1.3.0.6 - 13/12/2022
              - OSPREY_LDB-1171: [BLE] Add support for BLE TX output power level control
              - New PHY Version 1.1.22.5.10.62
              - New BLE CPE version to 0xF107/0xF207 (TX Power Support + 255B RX Support)
              - New BLE version ble_rev_1.0.0.5

eladso:    1.3.0.5 - 06/12/2022
              - OSPREY_LDB-1155: Added shared SDIO for BLE support
              - OSPREY_LDB-1169: Added restore WL_PHY_PLCP__PLCP_MIN_LEN__MEM_MIN_LEN reg after sleep
              - New BLE version ble_rev_1.0.0.4

Aviram:    1.3.0.4 - 30/11/2022
              - New PHY Version PHY_1.1.22.5.6.62
              - New FPGA Version L3_Va12

Aviram:    1.3.0.3 - 29/11/2022
              - OSPREY_LDB-1156: Assert: Assert while trying to connect to secured AP and the AP radio is off 

GuyM:      1.3.0.2 - 23/11/2022
              - OSPREY_LDB-1154: [BLE] Add option to read MCE version
              - OSPREY_LDB-1147: [BLE][RadioTool] RX with no payload included empty data
              - New PHY Version 1.1.22.5.1.62
              - New BLE CPE version to 0xF106/0xF206 (updated channel tune API)
              - New MCE version 0x0114/0x0124 (BLE RX Radio Tool Fixes)
              - New static_calibration.bin file v30

GalM:      1.3.0.1 - 17/11/2022
              - OSPREY_LDB-1132 - MMU address translation error, when AP changes security configuration

GuyM:      1.3.0.0 - 15/11/2022
              - Add new BLE define CONTROLLER_ONLY
              - Rebase and push BLE submodule to ble-agama mainline (6.30.00 Q3/22 release)
              - Merge RFLIB submodule from rflib mainline (6.30.00 Q3/22 release)
              - New BLE version ble_rev_1.0.0.3

Dana:      1.2.0.14 - 8/11/2022
              - Minor version number incremented for FCS 2

Dana:         1.1.0.14 - 6/11/2022
              - PHY API TIMEOUT , ChangeMode AbortNeeded Changed to True

BasharB:      1.1.0.13 - 27/10/2022
              - New PHY for FCS and DLL1 (Both are compiled)
              - Nw PHY for FPGA also
              - New FPGA Version L3_Va08

BasharB:      1.1.0.12 - 27/10/2022
              - Static Calibration Fix
              - Static Calibration new Default values

Aviram Dick:  1.1.0.11 - 27/10/2022
              - Enabled ROC and CROC commands
              - Katya: OSPREY_LDB-1112: Assert on LTXP Queues
              - Michal: PLT autoTB fix

BasharB:      1.1.0.10 - 23/10/2022
              - BasharB: Beacon Timestamp Fix (michal scripter)
              - New PHY Version 1.1.22.4.0.61

GalM:         1.1.0.9 - 12/10/2022
              - OSPREY_LDB-1108 - Enable PMF in STA an AP role


Michael:      1.1.0.8 - 02/10/2022
              - Moved BLE code into submodules
              - Removed Bootman assets from active submodules 

Katya:        1.1.0.7 - 02/10/2022
              - Merge p2p_release into latest
              - Fix NAV protection wrap-around issue in MR
			  - new static calibration file v21
GuyM:         1.1.0.6 - 2/10/2022
             - New PHY version 1.1.22.4.14.60
             - Update BLE CPE version to 0xF105/0xF205 (BLE RX Radio Tool Fixes)
             - Fix the BLE_TEST_MODE configuration build
             - Apply back the stack protection to chip


GuyM:         1.1.0.5 - 29/09/2022
            - OSPREY_LDB-1096: [BLE] Add BLE Controller Test Mode Build configuration (LL_TEST_MODE)

Bashar:       1.1.0.4 - 28/09/2022
            - PHY PLT manual calibration command added to radio tool
            - M3M0_IF_VERSION (22) change, validty bit map returned from PHY
            - Calibration validtybit map sent to Radio tool
            - OSPREY_LDB-127 -Problem with RATE POLICY in connection between Osprey-SUT to AP and APUT to STA
            - Michal PHY Unify for CHIP/FPGA
            - New PHY version 1.1.22.4.13.60

Michal:       1.1.0.3 - 18/09/2022
            - Extend static calibration information to 816 bytes (supporting 4 analog index)
            - M3M0_IF_VERSION (21)
            - Requires updates PHY FW & Driver

Katya:      1.1.2.2 - 20/09/2022
            - OSPREY_LDB-1102
            - OSPREY_LDB-1101
			
Katya:      1.1.1.2 - 13/09/2022
            - Add support for P2P Device and P2P GO

GuyM:       1.1.0.2 - 31/08/2022
            - OSPREY_LDB-1002: [BLE] Add LEGACY_CMD option to support for legacy BLE commands
            - New BLE version ble_rev_1.0.0.2

BasharB:       1.1.0.1 - 29/08/2022
            - OSPREY_LDB-639: APUT fragments multicast traffic
            - New PHY for FPGA (fixed BLE Tx)


GuyM:       1.1.0.0 - 24/08/2022
            - OSPREY_LDB-1072: MCU attach modifications to work with AM2434_ALX running Bluetopia stack
            - New BLE version ble_rev_1.0.0.1


BasharB:          1.0.0.128 - 22/08/2022
New PHY version
Driver and calibrator new ver

BasharB:          1.0.0.127 - 15/08/2022
                New Calibrator ver 1.0.08 - get tx params print fix
                OSPREY_LDB-961 APUT use unsupported rate for Multicast traffic and use protection mode when configured to B only
                OSPREY_LDB-607 Profiles with hidden SSID: SUT sends Probe_req only to the first hidden profile and connect only to him
                OSPREY_LDB-581 AP role - Basic rates are not configured properly by the Linux driver
                OSPREY_LDB-1060 Beacon rate is always 1
                OSPREY_LDB-127 Problem with RATE POLICY in connection between Osprey-SUT to AP and APUT to STA



GuyM:       1.0.0.126 - 15/08/2022
            - OSPREY_LDB-1073: [BLE] Add BLE revision number to release notes
            - OSPREY_LDB-1074: [BLE] Remove no-function-sections from Makefile
            - OSPREY_LDB-1075: [BLE] Fix compilation errors with BLE BRINGUP_TESTS_BLE

BasharB:          1.0.0.125 - 07/08/2022
            - New PHY version Osprey PHY FW 1.1.20.4.0.59 
            - YuvalM:    CC33XXPHY-506: WiFi Tx doesn't work after BLE Tx
            
BasharB:          1.0.0.124 - 04/08/2022
            - efuse mac address fliiped  Thick Mac Fix

BasharB:          1.0.0.123 - 04/08/2022
            - efuse mac address used in Thick Mac
            - BLE modifications to enable provisioning process with BlueZ

BasharB:          1.0.0.122 - 03/08/2022
            - linker updated for changes in the thick mac to load BLE properly corresponding to the “no common” chang
            - Thick MAC updated cproject with BLE compilation flags 
            - OSPREY_LDB-1046: [BLE] Add no-common compiler flag to Makefile
            - OSPREY_LDB-1048 (RAM_BM): Bypass write attempts to RAM BM code section

Michal:          1.0.0.121 - 02/08/2022
            - new phy version is integrated including usage of calibration values from static calibration file.
            - Phy version for PG1 - 1.1.20.4.0.58
            - Phy version for FPGA PG2 - 1.2.20.4.0.58 (for FPGA 908)
            - Static calibration file - v5
            - This version works fine with calibrated boards.
              otherwise - performance are worse than previous versions.

BasharB:         1.0.0.120 - 31/07/2022

            - fuse rom struct version fix
            - Static calibration fix – in case of mac address not found in the file or zero – use the default calibration (address ff:ff:ff:ff:ff:ff)
            - katya usleep addittion
            - OSPREY_LDB-1043 - Driver - FPGA Random generated mac address isn’t locally administrated
            - OSPREY_LDB-606 – Profiles with hidden SSID – Sometime receiving assert “periodic scan request while periodic scan is running” – The driver send scan stop and them immediately scan start (after ~5ms) without waiting for the scan stop to be completed. 
            - OSPREY_LDB-1044 - [IOP-AP]ping fails with some specific dongles due to arp problems with pmf 

BasharB:         1.0.0.119 - 24/07/2022
            - OSPREY_LDB-1038 Calibrator - get_rx_stats command fails
            -  New Radio Tool Command - CMD_GET_PHY_VERSION, returns PHY version ,CPE ,M3M0 version
            -  AP role Gets Locally adminstrated mac address for fuse rom struct ver 0x00 (BLE gets efuse mac +1 in ver 0x00 and efuse mac +2 in ver 0x01)
            -  EladZ:  OSPREY_LDB-1032 (RAM_BM): Update shared system info ()
            -  EladZ:  OSPREY_LDB-885  (RAM_BM): Debug Image Support
            -  EladZ:  OSPREY_LDB-886  (RAM_BM): Debug request Support
            -  Katya: OSPREY_LDB-931 Robustness of scan during TCP TX traffic, after ~11 Hours. received assert of error: NAV protection.
            -  Katya: OSPREY_LDB-1023 assert after 3 hours when running TCP Rx and scanning every 30 sec

GuyM:         1.0.0.118 - 22/07/2022
            - OSPREY_LDB-1035: [BLE][CPE] Add CPE support for PG2.0
            - Update BLE CPE version to 0xF104/0xF204 (Add PG2 support)

Michal:       1.0.0.117 - 18/07/2022
            - Static calibration fix - copy the data upon command, mark it as valid.
              Phy is reponsible to fetch the information when valid (upon warm init or tune command).
            - Note that phy version that is delivered as part of this version does not use the static calibration info !

BasharB:      1.0.0.116 - 07/07/2022
            - BLE is off by default, needs to be turned on from host/radio tool
            - BLE Enabled By Host through script 
            - Patches and Linker (DAN)
            - OSPREY_LDB-844: Stability: Assert due UDP TX traffic with open security stopped after 6 hours - PHY API Timeout!
            - OSPREY_LDB-932: ASSERT ERROR PhyApiDispatcher after PHY API Timeout
            - OSPREY_LDB-949: MR. Station AP, assert after 50 min of robustness test: AP Idle, Station TCP Rx with application scan every 10 sec ('Error! PHY API Timeout')
            - OSPREY_LDB-740: Robustness: Reconnection in loop after 773 loops received assert of ERROR! PHY API Timeout
            - OSPREY_LDB-835: PLT packet Length
            - OSPREY_LDB-937: Osprey: Low TP in rx (TCP/UDP) when 11AX is enable
            - increase heap in ram bootloader (ELAD)
            - Adding main.elf for BLE and Thick Mac BLE

Aviram:       1.0.0.115 - 11/07/2022
            - Aviram: Added "static_calibration.bin" file to a static location <osprey_repo>/firmware/MainBuild/integration/phy/
            - Aviram: As part of the FW build process, the static calibration file will be sent to the out folder, and can be directly downloaded to
                      host via SCP.
            - GuyM:   OSPREY_LDB-1003: [BLE][RadioTool]Update CMD_BLE_TX_TEST into CMD_BLE5_TX_TEST
            - Michal: PHY version unified for chip & FPGA – 1.1.19.4.1.57
            - Michal: RX only & listen mode are NOT supported for chip and FPGA
            - EladZ:  Added WSoC FW critical version section
            - EladZ:  Set WSoC FW container type to operational (works on debug devices as well)
            - Katya:  OSPREY_LDB-984: Multirole: assert after 48 hours of Idle connection WPA2 on AP and STA role
            - Katya:  OSPREY_LDB-999: Thick MAC: MultiRole-STA+AP+Scan caused FW crash

BasharB:      1.0.0.114 - 07/07/2022
            - 57 PHY for chip
            - Driver cleanup
            - Mac From fuses
            - OSPREY_LDB-659	WLAN0 InterfaceMAC Address is changed each reset
            - RTTT interface to be enabled by default


DanH:         1.0.0.113 - 04/07/2022
            - Adding ability to compile ble with thick mac (PG1.0 and PG2.0)
            - Adding initialization for specific thick mac parameters (pg1.0)
            - Adding RegDomain feature for station mode
            - Changing wifi tool box in order to support with and without rom

BasharB:       1.0.0.111 - 29/06/2022
            - Offical FW Release for CHIP
            - PHY  Version 56
            - makefile now generates 2 containers, one for CHIP and one for FPGA
            - Same FW Version and release for both chip and FPGA
            - PG2 for FPGA added
            - Version 908 for FPGA with supplicant Rom 
            - PHY API Timeout Fix
            - PG1/PG2 and FPGA version can be checked within software 
            - Scripter takes into consideration Chip version (scripter fix for PG2)
            OSPREY_LDB-593:Connect scan while hidden profile defined: measured time from starting scan to connection completed is ~6 sec
            OSPREY_LDB-607:Profiles with hidden SSID: SUT sends Probe_req only to the first hidden profile and connect only to him
            OSPREY_LDB-996: [BLE][RadioTool] Add support for CMD_BLE_TX_TEST command


BasharB:       1.0.0.110 - 23/06/2022
            - Update BLE CPE version to 0xF103 (WA for BLE ADV stuck)
            - Driver modified for working on Chip and FPGA
            - SDIO Product ID modified for trimmed ( works on both )
            - OSPREY_LDB-993: Radio Tool Get TX params works only first time.
            - OSPREY_LDB-992: WLAN PLT OFF
            - OSPREY_LDB-994: [BLE][RadioTool] Add support for ADV_AUX without extended header
            	OSPREY_LDB-988: Profiles-Added 10 hidden profiles(one broadcast) and stop and start STA received Assert

GalM:          1.0.0.109 - 21/06/2022
            - OSPREY_LDB-990: Enable PMF in WPA2 - BIP decryption with aes_dma_driver
            - OSPREY_LDB-991: Enable WPA3

BasharB:       1.0.0.108 - 20/06/2022
            - OSPREY_LDB-986: Periodic scan stuck after first attempt
            - OSPREY_LDB-982: Periodic scan - periodic scan stops by the FW after getting a match
            - OSPREY_LDB-633: PERIODIC_SCAN_REPORT_EVENT_ID after cycle
            - Basic Scan and Scan adapter are now started after scheduler is up and is full state


BasharB:       1.0.0.107 - 15/06/2022
            - OSPREY_LDB-969:[MR] Assert: periodic scan request while periodic scan is running 
            -  FPGA VERSION UPDATE - O9 - 907
            - PHY Version Update to 55 (FPGA_905_1.0.19.3.5.55_PG2)
            - OSPREY_LDB-983	- PLT Disable Crashes


GuyM:       1.0.0.106 - 12/06/2022
            - OSPREY_LDB-976: [BLE][RadioTool] Fix the CMD_BLE5_GENERIC_RX packet reception process
            - Update BLE CPE version to 0xF102 (Fix the CMD_BLE5_GENERIC_RX flow to enable RX after RX)
            - Open XDS110 debugger option

Bashar:      1.0.0.105 - 06/06/2022
            - OSPREY_LDB-980: STATIC CALIBRATION file sent from Radio tool to driver dispatcher handler


Katya:      1.0.0.104 - 06/06/2022
            - OSPREY_LDB-975: MR: fail to complete start role when SCHED state is FULL
            - OSPRET_LDB-931: NAV protection - do not use navEndTsf in asu if not valid
            - changed macros into functions
            - Static calibration radio tool bug fix - status after success sent to radio tool @Bashar

Bashar:     1.0.0.103 - 02/06/2022
Static calbration radio tool bug fix - status after success sent to radio tool
New trigger based TX params added for transceiver
New Calibrator commands added for ti_utils calibrator  (set_tx, set_tb_tx, get_tb)
Calibrator interface made more user friendly
               -ROM_BM: OSPREY_LDB-973: Write reset cause to PMAC_RX__RX_SPARE_REG1
               -OSPREY_LDB-951: APUT - retransmittion qos-null
               -OSPREY_LDB-974: SW Tag Changed to INVALID despite not being scan tag after stop scan

EladZ:     1.0.0.102 - 01/06/2022
           - Added ROM_BM and RAM_BM projects.
           - ROM BM (PG2 Updates):
               - OSPREY_LDB-739: Updated signature verification return value to be more secure
               - OSPREY_LDB-863: Removed GPADC samples traces
               - OSPREY_LDB-967: Added OTP SW CRC enable fuse field support
           -RAM BM
               - OSPREY_LDB-923: Workaround for Host/NAB race condition (MichaelN)
               - OSPREY_LDB-940: Reduced DSSM container download log traces
               - Set MEMSS mode as the WSoC FW minor version (temporary)
           - Updated container builder tool to version 1.0.5 (Danil)
               -Supplicant ROM section is discarded from container

GuyM:      1.0.0.101 - 26/05/2022
           - OSPREY_LDB-918: [BLE][RadioTool] Add command complete support
           - Update BLE CPE version to 0xF101 (custom channel support)

Katya:     1.0.0.100 - 24/05/2022
           - OSPREY_LDB-849: assert pwrmgt_bet.c:576 beacon length < 0
           - OSPREY_LDB-968: [MR] Presence adapter not initializes values for AP role
           - Ap jitter - fix use sta based role indexing instead of wlan role indexing.
           - OSPREY_LDB-964: Add support for RPU Tx flow control
           - Return 3ms delay to TX_FLOW_CONTROL in asu
           - Thick MAC: Changing the default mac address to different addresses for different roles
           - Thick MAC: when closing BA session - disabling closing link request if not needed

GuyM:      1.0.0.99 - 19/05/2022
           - OSPREY_LDB-955: [BLE] Fix the BLE BRINGUP_TESTS_BLE compilation errors
           - Verify CPE_PCP_IF_VERSION value on init
           - Update codesize.py script with Max Overall Code and Data
           - Rebase RF driver to version 6.20.00.09
           - Fix warnings in RF driver

Bashar     1.0.0.98 - 17/05/2022
          - OSPREY_LDB-735 - SUT: Assert after TCP RX stopped during a night run 
          - OSPREY_LDB-604 - Received an RX frame with tag 2 (or 1) (for scan), after scan was completed (causing assert) 
          - OSPREY_LDB-928 - APUT doesn't open BA session in TX direction while running traffic to a connected STA
          - REGISTER_DB_ENTRY_INIT(PMAC_TX__TX_BF_CFG__ADDR) added


Bashar     1.0.0.97 - 12/05/2022
           - OSPREY_LDB-803: long suspend after ADDBA req
           -  System Info Address from the Shared RAM Brought and Giving the MAC address back to the radiotool for Static Calibration File
           - 	Getting the static calibration data from Radio tool to FW and then PHY
           -  Static Pattern from statc calibration file  is not Sent To FW Anymore
           -  Katya pRoleCtx

Katya:     1.0.0.96 - 10/05/2022
           - OSPREY_LDB-931 - ERROR: NAV protection low 32, high 32, invalid
           - OSPREY_LDB-944 - Fix presence adapter to remove request from scheduler while isStopping role bit
           - OSPREY_LDB-942 - fix suspend Client Traffic links bitmap in LTXP QUEUE
           - OSPREY_LDB-933: -Added 20uSec delay when radio state is idle (EladZ)
           - closed trace Group LTXP_QUEUE
           - flush logger buffer in idletask on each iteration

Aviram     1.0.0.95 - 02/05/2022
           - OSPREY_LDB-911: Static calibration. Added support for static calibration, in cmd configure.
           - Katya: added a notify on suspend in TX flow control and removed flow control delay
           - OSPREY_LDB-933: Fix debug reset causing assert -- EladZ
           - Michael: Fixed a typo in "fw_update_win10.bat" file

Bashar     1.0.0.94 - 25/04/2022
           - OSPREY_LDB-913: SUT is sending ADDBA while connecting to Cisco AP configured to n rates-disabled
           - Aligned shared PHY log header with M0 executable.

Katya      1.0.0.93 - 19/04/2022
           - OSPREY_LDB-916: assert in bpoolv
           - OSPREY_LDB-914: assert in RPU CTS NAV - due to max tx retries
           - OSPREY_LDB-919: Configure set PSD mode only in STA mode
           - add support for Tx flow control - Michal
           - OSPREY_LDB-922: Add support for reading CPE M0 FW version - @GuyM

GuyM       1.0.0.92 - 17/04/2022
           - OSPREY_LDB-917: Support DSSM for Radio-Tool
           - OSPREY_LDB-902: Add BLE commands support for Radio Tool

Michael    1.0.0.91 - 14/04/2022
           - Added support for host FW

Aviram     1.0.0.90 - 11/04/2022
           - OSPREY_LDB-694: APUT- assert while connecting the 5th STA
           - OSPREY_LDB-912: gLogger assert fix

GuyM       1.0.0.89 - 27/03/2022
           - OSPREY_LDB-907: [BLE][HCI] lastAppOpcodeIdxSent variable should be added back
           - OSPREY_LDB-908: [BLE] Add ble main h file to support both functional and test modes
           - OSPREY_LDB-910: [BLE] Avoid the BGET assert for zero length malloc

Aviram:    1.0.0.88 - 21/03/2022
           - OSPREY_LDB-745: Optimize GTrace code that writes to UART

GalM:      1.0.0.87 - 16/03/2022
           - Update PHY 1.0.18.3.10.54

YoavBY     1.0.0.86 - 06/03/2022
           - OSPREY_LDB-845: DPL work for BLE iCall
           - OSPREY_LDB-850: Add BLE OSAL/ICALL/APP files to osprey env
           - OSPREY_LDB-858: Add BLE bringup tests infra to osprey env
           - Update osprey_setup.py script to v1.0.4

Michal:    1.0.0.85 - 24/02/2022
            - OSPREY_LDB-820 - Memory corruption upon RX BA release
            - Maintain security header on packets sent to host (aligned driver is required)
            - OSPREY_LDB-852 - BB timeout enabled - remove the assert due to 11n long packets issue

Michael     1.0.0.84 - 23/02/2022
           - Changed linker script to lock all ROM global vars in place
           
Katya:     1.0.0.83 - 22/02/2022
           - New non-blocking Sched State module
           - Changes in role start and role stop. Role Stop/Start handling through Sched State

GuyM:      1.0.0.82 - 22/02/2022
           - OSPREY_LDB-864: Add the BLE NPI module to Osprey Environment
           
Aviram:    1.0.0.81 - 21/02/2022
           - OSPREY_LDB-810: Added statistics for 'AX: Too short trigger frame - duration calculation error'

Dan:       1.0.0.80 - 09/02/2022
           Upper mac - Adding Seamless functions

GalM:      1.0.0.78 - 26/01/2022
           - Update FPGA ver816

Michael:   1.0.0.78 - 26/01/2022
           - Enable multiple new warnings during FW build (Wall, Wextra)
           - Fix all the warnings emitted due to the above
           - Cleanup of CCS's .cproject settings file
           - Calls to BLE and Upper MAC makefiles are integrated into FW's makefile as externals libs
           - Updated VScode's build scripts

Dan:       1.0.0.77 - 25/01/2022
           Upper mac fixes
           - OSPREY_LDB-819
           - OSPREY_LDB-824
           - OSPREY_LDB-828

Katya:     1.0.0.76 - 25/01/2022
           - Fix OSPREY_LDB-839

GuyM:      1.0.0.75 - 24/1/2022
           - OSPREY_LDB-846: Add BLE RF as symbolic links to Osprey repo
           - Update osprey_setup.py script to v1.0.3

GuyM:      1.0.0.74 - 23/1/2022
           - OSPREY_LDB-831: Create separated makefile for BLE
           - Update osprey_setup.py script to v1.0.2

YoavBY:    1.0.0.73 - 20/1/2022
           - Build BLE controller + minimal RF driver (removed all refs to oscillators, power, AON, etc.)

GuyM:      1.0.0.72 - 19/1/2022
           - Fix the BLE build option
           - Update osprey_setup.py script to v1.0.1

GuyM:      1.0.0.71 - 18/1/2022
           - OSPREY_LDB-829: Update Osprey makefiles to compile BLE LL source files
           - OSPREY_LDB-833: Add Osprey post clone setup script

Michal:    1.0.0.70 - 18/1/2022
            - Fix BET stop role handling
            - Fix BET threds deadlock (in case of call through LRXP)
            - Fix missing aggregation in AP role LDB-825

DanH:      1.0.0.69 - 16/1/2022 (THICK_MAC release)
            - Adding Hooks capability to the upper mac
            
Michael:   1.0.0.68 - 13/1/2022
            - Enabling -Oz optimizations for FW build 
            - Adding VScode configuration files
            - Update paths in delivery archive script
            - Remove some unused files

GalM:      1.0.0.67 - 10/1/2022
            - OSPREY_LDB-764: PLT assert after start tx

RazB:      1.0.0.66 - 5/1/2022 (THICK_MAC release)
            - Adding support for PMF/WPA3
            - Fixing scan bugs:
                - OSPREY_LDB-797
                - OSPREY_LDB-799
                - OSPREY_LDB-780
            - Moving libc to ROM section (see .cmd file)
            - Adding support for WPA3 in scan results

YoavBY:    1.0.0.65 - 3/1/2022 (1st release of 2022)
            - 1st integration of the BLE code into the osprey_int branch (new baseline branch for Osprey BLE development)
            - new build configuration (BLE)
            - new memss configuration options by setting XX in #define MEMSS_MODE_SELECTED_XX (where XX can be 0/5/8/11) in link_33xx_cortex.cmd linker file
              code and packet RAMs will be configured accordingly
            - Removal of redundant RF Driver code to remove dependencies from BLE core SDK
            - Rebase to latest rflib repo
            - fix from Michal - set MEM_TX_LA_CFG_BEACON_RSSI_EN to 0 (cannot rely on beacon rssi jumps)

Katya      1.0.0.64 30/12/2021
            - OSPREY-696

Michael    1.0.0.63 - 26/12/2021
            - Fix GTRACE aliasing bug

Michael    1.0.0.62 - 22/12/2021
            - Add ITM/TPIU module

RazB:      1.0.0.61 - 21/12/2021
            - Adding UpperMAC to ROM code, compilation by THICK_MAC_COMPILE FLAG
            - Reorder buffer for thickMAC compile
            - New uppermac command 
            - ThickMAC compiled by FLAVOR=THICK_MAC  see ROM_compile.mak

Michael    1.0.0.60 - 21/12/2021
            - Attached PACKED attribute to BeaconTimBitmapControl_t 
                (should have been done in previous version)


Michael    1.0.0.59 - 19/12/2021
            - Attached PACKED attribute to key bitfield structs
            - Disable Rx suspend for long data broadcast frames

Michael    1.0.0.58 - 15/12/2021
            - Migration to TI CLang 1.3.1 LTS
            - Many-many files were modified for this to work 

Katya:     1.0.0.57 - 14/12/2021
            - OSPREY_LDB-744
            - OSPREY_LDB-732
            - new link adaptation params Power + TX Combo All
            - OSPREY_LDB-746

Michael:   1.0.0.56 - 12/12/2021
            - Fix dynamic clock-gating overwrite 
            - Fix ELP sleep time calculation which had a ~6% error 
            - Fix multiple cosmetic issues after CR to the ELP code

Katya:     1.0.0.55 - 06/12/2021
            - fix in link adaptation configuration parameter
            - OSPREY_LDB-743

Katya:     1.0.0.54 - 01/12/2021
            - Support FPGA V800 (RTL FREEZE VERSION)
            - Support new PHY 1.0.17.3.0.54
            - new link adaptation parameters

Michael:   1.0.0.53 - 29/11/2021
            - ELP code for Osprey Lx
            - New PHY 1.0.17.3.0.53
            - BET is disabled
            - New PHY shutdown sequence
            - Enable dynamic clock gating
            - Cosmetic cleanups.

Gal:       1.0.0.52 - 23/11/2021
            - support FPGA V7619
            - support PHY 1.0.17.3.22.52
Gal:       1.0.0.51 - 21/11/2021
            - support FPGA V7599
            - support PHY 1.0.16.3.21.52
            - OSPREY_LDB - add resume/suspend BET in rpu
            - OCLA configuration
Katya/Michael: 1.0.0.50 - 15/11/2021
            - support FPGA V7568
            - support PHY 1.0.16.3.19.52
            - New PMAC Tx and Rx Scripter code

Katya:     1.0.0.49 - 09/11/2021
            -Multicast Rx - currently do not suspend
            -trace fix
            -stat macro typo fix
Katya:     1.0.0.48 - 07/11/2021
            - New configuration parameters for link adaptation
            - Enable stack protection mechanism

Katya:     1.0.0.47 - 02/11/2021
            - Support FPGA 7398 (O7.8)
            - Support PHY 1.0.16.3.14.52
            - Gal's fix - change clock 40 MHz to 80 MHz
            - OSPREY_LDB-613 - Aggregation and BA session AP role: Wrong BA sequence number for RX aggregated data
            - change CODE RAM size to 288K

Michael:   1.0.0.46 - 24/10/2021
            - Support FPGA-V7255 (7.6)
            - Integrate new patchtrap code
            
Michal:    1.0.0.45 - 04/10/2021
            - Support FPGA-V7105 (7.5)
            - fix configuration for BSS color
            - fix confiuration for gi-ltf certification tests
            - disable unsupported debug (until fixed)
            - limit the RX interrupt to host (to improve TP)

Michal:    1.0.0.44 - 05/09/2021
          -Support for FPGA 750 - including Fips support, tx mute, fix for PHY stomp bug.

Katya:    1.0.0.43 - 01/09/2021
          -OSPREY_LDB-637 - APUT TCP TX - wrap with disable/enable interrupts forward to nab
          -OSPREY_LDB-670 - workaround for wifi6 - read from he operation txop value
          -OSPREY_LDB-697 - MIC failure issue. False indication of MIC
          -statistics for PHY HE/HT packet error and Legacy packet error
          -add trace in stats for PHY unsupported counter
Michal:   1.0.0.42 - 19/08/2021
          -New FPGA 742 - extend RX_BB_TIMEOUT counter to 256uSec.
          -OSPREY_LDB-689 - Fix BET behaviour in stop role (take 2)


Katya:   1.0.0.41 - 12/08/2021
          -OSPREY_LDB-674 - [HW Validation] Stomp and Go statistics
Gal:   1.0.0.40- 12/08/2021
          - OSPREY_LDB - 675 - O7 release on top of FPGA V733
                             - new PHY FW 1.0.14.0.51  
Michal:   1.0.0.39- 10/08/2021
          - PSD BUG fix - Clear timer (latest RX & latest TX) upon entering power save to avoid long delay before first service period.
          - Add support for MU EDCA parameters - AX TB feature
          - Add stop role support for BET module
          
Katya:    1.0.0.38 - 02/08/2021
          - OSPREY_LDB-667 - Rx Suspend: add statistics
          - OSPREY_LDB-666 - [BET] : Missing feature implementation
          
Gal+Amir: 1.0.0.37- 29/07/2021
          - OSPREY_LDB - 671 - Support for O6 release - FPGA 689 - PHY version, registers files and updated interrupt controller.
          - OSPREY_LDB - 589 - Fix next DTIM calculation bug.
                             - Fixed bug in PWRMGT_CLIENT_STA_OUT_OF_SYNC removal, thus preventing going to sleep.
                             - Added temp patch that assume we are out of sync till receving first beacon.

Katya:    1.0.0.36- 15/07/2021
          - missing a padding in assoc config command

Daniella:   1.0.0.35 -15/07/2021
        - OSPREY_LDB - 664 - Antenna diversity validation manual tests.
Katya:    1.0.0.34 - 13/07/2021
          - add support for Multiple BSSID

Daniella:   1.0.0.33 -11/07/2021
        - OSPREY_LDB - 663 - Patch trap mechanism validation - added to bringup tests.

Michal:   1.0.0.32 -28/06/2021
        - OSPREY_LDB - 655 - UPSD hw configuration fixed (PMAC configuration should include RX & TX)
        - OSPREY_LDB - 595 (& 602, 605) - MMU address translation error (due to aborted frames handled in the RX path).
        - OSPREY_LDB - 648 - PLT- change the parameter RU tone index to AID (Yedidya)


Dana:   1.0.0.31 -17/06/2021
        - OSPREY_LDB - 644 - Interrupt Controller Validation - Bringup Test Timers and Bringup Test Descriptor Fifos

Gal:       1.0.0.30 -09/06/2021
          - add TX LIMIT HW test
          
Yedidya:   1.0.0.29 -27/5/2021
         - UART HAL - Add support to UART HAL APIs.
           Including DMA UART support TX and RX.

Dana:   1.0.0.28 -19/05/2021
        - OSPREY_LDB-644 - Interrupt Controller Validation - Reviving bringup tests

Yedidya:   1.0.0.27 -11/05/2021
        - OSPREY_LDB-632 - driver crash in several PLT scenarios - solved the problem with not notify tx event to driver when tx complete
        
Michal:   1.0.0.26 -09/05/2021
        - OSPREY_LDB-571 - PHY timeout - temp PHY FW version solving the channel tune + suspend + stomp bug.

Dana:   1.0.0.25 -28/4/2021
        - OSPREY_LDB-573 - Beacon Late Synchronization - added trace


Dana: 	1.0.0.24 -27/4/2021
        - OSPREY_LDB-573 - Beacon Late Synchronization

Michal:   1.0.0.23
        - add support for RX BA session STA+AP.
        Feature is enabled by default, can be disabled through CLI.

Amir:      1.0.0.22 - 22/4/2021
        - OSPREY_LDB-585 - Configure MemSS to configuration #8

Gal & Yedidya:      1.0.0.21 - 21/4/2021
        - OSPREY_LDB-585 - Support for O6 release - FPGA 658.
        - OSPREY_LDB-552 - Scheduled scan: assert after loading STA role with profiles configuring in the supplicant.
        - OSPREY_LDB-568 - Incorrect delay in OFDM Rate - Update Backoff registers to 0.
        - OSPREY_LDB-569 - Fixing "PLT assert- start Tx when running Transceiver cont. mode".
          - Add new role identfier for PLT role (doesn't effects current behavior, only design change).

Michal:   1.0.0.20 - 4/21/2021
        - OSPREY_LDB-585 + OSPREY_LDB-138 -
        - Support for O6 release - FPGA 651 - PHY version, registers files and updated interrupt controller.
        - Contain some workarounf for 'last2idle' false detection.

Dana & Michal & Amir:   1.0.0.19 - 4/4/2021
        - OSPREY_LDB-150 - Link adaptation policy mask bug - SW workaround for case when 11b is not supported
                         - Link adaptation Ax fix
        - OSPREY_LDB-577 - STA IPV6 - ping from remote does not work bug fix

Michael: 1.0.0.18 - 18/3/2021
        - O5 FPGA header alignment (Michal AP)
        - Core status race condition fix

Amir:  1.0.0.17 - 15/03/2021
        - OSPREY_LDB-578 - Don't filter multicast frames in PMAC
        - OSPREY_LDB-128 - Max TX retry assert after changing configuration to AP while connected

Gal:  1.0.0.16 - 11/03/2021
        - OSPREY_LDB-549: Threads priority - Make sure they all have different priority levels
		
Dana: 1.0.0.15 - 07/03/2021
        - OSPREY_LDB-141 - immediate response

Dana & Michal:   1.0.0.14 - 23/02/2021
        O4 bug fixes:
        - OSPREY_LDB-556 - Enable power save (clear active request was missing for power save)
        - OSPREY_LDB-150 - Bug fix in link adaptation (AP mode)
        - Enable Tx fragmentation

Dana & Michal:   1.0.0.13 - 09/02/2021
        - Aligning o4_fw_prep -> osprey_int (all previous o4 deliveries now integrated in osprey_int)
        !!! NEW FPGA is required - 867 (o4 + bug fixes) !!!
        !!! NEW PHY firmware is integrated  - 1.0.14.0.49 !!!

Dana & Michal:   1.96.0.12 - 07/02/2021
        !!! NEW FPGA is required - 867 (o4 + bug fixes) !!!
        !!! NEW PHY firmware is integrated  - 1.0.14.0.49 !!!
        - OSPREY_LDB-150    - Link adaptation Short Term Policy (legacy Rate Policy)
        - OSPREY_LDB-138 - O4 bringup & bug fixes.
        - Move to MMU 8k descriptor support
        - bug fixes - OSPREY_LDB-184,OSPREY_LDB-177

Dana:   1.97.0.11 - 26/01/2021
        !!! NEW PHY version - 1.0.14.0.48 !!!
        - OSPREY_LDB-150    - Link Adaptation AP mode bug fix
                             - Setting supported rates and basic rates not via driver
        - OSPREY_LDB-176 - (Amir) - nab.c file cleanup

Yedidya 1.0.0.10 - 20/01/21
        - Support PLT (Transceiver) mode
        - Support calibrator PLT (Transceiver) commands

Dana:   1.98.0.10 - 17/01/2021
        !!! NEW fpga is required - 866 !!!
        - OSPREY_LDB-150 - Link Adaptation software integration

Michal:   1.99.0.6 - 21/12/2020
        !!! NEW fpga is required - 863 !!!
        - OSPREY_LDB-138 - FPGA O4 support
            - Remove rate management code (new link adaptation still missing).
            - gcc_xml_parser.py - assertion remove. TBD
            - WSOC_WGCM__GNS_HOST_IF_LOCK__MEM_GNS_HOST_IF_LOCK_WROPT - temporarily marked out of the code
            - Replace the bit banding macros.

RazB:     1.0.0.9 - 14/1/2021
        - OSPREY_LDB-167 - [DEBUG] Calibrator tool debug configuration       
        - OSPREY_LDB-135 - [SCAN] Need to handle command stop scan that already stopped

Gal:    1.0.0.8 - 7/1/2021
        - OSPREY_LDB-165 - Osprey L2 API - BSS_LOSS_EVENT_ID
        - OSPREY_LDB-166 - Osprey L2 API - MAX_TX_FAILURE_EVENT_ID
        - OSPREY_LDB-136 - The mask of the events to host should be set only at initialization
        - OSPREY_LDB-157 - AP role: QoS not supported and traffic sent with default TID (BE)
        - OSPREY_LDB-156 - AP role: Assert during or after traffic stopped
        - OSPREY_LDB-154 - Scan res: At the tera term the rssi of the AP's appears ~18db more than scan res at the logger

Dana:   1.0.0.6 - 20/12/2020
        - OSPREY_LDB-9 - Unify ULL and GLX in DB RAM

Avichai: 1.0.0.5 - 13/12/2020
        - OSPREY_LDB-134 - Osprey Versioning
        - Avoid calling to sprintf that causes the code_ram (.map) to grow from 0x3a27c to 0x3b834

RazB:   1.0.0.4 - 03/12/2020
        - OSPREY_LDB-47 - CMD_ROLE_START- AP specific
        - OSPREY_LDB-51 - Osprey L2 Configuration API - CMD_AP_SET_PARAMS 
        - OSPREY_LDB-52 - Osprey L2 FW API - CMD_ADD_PEER
        - OSPREY_LDB-53 - Osprey L2 FW API - CMD_REMOVE_PEER
        
Michal: 1.0.0.3 - 22/11/2020
        - Fix TX header for encrypted frames (add mic len).
        - Remove security header on rx path (until supported by driver).
        - Avoid adding multicast addresses to mac address table until fixed.

RazB: 1.0.0.2 - 19/11/2020
        - OSPREY_LDB-70 - [SCAN] ERROR Scan min duration is 0
        - OSPREY_LDB-71 - [SCAN] Sending Scan complete event for CMD_STOP_SCAN
        - OSPREY_LDB-72 - ping over 1460 Bytes not reply from osprey
        - OSPREY_LDB-74 - STA: connect and disconnect fails after ~11 times.
        - OSPREY_LDB-77 - Osprey L2 Configuration API - FORCE_PS_CFG
        - OSPREY_LDB-78 - Osprey L2 FW API - DOT11_GROUP_ADDRESS_TBL
RazB: 1.0.0.1 - 15/11/2020
        - Osprey new official version 1.0.0.1
        
RazB: 0.0.1.46.469 - 05/11/2020
        - Change PHY initialize sequence
        - Scan commands
        New PHY version 1.0.13.0.47

RazB: 0.0.1.46.468 - 29/09/2020
        - OSPREY_LDB-47 - AP role start
        - OSPREY_LDB-51 - Osprey L2 Configuration API - CMD_AP_SET_PARAMS
        - OSPREY_LDB-52 - Osprey L2 FW API - CMD_ADD_PEER
        - OSPREY_LDB-53 - Osprey L2 FW API - CMD_REMOVE_PEER

Michal: 0.0.1.46.467 - 02/09/2020
        - move host interface related file to firmware\FW\host_if
        - OSPREY_LDB-16 - prepare TX data path for O3 FPGA version

EladZ: 0.0.1.46.466 - 27/08/2020
        - Fix compilation on Ubuntu 18.04

Dana: 0.0.1.46.465 - 25/08/2020
        - OSPREY_LDB-10 - Remove all references to HDI & AUX MCU modules from relevant code: Removing HDI and TKIP, WEP and FRAG
        - OSPREY_LDB-47 - AP role start: desc allocation for beacon was too small
		  New PHY version 1.0.13.0.46

NirG: 0.0.1.46.464 - 20/08/2020
       - OSPREY_LDB-50 event and complete mailbox


MichalM: 0.0.1.46.463 - 06/08/2020
        - OSPREY_LDB-2 - O2 FPGA support - new FPGA is required ver 663 !!!
        - Support for suspend feature
        - Beacon early termination (BET) support - validation version only.


RazB: 0.0.1.46.462 - 04/08/2020

        - Osprey release version
        - OSPREY_LDB-4 - Scan Request Flows
        - OSPREY_LDB-13 - STA role enable
        - OSPREY_LDB-14 - STA role start
        - OSPREY_LDB-17 - Port FW API to work with SL Dispatcher
        - OSPREY_LDB-20 - Role Disable
        - OSPREY_LDB-21 - Role Stop
        - OSPREY_LDB-22 - Osprey L2 FW API - CMD_SET_LINK_CONNECTION_STATE
        - OSPREY_LDB-24 - Osprey L2 FW API - CMD_CONFIGURE
        - OSPREY_LDB-25 - Osprey L2 FW API - Osprey L2 Configuration API - CTS_PROTECTION_CFG
        - OSPREY_LDB-26 - Osprey L2 Configuration API - TX_PARAMS_CFG
        - OSPREY_LDB-27 - Osprey L2 FW API - CMD_SCAN
        - OSPREY_LDB-28 - Osprey L2 FW API - CMD_STOP_SCAN
        - OSPREY_LDB-36 - CMD_SET_PROBE_IE
        - OSPREY_LDB-40 - Osprey L2 Configuration API - SLOT_CFG
        - OSPREY_LDB-41 - Osprey L2 Configuration API - PEER_CAP_CFG
        - OSPREY_LDB-42 - Osprey L2 Configuration API - BSS_OPERATION_CFG
        - OSPREY_LDB-43 - Osprey L2 Configuration API - ASSOC_INFO_CFG
        - OSPREY_LDB-44 - Osprey L2 FW API - CMD_SET_KEYS
        - OSPREY_LDB-46 - Osprey L2 FW API - PREAMBLE_TYPE_CFG

GalM: 0.0.1.46.461 - 23/4/2020
        - New FPGA version is required - 650
        - PHY version 44
        - CC3XXX-3088: AP role start - assert in case the role was not started successfully
        - CC3XXX-3091: Wrong configuration of force awake

gilsho: 0.0.1.46.460 - 29/3/2020
        added rx filters code and macros (not in build)_during covid-19 :)

gilsho: 0.0.1.46.459 - 11/2/2020
        Bug fix for scan site survey

gilsho: 0.0.1.46.458 - 03/2/2020
    - CC3XXX-3045, CC3XXX-3066, CC3XXX-3072:
      WPS bugs, fixed WPS fail function (not clearing wps bit), fixed CME state machine - return on not valid events
      fixed user call to CME change to getCurrentUser


RazB:
    - CC3XXX-2785: PLT feature - The feature transmit one more extra packet than chosen
    - CC3XXX-3051: PLT feature - fail to transmit packets on channels 12-14
    - CC3XXX-3050: PLT feature - Encrypted Packets are being transmitted
                   Adding new option on startTranceiver API
    - CC3XXX-3062: Adding encryption type option to transmit in Transceiver role
    - CC3XXX-3053: PLT feature - fail to transmit in the chosen delay.
    - CC3XXX-2787: PLT feature - Rx mode, the "mismatchpackets" and "totalpackets" output parameters are incorrect.
    - CC3XXX-3063: Assert after changing role to STA (After test AP role connect 64 stations)

Elad:
    - CC3XXX-3078, CC3XXX-3068:  Worker thread resource lock is unlocked before calling the garbage collector.


EladZ - 0.0.1.46.457 - 23/01/2020
 - Assign debug ports to NWPW GPIOs and connect GPIOs
 - CC3XXX-2754: SSL, Integrity test with cipher chacha poly_1305 only tx work 0 TLS1.2

EladZ - 0.0.1.46.456 - 15/01/2020
- CC3XXX-3057: Fix ciphers bit-to-name mapping
- CC3XXX-2946: Verify HCC is valid before removing it
- CC3XXX-3057: Added checks afters memory allocations; Increased heap size from 32kB to 64kB.

  gilsho-
- CC3XXX-3054: fixed support for non qos AP – removed wmm IE from assos req for unsupported APs
- CC3XXX-2723: added full pbac support
- CC3XXX-3049: added support for ADDBAreq frame before connection phase is finished

GalMilner - 0.0.1.45.455 - 13/01/2020
- CC3XXX-2952: Broadcast re-key - Need disregard the RSC from the AP and zero the PN counter.
- CC3XXX-3033: Idle connection after ~30 hours - MAC stopped the power save transitions and PHY was kept in "normal" mode.
               Seems that the STA entered out of sync state
               and from that state the out of sync client (PWRMGT_CLIENT_STA_OUT_OF_SYNC) was holding the system from entering power save.
- CC3XXX-3044:
               1.Tx BA timeout set to zero.
               2.Open aggregation support by default.
               3.Add some debug for unknown action BA frame.
               4.Remove security from PLT mode (until configurable).

EladZ - 09/01/2020
- TLS Socket Close:
    - CC3XXX-2963: Support NAB socket close initiated by host
    - CC3XXX-2964: Support TLS NAB socket close initiated by RST & FIN
    - CC3XXX-2966: Initiate TLS close in case of alert record recevied
- Warnings reduction

Michal & Gil 0.0.1.45.453 - 26/12/2019
AX FPGA version:
- New FPGA version is required - 635 - including AX MAC & PHY
- PHY version 41


Shaul 0.0.1.44.452 - 3/12/2019
AP Jitter:
-CC3XXX-2931 - restore timing and score and prevent learning during/after
               deafness period.
- after miss beacon, precede wake up time according to value from table (and not
  constant value).
- SW request LISTEN mode (when possible) for Beacon from PHY instead of RX_ONLY.
  PHY is using RX_ONLY (until LISTEN mode will be supported).

Shaul 0.0.1.44.451 - 21/11/2019
- CC3XXX-2898 new beacon track module + ap jitter (adapted from simple link)
- fix a bug in lrxp causing all PMG messages to not be handled correct
- CC3XXX-2922 WiFi PMF: STAUT doesn't send SA query Response to the AP.
- move "prevent sleep" command from traffic module to PM module

NirG  0.0.1.43.450 - 30/10/2019
- CC3XXX-2918 TLS1.3 data-path CCM and CCM_8

NirG  0.0.1.43.449 - 27/10/2019
- CC3XXX-0000: sockOpt update
- CC3XXX-2913 support TLS1.3

GalM   0.0.1.43.448 - 03/09/2019
- CC3XXX-2826 : Broadcast re-key: we accept 2 different keys with the same key ID.
- CC3XXX-2902 : DNS- dnsGetHostByName command cause the device get stuck.
- CC3XXX-0000 : resolved some warnings.
- CC3XXX-2900 : WLAN disconnect takes a long time.

EladZ   0.0.1.42.447 - 02/09/2019
- CC3XXX-2895 Upgrade Wolfssl to 4.1.0
- CC3XXX-2894 : Add IRQ mode to Sec-SS Doorbell & update Doorbell tests.
- CC3XXX-2893 : Support TOP events in MAIN_MCU & AUX_MCU
- CC3XXX-2832 : Added temporary handling for the case where record_status == TLS_RECORD_PREPARE_REASON_HEADER_ALERT_TYPE
- CC3XXX-2833 : In sl_nx_tx_set_descriptor(): The record trailer size is substracted from payload_len for crypto sockets.
- CC3XXX-2840 : Changed GTRACE to GTRACE_NVIC in LrxpFrags_AccumFragsListPrint.
- CC3XXX-2877 : Added a check for HCI_RX & HCI_TX validity before the HCC is removed.
- CC3XXX-2896 : Removed non-operational API sl_NetCfgSet(SL_NETCFG_IF)
- CC3XXX-2897 : In sl_NetCfgSet, if no profile is present in flash, set ipMode to default value.

GalM   0.0.1.41.446 - 25/08/2019
- CC3XXX - 2792, 2885, 2886, 2887: fix bugs in MFP - management frame protection

RazB   0.0.1.40.445 - 06/08/2019

NOTE: There no be longer releases for VIRTEX6

- CC3XXX-2828 : NWPW reset - core, aon & a2on isn't working
- CC3XXX-2890 : [HDI] Waiting for hdi idle IRQ forever
- CC3XXX-2866 : Periodic scan - Assert when periodic scan starts
- CC3XXX-1886 : Update NWPW addresses in VSOC P3
- CC3XXX-2836 : [PSD] Site Survey while role STA connected to an AP - Causes assert in PSD
- CC3XXX-2868 : Logger: LPDS masages are not printed in G logger
- CC3XXX-2860 : [DNSv6] assert - after connecting to network
- CC3XXX-2863 : Stability:SUT 2.4GHz TCP TX OPEN, stopped after ~20 min.
- CC3XXX-2845 : SUT Rx fragmentation traffic ,the data isn't received.
- CC3XXX-2878 : WEP -  udp Tx gets assert
- CC3XXX-2875 : EAPOL Messages are not Sent in rate policy rates
- CC3XXX-2871 : SUT: No connection to AP with WEP security.
- CC3XXX-2849 : SUT: No connection to AP with WPA security.
- CC3XXX-2851 : Assert after enabling/Disabling DHCP server
- CC3XXX-2862 : Krack: 5.2.2: Error while sending M3 message, unexpected deauth from AP
- CC3XXX-2844 : Connection scan: Out of date setting in the interval of scan plan.
- CC3XXX-2847 : SUT: Assert after add or delete profile.
- CC3XXX-2850 : IOP SUT: No connection to Cisco AP with WPA2 security.
- CC3XXX-2861 : [stability] DHCP no IPv4
- CC3XXX-2865 : Robustness test: DUT does not responds to any commands after running -periodicDeauthFromAP test
- CC3XXX-2869 : [Performance] SW is running without cache
- CC3XXX-2864 : APUT: Fatal event while setting SSID name in AP role
- CC3XXX-2839 : Power save: RX traffic failed after configuring Auto or forced power save
- CC3XXX-2852 : [Congested Cell] Ping does not work as expected
- CC3XXX-2838 : Some commands by tt_rpc_devpc.exe cause the device to stuck and reload needed
- CC3XXX-2841 : [DRIVER] While sl_recvfrom we got stuck on FIFO is always emprty
- CC3XXX-2843 : SUT manual connect scan: periodic scan stopped after plan 1.
- CC3XXX-2815 : rebase VSOC to master
- CC3XXX-2601 : Change any non-VsoC address/info in NWPW Pre/Post Compilation scripts
- CC3XXX-2532 : Change Download M0 code to VSOC's mode
- CC3XXX-2854 : VSOC- TLS poly and GCM

- New PHY version
- New Host Interface
- No HDI scripter


Gilsho 0.0.1.38.444 - 18/06/2019
- CC3XXX-2775 : Scan interval - changed intervals plan to match the time for 5Ghz scan added
- CC3XXX-2770 : Scan interval - changed next scan period to start immediatly if interval has passed
- CC3XXX-2714 : Assert on multi role scan - fixed dhcp timeout to the correct value

EladZ 0.0.1.39.443 - 21/5/2019
- CC3XXX-1976 - Added a check for socket validity before dispatch
- CC3XXX-2119 - Added a check for last ULL master socket before GLX pool delet
- CC3XXX-2726 - Added a checko for socket open in sl_cmd_socket_listen

NirG 0.0.1.39.442 - 15/5/2019
- CC3XXX-2755 Assert:SSL IPv6, Integrity test get assert when using IPv6
- CC3XXX-2754 Integrity test with cipher chacha poly_1305 only tx work.
- CC3XXX-2725 M0- data path with tables
- CC3XXX-2702 TLS: supporting chacha - poly
- CC3XXX-2214 remove slp_convert, reducing NWP code by 1356 bytes

RazB 0.0.1.39.441 - 24/4/2019
- CC3XXX-2694 : Transceiver/PLT feature.
- CC3XXX-2699 : Adding new role - TRANSCEIVER. stop and start role available
- CC3XXX-2696 : Start Tx in transceiver role.
                - open new transceiver socket.
                - configure the tx parameters over the socket by sl_setsockopt.
                - start tx.
                - close transceiver socket by sl_close.
- CC3XXX-2695 : Tx start tone - adding new phy API - start and stop tone.
- CC3XXX-2697 : Rx statistics.
                - reset counters and start collecting statistics by sl_WlanRxStatStart.
                - getting the statistics by sl_WlanRxStatGet.
                - stop the statistics by sl_WlanRxStatStop.
- CC3XXX-2722 : Sending TCP tx from CLI with -len 2000 and fragmentation causes assert –
                Bug fix. Maximum lengths updated to allow room for fragments
                (duplicating headers and fragmentation headers).
- CC3XXX-2116 : Bug fix Host Terminal show wrong AP ip address

Gilsho 0.0.1.38.440 - 21/4/2019
- CC3XXX-2724 : 11ax Support - Added parser for FILS Discovery frame, offset to Info (TIM) element.

Gilsho 0.0.1.38.439 - 1/4/2019
- CC3XXX-2666 : 11ax Support - added support for 11ax type connection for SUT.
- CC3XXX-2155 : PMF Required Support - SUT not connected to a PMF required AP, added SHA-256 support.

- CC3XXX-0000 : Fixed HT post association BSS params struct, as part of HE params
- CC3XXX-0000 : Added PMF support for pbac bit in STA role DB (indicates support for PBAC betwwen SUT and AP)
- CC3XXX-0000 : Moved PMF code from CME to LRXP path, to handle BA frames as well.

Dana 0.0.1.38.438 - 31/3/2019
- CC3XXX-2708 : Bug fix - Multicast Tx gets MMU translation error with IP address 224.0.0.1.
- CC3XXX-2719 : Deliver GPIO toggling for TLS and TKIP + fragmentation.
- CC3XXX-2658 : Bug fix - SUT traffic stopped or assert due to secured fragmentation UDP Rx.
- CC3XXX-2670 : Bug fix - SUT multicast traffic with TXOP limit.

- CC3XXX-0000 : New AUX-MCU version: 00.020

AmirA 0.0.1.38.437 - 21/3/2019
-CC3XXX-2715: Incorrect logic in QuietDetection_isStopped polling function


MichaelN 0.0.1.38.436 - 27/2/2019
- CC3XXX-2661: More fixes (better quiet period handling, sync between CME and supplicant states, better support of MR suspend/resume).
- CC3XXX-2682: Fast wake up - beacon with not role id.

Eran 0.0.1.38.435 - 18/2/2019
- Disable HTTP_SERVER and format the flash
- getHttpServerXXX - severals apis implementation

DuduL 0.0.1.37.434 - 14/2/2019
- CC3XXX-1890 : TLS Socket Close:
                1. BSD close() Clears all RX H/W pipe.
                2. BSD close() Terminates the NetX tcp receive from internal queues.
                3. Invalid HCC support in AUX-MCU.

- CC3XXX-0000 : New AUX-MCU version: 00.019

Nirg:
- CC3XXX-2686 : TLS: handshake using primitives directly.
- CC3XXX-2687 : Memory reduction - change job pool to real size structures and not unions.

MichaelN 0.0.1.37.433 - 10/02/2019
- CC3XXX-2661 - Fixed issue of not sending broadcast probe request for open profiles.
- CC3XXX-2674 - Fix ap psd with BA session.
- CC3XXX-2675 - Wrong HT cap - fixed to match l2cfg values.

Eran 0.0.1.37.432 - 07/02/2019
- CC3XXX-2111 - Assert on listen on wrong sd
- CC3XXX-403 - assert after changing role to none
- disable the HTTP_SERVER (must Format the flash first)


Dana 0.0.1.37.430 - 31/01/2019
- CC3XXX-2658 : 'CC3300 SUT Security Traffic' Dragon sanity test WPA2 TKIP 63chars - fails
                TID when non QOS Rx broadcast packets was fixed.
- CC3XXX-2663 : Fragmentation: Can't connect with AP on WEP security when configured TXOP
                Fixed WEP alignment issues.
Gil -
- CC3XXX-2319 - fix beacon txop
-CC3XXX-2671 - fix AP txop limits

MichaelN 0.0.1.37.429 - 24/01/2019
- CC3XXX-2536 - rate policy fix - deleted GTRACEs . Fixed rate override type.
- CC3XXX-2608 - Fixed missing comments for supplicant integration.
- CC3XXX-2630 : update code size script to match memory map after cost reduction and patches.
- CC3XXX-2656 : Add a field in WLAN connection async event to indicate a connection to the same AP.
- CC3XXX-2667 : Fixed no Aggregation when MCS rate configure to 0 only.
- CC3XXX-2668 : (Same as CC3XXX-2665) Fixed AP doesn't declare in the beacons for HT capabilities.


Eran 0.0.1.36.428 - 20/01/2019
- Patches/Hooks: change files name
- adding HOOK() to modules: dispatcher_netapp, dispatcher_netcfg


Dana 0.0.1.36.427 - 16/01/2019
- CC3XXX-2639 : Fragmentation implementation
- CC3XXX-2640 : M3 - preparation for fragmentation - For a packet that should be TKIP/WEP encrypted,
                determine if it should also be fragmented and set the number of fragments, the fragments
                length and the rate to the descriptor
- CC3XXX-2647 : M0 - Fragmentation and PMAC Tx Scripter - Performing fragmentation in M0 on a single
                new allocated descriptor and sending the fragments with the PMAC Tx scripter
- CC3XXX-2507 : Defragmentation implementation
- CC3XXX-2619 : Defragmentation with TKIP decryption not working with Cisco Aironet 1200 AP
- CC3XXX-2594 : Defragmentation - Remove expired sequence from list timer
- CC3XXX-0000 - New AUX M0 version : 00.016
DuduL -
- CC3XXX-1888 : Changed TLS Connect/accept worker thread priority,
                in order to avoid massive beacon loss.
- CC3XXX-2621 : Added TLS/TCP connect() support using bsd_connection_manager.
- CC3XXX-1888 : BSD connection manager: Unify flow for internal & host sockets.
- CC3XXX-2646 : Reduced stack size for TLS threads. (Secure server/client).
- CC3XXX-2621 : Fixed 'set time date' procedure for certificate time validation.
- CC3XXX-2650 : Have Ad-Hoc WLAN Connect cmd send user Async event as well as internal host driver event.

Eran 0.0.1.36.426 - 08/01/2019
- Patches/Hooks to M0 (Hooks works, Patch - HW still not support)

Michal 0.0.1.36.425 - 06/01/2019
- CC3XXX-2599 - BA managemtn module - add suspend of data queues for opening BA session sequence
                in order to solve inter-op issues.

Gil 0.0.1.36.424 - 26/12/2018
- CC3XXX-2608 - Supplicant integration to latest supplicant code in the open source

Eran 0.0.1.36.423 - 13/12/2018
- add some HOOKs for sanity and some hooks examples
- add hooks to flash module

Eran 0.0.1.36.422 - 09/12/2018
- Patch and Hook mechanism: disable "hook in trace"

MichaelN 0.0.1.36.421 - 04/12/2018

- CC3XXX-2629 : Implement Channel Switch support for STA role
- CC3XXX-2609 : PHY Abort parameter is missing during M0 cold init

Eran 0.0.1.36.420 - 04/12/2018
- Patch and Hook mechanism

Dana 0.0.1.36.419 - 03/12/2018

- CC3XXX-2627 : Bug fix - Setting manual time fails when month=12
- Gil - Fixed wps push connect gets assert

Michal 0.0.1.36.418 - 20/11/2018

- CC3XXX-2599 : Added TX BA management

Gil 0.0.1.36.417 - 15/11/2018

- CC3XXX-2540 : Changed the way rate policy set rates for broadcast links
- CC3XXX-2680 : Added comments for supplicant integration
- CC3XXX-2590 : Fixed the way supported rates are calculated in the rate policy

Raz 0.0.1.36.416 - 06/11/2018

- CC3XXX-2537 - PHY API timeout fix in the new PHY version.
- CC3XXX-0000 - New PHY M0 version : 1.0.5.0.37
- CC3XXX-0000 - Disable GRP_LTXP_SEND

Dana 0.0.1.36.415 - 24/10/2018
- CC3XXX-2507 : Defragmentation implementation - Including TKIP, WEP and non-TKIP/WEP
                - Missing optimization and MIC calculation word-by-word.
- CC3XXX-2127 : Assert in Ltxp Exchange Handler while profiled connection with UDP and ping when AP changes its channel
- CC3XXX-2232 : Fix counters for descriptors allocated from mgmt pool.
- CC3XXX-0000 - New AUX M0 version : 00.014


RazB 0.0.1.36.414 - 15/10/2018
 - CC3XXX - 2549
 - CC3XXX - 2552
 - CC3XXX - 2555
 - fix non ASCII in release.c


RazB 0.0.1.36.413 - 4/10/2018
 - Open 5Ghz scan and connect
 - Update PHY (1.0.5.0.36)

Gil 0.0.1.36.412 - 4/10/2018
 - CC3XXX-2536: Rate policy feature added, as part of the feature ltxpSend_SWGenerated and
 - ltxpSend_CCdriver are now 1 function only in the name ltxpSend_SWGenerated

Eran 0.0.1.36.411 - 2/9/2018
 - Flash: change number of FTL sectors from 49 to 25
 - Flash: move Shared Buffer to GLX
 - CC3XXX-2506, STA: Sometimes we get ASSERT after sending probe request -
                      probe request frame start address was not set correctly.

Eran 0.0.1.36.410 - 27/8/2018
 - Flash: add FTL version into flash
 - Flash: issue an FlashFormat + ASSERT when flash content change
 - CC3XXX-2501, APUT: AP with max devices of 4 - non QoS priority queues were always suspended
 - CC3XXX-2455 - disable sl_nx_check_for_available_socket_to_flush in txc_manager

 Eran 0.0.1.36.409 - 21/8/2018
 - support Flash OW HALs

Gil 0.0.1.36.408 - 16/8/2018
 - CC3XXX-1705 : Fixed RX From Unknown Session ID and link
 - CC3XXX-2320 : Fixed low throughput in UDP RX when descriptor pool goes under threshold.

Nir 0.0.1.36.407 - 05/8/2018
 - CC3XXX-2185 : TLS AES-CBC connection via sec_ss job (data not supported)
 - CC3XXX-2204 : aux_m0 add wolfssl crypto`s


Eran 0.0.1.36.406 - 31/7/2018
 - CC3XXX-1794 : increase heap_ull size from 22KB to 26KB
 - CC3XXX-1836 : STA Psd not closed
 - CC3XXX-2175  : PMF rx frame - mic failure is not detected
 
Eran 0.0.1.36.405 - 29/7/2018
 - CC3XXX-1888 : TLS Handshake: Change M's units to 128 Bytes.
 - MMU Pool optimization
 - CC3XXX-2118 : LLA - IP collision fix bug.
 - CC3XXX-2028 :HTTP SERVER - handle delete method

DuduL 0.0.1.36.404 - 24/7/2018
- CC3XXX-2110 : Fixed a race between startTLS and tcp accept done
                Updated isOnGoingTLSHandshake according to global count.
- CC3XXX-1888 : TLS data path - Change the calculation for Pu,Ds,Hs to be signed.
- CC3XXX-1888 : Fixed ASSERT() in non-blocking TLS accept.

DuduL 0.0.1.36.403 - 22/7/2018
- CC3XXX-2110 : Request parameters are kept in ULL for req_handler (TLS accept())
- CC3XXX-1888 : Message allocation & release, is modified to single allocation/free (TLS accept.).

Dana 0.0.1.36.402 - 18/7/2018
- CC3XXX-2099 - [WEP128]: Ping fail after AP switches channel, STUT with profile connection.
                Ping is now allocated from mgmt pool (not Tx pool).
- CC3XXX-1879 - [TKIP] MIC failure countermeasures.
- CC3XXX-1880 - [TKIP] TSC verification (replay).
- CC3XXX-0000 - New AUX M0 version : 00.013

Michal 0.0.1.36.401 - 16/7/2018
- CC3XXX-1870 - Management Frame protection: fix a bug. Links are mistakenly recognized as PMF support.
- CC3XXX-1870 - WEP shared integrated back into the code.
- CC3XXX-2098 - OCP TIMEOUT_CFG_MASTERS was extended to 0xFF (due to timeout events with 0x3C configuration).
- CC3XXX-1888 - in sl_bsd_conn_mngr_req_create.c change memory allocation from glx to ull.

Michal 0.0.1.36.400 - 12/7/2018
- CC3XXX-1863 - LRXP modification - re-structure & add support for M0 decryption fifo mode.
- CC3XXX-2032 - Fix CME check for session ID (verify link is valid first).

DuduL 0.0.1.36.399 - 12/7/2018
- CC3XXX-2055 : Calling GTRACE in IRQ context in Wolfssl_init();
- CC3XXX-2053 : Get socket status bitmap function returns the wrong type.
- CC3XXX-2054 : TLS data path fixes:
                internal TLS socket send() moves only netX prepend and append,
                not changing the descriptor addr & len to match NAB descriptor.
- CC3XXX-1888 : Added support for TLS on server side (accept)
                Cancel BSS loss during TLS handshake.

Shaul 0.0.1.36.398 - 10/7/2018
- CC3XXX-1870 - Management Frame protection: fix a bug. Deauth was sent unprotected

Dana 0.0.1.36.397 - 4/7/2018
- CC3XXX-1910 - Adding support of mixed-mode
- CC3XXX-1908 - Adding support of WEP shared key authentication
- CC3XXX-0000 - New AUX M0 version : 00.011


Gil 0.0.1.36.396 - 2/7/2018
-   Changing stat parser to cumulatively handle module names even if each module has multiple sections.
-   Added missing ASSERT macros for compilation w/o asserts.
-   CC3XXX-1645, Bug fix: Connection to Open AP - Get SSID from info element and update the profile with it
-   Warnings cleanup
-   CC3XXX-1852, Bug fix: Basic Rates are not published correctly in association request frame
-   CC3XXX-1654, Bug fix: APUT: wrong basic rates published in APUT beacons (fixed in version 0.0.1.30.379)

Shaul 0.0.1.36.395 - 1/7/2018
- CC3XXX-1870 - Management Frame Protection - for STA only (802.11 w)
                unicast robust mgmgt frames are encrypted
                broadcast robust mgmt frames should be decrypted by BIP - (Broadcast
                Integrity Protecol) - not supported yet (waiting for Security SS).
                SA Query procedure is supported

Eran  0.0.1.36.394 - 1/7/2018
- HTTP-SERVER: reduce code size
- CC3XXX-1912 - disable ARP module when STA is disconnected

NirG  0.0.1.36.393 - 24/6/2018
- CC3XXX-1665 - [SNTP] Getting invalid param error -14338 while entering valid time zone (GMT 8:45
- CC3XXX-1666 - [SNTP] Getting time from invalid time zone (GMT -12:01)
- CC3XXX-1887 - Ping when a STA is not connected - assert!
- CC3XXX-1909 - TCP Tx: low throughput (~20k) forever after TCP retransmission
- CC3XXX-1929 - Clean warnings

RazB  0.0.1.36.392 - 19/6/2018
- CC3XXX-1903 - Create socket (udp/tcp) with same max payload for ipv4 and ipv6
- CC3XXX-1875 - Remove compilation warnings
- CC3XXX-1796 - Add print for DEBUG ONLY!!

DuduL 0.0.1.36.391 - 17/6/2018
- CC3XXX-1900 : Fixed memory leak found in req_handler & moved message allocations to GLX.
- CC3XXX-1871 : Pmac writes to socket database results in hard faults.
- CC3XXX-1864 : Integrate TCP listen() with conn_mngr worker thread module.
- CC3XXX-1877 : Integrate TCP accept() with conn_mngr worker thread module.

Eran 0.0.1.36.390 - 14/06/2018
 * HTTP SERVER - support head method
 * HTTP SERVER - API for configurations

Dana 0.0.1.35.389- 12/06/2018
- WEP fixes + TKIP delivery:
    *   CC3XXX-1745 - TKIP WEP - WEP open issues resolved:
                                 - Allocation of descriptor for encryption moved from M3 to M0.
                                 - Allow a single descriptor can be allocated from the MMU mgmt pool.
                                 - Use a joined IRQ handler for RX and TX WEP/TKIP in M0 so that Rx and Tx Fifos can be
                                 polled with "round robin".
    *   CC3XXX-1847 - TKIP WEP - Tx mgmt frames shouldn't pass though M0
    *   CC3XXX-1755 - TKIP WEP - encrypt decrypt - verify encryption for packets of different lengths
    *   CC3XXX-1756 - TKIP WEP - TCP Tx FIN not sent (WEP)
    *   CC3XXX-0000 : New AUX M0 version : 00.010

Gil 0.0.1.35.388- 10/06/2018
- Fixed the following Sub-Tasks of CCXXX-1731:
    *   CC3XXX-1882 - Added a state check in the CME so old packets wont be handled if Disconnected
    *   CC3XXX-1883 - When De-associating a connection free memory of DH keys in glx if they were created
    *   CC3XXX-1885 - When WPS fails to connect through PUSH button free memory of DH keys in glx if they were created
    *   CC3XXX-1884 - When WPS fails to connect with incorrect PIN free memory of DH keys in glx if they were created

Eran 0.0.1.35.387 - 10/06/2018
 * IPV6 - send ANY / recv ANY / connect ANY - bug fixes

Nir 0.0.1.35.386 - 06/06/2018
 * CC3XXX-1802  - support long ping

Shaul 0.0.1.35.385 - 06/06/2018
 * changes in mem.c - 1. remove function xxx_with_or_without_assert. instead
                         it is possible to use SUPPORT_MEM_ASSERT
                      2. remove several memory pool validation calls as they are
                         double checked
                      3. change functions to make the mmore eficient
 * CC3XXX-1704 - assert while periodic manual reconenct + UDP TX
 * CC3XXX-1713 - Robustness scan + UDP RX. fix a bug in interface between STA PSD + ASU
 * scan module - fix memory leak at periodic scan (plan was not freed)


Eran 0.0.1.35.384 - 29/05/2018
 * CC3XXX-1644 - no packet is sent if sendto() ...  (this is only application fix
 * CC3XXX-1738 - ipv6: multicast address of mld group is reversed
 * CC3XXX-1865 - basic LPDS test failed since HTTP_SERVER select timeout

Shaul 0.0.1.34.383 - 29/05/2018
 * 1/4KB code reduction. remove forceUnsecure flag from cc33_driver_send_l2_data().
   this flag was used only when sending EAPOLs. instead - check link connection state
   inside LtxpSend_checkSecurityNeeded(). this verify that EAPOL is sent witohut
   security at first 4 way hand-shake
 * Integrated post-build memory analysis script. (MichaelN)
 * Fix unallocated parameters in request handler (bsd connection mgr) - Dudu

Eran 0.0.1.34.382 - 21/05/2018
 * HTTP SERVER - add configuration to the flash
 * CC3XXX-1710 - assert after setting ipv4 address while connected to AP
 * CC3XXX-1643 - empty host name
CME handling
Eran 0.0.1.33.381 - 21/05/2018
 * TCP Flags bug fix - now can get 2 simultanic SYN
 * ability to run HTTP server according to role_type
 * save HTTP configuration in flash
 * Init/start_application fixes

Raz 0.0.1.32.380 - 16/05/2018
 *CC3XXX-1769 - Move ping thread stack from static allocation to dynamic allocation.
 *CC3XXX-1819 - DNS query received with ip src same as APUT, DNS server error.
 *CC3XXX-1628 - Host: Select does not return an error if unopen sd in read list
 *CC3XXX-1819 - DNS query received with ip src same as APUT, DNS server error

Shaul 0.0.1.30.379 - 9/05/2018
 * CC3XXX - 1861 : AP RX Beacon protection protecssing (ERP + HT + Short Preamble)
 *                 fix STA RX Beacon protection processing
 * AP - enable B rates into Basic rates. Beacon is now transmitted in 1 MBs

Shaul 0.0.1.30.378 - 6/05/2018
 * CC3XXX - 1860 : Stop role fails due to incomplete handling of Hangover + APS

Yoav 0.0.1.30.377 - 3/05/2018
 * CC3XXX-1859 - fix more left overs from bsd flags memory reduction
                 (non-blocking tcp connect)

Shaul 0.0.1.30.376 - 2/05/2018
 * CC3XXX-1835 - integrate Hangover module. replace early termination timer with
                 Quiet Detection mechanism. still not fully functional as MAC vRIP
                 indication does not exist (RX with RA match) and therefore RXIP
                 is not detected at timer expiration and hangover decision is not
                 correct at this case.
 * Quiet Detection - fix race conditions and move parameters to GLX

Shaul 0.0.1.30.375 - 1/05/2018
 * CC3XXX-1794 - better handing of Beacon with FCS Error/aborted

Eran 0.0.1.30.374 - 29/4/2018
  * HTTP Server - RX chunk mode
  * CC3XXX-1840 - http - return wrong version
  * CC3XXX-1839 - http - support error 500
  * CC3XXX-1773 - (DuduL) bget.c

Yoav 0.0.1.30.373 - 26/4/2018
- the following sub-tasks of CC3XXX-1740:
  * CC3XXX-1757 - memory optimizations change byte flags to big flags in BSD and TCP socket structs
  * CC3XXX-1809 - refactor req hndlr to be a generic service (not just for crypto management)
  * CC3XXX-1810 - use a generic list module instead of 4 separate priority lists to implement the prioritized list
  * CC3XXX-1811 - statically link handlers (not dynamic registration) without affecting encapsulation of the msgq/list services
  * CC3XXX-1812 - build the BSD socket connection machine using the req hndlr/priority list infrastructure (above)
  * CC3XXX-1844 - move socket creation to pass through the BSD Conn Mngr state machine (as implemented above)
  * CC3XXX-0000 - many typos and editorials throughout the project
  
Gil 0.0.1.30.372 - 25/4/2018
- CC3XXX-1705 - APUT now sends deauth when received data frame from non-associated STA
- CC3XXX-1655 - Ps poll not getting deauth from d-link AP now
- Fixed Block Ack wrong packet size

Shaul 0.0.1.30.371 - 22/4/2018
- CC3XXX-1800 - Tim Element is now parsed and set on descriptor by PMAC
- CC3XXX-1845 - increase host IF thread stack from 1200 to 1400 bytes (Amir)

DuduL 0.0.1.30.370 - 22/4/2018
- CC3XXX-1831 : Makefile fix: Added prints for the Stand-alone AUX-M0+ compilation
- CC3XXX-1784 : ENOMEM error if 11 host sockets are opened.
- CC3XXX-1831 : Makefile historical issue fix: Scripter changes needs two compilations to take effect.
- CC3XXX-0000 : New PHY M0+ Version: 1.0.5.0.35
- CC3XXX-1768 : Update codesize.py script to include 64K execution code reserved for M3 patches.

Amir 0.0.1.30.369 - 16/4/2018
- CC3XXX-1815 - APUT multicast traffic: APUT sends multicast traffic to STA in power save in a raw but, "more data" bit is always off
- CC3XXX-1805 - APUT connected to a STA - ASSERT when starting site_survey
- CC3XXX-1814 - APUT power management: unicast traffic sent without trigger packet if multicast traffic running at the same time to STA in power save mode
- CC3XXX-1828 : DUT crash while two TCP Rx streams were running
- Fast wakeup - init IRQ before turning on PHY.

Shaul 0.0.1.30.368 - 16/4/2018
- C3XXX - 1775 : Scan - insert quiet detection support
- CC3XXX- 1774 : Beacon RX Fast Wakeup: insert quiet detection support
- change INT_XXX_IRQ to INT_XXX_BIT

Eran 0.0.1.30.367 - 15/4/2018
- CC3XXX-1804 : assert while stop start STA
- CC3XXX-1822 : HTTP in AP mode

Shaul 0.0.1.30.366 - 15/4/2018
- CC3XXX-1764 Fast wakeup - wait for phy complete event at end of sequence
- CC3XXX-1818 Scan - if open AP is supported, wildcard result should be dropped

Eran 0.0.1.30.365 - 11/4/2018
- CC3XXX-1781 : Fixed IPv6 acquired events rise every 30 sec
- CC3XXX-1795 : Fixed assert after connection samsung galaxy
- HTTP server:
    - change the recv flow
    - connections againg
    - ELP

Amir 0.0.1.30.364 - 11/4/2018
- gCmeRoleSwitchActive variable moved to ULL
- When CME queue is full, we filter every TX complete event (not only the 'safe' ones that we filter in case we are above threshold)
- Protect hostIF events queue by asking GLX retention
- SSID debug print fix
- Set PMAC register PMAC_CSR_RX_CFG_ADDR to be restored on every wakeup
Fixes the following bugs - CC3XXX-1516, CC3XXX-1783, CC3XXX-1799

DuduL 0.0.1.30.363 - 10/4/2018
- CC3XXX-1797 : Fixed Cortex-M3 makefile, to compile dependencies only on 2nd build.
- CC3XXX-1797 : Python dependency generator creates false dependencies for scripter objects
- CC3XXX-1798 : Unite both CM0+ and CM3 makefiles, so project is now single build.

Shaul 0.0.1.30.362 - 5/4/2018
- CC3XXX-1778 : fixed with new PHY
- CC3XXX-1780 : Manual Connection: Manual connect adjacent to connect scan resultant
                from enabled Auto connect with non existing profile fails
- put IRQ Common Error handing in seperated fucntion

Shaul 0.0.1.30.361 - 4/4/2018
- CC3XXX-1782 : Assert on wlanconnect() command while connected
- CC3XXX-1785 : Stability: Open Security. TCP Rx stability failed due to assertion after 40 minutes
- CC3XXX-1786 : Assert while SUT received data from WEB Server
- CC3XXX-1780 : Manual Connection: Manual connect adjacent to connect scan resultant from enabled
                Auto connect with non existing profile fails

Eran 0.0.1.30.360 - 2/4/2018
- HTTP SERVER - aging on connection_table

DuduL 0.0.1.30.359 - 29/3/2018
- CC3XXX-403 : Assert after changing role to none (AP DHCP POOL full)
- CC3XXX-1768 : Update codesize.py script to reflect true size of memory
- CC3XXX-1773 : Bget miscount the in-use portion of the heap.

Dana 0.0.1.30.358 - 28/3/2018
- CC3XXX-1770 : Fix - Not entering LPDS when WEP connected.
- CC3XXX-1763 : (DuduL) Added proper init sequence to AUX-M0 & raising GNS indications.
- CC3XXX-0000 : (DuduL) New AUX M0 version : 00.008

Amir 0.0.1.30.357 - 28/3/2018
- CC3XXX - 1676, AP PSD

Eran 0.0.1.30.356 - 28/3/2018
- HTTP SERVER - adding IP config

Eran 0.0.1.30.355 - 26/3/2018
- HTTP SERVER 1.1


shaul 0.0.1.30.354 - 25/3/2018
- CC3XXX-1697 Fast wakeup for Beacon RX
- new PHY version 1.0.5.0.31
- FPGA 314
- CC3XXX-1579 : Beacon DTIM - process Beacon with FCS Error (Beacon DTIM + AP Jitter)

Dana 0.0.1.30.353 - 25/3/2018
- CC3XXX-485 : WEP implementation of TX and Rx. Allocation of descriptor is
               still in M3 and needs to move to M0.
- CC3XXX-0000 : New AUX M0 version : 00.007

Michal 0.0.1.30.352 - 11/3/2018
- CC3XXX-1753 : new FPGA312 !
    Modification:
        -Updated NVIC configuration
        -Increase retry limit threshold
        -Add HW support for AP PSD
        -Small modification for STA PSD (hw & scripter)
        -MMU init fix (remove SW workaround)
        -New PHY version - 1.0.4.0.29
        -GLX data ram size was reduced, and code ROM was extended.

Shaul 0.0.1.30.351 - 27/2/2018
- CC3XXX-1664 : reset PM Statisitic upon command "-setPowerConfig 6 0"

Eran 0.0.1.30.350 - 21/2/2018
- add http_server code (disabled)
- CC33XX-1643 - empty host_name string in DNS request

YoavBY 0.0.1.30.349 - 19/2/2018
- CC3XXX-0000 : fix to compile aux m0 sec-ss tests file (new aux-m0 version)
- CC3XXX-1727 : fix to datetime utils (flatten day in year)
- CC3XXX-1733 : fix to SUT doesn't go to LPDS since v345 (Sec-SS imitation thread wakeup every 10ms)
- CC3XXX-1637 : fix to SUT LPDS enabled: Assert occurred during Tx Ping (glx stack inside the watchpoint boundaries)

DuduL 0.0.1.30.348 - 19/2/2018
- CC3XXX-1724 : Preparing a TLS record fails if record spans along two TCP packets.
- CC3XXX-1725 : TLS record header that spans across more than one packet,
                isn't consumed from netX queue.
- CC3XXX-1726 : Added support for non-blocking recv() on internal crypto sockets & regular.
- CC3XXX-1729 : Fixed potential race condition in setting/reading BSD bitmap flags.
- CC3XXX-1558 : TLS RX data path through Aux-M0 is working.
- CC3XXX-0000 : New AUX M0 version : 00.005

Yoavby 0.0.1.30.347 - 15/2/2018
- CC3XXX-1681 (sub task of CC3XXX-1679) - add crypto manager connections handler

MichaelN 0.0.1.30.346 - 11/2/2018
CC3XXX-1663: NAV Protection supoport.
             New PHY M0.
             PM measurement optimization.

Yoavby 0.0.1.30.345 - 8/2/2018
- CC3XXX-1680 (sub task of CC3XXX-1679) - add crypto manager requests handler

Dudu 0.0.1.30.344 - 1/2/2018
- CC3XXX-1558 : Moved HCC pointer table & crypto bi-dir ULL blocks to AUX M0 ULL.
- CC3XXX-0000 : New AUX M0 version : 00.004

Eran 0.0.1.30.343 - 31/01/2018
-CC33XX-1643 - empty host_name string in DNS request

Shaul 0.0.1.30.342 - 28/01/2018
-CC33XX-1661 - get Scan Params
-CC33XX-1664 - PM - measure the duration spent at each PM Mode

Eran 0.0.1.30.341 - 24/01/2018
-CC33XX-1651 - assert when changes address to new prefix (ipv6).
-CC33XX-488 - recvfrom bridge address.
-CC33XX-1636 - mld fix bug.
-Shaul change: getScanParamsFromFlash.
-CC33XX-1657 (Dana) - missing mem_Free
-CC33XX-1650 (Michal) - fix AIFS configuration.

Shaul 0.0.1.30.340 - 24/01/2018
-CC33XX-1631 - AP Jitter - after TSF Jump: calculate DTIM Count Offset
-CC33XX-1639 - AP Jitter - if Pattern 1 is forced and has 10 consecutive misses - start learning phase
-CC33XX-1640 - Beacon DTIM - add configuration of Wakeup condition
-CC33XX-1641 - AP Jitter - change the decision when to force Pattern 1
-CC3XXX-1648 - Dana - Valid time zone can be -12  14
-CC3XXX-1638,- Amir: Define 'pDhcpServerIPv4' as volatile to prevent compiler optimizations that access to 'pDhcpServerIPv4' even if it uninitialized and cause the watchpoint assert of accessing 0x57575757 data value to occur.
-CC3XXX-1592 - Amir: Missing initialization for BW list and private IE pointers

Eran 0.0.1.30.339 - 21/01/2018
- enable LPDS by default (when flash is erased)
- CC33XX-1630 - IPv6: change the Ping/UDP/TCP payload_max_len since that MTU is now 1280
- CC33XX-1464 - IPv4/IPv6: add 8 bytes to payload since that NX_PHYSICAL_HEADER is not included in the packet_header
- CC33XX-1574 - IPv6: not wake up every 1 sec (while sleep) if STA is not connected.
- CC33XX-488  - IPv6: recvFrom when client send on IPv4 address to IPv6 port

Shaul 0.0.1.30.338 - 15/01/2018
- CC3XXX-1586: save TSF offset of scan result and use it in connection instead
               of sending Probe Request

Shaul 0.0.1.30.337 - 10/01/2018
- CC3XXX-1599: non wakeup Beacon should be reported to AP Jitter as received or
               missed according to reception time vs expected time
  CC3XXX-1589: if waking on n Dtim and Beacon is missed, wakeup for next Beacon
  CC3XXX-1607: DTIM beacons may be received at non-DTIM windows
  CC3XXX-1615: device may wakeup every Beacon and not every DTIM
  CC3XXX-1600: AP Jitter - change update of Min T0

Eran  0.0.1.30.336 - 08/01/2018
- CC3XXX-488: IPv4-IPv6 bridge

MichaelN  0.0.1.30.335 - 08/01/2018
- CC3XXX-1572: Added support for PHY assert event.
   Integrated new PHY 1.0.26.
   New PHY traces.
- CC3XXX-1572: Added support for PHY trace flush on ASSERT.
- CC3XXX-1572: Added support for AUX trace flush on ASSERT.


Shaul  0.0.1.30.334 - 07/01/2018
- LRXP: 1. fix PM datapath client to not stay set after traffic (using GNS notification interrupt)
-       2. dont clear datapath client during TXing
- STA APS: fix writing power_save_state_req to be byte address instead of uint32
- RX Scripter: Update tim element information into the descriptor.
- CC3XXX-1585: STA wakes up for Beacon with RX Only until SOC will move PHY from Listen to RX Only
- Beacon DTIM: if beacon is missed but broadcsat frames are received, remove beacon timeout
               and PM request and also order next beacon timer
- add more GPIOs - PS, data path client, broadcast

Tomer 0.0.1.30.333 - 04/01/2018
- CC3XXX-1577 - fix sl_sendto() with 2 different destination ports

Amir 0.0.1.29.332 - 04/01/2018
- CC3XXX-1597, TCP ack: bug fix: Debug traces accesses to non initialized memory
- CC3XXX-1592, AP WhiteList Removing by index a non existent entry cause assert
- CC3XXX-1584, Close the broadcast PN reply check patch
- CC3XXX-1523, CME: Fix race condition between disconnect request from user and scan_done event from scan module
- CC3XXX-1516, When CME queue is full, try to clean it by removing 'unimportant' messages
- CC3XXX-1550, Scan module - properly search for WPA/WPS/P2P info elements, which are all vendor specific info element with different oui types
- CC3XXX-1583, PN reply - fix the comparison between the header and the old PN
- CC3XXX-1484, PN reply - Patch!! - Do not check PN reply in broadcast frames

Dana 0.0.1.29.331 - 01/01/2018 - First delivery of 2018!
- CC3XXX-485, Added WEP and TKIP standalone algorithms to M0 and
               storing shared security keys from M3 in M0.
- CC3XXX-0000, New AUX M0 version : 00.003
- CC3XXX-1578, (Michal) Manage WMM links bitmap correctly for more than 8 stations.

Dudu 0.0.1.29.330 - 28/12/2017
- CC3XXX-1398, Added 'ASSERT()' capability to AUX M0 logger.
- CC3XXX-1562, TEMP: put WolfSSL's enc\dec flat buffer in DBRAM_GLX (HIGH)
- CC3XXX-0000, New AUX M0 version : 00.002

Eran 0.0.1.29.329 - 27/12/2017
- fix TCP issues - found in ixia

Shaul 0.0.1.29.328 - 27/12/2017
- CC3XXX-1504 new AP Jitter algorithm (support patterns).
- reduce logs during wakeup
- fix a bug in TSF restoration after wrap around

Amir 0.0.1.29.327 - 25/12/2017
- CC3XXX-1552, Security type was not recognized correctly in RSN IE when probe response length was greater than 255.
- CC3XXX-1551, STA PSD mixed mode - partial support - for passing the certification tests

Yoavby 0.0.1.29.326 - 24/12/2017
- CC3XXX-1559 - fix to HCI invalidation after 3-4 hours

Yoavby 0.0.1.29.325 - 21/12/2017
- new phy version (v25)
- CC3XXX-530 - malloc/free rom size optimization

Yoavby 0.0.1.29.324 - 21/12/2017
- CC3XXX-1520 - phase 1a - BSD recv part
- fix pm bug fixes (told you in v323 that this will happen)
- fix from shaul in rsn_getSecurityType

Yoavby 0.0.1.29.323 - 7/12/2017 
- CC3XXX-312 task M0/M3 crypto jobs infrastructure (up to the BSD recv part) - phase 1a
- sub tasks: 500, 507, 509, 513, 1502, 1519
- a VERY big delivery (6 months work) ... apologies in advance for all the bad
  things that will happen

Yoav 0.0.1.29.322 - 7.12.2017
- CC3XXX-1528:
- Code reduction - temp removal of p2p feature (task 1528 details how to revert this removal)
- Add capability to group supplicant debug messages into separate groups (info/debug/err/warn)
- Reduce mem protect to protect read/write on a small part (as wolfssl code read occurs on the currently protected area @0x400)

Amir 0.0.1.29.321 - 7.12.2017
- CC3XXX-1522, AP White/black list support
- PM bug - did not wait for mmu init done if GTRACE was closed
- CC3XXX-486, TCPv6 accept returns bad family type

Dudu 0.0.1.29.320 - 30/11/2017
- New PHY version   : 1.0.3.0.24
- New AUX M0 version: 00.01
- CC3XXX-1512 : Re-organize the common aux logger header.
- CC3XXX-1392 : Add new traces to AUX M0 logger.


Eran 0.0.1.29.319 - 28/11/2017
- IPv6 TCP loopback
- IPv6 UDP loopback


Tomer 0.0.1.29.318 - 26/11/2017

CC3XXX-533 : osi_SyncObjCreate Does Not Clear the Allocated Event Group Bit

Dudu 0.0.1.29.317 - 20/11/2017
CC3XXX-1392 : Initial AUX M0+ infrastructure.
CC3XXX-1393 : Basic functionality: IRQ, FIFO and Logger.
CC3XXX-1512 : Break the Phy logger module to general engine and a phy specific one.
CC3XXX-1513 : Temp. move IRQ2 (low) to IRQ1 (med)


Eran 0.0.1.29.316 - 19/11/2017
- support DSCP (traffic type) for IPv6
- support RX fragmentation on IPv6
- CC3XXX-488 - recFrom on IPv4 IP on IPv6 socket works


Amir 0.0.1.29.315 - 15/11/2017
 CC3XXX-1389, os_get_random() - Make sure we don't return the same number every time. Temporary, till HW random generator will be implemented
 CC3XXX-1501, Check AES PN replay attack in broadcast packets
 CC3XXX-1498, Add WEP support in CME and scan modules
 CC3XXX-1417, AP - Bug fix: ADDBA request was not properly rejected
 CC3XXX-1389, WPA2 security vulnerability - as published by WFA on Oct 17 2017

MichaelN 0.0.1.29.314 - 13/11/2017
- CC3XXX-1500 : GPIO Module for Real Time Analysis.
- CC3XXX-424  : Add an IO on the FPGA that represents the PM state.

Eran 0.0.1.29.313 - 12/11/2017
- support IPv6 Multi interface and multi role
- support IPv6 MLD
- support IPv6 transmit flow

Amir 0.0.1.29.312 - 31/10/2017
- CC3XXX-1408, txop limit parameter is only uint16 - make sure we don't exceed
- CC3XXX-1407, SL Drive: Ping test timeout calculation bug fix
- CC3XXX-1404, AP: Translate CWmin/CWmax which are received from Supplicant to ECWmin/ECWmax (the exponent), which are used by PMAC
- cc3xxx-1406, netx parameters inherit also by NAB on listen socket (recv timeout enabled and non-blocking socket options) (Yoav)

Eran 0.0.1.29.311 - 29/10/2017
- support getHostByName (IPv4/IPv6)
- Enable IPv6 by default
- IPv6 pass LPDS

MichaelN 0.0.1.27.310 - 26/10/2017
- Integrated new PHY M0 binary (1.0.2.0.23)
Fixes:
- CC3XXX-1387 : _SlDrvMsgRead ignores error code when calling _SlDrvRxHdrRead
- CC3XXX-546  : PHY logger buffer size definition
- CC3XXX-548  : FPGA HW version vs. PHY FW expected version is missing
- CC3XXX-479  : Manual connect with mac address and security - 4 way handshake fails
- CC3XXX-532  : STA -> AP -> STA Role Change fails if STA is Connected and LPDS Enabled
- CC3XXX-1390 : No TCP Rx throughput at 11b rates.


Amir 0.0.1.27.309 - 22/10/2017
- cc3xxx-377, AP: linked list of add sta commands (add_sta and sta_set_flags) - bug fix
- cc3xxx-481, Bug fix - Race condition in role switch
- cc3xxx-551, bget - Free wipe only the relevant memory
- cc3xxx-529, Move the type "CommTest_e" from protocol.h to sltCtst.h
- CC3XXX-1386, Init Complete message is sent before host interface is enabled
- CC3XXX-481, On scan complete, trigger profile search also after role switch (in case it role switch was pending)

Michal 0.0.1.27.308 - 17/10/2017
- CC3XXX-484 update AC configuration in PMAC registers
- CC3XXX-534 TCP RX low TP when working with PS enabled. Fix STA PSD HW emulator patch

Shaul 0.0.1.27.307 - 03/10/2017
- cc33XX395 - Scan support channels mask + rssi threshold from sl_WLAN_Set

MichaelN 0.0.1.28.306 - 02/10/2017
- CC3XXX-375: Integration of the PHY SW Module.

Shaul 0.0.1.27.305 - 01/10/2017
- cc33XX493 - AP: assert if setting empty string in SSID name and running AP

Eran 0.0.1.27.304 - Sep 27
- CC33XX-527 - assert on wlan connect to WPA2 AP
- CC33XX-515 - BetCfgFet return non-zero result
- IPv6 SM done

-Dana 0.0.1.26.303 - Sep 14
- cc3xxx508 - Driver fixes in socket.c socket.h
- cc3xxx518 - Driver fix which adds retVal check in sl_accept which was accidently removed.
- cc3xxx519 - Closing a GLX socket can cause too many tx_block_pools to be deleted. We now check that we can only
  delete tx_block_pool when the socket being currently closed is glx.

-Eran 0.0.1.26.302 - Sep 05
- support DNSv6
- add black list to IPv6 APs

-Shaul 0.0.1.25.301 - Sep 03
- CC33XX-470: device doesnt set a timer to wake up if no timer is set.
- add support for sleep period greater than 32 bits. however, as mini pm support
  sleep of 67 seconds, the sleep duration is supressed.

-Shaul 0.0.1.25.300 - Aug 30 - WATCH POINTS
- move 'const" section to be after 'text'. this allow to unify read + write watchpoints
  to single watchpoint
- add bus fault info to the logger dump
- remove watchpoint to 0x5757xxxx (GLX reset value after reset). in case of reference 
  to such address, it will generate IRQ Error / Bus fault
- change Heap reset value from 0x55 to 0x47
- remove watchpoint to 0x5555xxxx (Heap GLX reset value after reset). in case of reference
  to such address, it will generate IRQ Error / Bus fault
- set watch point to protect against read/write to memoey in GLX RAN (0x20520000 - 0x20530000)
  with data value 0x57575757
- fix a bug in ASU of TSF comparasion in 32 bits (wrap around)
- move some parameters from GLX to ULL (detected after new watch point)
- fix HDI registers that were not restored

-Dana 0.0.1.25.299 -Aug 24
 - Host driver alignment to MCPI 2.0.1.20
   please note! *major* driver changes applied in this delivery.

-Eran 0.0.1.24.298 - Aug 17
 - move FTL version to sector 51.

-Eran 0.0.1.24.297 - Aug 17
 - DHCPv6 - add Black list: enter "bad" servers to black list in order to avoid from Blocking on Adverstise packet process
 - CC33XX-472 - fix bug (By Amir): mac address was swapped uncecessarly

-Yoav 0.0.1.24.296 - Aug 10
 - fix bug: CC3XXX-464 added a new data path for the tcp loopback of nab sockets
 - add new version of wolfssl (3.12.0) zip file, without integration to the new package
 - change the nx packet defines to map special ptr markers to non-mapped memory such
   that bus fault will occur if trying to access these ptrs.

-Eran 0.0.1.24.295 - Aug 8
 - support ipv6 fast path
 - fix bug: CC3XXX-462 enable Global IP while Local is disabled

-Gilboa 0.0.1.24.294 - Aug 1, thats a remarkable date. remember this.
 - new FPGA support version 0x33003206 with SPI crucial fix for MSP development - burn before rebase
 - fast path periodic statistics prints, to avoid case that fast path is disabled for so long
 - make file cleanup
 - test tool fix udp rx (version 8.0.0.76)

-Dana 0.0.1.24.293 - July 31
 - cc3xxx-450 - There is no need for an assert after flushing the p_desc for BSD sockets in utxp_tx.
 - cc3xxx-451 - Test tool: get auto time configurations bug fix.
 - Gilboa - Fix windows compilation - in TCP header global variable declaration add memory attribute
            only to declaration call and not the extern callers

-Eran 0.0.1.24.292 - July 31
 - IPv6 new SM (get state configuration from RA)
 - IPv6 UDP/TCP works on full path


-Amir 0.0.1.23.291 - July 31
Bug fixes:
 - cc3xxx-446, Keep Alive bug fix: 1. Don't update last_rx_timestamp on broadcast. 2. Use last_rx_timestamp from linkDB instead of from roleDB.
 - cc3xxx-334, Move Supplicant's 10 sec periodic timer from os timer (eloop) to non-wake FW timer, so it won't wake the device from sleep.
 - cc3xxx-441, When allocating a descriptor for a new beacon, wait till allocation succeed. Also, remove the assert in case of failure.
 - Beacon interval bug fixe: use uint16 instead of uint8.

-Eran 0.0.1.23.290 July 23
 - IPv6 new SM


-Gilboa 0.0.1.22.289 July 16
 - CC3XXX-403 - ARP table print causing assert due to protected section GTRACE usage
 - host driver temporary patch to allow working with new MSP SDK (sempahore calls without return codes )
   it is temporary since we will work on proper alignment to new host driver (2.0.1.17)
 - fix logger print on gtrace during disable interrupts

-Dana 0.0.1.22.288 July 13
 - Yoav - CC3XXX-429: print errno when no more sockets
 - Amir - CC3XXX-447 - gTrace: When %s is used with a NULL pointer, just display NULL (and don't crash)
 - CC3XXX-443 - Assert caused by bpoolv of malloc because malloc was missing disable interrupts
 - CC3XXX-401 - Multiple Sockets: assert occurred after 5 min during simultaneous activity via 32 sockets -
                getting mutex before counting number of glx sockets in soc_close.
 - CC3XXX-393 - CMD_START_ARP_MGMT_SEQ missing from some arp send callers -
                added start mgmt sequence on ARP send to gateway, and made ARP to gateway occur no more than
                once every 3 seconds.


-Eran 0.0.1.22.287 July 13
 - CC3XXX-432 - fix the sl_dispatcher function
 - CC3XXX-417 - send the ICMP reply with the relevant TID

-Michal 0.0.1.22.286 July 13
 - CC3XXX-423 - Consider ACM configuration from AP. If Admitted control is mandatory for voice
                or video - traffic is degraded to lower AC.
 - CC3XXX-345 - Tx schduler fix.
 - CC3XXX-439 - RSSI score fix (Shaul)
 - CC3XXX-433 - Fix in DNS server init to solve role AP->role none->hard fault bug (Yoav).
 - CC3XXX-428 - supplicant: Don't send NULL pointer GTRACE (Amir).

-Amir 0.0.1.22.285 July 10
 - cc3xxx-43, Auto PS fix in PMAC TX scripter
 - cc3xxx-422, Assoc timeout - don't release the descriptor ID, before receiving the callback
 - cc3xxx-369, Patch: One shot connection scan starts with channels 1,6,11. This is a temp patch
 - cc3xxx-414, In WPS, don't perform early termination while in scanning. Pass all results to supplicant.
 - CC3XXX-419 - get profile if Sleep is enabled - crash

-Eran 0.0.1.22.284 July 10
 - CC3XXX-409: add free to cmeGetPreferredMetwork
 - CC3XXX-404: fix the getMacAddress function
 - CC3XXX-409: run garbage collector on heap when malloc ull failed
 - reduce flash stack size from 4K to 1.5K
 - CC3XXX-411 - ability to config static IPv6 (change only in the test tool)
 - CC3XXX-417 - fix ping ignore the TID in the ping request

-Yoav 0.0.1.22.283 July 5
 -cc3xxx-412, do send when arp table full with high priority entries fails to add new arp entry for the send

-Amir 0.0.1.22.282 July 5
 -cc3xxx-32, RX from unknown
 -cc3xxx-405, Get profiles doesnt work for more than 1 profile
 -cc3xxx-403, Don't call DISABLE_MED_PRIORITY_INTERRUPTS_NO_RT with { on the same line
              + add assert
 -cc3xxx-354, TCP TX low throughput with power management
 -cc3xxx-400, Bug fix: Don't publish greenfield in ht capabilities
 -cc3xxx-43, Assert when supplicant sets a key to a non existing link
 -cc3xxx-377, AP: Bug fix: Connecting more than 1 peer simultaneously
 -c3xxx-43, Bug fix: Disconnect reason was not updated

-Dana 0.0.1.22.281  July 5
 -CC3XXX-397 STA didnt move to active after crossing TX counter threshold
 -CC3XXX-395 Scan result dump error
 -CC3XXX-387 Scan result dump error

-Dana 0.0.1.22.280  July 3
 -CC3XXX-391 - Assert on ping Stop - deinit is wrong, the thread should finish its job and terminate itself
 -CC3XXX-301 - Wlan LP Thread Command Queue gets full because of ARPs - arpFindDynamicEntryAndHandale returns after
               sending arp to the gateway and it shouldn't. Instead it should continue the loop to find the arp entry
               with the destination address ip and delete it. Solved by removing the return.
 -CC3XXX-390 - slBsd_CreateWorker does disable preemption without enable preemption before returning in the case of
               SL_BSD_WORKER_CREATE_RX_RST: RX FIN or RX RST during an already running CLOSE.
 -CC3XXX-389 - add thread preemption threshold statistics to idle task - print ERROR if tx_thread_priority not equal
               to tx_thread_preempt_threshold.

-shaul 0.0.1.22.279  July 2
 -CC3XXX-351 - add AP support to Get STA list

-Gilboa 0.0.1.22.278  July 2
    - CC3XXX-275 - DHCP management sequence stop with role id 255 - changed setting the invalid role
                   on the interface to the end of the bringdown (Also for AP)
    - CC3XXX-275 - DHCP delete setting netmask to 0xFFFFFFFF (instead of 0) so that packets will not
                   go through routing table after DHCP IP is down

-Michal 0.0.1.22.277  June 29
    - Bug fix - Modify PM disallow to request FULL_PHY

-Michal 0.0.1.22.276  June 28
 - Fix STA PSD in case of non qos link - extract more data indication from non qos frames as well.
 - Modify PM disallow to request FULL_PHY
 - Fix beacon parsing - search tim IE through the entire beacon, not only 10 first information elements (Dilink IOP).

-Shaul 0.0.1.22.275  June 27
 - CC33XX - 323: as PMK is calculated in SW and it takes several seconds, this starve
   lower priority threads such LRXP and this cause miss beacons to lead to BSS Lost.
   Ignore miss beacons during PMK calculation.

-Amir 0.0.1.22.274  June 26
    - Periodic scan: if result is found at the end of period, stop the scan
    - CME: Dump 'unnecessary' messages in the the number of messages in the CME queue is
      above threshold

-Shaul 0.0.1.22.273  June 26
-CC3XXX-357 - enable BSSID Filter during scan. relevant only to connection scan
              of single profile with BSSID set
-replace LtxpSchedTask + NetXIP0 threads priority

-Michal 0.0.1.22.272  June 26
* Fix gain issue - B rates gain comparing to OFDM.
* CC3XXX-335 - Remove force exchange compilation flag. Now TX exchange for all frames is
               forced while device is in power save only.
* CC3XXX-343 - Update slot configuration in PMAC, based on role configuration.
* Fix bug in the ltxp queues flush (related to AP with 4 stations configuration).
* Add Debig information for missing beacons in AP role.
* Fix AP role supported rates configuration to support B rates as well.
* CC3XXX-362 - Fix traffic module default parameters - TH for moving to active is 4
*              packets (RX or TX), timeout for moving to PS is 5mSec of inactivity.
* Fix descriptor fifo module store / restore.



-Dana 0.0.1.22.271  June 22
* CC3XXX-365 - (Eran) Assert on ping while SUt not connected
* CC3XXX-355 - Host stuck on second getNetworkList - The poolObj was being identified as a
*              socketAction because in getNetwork list, we use the additional data field of the poolObj
*              for the number of returned scan results, however the SL_BSD_SOCKET_ID_MAX_BIT which indicates
*              that this is not a socket index, was not set. Set max number of results to 31.
* CC3XXX-346 - AP: Assert after event queue of host events is full -
*              cleaned up driver defines. Number of silos is 5.

-Eran 0.0.1.21.270  June 20
* ipv6 ping


-Shaul 0.0.1.21.269  June 19
* enable SSID Filter during Scan

-Amir 0.0.1.21.268  June 18
* Send deauth/disassoc frame in a managed sequence
* AP: sta aging configuration
* Bug fixes:
*           1. Ad hoc connect when connected to open AP
*           2. Fast connect cache cleanup
*           3. LTXP send data frames over global link - fix
*           4. LTXP send data frames when link is already does - fix
*           5. Remove unnecessary protection in sl_dispatcher
*

-Dana 0.0.1.21.267  June 18
* CC3XXX-236 - flash bound actions - Wlan policy set changed to flash bound.
* CC3XXX-221 - Flash Access in CME Thread Might Cause Overflow of It's Event Queue -
*              Flash access is now only through the dispatcher context (profile add/del)
*              and copying profiles to global is done in the cme.
* Profile add/del cannot be performed during WPS
* Small fix in profile get which caused overflow in the case of non enterprise profiles.

-Michal 0.0.1.21.266  June 17
* Integrate rate management HW module (CC3XXX-297).
* Add database (entry per link) and registers configuration.

-Eran 0.0.1.21.265  June 14
* Enable IPv6 Global Stateless
* Enable IPv6 Local and Global Stateful
* fix IPv6 bugs
* add host command getIpv6

-Shaul 0.0.1.21.264  June 14
* do scan with active scan before passive scan
* PM - if sleepa duration is too big - do assert

-Yoav 0.0.1.21.263  June 14
* added more ciphers to the code (especially those needed to test STA enterprise by the supplicant).

-Michal 0.0.1.21.262  June 13
* CC3XXX-319 - Modify the implementation of the clean RX pipe notification
               (now using the GNS mechanism).

-Yoav 0.0.1.21.261  June 13
* integration of wolfssl v3.11.0 into the code

-Shaul 0.0.1.21.260  June 12
* Basic Scan - CC3XXX-326:  send broadcast probe request only once per channel (like WL8)
*              and not per ssid which is not hidden
* Basic Scan - fix a bug: if scan adapter stop basic scan before it even started to work,
*              the Basic Scan SM State was not intialized.

-Shaul 0.0.1.21.259  June 8
* PM: 1. fix bug in MMU init sequence
*     2. patch - if host interface closed and nab is not idle - revert
* move part of g_fault to ULL
* notify stop role to power save control

-Eran 0.0.1.21.258  June 7
* compile IPv6 code.
* Disable IPv6 by default.
* ability to configure IPv6 States from test tool

-Yoav 0.0.1.20.257  June 7
* fix tcp tx sequencing (separating it to oow and inside window)
* assert on no spi (we cannot work without flash)


-Dana 0.0.1.20.256  June 5
* Please note! *major* driver changes applied in this delivery.
* CC3XXX-208 - host driver API modifications - modified API's with the new dragon nwp host driver
  architecture guidelines.

-Amir 0.0.1.19.255  June 5
* Parse RSN information in Survey scan

-Shaul 0.0.1.19.254 June 5
* PM - when going to sleep wait until Dealloc FIFO is empty and there is no
       pending request to the MMU

-Michal 0.0.1.19.253 June 4
* Optimize MMU init - instead of lwtting the HW run the auto init, fix configuration
  registers, and then run HW init again - run HW init only once, and fix the internal memory
  initialization, as well as head & tail registers.
* Change packet ram order - start with generic SW variables, followed by the MMU mapped area,
  and then OCLA area.
* Fix Ocla memory allocation to be flash variable (inside defvice params) instead of
  compilation parameter.

-Shaul 0.0.1.19.252 May 29
* limit Time Schedluer Event duration to 0x7FFFFFFF (otherwise it will be
* changed to 50 (CC3XXX-285)

-Shaul 0.0.1.19.251 May 29
* add LPDS allow flag to Flash. allow/disallow LPDS according to this flag. LPDS is
  disallowed by default
* fix a bug in wakeup from Sleep - sleep duration calculation (CC3XXX-310)

-Amir 0.0.1.19.250 May 24:
* Disregard BSS loss event in WPS (temporary till HW accelerator will be used in crypto)
* Bug Fix: Connect to Open AP when open policy is enabled
* Bug Fix: Allow supplicant Disconnect event while CME is in idle
* Add enterprise certificates for the Testing team server (hard coded, closed by default)

-Shaul 0.0.1.19.249 May 22 - PM Version :-)
* This version contains the PM code to allow the device to enter Sleep.
* This version requires FPGA version >= 205
* The device start with Sleep disallowed. This can be changed by SL Tool command: -setPowerConfig 5 0
* If device enters sleep mode, the JTAG disconnectes.


-Eran 0.0.1.19.248 May 17 - fix bug
* backward compatibility to command: sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE,SL_NETCFG_ADDR_DHCP,0,0)

-Gilboa 0.0.1.19.247 May 15 -
* CC3XXX-187 ad hoc reconnect fail if profile scan exist due to connect-on-connect illegal check
* CC3XXX-216 recieve timeout based on time scheduler for timeouts lower then ~35 minutes
* new test tool 8.0.0.54 supporting udp rx with configurable recieve timeout

-Eran 0.0.1.19.246 May 15 - same content as 0.0.1.18.246

-Eran 0.0.1.18.246 May 14:
    * Bug fix: CC33XX-280 - Client not answer to DHCP offer unicast - "fix the fix"
    * Bug fix: CC33XX-289 - add option in the test tool to configure open AP

-Eran 0.0.1.18.245 May 9:
    * Bug fix: CC33XX-280 - Client not answer to DHCP offer unicast
    * Bug fix: CC33XX-281 - assert on DNS
    * Bug fix: CC33XX-247 - arp statistics unprotected

-Amir 0.0.1.18.244 Apr 30:
    * Configurable max number of stations in AP
    * Bug fixes:
        - Don't send L2 frames over global link
        - AP: Usage of chanel 0 is illegal
        - Resume traffic on role start in AP beacon and max TX retries clients
        - Initialize link entry fields on role start

-Eran 0.0.1.18.243 Apr 24 - Enable "Fast Path" (it was disabled by mistake in release 0.0.1.18.242)

-Eran 0.0.1.18.242 Apr 24 - DHCP server bug fixes
* CC3xxx-269 - assert as a result of DHCP collision - manage the flag gDoesDhcpServerRunning
* CC3XXX-265 - set AP IP address fails when DHCP server is idsabled


-Yoav 0.0.1.18.241 Apr 20 - 35 years since apollo 16 (commanded by John Young) successfully landed on the moon:
* cc3xxx-259 - fix on fix:
    - forgot to release the flush/close semaphore after taking it in some obscure cases
      (e.g. no socket or on different socket id instance).

-Dana 0.0.1.18.240 Apr 18 2017:
* new host driver 1.0.1.18.27, tiny little bug fix in sl_select
* new test tool 8.0.0.50, changes in TCP server with select test - now can support 31 client sockets on select
* CC3XXX-255 - TCP listen sockets changed back to 16 and bug fix in sl_cmd_socket_select.
* Building command for select thread BEFORE select command is called because it resets descriptors.

-Yoav 0.0.1.18.239 Apr 9 - happy pesach:
* cc3xxx-259 - scent of stop role bugs:
    - if during an ongoing close a bind command from the host was issued, an unbound socket
      (done beginning of the socket close) becomes rebound (and assist and foul and sal) and
      then the socket is gone due to the close... leaving the ip ptr circular list of bound
      udp ports with one of the sockets pointing to NULL instead of to itself (or not at all).
    - recv timeout and close on the same thread were racing on the NAB flush semaphore. if the close wins,
      the recv timeout worker starts after the close finished working and when it tries to access a BSD ptr
      it is already null-ed and cleared by the socket close

-Yoav 0.0.1.18.238 Apr 6 - happy birthday dana:
* cc3xxx-246 - moved recv timeout (sw) to worker thread to avoid getting stuck on lock object from ip thread
* cc3xxx-252 - going into sleep assert fix on is any socket active (from shaul)
* cc3xxx-253 - replace #ifdef 0 with #if 0 (from shaul)

-Gilboa 0.0.1.18.237 Apr 5 2017:
* new FPGA 189
* being back support LE8 for MSP432
* new host driver 1.0.1.18.26 (still backward compatible), add commentary + support nonos compilation (reentrance)
* testtool delivery test fixes with the local ip
* CC3XXX-257 - sl_stop with non-zero timeout cause to start with invalid role -
               CME_WlanSetMode now expects a Boolean (updateSetModeFlash) to determine
               if setMode bitmap should be saved to flash.
* CC3XXX-255 - TCP listen sockets enlarged from 16 to 31 (additional ~600 bytes ULL)


-AmirA 0.0.1.18.236 Mar 30 2017:
     * AP: STA Aging.
     * Max TX retries Handling.
     * Peer remove process - notify wlan modules and wait for completion.
     * Add link remove notification for wlan modules that can be used either from
       remove peer or role stop.
     * Separate start/stop role process and add/remove peer process to different modules.
     * Bug fixes: cc3xxx-232, cc3xxx-233, cc3xxx-87


-Yoavby 0.0.1.18.235 Mar 23 2017 - managed to pass first ever tls gcm frames with reduced secured context (excluding buffers)
* fixes to previous delivered reduced context code
* work with GCM mode in SW (couldn't activate the HW engine yet)

-Gilboa 0.0.1.18.234 Mar 16 2017 - netx bsd Rev2 (not-so-)glorious merge
* import fixes and merge TCP/UDP netx core from SL NWP version 3.0.1.4
* import fixes and merge netx bsd layer from SL NWP version 3.0.1.4
* all merge deatils (+the ipv6 ones) under:
* https://sps09.itg.ti.com/sites/Dragon/nwp/SitePages/Home.aspx?RootFolder=%2Fsites%2FDragon%2Fnwp%2FShared%20Documents%2FNWP%2DECS%2FSoftware%2FDesign%2FSW%5FInfrastructue%5FComponents%2Fsl%5Fdragon%5Fmerge%5Fstatus&FolderCTID=0x01200045DDF660611BDF48877764FBA01CDF88&View={FFEEDAFB-7312-4A2E-8DC2-6763B6614C0E}
* close in the middle of reset fix
* CC3XXX-162 fix max payload size for all socket types (allow real 1472 in UDP)

- Eran 0.0.1.18.233 Mar 15 2017:
    * ping on IPv6 local stateless - not compiled yet

- YoavBY 0.0.1.18.232 Mar 14 2017:
    * CC3XXX-172
    * New FPGA v187 (Sec-SS delivery) + some changes to SOC (HDI/NAB/PMAC Rx/WSOC_MCU) registers
    * disabled compilation flag of reduced sec-ss context (TLS1.2 AES-GCM only for now)
    * disabled compilation flag of porting of sec-ss code M0 vs. NWP
    * disabled compilation flag of lots of bringups for the aes_gcm of the Sec-SS ... in progress
    * nab endianess ignore, mark little endian as default
    * avoid i2c access in delivery-test
    * dispatcher using opcode enum prints instead of opcode hex values

- Gilboa 0.0.1.18.231 Mar 8 2017:
    * opcode builder enum for proper opcode printing (Shaul's enhancement)
    * MSP432 project alignment to new version (test tool not working yet)

- Lior 0.0.1.18.230 Mar 7 2017:
    New host delivery

- Dana 0.0.1.18.229 Mar 7 2017:
    TCP RX checksum calculated by PMAC.
    partial_checksum_valid bit changed to 0 when PMAC calculation is expected to be incorrect.
    When PMAC calculation is invalid or fails, calculation is verified in SW.

- Amir 0.0.1.18.228 Mar 5 2017:
    1. Get AP security configuration from flash.
    2. AP: Move some memory from ull to glx.
    3. Bug Fix: Remove unnecessary mem_free of scan results table.
    4. Bug fix: Remove unnecessary osi_sleep at PMK calculation.

- Eran 0.0.1.18.227 Mar 5 2017
   * Support SPI Flash


- Lior 0.0.1.18.226 Mar 2 2017
   * CC3XXX-150: configurable API timeout
   * CC3XXX-117: duplicate declaration of const/enum in device.h


- Shaul  0.0.1.18.225 - Mar 01 2017
  1. CC3XXX-213: do cortex gating from idle thread as follow:
    a. if STA in Active mode - set new timer to 1 ms. it is used to exit cortex gating every 1 ms to
       check if it is time to move to power save
    b. other - do cortex gating until an interrupt will wake the cortex
  2. add new timer (TIMER4) and connect it to NVIC 4 - used to exit cortex gating
  3. delete PHY SOC nvic which is not used
  4. unify osi_TimerCreateLegacy + osi_TimerCreate into osi_TimerCreate. the API allow the user to
     add MSI (currently only "socket receive" timer deliver the socket id as MSI)
  5. CC3XXX-212 - if Sleep is not allowed: STA link should not move to active mode if tx/rx Thresholds
     are greater than 1 and it receive/transmit single packet with long interval (i.e. ping)
  6. CC3XXX-216 - Manual connection to the AP with strongest rssi at channel 11 fails
  7. rename lpds.c to pwrmgt_lpds.c
  8. rename asm_common.s to asm_common.asm
  9. calculate wakeup point instead of sleep duration

- MichaelN  0.0.1.18.224 - Feb 28 2017, end of the 1st Gulf War anniversarie.
   * CC3XXX-215: Implementation of major WLAN Silo APIs (Set/Get).

- Gilboa 0.0.1.17.223 - Feb 27 2017,
* CC3XXX-140: support multiple selects from host (up to 4),
*   new host version (1.0.1.17.21) - protocol change
*   new test tool (8.0.0.46) allowing parallel selects on different ports
* CC3XXX-183: GLX Corruption date-time fixes (GLX buffer never released)

- Eran 0.0.1.16.222 - Feb 27 2017,
   * support new FPGA 184 (Must take this FPGA in orger to work with this version)
   * include flash SPI code (under compilation flag)

- Amir 0.0.1.16.221 - Feb 26 2017,
   * Bug Fixes:
        1.  Move HW generated buffer (used by PMAC TX) from ULL DB RAM to GLX DB RAM.
        2.  Fix PMAC TX scripter to update security header data base only in case of encrypted frame
           (fix a problem of clearing extended IV right after connection).
        3. Disconnect while scanning: When supplicant was in between two 'one_shot' scans
           (a failed scan causes the supplicant to start a new one shot scan after 1 sec),
           the disconnect was not handled properly.
        4. Scan stop event to cme, when scan is stopped - initialize to 0 the results table pointer and the num of results received.

- Eran 0.0.1.16.220 - Feb 23 2017,
   * change the FTL version flow for SPI: burn the FTL version to the flash after the format is done


- Eran 0.0.1.16.219 - Feb 23 2017,
   * change the FTL version flow: burn the FTL version to the flash after the format is done


- Eran 0.0.1.16.218 - Feb 21 2017,
   * Merge IPv6 changes from CC3100 to Dragon (IPv6 under compilation flag)


- Lior 0.0.1.16.217 - Feb 21 2017,
   * Change assert to warning upon connection when there are scan results
   * Avoid redundant basic response when for illegal dns call


- Michal  0.0.1.16.216 - Feb 19 2017,
  * CC3XXX-202: add option to configure PSD mode (legacy / UPSD) therough host API.
       1. Move related parameters from const struct to ULL (l2_cfg.c, l2_cfg.h).
       2. Add api (SL_DEVICE_DEBUG_L2_CFG) for setting teh parameters.
  * Enable action frames in RX filters (forgotten in 0.0.1.16.209)
  * CC3XXX-212: re-init the time event structure on the APS module on role stop. 

- Eran 0.0.1.16.215 - Feb 21 2017,
  * Support SPI flash (under compilation flag)

    (namely, TCP & UDP recvfrom and sl_accept)


- Shaul 0.0.1.16.213 - Feb 20 2017,
  * move scan module periodic timer to ULL
  * when entering Sleep - verify there is no Time Event is GLX
  
- Dana  0.0.1.16.212 - Feb 19 2017,
  * CC3XXX-149: Test tool ipmemberadd and ipmemberdrop updated to open a new socket for the setsockopt, if socket isn't given
  * CC3XXX-144: Adding to the test tool dnsGetHostByName option not to run in separate thread.
  * CC3XXX-96: Creating run time analysis events for the full fast connect - in build_configs.h define
  *            RUN_TIME_ANALYSIS_FAST_CONNECT or RUN_TIME_ANALYSIS_DATA_PATH
  * new test tool 8.0.0.45

- Gilboa  0.0.1.16.211 - Feb 18 2017,
  * CC3XXX-188: IXIA test 1.19 fix, actually several fixes, here is the story:
  *    local server socket binded and listened (without accept) getting a connection from remote then closed locally
  *    during master socket close we should properly cleanup the secondary socket, but since it was in SYN-RCV state
  *    additional disconnect was needed prior to deletion.
  *    also, deletion in state SYN-RCV wasnt signaled back to the disconnect caller, so fixed that as well
  * Remove scan assert during stop scan (ShaulB)

- Yoav  0.0.1.16.210 - 16/2/2017, it's late on thursday night, I just want my pizza and to go to sleep
  * CC3XXX-193: BSD sockets PM fixes

- Amir  0.0.1.16.209 -
  1. WPA connection setup race condition - Minimize the chance for getting an encrypted packet from the peer
                                           in between the last eapol and the actual set key.
  2. ADDBA handling  send addba response with decline status (until feature is supported).
  3. PN replay detection, MIC failure detection, SW encription required detection.
  4. Bug fix - don't call set link state from cme context.
  5. CC3XXX-107  After Deauthentication sent from SUT, SUT sends Null Data packet


- Yoav  0.0.1.16.208 - context reduction driver.
 * CC3XX-172 (part 1): A new folder/file in the project called:
   nwp_crypto/sl_wolfssl_data_path_pools_mngr.h/c.
   This module handles the dynamic sized allocations for tls streams.

- Shaul  0.0.1.16.207 - Scan fixes
 * CC3XX-47: dont terminate scan cycle until all RX results which were received
   are processed.

-Gilboa 0.0.1.16.206
* CC3XXX-186 new sockopt for socket attribute. new host version (1.0.1.16.19 - backward compatible )
* CC3XXX-189 host-nwp shared headers comparison script fix
* CC3XXX-184 increase max parallel listen from 4 to 16
* CC3XXX-177 select with timeout error - fix select bitmap
* CC3XXX-169 increase amount of ULL sockets from 4 to 12 (the reason for the ULL large consumption)

- Shaul  0.0.1.16.205 - Scan fixes
 * CC3XXX-80: make sure that AP with lower RSSI and same SSID as other AP will receive lower score
 * CC3XXX-72: Scan module can do early termination according to RSSI and/or score threshold. stop
 *            counting results at scan module (not required for early termination)

- MichaelN  0.0.1.16.204 - Happy birthday to Harry Nyquist
    * New Internal Dispatcher mechanism that allows calling host-like APIs
      from within NWP threads (CC3XXX-185).
    * Fixed incorrect timestamp in AP probe responses (CC3XXX-134).

- Eran  0.0.1.16.203 - disable NAB recieve timeout
    * change NAB_RECIEVE_TIMEOUT_TICKS_DEFAULT_VALUE from 2 to 0

- Amir  0.0.1.16.201 - AES!!!
    * AES functionality both in station and AP (AP is hardcoded configured with no open)


- Yoav 0.0.1.16.201 - my 100th night run anniversary
  * CC3XXX-178 - rx complete RT interrupt calls threadx service functions.


- Lior 0.0.1.16.200 - Host driver alignment to MCPI 2.0.1.15
please note! *major* driver changes applied in this delivery.


- Shaul 0.0.1.15.199 -
  1. at LPDS Freeze - reset 200 KB of GLX with 0x57575757
  2. just before sleep raise priority to highest - this makes sure WFI will be
     the last commane before sleep.
  3. if there is not enough time to go to sleep - do cortex gating
  4. reset GLX Heap after LPDS Restore (it must be empty at sleep entry)
  5. add LtxpQueues_initAfterSleep() + cme_InitAfterSleep() to init functions after sleep
  6. remove UDP_CONNECT PM Activity
  7. add "_ull" to some parameters in the code
  8. add ability to trigger ocla from _handlerHardFaultTop (commented)

- Michal 0.0.1.15.198 - debug clean up
* Fix debug PSD to better support sleep sequence (move pointer to ULL).
* Add debug define in build_configs.h
* Change PSD debug function from inline to regular function.

- Eran 0.0.1.15.197 - add flash SPI files + fix AP bug
* add files to support flash SPI function
* fix AP_init configuration

- Yoavby 0.0.1.15.196 (January 29, 2017) - this is the first day of the rest of my life (as MGTS)
* add sl_bidir_blocks_pool module
* some other minor fixes
* added 3des code to compilation (accidentally removed as unsupported in CC31/2xx)

- Michal 0.0.1.15.195 (January 29, 2017) - STA PSD support
* Add STA PSD support to FW.
* Add option to configure traffic module parameters for testing (setPowerConfig)
* Change disable all interrupts to desibale medium interrupts in malloc / release functions.

- Eran 0.0.1.15.194 - Flash functionality
* Flash functionality: ability to FORCE FLASH PROGRAM even when the g_persistent is 0. Ignore the global persistent variable.

- Eran 0.0.1.15.194 (January 24, 2017) - ignore persistent API


- Gilboa 0.0.1.15.193 (January 23, 2017) - BSD Accept mechanism and fixes
* worker thread add ULL force mode for BSD
* BSD session end using the new WT mechanism as ull force and if fail glx fail
* nab flush semaphore tie to the actual semaphore in rx rst case
* tcp rx reset release mutex when waiting on rxbc0 event
* fix connect on non existing socket or exceed 32 index socket - CC3XXX-164
* avoid suspended thread udp bind (bsd and dns case) - CC3XXX-157
* new host driver 0.0.1.15.17 - changes sl_WlanSetMode to legacy ROLE_STA/ROLE_AP settings + all the multi-role combinations



- Eran 0.0.1.14.192 (January 23, 2017) - fix TCP bug: TX with large packets failed
    when pulling packet from the out_of_window_queue and "hole" created the tx_window_current is
    wrong since that it count bytes and not seq_num

- Yoav 0.0.1.14.191 (January 23, 2017) -
  19 years since Netscape announced Mozilla, with the intention to release Communicator code as open source.
   * Fix CC3XXX-165 - BSD read/modify/write race condition from two contexts (two different sockets) on a global bitmap.
   * Fix CC3XXX-166 (on behalf of michal) - FPGA RSSI reading error


- Yoav 0.0.1.14.190 (January 16, 2017) -
   Galius Julius Caesar Octavianus is granted the title Augustus by the Roman Senate,
   marking the beginning of the Roman Empire
   Let this be the beginning of the Dragon Empire
   * Fix CC3XXX-154 - BSD_IN_EXE error to host due to reverse ordering of event to host with BSD thread freeing.

- Michal 0.0.1.14.189 (January 15, 2017) - new FPGA 181
   * FW aligned to FPGA (181) - including new PHY control, new crypto sub-system and PMAC AES.
   * Main changes in SW are PHY address space for registers and scripter.
   * New code composer configuration is required (Daisy Chain).
   * Note - AES is not yet supported in this FW version !


- Shaul 0.0.1.14.188 (January 15, 2017) - PM changes
    1. allow sleep evaluation only if current thread == execute thread == idle thread
    2. socket pm request - set when block pool is allocated and removed when it is released
    3. glx sockets - threadx control block is ull
    4. before sleep - verify that ULL block pools are not pointing to GLX
    5. add logic in the PM to present the current requests per resource in bitmap
    6. move SUPPORT_MEM_STATISTICS to build_configs.h as it is used by PM
    7. remove PM client for flash manager (replaced with worker thread PM activity)
    8.  add PM client for worker thread destructor
    9. remove loggerapp.c file
    10.fix an iop issue: it might be that the AP is sending the beacon before the expected time.
       in this case next calculated beacon TSF will be in short time. BUT actually next beacon is in DTIM Period from now.


- Eran 0.0.1.14.186 (January 8, 2017) - flash features
    1) ability to run the flash_manager as task (and not worker thread)
    2) add Date & Time configuration element
    3) add Data & Time data element
    4) sync the NWP FTL code to the FTL simulation code

- Lior 0.0.1.14.185 (January 3, 2017)
    - Add ApRemovePeer netBound support

- Dana 0.0.1.13.184 (Jan 3, 2017) - Bug fixes
  1) CC3XXX-143: isInternal field of the slDnsMsg_t gets run over when allocating pName
  2) CC3XXX-142: Assert on reconnect with Fast policy
  3) CC3XXX-132: getSockOpt KEEPALIVE doesn't verify TCP socket only

- Eran 0.0.1.13.183 (Jan 1, 2017) - add flash elements - support persisten mode
  1) DEVICE_PARAMS element: mac_address
  2) DEVICE_PARAMS element: persistent.

- Gilboa 0.0.1.13.182 new years, first 2017 delivery (Jan 1, 2017) -
* add support to multiple selects in nwp bsd (not supported by driver yet)
* worker thread support allocation from ull then glx
* worker thread changed API to single flag caller input argument
* worker thread support PM hook for GLX based worker threads
* set-connection-policy when no role configured - bug fix
* consequitve scan attempts counter zero when reading profiles from flash after reboot - bug fix
* new host version 0.0.1.13.15 - add "handler" to select operation, preperations for multiple selects (not supported yet)
* new testtool based on new host - 8.0.0.39


- Eran 0.0.1.13.181 (December 28, 2016) - add flash elements
  1) cme: Preferred Profile
  2) cme: Connection Policy
  3) cme: Enterprise Profile

- Shaul 0.0.1.12.180 (December 26, 2016) -
    1. calculate LPDS sleep duration
    2. restore sleep clock with TSF instead of RTC
    3. restore uart + logger after sleep wakeup
    4. close rx filter at sleep entry
    5. flush logger before sleep
    6. prevent sleep for less than 10 ms
    7. restore watchpoint after sleep instead of full init
    8. add descturctor to worker threads that use GLX. the descturctor is used to remove the GLX retention request
    9. fix a bug in TimeScheduler_GetTimeToEarliestWakeupEvent
    10. fix a bug in uart_is_idle
    11. scan cncn - fix a bug. replace periodic requst to be ull
    12. fix by Lior - sl_host_api_timeout_update_timer_ticks(). don't send host event
        as it used ThreadX resources when interrupts are disabled

- Amir 0.0.1.12.179 (December 22, 2016) - Bug fix: Probe requests were filtered by the pmac

- Eran 0.0.1.11.178 (December 22, 2016) - add flash elements
  1) AP IpConfig
  2) Logger
  3) fix some elements flows (add ULL instead of GLX)

- Lior 0.0.1.12.177
>> Add Remove peer support (from host to CME)
>> Final part *must* be implemented

- Eran 0.0.1.11.176 (December 19, 2016) - add flash element and fix close UDP socket
  1) fix bug in close UDP socket
  2) add RoleApps flash element

- Gilboa 0.0.1.11.174 (December 18, 2016) -
* Recvfrom and async response family type fix ( CC3XXX-116)
* NAB allow udp sendto on create, and not only on bind
* host driver move all ctest related defines to testlib slctst.h
* new host driver 0.0.1.11.13
* new test tool 8.0.0.35


- Amir 0.0.1.11.175 (December 19 2016)
  UAPSD configuration

- YoavBY 0.0.1.10.173 (December 15, 2016) -
  * CC3XXX-111 fix - CLOSE command from host cut RX RST thread on worker group causing a deadlock
  * add MMU prints to the pools state in periodic statistics

- Lior 0.0.1.10.172 15-Dec-2016
- add big element support: support up to 4K payload

- Eran 0.0.1.9.171 (December 15, 2016) - Some flash features
  1) adding Dhcp_Server_Parameters as Flash element (program and read from flash).
  2) adding flashFormat command to the test_tool and to the NWP.
  3) adding FTL_version to the ftl code and program the version to the flash, in case that the version_Major
     changed the FlashColdInit will execute flashFormat first.


- Amir 0.0.1.10.170 (December 14,2016) - ULL/GLX handling for enabling sleep in between scan periods.

- Eran 0.0.1.9.169 (December 13, 2016) - Add DnsConfig element to Flash
  1) ability to save the DNS Config to flash


- YoavBY 0.0.1.10.168 (December 13, 2016) -
* warnings remove

- Gilboa 0.0.1.10.167 (December 13, 2016) -
* start-stop-start crush due to osi_sleep in the flash i2c driver (CC3XXX-119)

- Shaul 0.0.1.10.166 (December 13, 2016) -
  1) pwr mgt - wakeup from timer and host commands
  2) multi role client request PHY Full istead of NWP Awwake
  3) devide register to restore: cortex + soc registers. for now soc registers are not restored as they keep their values
  4) add new PM client for host command malloc

- Gilboa 0.0.1.10.165 (December 13, 2016) - driver element fix
* SL_DEVICE_EVENT_MAX is defined as define and as enum (different values) - CC3XXX-117
* new host version (0.0.1.10.11) and new test tool (8.0.0.33)
* fix hdi init/elp partition (moved registers init to warm boot)


- Eran 0.0.1.9.164 (December 12, 2016) - Add IpConfig element to Flash
  1) ability to save the Ip Config (9 profiles) to flash

- Amir 0.0.1.9.163 (December 12, 2016)
  1) GLX / ULL memory handling in supplicant, CME and cc33xx driver

- Dana 0.0.1.9.162 (December 8, 2016) -
  1) (CC3XXX-35) Multicast L2 filtering management supported
  2) (CC3XXX-86) getRtcToSec bug fix

- Eran 0.0.1.9.161 (December 8, 2016) - bug fix
  1) while static IP set (NetCfg) add the ability to clear the netCfg parameters

- Eran 0.0.1.9.160 (December 7, 2016) - bug fixes
  1) (CC3XXX-108) Netapp event triggered twice if static Ipv4 is set
  2) (CC3XXX-109) NetCfg results not cleared after disconnection, if DHCP is set

- Eran 0.0.1.9.159 (December 7, 2016) - Support Flash (I2C)
  1) support full flash feature (flash_manager & FTL). works based on I2C flash
      in order to add new flash element - follow the ftl_export.h instructions
  2) device name saved in the flash: setDeviceName Program the device to the flash
     and getDeviceName read it from the flash.

- MichaelN 0.0.1.9.158
  Fixed incorrect frame expiry criteria in interface_nx.c

- Lior 0.0.1.9.157
  NetBound APIs support
  
- Yoav 0.0.1.8.156 (November 30, 2016) -
* Logs  removed major contributors to traces


- Shaul 0.0.1.8.155 (November 28, 2016) -
    reduce role types: (BT, BT Calib, DFS, Measure)
    pwr mgmt fix - add/remove request: resource should be bit and not index
    RPU - delete RPU_APS_State_Notify_Func as it is not required anymore
    process scan result - check bssid as well
    fix a bug at elp wakeup at reinit_system_handlers_priorities

- Gilboa 0.0.1.8.154 (November 28, 2016) - optimizations and reductions
     Logs Supports GTRACE complete shut off (GTRACE_ENABLE compilation flag now works).
       Asserts  support enable/disable all asserts from build config.
    (Note: Only when GTRACE and ASSERT are disabled we can activate the subsection unreferenced optimization and then create the deadfunks list)
       Warnings (cleaned up ~50 warnings), some actions below
       Cleaned up some Todos although it is a big mess (lots of to-do's imported from previous projects), some interesting actions below
       Osi preemption apis moved from static to non-static.
       Deadfunction list  removed a lot of un-referenced functions from the code, from all around (~20K code). Left out what seems to be candidate for usage in the future+ all packages that havent completely integrated - FTL, Supplicant and wolfSSL. Attached updated dead-function list. I suggest we will revisit this list once wolfSSL integration with SecSS is done, as well as supplicant AP+P2P mode will be completed.


- Eran 0.0.1.8.153 (November 22, 2016) - reduce FTL traces
* reduce trace in FTL module

- Shaul 0.0.1.8.152 (November 21, 2016) -
    1) add code for wakeup from sleep. the code is triggered from idle task by calling SVC 0.
       if going to sleep from SVC than restoration of registers at wakeup is seamless and is done by Threadx
    2) AP - add requirement to be awake from cme instead of FW
    3) add support to suspend PSD triggers from ASU + RPU (for PSD feature)
    4) move initialization of modules to pwrmgmt instead of initappcc33xx.c
    5) delete sl device name init function. it is called at init of dns server
    6) move scan module queue to glx

- Eran 0.0.1.8.151 (November 21, 2016) - Support FTL code
* Support FTL code, run on windows simulation and on NWP I2C flash

- Yoav 0.0.1.8.150 (November 21, 2016) -
 164 BC  Judas Maccabeus, son of Mattathias of the Hasmonean family,
 restores the Temple in Jerusalem.
 This event is commemorated each year by the festival of Hanukkah (or Pestigal in recent years).
* init wolfssl fixes and measurement of aes encrypt/decrypt functions of 1410 bytes.
* many prints rearranging (e.g. giving them group ids)
* removal of wolfssl session/client cache functionality
* added OCLA trigger on hard faults of the ARM
* removed redundant tests from the code (e.g. timers_mng_test and calib_agent_test) by surrounding with proper ifdefs
* added updated python test for tls.

- Michal 0.0.1.8.149 (November 20, 2016) - NEW FPGA - 160 !!!
* Support PHY abort by PMAC.
* APS fixesFALSE
* LTXP queues memory partiotin - ULL / GLX
* L2 data path client for power management - first draft.

- Shaul 0.0.1.8.148 (November 13, 2016) - power mgmt work. init after sleep functions,
  second wakeup handling. ot functional yet

- Shaul 0.0.1.8.147 (November 13, 2016) - fix bug in scan. cme fail to start new
  one shot connection scan when survey scan is ending

- Eran 0.0.1.8.146 (November 13, 2016) - Add FTL functions (directories and files)

- Amir 0.0.1.8.145 (November 11, 2016) - Donald Trump is the president...
* Supplicant tls implementation over wolfssl
* EAP enterprise support

- YoavBY 0.0.1.8.144 (November 9, 2016) - Donald Trump has not won the US presidency.
* fix CC3XXX-20 - drop packets with unrecognized ethertype by HW (or TODO in the future by lrxp SW).

- Gilboa 0.0.1.8.143 (November 7, 2016) -
* scan early terimnation threshold configuration from CME support (CC3XXX-73)

- YoavBY 0.0.1.8.142 (November 6, 2016) - Zohar Argov is not with us for exactly 29 years
* wolfssl update to latest and greatest version (from 3.9.6 to 3.9.10)

- YoavBY 0.0.1.8.141 (November 6, 2016) - Zohar Argov is not with us for exactly 29 years
* wolfssl client connect test fixes

- Eran, 0.0.1.8.140 (November 3 2016) -
* bug fix: upload "dns server ip" to host per getIp command
* bug fix: support up to ssid len = 32
* in case that AP set gateway ip address while STA already connected - ignore the AP setting

- Dana, 0.0.1.8.139 (November 2 2016) -
* new test tool (version 30) - tcp rx and tx support non blocking. tcp rx also supports port, priority etc.
* getSockOpt supports priority

- Gilboa, 0.0.1.8.138 (November 1 2016) -
* New FPGA version 33000148 support
* new NAB recv byte count on the fly
* cme connection manager fix priority of profiles connection (CC3XXX-76)

- Gilboa, 0.0.1.8.137 (october 31 2016) -
* fix one shot scan bug not pulling profiles (Shaul)
* fix cme eap connection going to profiles_connection in default
* fix deauth reason_code in bssloss case (CC3XXX-75)

- Gilboa, bim bam bam, 0.0.1.8.136 (october 27 2016) -
* wlan fast connect scan frame support intermittent mode, triggered by wlan_connect
* store fast connection in ull, dynamically upon request, support toggeling of fast connect policy
* optmize fast connection ull memory in adhoc case (no profile needed)
* support wlan_connect(PROFILES) - one shot profiles connection, driver and NWP
* support fast connect for wlan_connect(PROFILES) rely on the last connected profile
* new test tool (version 29), allowing wlan_connect -c PROFILES

- YoavBY 0.0.1.8.135 (october 27 2016 - exactly 81 years since the patent on water skis by Fred Waller)
* wolfssl integrated
* added wolfssl bringup tests:
  a. Certifications time verifications
  b. SSL client with one cipher suite (ECDHE). Works with python on the PC side: JSimpleSSLServer.py

- Shaul 0.0.1.8.134 (october 26 2016 )
 * add ELP Wakeup Init functions
 * move some parameters to ULL
 * add code to init after reset for first time - not first time. not functional yet
 * add code to save and restore registers before/after ELP. not functional yet

- Eran 0.0.1.8.133 (october 18 2016 )
* cosmetic change: Moved the init of the g_simplednsServerSocket to DnsServer_init function. delete the dns_wakeup function

- Michal 0.0.1.8.131 (october 13 2016 )
* Add station autonomous power save (APS) module.

- Eran 0.0.1.8.130 (october 13 2016 - ready for power down)
* move some variables to ULL in order to support ELP. (DNS client - g_ipResult, flash: , )

- YoavBY 0.0.1.8.129 (october 09 2016 - happy birthday Sean Lennon)
* fix to the tcp client tx loop (bad sd instance id in hdi_mgmt)
* cyclic buffer module moved from sole possession of pmac tx/rx (configurable with build_configs.h flags)
* rxpp module enable/disable (configurable with build_configs.h flags)

- Shaul 0.0.1.8.128 (octp 06 2016)
* verify wlan lp thread queue is empty before stop role is completed

- Shaul 0.0.1.8.127 (sep 27 2016)
* stop one shot connection scan if ET threshold exceeds
* if no role is left resumed - close rx filters (done from sched_state.c when moving to NULL State)
* enable ssid filter in basic scan
* remove rx filters obsolete code from basic scan
* add pm clients (not operational)
* move scan adater message queue to glx
* traffic module should move link to active after beacon rx if rx threshold is 1


- Michal 0.0.1.8.126 (sep 26 2016)
* New FPGA - 147 !!!!!
* Support PMAC RX prioirty - dropping data frames when memory is almost full in order to save room for RX management frmames.
* Add some debug tools in NAB, PMAC & MCU.

- Michal 0.0.1.8.125 (sep 21 2016)
* add API for dsender address (MAC source address) - CC3XXX-40

- Gilboa 0.0.1.8.124 (Sep 20 2016)
* mmu cyclic buffer debug support for hdi and rxx
* sl test tool version 27 support udp rx non blocking for Naftali (-nb 1)

- Eran 0.0.1.8.123 (Sep 20 2016)
  * move the dhcp_client_socket structure from the heap to be global ULL
  * move the dhcp_client_server_socket structure from the heap to be global GLX
  * dns server fix bug: while copy the rcvPacket to the sendPacket the copy_len was too big.
- Michal 0.0.1.8.122 (Sep 19 2016)
  * Add support for PMAC RX filters
  * Fix channel tune
  * extend SW RX TAG usage
  * Update clean RX pipe to support the new addition of RXPP (used by stop role sequence)
  * Add debug information for different SOC errors.

- Shaul 0.0.1.8.121 (Sep 15 2016)
  * fix memory leak in scan cncn

- Shaul 0.0.1.8.120 (Sep 14 2016)
  * print statistic about heaps max in use
  * move ping thread stack to gStackPool_glx

- Dana 0.0.1.8.119 (Sep 14 2016)
* (CC3XXX-51) Fix driver ActiveActionsBitmap to support 32 sockets

- Gilboa 0.0.1.8.118 (Sep 13 2016)
* (CC3XXX-52) Fix makefile clean not removing files in linux compile, and dependency of scripters which required 2 compilations every time -
* (CC3XXX-53) Support worker thread group priority insertion, during calling worker thread which is already running (in group mode), support
              priority of insertion (0 is lowest). motivation is the flash manager which is coming.

- Shaul 0.0.1.8.117 (Sep 13 2016)
    1)  Move the following thread stacks to ULL: WLAN LP + HP, CME, LTXP, UTXP, IP, DHCP Client, Idle, Host IP
    2)  ASU + RPU  remove RX Filter bit (was used only for coex antenna mode of rx/rx)
    3)  Presence adapter  when MR is enabled  do not add request if role is suspended
    4)  Scan module  if result causes early termination of cycle, first save the result to CME DB
    5)  WLAN LP + HP + CME: turn on stack protection
    6)  WLAN LP + HP: decrease stack size from 4 KB to 2 KB
    7)  Add cortex gating code  not functional yet
    8)  Increase one shot guard timer to 45 seconds

- Michal 0.0.1.8.116 (Sep 11 2016)
* Dynamic load of PMAC TX & RX scripts (not as rom, but as ULL).

- Amir 0.0.1.8.115 (Sep 08 2016)
* When scan fails, Supplicant performs 3 'normal scan' retries with 5 sec interval,
  before entering sched scan. Interval reduced to 1 sec.

- Amir 0.0.1.8.114 (Sep 06 2016)
* 'Connected role ID' in links database was not initialized
* Sometimes the driver sends 2 stop scan commands to the scan module -
  one for periodic and another for connection scan

- Yoav 0.0.1.8.113 (Sep 06 2016)
 * Add debug info on packets on assert
   (in assert, stop MMU alloc/deallocations and go over allocated descriptors and print info)
 * Add assert in case of tx frames being stuck in NAB and do not reach the utxp fifo (happened in close of TCP socket night run)

 Gilboa (Israel-Italy is on today!) 0.0.1.8.112 (Sep 5 2016)
 * ARP flush avoid in station deinit. instead, flush ARP table only in case station re-connecting to a different
  bssid then the last one

- Dana 0.0.1.8.111 (Aug 31 2016)
 * utxp_tx_is_internal_tcp_socket_valid bug fix - socket pointer check if instance
   id does not matches currently opened socket's instance id not relevant for TCP

- Gilboa 0.0.1.8.110 (Aug 30 2016)
 * TCP RX bug fix - illegal packet ram access after NAB released packet

- Gilboa 0.0.1.8.109 (Aug 29 2016)
 * rxpp scripter enable - pmac handing off packets through the scripter now on the way to lrxp
 * fixed bug in deliverytest python
 * new ARP features
 * msp432 platform testlib support (not tested yet)


- Shaul 0.0.1.8.108 (Aug 29 2016)
 * change one shot scan from 10 to 30 seconds (dhcp takes ~25 seconds in which
   it maintain mgmt seq request to MR Scheduler preventing scan request to run)

- Dana 0.0.1.8.107 (Aug 29 2016)
 * CC3XXX-22: enlarge amount of BSD sockets to 32
   CC3XXX-29: disallow BSD simultaneous operations in the same socket
   CC3XXX-23: dateTime not set should return invalid

- Shaul 0.0.1.8.106 (Aug 29 2016)
 * fix a bug in scan module: scan is ended and a lrxp try to push frame
   to the thread queue just after queue is deleted

- Shaul 0.0.1.8.105 (Aug 24 2016)
  * relese periodic scan plan memory when it is ended.
  * fix corner case wherer probe request (out of sync) was not released.
  * fix radio tune stuck issue by doing second tune (by Michal). although
    it seems that second tune might not work well - it prevent the stuck.
  * add power manag,ent files - disabled


- UTXP internal netx socket instance add- Gilboa 0.0.1.8.104 (Aug 22 2016)

- New Supplicant integration - Amir 0.0.1.8.103 (Aug 22 2016)
- Based on the following supplicant open source commmit:
commit f885b8e97cf39b56fe7ca6577890f2d20df7ae08
Author: Jonathan Afek jonathan@wizery.com
Date:   Tue Jun 7 16:42:43 2016 +0300
    tests: ap_ciphers tests start using general cmd_execute() function
    The ap_ciphers tests used to do file operations locally in python. Start
    using the cmd_execute() general function for file operations so that
    this would also work on remote setups.
- Scan bug fixes
- BSS lost event handling
- Supplicant WPS registrar code reduction


- fix stop periodic scan bug (between cycles) - shaul 0.0.1.8.102 (Aug 17 2016)

- New PMAC RX  - Michal 0.0.1.8.101 (Aug 16 2016)
* New FPGA is required - version 146 !!!!!

- Post association configuration is done though command from CME. queues are suspended
  before operation and resumed after.  - Shaul 0.0.1.8.100 (Aug 15 2016)

- Keep Alive - let TX complete release descriptor. also, add service to send Null data
  for keep alive/aps/... - Shaul 0.0.1.8.99 (Aug 14 2016)

- BSD connect non-blocking bug fix - Dana 0.0.1.8.98 (Aug 8 2016)

- don't send while role_id is 0xFF - Eran 0.0.1.8.97 (Aug 7 2016)

- Keep Alive + fix some WLAN LP+HP functions names - Shaul 0.0.1.8.96 (Aug 7 2016)

- fix bug in dhcpClient_addMacToDomainNameIfNeeded  - Eran 0.0.1.8.95 (Aug 3 2016)
* take the role_id from the interface_id and not as const

- new git server bitbucket - Gilboa 0.0.1.8.94 (Aug 2 2016)
* change git hooks to verify if proper jira bug id is set

- add DNS  mgmt sequence - Shaul 0.0.1.8.93 (July 31 2016)

- Dhcp Server - fix bug - Eran 0.0.1.8.92 (July 31 2016)
 * while dhcp server de-init run: need to Free all records

- UDP RX fixes - Gilboa 0.0.1.8.91 (July 28 2016)
- fix some sl test tool close thread routines when halting udp test (new sl test tool 8.0.0.24)
- fixed sl close bug when recieve timer was ongoing - proper cleanup of the receive timeout in nab and in sw

- add ARP mgmt sequence - Shaul 0.0.1.8.90 (July 28 2016)
  also, change how Netx send mgmt sequence commands to WLAN LP Thread (as Netx run
  from several threads). send directly to WLAN LP and not through CME service.

- mgmt fixes/changes - Shaul 0.0.1.8.89 (July 25 2016)

- DateTime bug fixes - Dana 0.0.1.8.88 (July 14 2016)
 * soc_close bug fix - not releasing socket IDs for internal sockets
 * NUM_NAB_SOCKETS bug fix

- Euro 2016 final today - Gilboa 0.0.1.8.87 (July 10 2016)
 * add statistics for arp table and mutex owners if exist
 * add phy rf balcony new values
 * bsd lock element divided to glx and ull

- add dhcp + security mgmt sequences - Shaul 0.0.1.8.86 (June 21 2016)

- TCP Connect Loop 2nd night run pass since 1st night run pass back in v0.0.1.8.84 - Yoav 0.0.1.8.85 (Happy July 4 2016)
* Fix issue of CLOSE command preempting the Rx RST handling

- TCP Connect Loop 1st night run pass since ages ago - in the words of GilboaS - "Well, done" - Yoav 0.0.1.8.84 (June 27 2016)
* mutex get in reset worker thread was missing (was available only in close).
* Caused many disasters to happen (no rx handling and no tx handling).

- AP deinit flow change - Amir 0.0.1.8.83 (June 22 2016)
* Deauthenticate is blocking
* AP deinit is performed in one step in CME
* Deinit is started by calling wpa_supplicant_deinit_iface() (no need to explicitly call to deauthenticate in CME)
* Stop role is now performed in .deinit_ap() supplicant driver API
* Disable role is performed in .deinit() supplicant driver API

- power save control - Shaul 0.0.1.8.82 (June 21 2016)

- Bug Fixes - Yoav 0.0.1.8.82 (June 16 2016)
* RF Table (more entries)
* ARP and ICMP Reply from management pool (they were copies from default pool, and when no resources, they were not allocated)
* Fix not to allow the host to close non-exposed socket IDs. This happened when closing listener sockets.

"DHCP server mutex bug fix, TX result session ID check" - Amir 0.0.1.8.81 (June 15 2016)

"auto mode for date and time" - Dana 0.0.1.8.80 (June 14 2016)

"new FPGA 110 RF balcony fixes" - Gilboa  0.0.1.7.79 (June 08 2016)
* BSD structure size reductions
* New FPGA 110 support with RF balcony gain value settings on init upon FPGA dip switch value


- Introduce worker threads builder module - Yoav 0.0.1.7.78
- BSD activities features. 
  * Close, RX fin and Rx RST are done in parallel (up to 2 in ULL and 2 in GLX with no redoing of the same activity twice). 
    If more than the allowed number of parallel activities are requested, worker threads are stacked and ran in serialization 
    to complete these activities.
  * Connect/Accept are all done in parallel (and their worker thread resources is static in DB RAM).

"DHCP client - bug fix" - Eran  0.0.1.7.77 (June 07 2016)
-  replace ip_gateway_set by ip_gateway_clear

"Time scheduler and other bug fixes" - Amir 0.0.1.7.76 (June 06 2016)
* Time scheduler fix - On timer interrupt, remove only timer events that really expired.
* Check session ID validity on RX management packets and EAPOL packets.
* Auth timeout & Assoc timeout fix - timeouts were not cancelled properly.

"DNS client - cache optimization" - Eran  0.0.1.7.75 (June 06 2016)
*  access cache only if cache is not clear
*  direct access to global g_DnsCache val (without any copy to temp buffer)

"Yeruham is on the map delivery" - Gilboa 0.0.1.7.74 (June 05, 2016)
* Hostap os.h and os_internal.c logic move to Services/osi.c and osi.h having a share location for all OS/toolchain services
* NAB UDP socket set max paylod size = pu so that send with buffer bigger then mtu will fail (not supporting tx ip frag)
* new sl test tool allowing '-large' in udp tx to allow 32K udp buffer send in the fpga so that we can bypass the spi transaction latency
* new delivery-test python complying with the above new UDP approach
* DNS clear cache entries upon following events:
    - TCP max send retries
    - TCP keepalive timeout
    - ICMP dest unreachable on selective icmp codes


"add internal api - ClientDnsResetCache" - Eran  0.0.1.7.73 (June 02, 2016)

"Link session ID allocation" - Michal  0.0.1.7.72 (May 31, 2016)
* LINK SESSION ID allocation was moved from cc33 driver responsibility to FW links module


"DNS client/Server done (testing completed" - Eran  0.0.1.7.71 (May 29, 2016)
* fix the cache aging (reset only when connection w/ DNS server success)
* fix string issues

"Reduce the number of changes in Supplicant code (Supplicant code is based on mcp 8.0.0.47)" - Amir 0.0.1.7.70 (May 26 2016)

"DNS client - clean up and bugs fix" - Eran  0.0.1.7.69 (May 26, 2016)
* clean up after code review

"Lag Baomer delivry" - Gilboa  0.0.1.7.68 (May 25, 2016)
* enlarge amount of bsd sockets to 31, 4 ull and 27 glx, shared pool for internal/external
* switch between MMU mgmt pool and TCP ack pool. now mgmt pool is the highest (with 4 gaurenteed memblocks).
* new simplelink host driver (0.0.1.7.7) with protocol.debug.h showing the actual opcodes

"str and mem stuff" - Gilboa  0.0.1.7.67 (May 19, 2016)
* string and memory os_internal functions move to ti-compiler intrinsic functions

"Internal GetHostByName" - Eran  0.0.1.7.65 (May 18, 2016)
* ability to call to sl_Internal_dnsgethostbyname from internal NWP thread

"Device_name (URN) re-design" - Eran  0.0.1.7.62 (May 16, 2016)
* support device_name DB of 256 char
* save it in flash
* per protocol (DNS/DHCP/SSID) the proper string derived from the basic device_name DB


"DNS client cache depth - 2 entries" - Eran  0.0.1.6.60 (May 8, 2016)
* support cache of NUM_OF_CACHE_ENTRIES entries

"fix bugs and warnings" - Eran  0.0.1.6.59 (May 3, 2016)
* while working with static IP or with DHCP LLA the DNS servers are set right

"Leicester city FC is the premier league champion" Gilboa Rev 0.0.1.6.58 (May 3, 2016)
* new sl test tool (version 16) with new delivery test method + python, added flash test and udp tx/rx


"Support DNS server" - Eran  0.0.1.6.57 (May 2, 2016)
* works on IP thread

"Happy bread and also International Workers day (this is why this is about worker threads)" - Yoav 0.0.1.5.56 (May 1, 2016)
* Fix memory allocation of worker thread stacks to be the same as socket memory type attibute.
* Fix is ongoing tx to support smooth closing of bound UDP sockets.

"passover over thank god" - Yoav 0.0.1.5.55 (April 27, 2016)
* fix priorities

"happy passover" - Gilboa 0.0.1.5.54 (April 24, 2016)
* sl stop support timeout

"new test tool stuff" - Yoav 0.0.1.5.53 (April 24, 2016)
* Linger fixes to sltesttool and to NWP

"new test tool stuff" - Gilboa 0.0.1.5.52 (April 18, 2016)
* making the test tool thread based (Shaul)
* several fixes related to the test tool and support multi interface

"DNS cache and fast_dhcp work with RTC clock" - Eran 0.0.1.5.51 (April 14, 2016)


"check for IP collision on Client" - Eran 0.0.1.5.50 (April 14, 2016)
*  run on ROLE_STA
*  check collision per arp_receive

"DNS client - saving the cache in the flash (and not in ULL global)" - Eran 0.0.1.5.49 (April 13, 2016)

"DNS client - fix bug: pointer null" - Eran 0.0.1.5.3 (April 12, 2016)

"DHCP server - fix bug while server is disabled" - Eran 0.0.1.5.2 (April 12, 2016)


"Support DNS Client" - Eran 0.0.1.5.1 (April 10, 2016)
* add checker in function netApp_Ipv4DhcpServerDeInit to make sure the pointer is not NULL

"Support DNS Client" - Eran 0.0.1.5.0 (April 10, 2016)
*  Set/Get DNS configuration
*  DNS protocol with DNS server
*  cache result in NWP

"Whether you think you can or think you can't  you are right." - Henry Ford (died exactly 69 years ago) - Yoav 0.0.1.4.48 (April 7, 2016)
*  A nice wrap around catch bug on the relative tx expiry time.

"This version doesn't contain any camel meat - can you say that about your lunch?" - Michal 0.0.1.4.47 (April 5, 2016)
*  New FPGA version - 99. FPGA new content -
   - PHY support for AGC (under compilation flag in buid configs, close at default).
   - PMAC TX support for txop burst.
   - PMAC TX BA seq number wrap around bug fix.
   - Code & Data memory size reduction.
*  SW support for different sizes of OCLA allocation.
*  TP optimization fix - decrease the sleep time after sl_send failure (200->50).
*  Force BA enabled by default in the build config, enabling TP tests.
   Won't affect anything unless you use MCS rates (11n).



"osi stands for iso backwards" - Dana 0.0.1.4.46 (April 4, 2016)
*  set/get time/date added input validation checks
*  host - added test to the sltesttool (-getTime and -setTime).

"osi stands for iso backwards" - Yoav 0.0.1.4.45 (April 4, 2016)
*  If you malloc multiple pointers... rollback those who succeed
*  fix group worker threads to release their params only after running theentry function.
*  os resources - fix thread HTTP/DHCP priorities to be unique (though HTTP is currently unused), and also add some parenthesis to defines to avoid annoying bugs.

"memory memory memory..." - Gilboa 0.0.1.4.44 (April 3, 2016)
*  Allocation validator, return real RC from the workerThreadCreate indicates if all allocations succeeded and thread created successfully.
*  All BSD workerthreadCreated return code read and respond accordingly  bsd commands return proper error code (ENOMEM), reset/fin handler just gtrace it (nothing else to do).
*  Add memory wrapper mem_malloc_ull_then_glx module, attempt allocation from ULL and if failed try GLX. This policy can be used in case no memory for ULL allocation, before failing operation (if no other choice)
   we can later determine which modules / worker threads should use this capability.


"tommorow is April 1st but its friday" - Gilboa 0.0.1.4.43 (March 31, 2016)
* Fixed HDI bug due to role stopping flag
* changed HDI script source attribute to 0
* some ip rx redundent code

"Happy birthday to me" - Yoav 0.0.1.4.42 (March 29, 2016)
* Fixed the connect on connect (bug was that the nwp disconnected and didn't continue to the next connect)

"Purim delivery, Esther would be proud " - Gilboa 0.0.1.4.41 (March 23, 2016)
* change netx interface indexing into defines - NX_IFC_STA/NX_IFC_AP
* host driver - nwp share headers checker in the end of every compilation
* API compatibility number checker between release.c NWP version and simplelink.h Host driver version
* Date and time API support get / set (manual mode only) - Dana


"Support DHCP server (completed)" - Eran 0.0.1.4.40 (March 16, 2016)
* support up-to 32 clients that can get address from the DHCP service that run on AP ROLE
* Dynamic allocation the records for the clients
* support collision on allocation IP

"Support DHCP server - up-to 32 clients" - Eran 0.0.1.4.39 (March 16, 2016)
* support up-to 32 clients that can get address from the DHCP service that run on AP ROLE
* Dynamic allocation the records for the clients
* need to take also the host since that NetAPP API changed

"1876  Alexander Graham Bell is granted a patent for an invention he calls the 'telephone'." - Yoavby 0.0.1.3.38 (March 7, 2016)
* Using the new versioning method - the new major number in this version is now 3
* Added new socket handler event for completion of the socket close command (disabled by default).
* Host change only.


"API compatibility versioning" - Gilboa 0.0.1.2.37 (Feb 28, 2016)
* new versioning method - major number (in this version-2) is now the
  API compatibility version number between NWP version and Host driver.
  so in order to match NWP version and SL Host driver version look for match in the API comp' number


"almost last day of the month" - Shaul 0.0.1.1.37 (Feb 28, 2016)
* new FPGA support - 95: fix TX stuck due to RX Abort
* add sequence to make sure immediate response is 1completed before tune
* AP - Beacon is delivered to the Supplicant for protection processing.
* AP - disable queues before Beacon TX
* AP - add pending desc id to allow new beacon template delivery by hostap
* GLX data is started with 0x4F4F4F4F
* fix GLX not initialized issues - ASU + RPU + Scan adapter + Basic Scan + Beacon Dtim + Lrxp
* change AP List size at hostap from 255 to 5
* close memory validation at idle task - currently not working well

"Just another day at the office" - Gilboa 0.0.1.1.36 (Feb 15, 2016)
* new FPGA support 33000094
* main objectives for this FPGA:
*   support RESET (sl_stop sl_start)
*   NEW FLOW - first issue sl_start only then load JTAG!
*   I2C HW support
* DHCP client and ip config fixes

"Shirley temple memorial day" - Gilboa 0.0.1.1.35 (Feb 10, 2016)
* fix rpu suspend queues issue which prevented AUTH to be transmitted (Shaul and Michal from home)
* fix supplicant DISCONNECTED state in case FW ROLE is not yet stopped  (Ariela)
* gtrace count fixes
* additional minor bug fixes

19 years to Helicopter disaster - Gilboa 0.0.1.1.34 (Feb 04, 2016)
* new FPGA support 33000093
* main objectives for this FPGA:
*   support SPI 30Mhz
*   cortex reset release on power up / hard reset - no need to release cortex reset on init
*   hard reset register support - to allow stop start of device internnaly
*   GPIO support - OCLA gpio muxing configuration register



Foundation day of Facebook (2004) - Shaul 0.0.1.1.33 (Feb 04, 2016)
Traffic module support:
- requests are added/removed to/from MR Scheduler if traffic exists/stops and
  Scheduler State is FULL
- Exit/Enter PS when traffic start/stop (TBD)


"IP config API" - Eran  0.0.1.1.32 (Feb 2, 2016)
- ability to save up-to 8 IP config profiles and select the specific one per WlanProfile

"DHCP support" - Eran  0.0.1.1.31 (Jan 27, 2016)
- supporting Dhcp protocol
- lla enable/disable according to host configuration (disable by default)
- Fast renew (Enable by default)
- DHCP Release (disable by default)

"Birthday of Gadi Yagil" - Michal 0.0.1.1.30 (Jan 18, 2016)
- FPGA version 90
- Move TX scheduler to interrupt based trigger

"First delivery of 2016!!" - Gilboa 0.0.1.1.29 (Jan 10, 2016)
- load ULL RAM with default value 0x5F so that we will be ready for rom load and reset
- added image creators (hdi/phy and code)
- handled all ULL init values in declare/init functions

"Last delivery of year 2015?" - Gilboa 0.0.1.1.27 (Dec 23, 2015)
 ** NWP version alligned with driver 8.0.0.2
 ** host driver - NWP protocol change, added bit to opcode for async event drop.
 ** sl test tool is fixed, mandates -start as a first command
 ** Opened Device silo  - async event drop feature
 ** Opened NetCFG silo - static ip only for now


"Iranian student day!!!" 0.0.1.1.26 (Dec 7, 2015)
 ** R2 driver alignment with cc31xx - 2.0.0.40 version alignment
 ** NWP AND Host rebase required (changes in all host-shared folders)
 ** release notes for this version:
 **     - supported silos: socket, and WLAN partially (no WLAN events)
 **     - No linux support yet
 **


"1963 - The first push-button telephone goes into service Rev 0.0.1.1.25 (Nov 18, 2015)
---------------
 New features:
---------------
 - UPDATE OR DIE... FPGA version 78.
 - Support AP link connection and running of TCP/UDP Tx/Rx data over WLAN interface (Antenna to Twitter over WLAN interface).
 - CME tests disbaled by default (can be disabled from build_configs.h, undefine CC33XX_NO_CME_TEST).
   Carefully review the related comment in build_configs.h.
 - All PMAC Tx are hard coded to operate at 6mbps (data and IR).
 - Host commands that operate WLAN silo checked (e.g. WLAN CONNECT, set WLAN mode, policies).
   More example codes will be provided in host/windows/...
----------------------------
 Disabled/Enabled features:
----------------------------
 - compilation - TAP interface disabled by default (can be enabled from build_configs.h, define USE_RAW_TAP_NAB_HOST_INTERFACE and undefine USE_CC33XX_WLAN_NAB_HOST_INTERFACE)
 - compilation - PMAC Tx and Rx enabled by default (can be disbaled from build_configs.h, undefine FPGA_PMAC_TX and PMAC_OLD_RX, respectively)
 - compilation - netx-multi-interface support disabled by default (can be disabled from build_config.h, CC33XX_NO_NX_MULTI_INTERFACE)
 - compilation - OCLA disabled by default (can be enabled from build_configs.h , d
[obj2 default:  eap_peap]efine OCLA_128).
   The current limitations on working with OCLA are:
    a. It is expected to work with Tx UDP, or low throughput Rx UDP.
    b. It may fail with TCP
    c. It may fail with high throughput UDP Rx.
 - code - HDI is disabled and not working (pending bug fixes)
 - code - WMM disabled by default (to temporarily overcome PMAC Tx bug of 4 bytes unaligned WLAN header size)
-------------------------
 WLAN Setup Requirements
-------------------------
 In order to run AP and PC behind it to operate with WLAN interface:
 - AP must be open security.
 - Strong recommendation - configure the AP to operate only with 6mbps
 - Python scripts for PC behind AP are available from host/windows/python, refer to host wiki for clarifications.
 - Need to change MAC to a unique value (currently fixed in FW) and also change the static IP.
-------------------------

... and that's it. we can now all go to the sea (or at least to a warmed up pool with nose clips).

"Berlin wall & crystal night anniversary Rev 0.0.1.1.24 - Gilboa (Nov 9, 2015)
 - supplicant STA interface is successfully terminated/restarted
   (tested by simulated start/stop role commands from idle task - make sure it is disabled by default)
 - first started role initializes supplicant, second one only adds an interface
 - changes to supplicant source  code:
     o supplicant allocates memory for a new interface according to API new flag
     o supplicant free memory of a removed interface based on drv_flags
 - driver_cc releases AP role resources when role is disabled (beacon descriptor & probe response memory)
 - fix random assert in BeaconDtim_TSFOutOfSyncSendProbeReq
 - added tx exchange handler for association request packets
 - fixed AP beacon related issues, AP beacons are transmitted every beacon interval.

"Wow! first rain of the season" Rev 0.0.1.1.23 - Gilboa (Oct 7, 2015)
 - same FPGA (33000071)
 - scan, FW and rx path major building blocks
 - simple link driver adaptations
 - dhcp client netapp infrastructure
 - ip configuration support (no commands yet)
 - cme role activation module

"Stock market going downnnn" Rev 0.0.1.1.22 - Gilboa (July 9, 2015)
 - new FPGA version support 33000071
 - nab recieve timeout support
 - several linux tools

"Cleveland Cavaliers All In" Rev 0.0.1.1.21 - Gilboa (June 14, 2015)
 - HDI operational high level link (on top of TAP) (Gilboa)
 - some osi changes (Gilboa)
 - driver tap multiple fixes + initial rx support (Jigal)
 - LTXP initial interface (Michal)
 - Firmware changes (Shaul)



"Supplicant is in the house" Rev 0.0.1.1.20 - Ariela (June 2, 2015)
 - hostapd + wpa_supplicant push into the project, compiled and partially operational (Ariela)
 - cme thread   (Ariela)


"First Delivery in Linux" Rev 0.0.1.1.19 - Gilboa (May 3, 2015)
 - new FPGA 33000063 - HDI fixes and new NAB version
 - TAP driver with raw socket package to bypass L2 - Jigal
 - WLAN FW initial sources  - scan (Shaul)
 - HDI modifications and test library

"Its Raining In April" Rev 0.0.1.1.18 - Gilboa (April 15, 2015)
 - new FPGA 33000061 - HDI fixes formal drop
 - common make files support for both windows and linux machines
 - lots of hdi scripter modiciations, beginning of preparations for SW usage


"Goodbye Yaniv Katan" Rev 0.0.1.1.17 - Gilboa (April 6, 2015)
 - new FPGA 33000060 -  HDI new design with peripherlas, partially verificated
 - HDI scripter + rom DB's infra
 - PMAC scripter infra
 - gcc xml infrastructure for calculating c structures offsets and sizes
 - URXP initial infra
 - lots of make file changes and adaptations
 - adaptations to linux make compilations


"Adar has entered" Rev 0.0.1.1.16 - Gilboa (Mar 5, 2015)
 - new FPGA 33000057
 - NAB and NetX socket control
 - UTXP module
 - beginning of HDI infrastructure


"NewBorn" Rev 0.0.1.1.15 - Gilboa (Jan 22, 2015)
 - new FPGA support 33000056 with nab new registers and lots of other phy goodies
 - netx import + beginning of socket control interface


"Haleluja" Rev 0.0.1.1.14 - Gilboa (Nov 5, 2014)
 - new FPGA support 33000047 with nab endianess fixes+ rx path
 - osi threadx worker thread infrastructure for netx bsd sockets (with "hachana lemazgan" for worker thread group)



"EL EL ISRAEL" Rev 0.0.1.1.13 - Gilboa (Oct 12, 2014)
 - new simple link host protocol interfaced
 - opcode builder according to silo
 - lots of host driver changes, mostly moving all protocol related strucutres to protocol.h
 - new FPGA support 33000044

"LF (La Famailia) --> ET (Eli Tabib) go home" Rev 0.0.1.1.12 - Shaul (Sep 22, 2014)
add: Wlan HP + LP threads
     Beacon Dtim Handler (partially)
     AP Jitter (partially)
     Roles (partially)

"Back To School" Rev 0.0.1.1.11 - GilboaS (Sep 1, 2014)
   - support FPGA 33000030
   - interrupt handler support - MMU allign allocation dones and going under threshold aggregates
   - tx concentrator fixes



"Babylon" Rev 0.0.1.1.10 - GilboaS (Aug 25, 2014)
   - support FPGA 33000027
   - interrupt handler support - direct nvic and irq handlers
   - timers manager fixes (Ariela)
   - tx concentrator loads of fixes
   - spi and nab stuff


"Its raining outside" Rev 0.0.1.1.9 - GilboaS (Aug 19, 2014)
    - support FPGA 33000026:
     * new interrupt controller, support MMU, NAB TIMERS and FIFO interrupts
     * MMU DB RAM fix - DEALLOC
     * HDI stuff
    - Jigal NAB and SPI things
    - TX Concentrator sources withouth interrupt handlers , not tested yet
    - MMU interface fixes
    - some other goodies


"Its quiet around" Rev 0.0.1.1.8 - GilboaS (Aug 13, 2014)
    - support fpga 33000024 - new headers
    - Lots of HW drivers:
        - mmu driver interface (no interrupts yet and bug with dealloc - waiting for next FPGA)
        - descriptor fifo driver interface (Jigal)
        - nab drivers
        - socket db
    - bringup tests for mmu (partial) and for nab (Also partial)
    - hangover initial source place holders (lots of work left to be done when PMAC arrives)
    - tx concentrator headers (not implemented yet)


"Truce Fire" Rev 0.0.1.1.7 - GilboaS (July 17, 2014)
    - Plot stat support client side


"Tzuk Eitan" Rev 0.0.1.1.6 - GilboaS (July 10, 2014)
    - run time analysis cyclic buffer support
    - assert dump additional info printouts
    - Support FPGA version 33000023:
       * Max stack depth HW calculation support, collect and printout
       * OCP access error masterid and address collection

"Noah's Ark" Rev 0.0.1.1.5 - GilboaS (July 6, 2014)
    - OSI dynamic statistics collection, gtrace statistics (static)
    - rich trace client support - GTRACE with up to 16 parameters mix of integers and strings
    - run time analysis client infrastructure and default OS & interrupts hooks



"The tower of babel" Rev 0.0.1.1.4 - GilboaS (June 24, 2014)
    - Support FPGA version 33000022
    - new bus fault cause addresses
    - cleanup of cause in init time


"Cain and Able" Rev 0.0.1.1.3 - GilboaS & Ariela ((June 22, 2014)
    - New versioning method
    - Concludes:
      Shontal Rev 33.0.0.0.3 - GilboaS
        - Supports FPGA version 33000021
        - Stack overflow mechanism (HW/SW)
        - Thread Stack management
        - ThreadX OSI wrappers
        - new makefiles, optimization levels, parallel make
        - new time scheduler (Ariela).
        - Statistics infrastructure.
        - lots of other goodies and redicilous comments.

"Adam and Eve" Rev 33.0.0.0.2 - GilboaS (June 1, 2014)
    - Supports FPGA version 33000020

"Genesis" Rev 33.0.0.0.1 - GilboaS (April 3, 2014)
    - don't even know where to start
    - Supports FPGA version 33000011

*/
