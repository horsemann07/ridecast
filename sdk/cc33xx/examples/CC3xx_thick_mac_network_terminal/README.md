## Network terminal Example Summary

This example demonstrates the various range of networking capabilities which the Osprey CC33xx device.


| Silo          | Description           | 
| ------------- |:-------------|
| [Wlan](#Wlan-Commands)          | Contains link layer functions like scan, connect, etc. | 
| [Socket](#Socket-Commands)        | Shows variety of socket API and responsible for sending and receiving data.      |

Each silo contains several commands, accessible by the user via command line interface,    
which implemented using Sitara AM2434 user UART. Any events or application output, would be displayed over 
the UART terminal screen. Later on, in the [Available Commands](#Available-Commands) section, we'll discuss in greater detail about each silo and the commands it includes.

## Example Usage

* Build the project and run it, see [Getting started guide](../../docs/html/CC33XX/getting-started.html).
* Open a serial port session (e.g. 'HyperTerminal','puTTY', 'Tera Term' etc.) to the appropriate COM port -   
listed as 'User UART'.
The COM port can be determined via Device Manager in Windows or via `ls /dev/tty*` in Linux.

The connection should have the following connection settings:
```
    Baud-rate:    115200
    Data bits:         8
    Stop bits:         1
    Parity:         None
    Flow Control:   None
```


Once the application has complete it's initialization and the network processor is up,  
the application banner would be displayed, showing version details:
```
******************************************************************
********************   Network Terminal   ************************
******************************************************************
```
At this point, a menu showing the available commands would be printed followed by command prompt,  
indicating that the application is ready for user input.
  
## Application's Overview
  
Network Terminal is a command line interface (cli) based application,  
used to demonstrate the Osprey CC33xx device networking capabilities. It does that by offering a list  
of commands, fed to the application via UART and later parsed and dispatched.  
  
The application starts with invoking `cmd_prompt()` which polls the UART input for a command.  
Polling is completed once a cartridge return is detected. At this point in time, `cmd_prompt` possess  
the command name, followed by the user's choice of parameters. Once command is obtained,  
 `cmd_prompt` searches for the user's command in the global command table, `gCmdList`.  
For each command, this table stores three entries: the command string,  
 a callback which handles the command and a menu print callback that prints the help menu.  
  
If the typed command matches a table entry, the handler is dispatched and the parameters  
fed by the user are passed to the command callback.  
  All callback functions follow the same naming convention: `cmd<command_name>Callback`  
and resides at the appropriate file. For example, all WLAN related callbacks are implemented in: `wlan_cmd.c`.   
  
Inside each command callback, the first action is to parse the user's input. This is achieved by  
calling the corresponding parser function. A structure containing each command's parameters  
is also passed to the designated parser and returned filled with the parsed user's parameters.  
Later, the parsed structure is handled by the callback.  
All command parser function follows the same naming convention: `Parse<command_name>Cmd`.  
Parser functions resides in `cmd_parser.c`.  

Once a dispatched callback finish it's run, the application's return to the `cmd_prompt` function,   
ready of another input from the user.
 

## Available Commands
```
================================================================================
Available commands:

help                clear               wlan_ap_role_up     wlan_ap_role_down   
wlan_sta_role_up    wlan_sta_role_down  wlan_connect        wlan_disconnect     
wlan_scan           wlan_get_mac        wlan_set_mac        wlan_get_ps         
wlan_set_ps         wlan_set_pm         wlan_set_LSI        wlan_start          
wlan_stop           ble_adv_cfg         ble_adv_enable      ble_scan_cfg        
ble_scan_enable     ble_connect         ble_disconnect      ble_peers           
ble_start           ble_stop            ble_test_mode       calibrator          
send                recv                socket_show         kill                
================================================================================
```

### *Wlan commands*

- [wlan_start](#wlan_start)
- [wlan_stop](#wlan_stop)
- [wlan_scan](#scan)
- [wlan_connect](#wlan_connect)
- [wlan_disconnect](#wlan_disconnect)
- [wlan_get_mac](#wlan_get_mac)
- [wlan_set_mac](#wlan_set_mac)
- [wlan_get_ps](#wlan_get_ps)
- [wlan_set_ps](#wlan_set_ps)
----------
#### **wlan_start**
* This command will turn on the wlan device, initialize the upper mac thread, 
and initialize the SPI interface 

> **Note**: this command calling by the **main function**, and it using only if the user calling wlan_stop command first.
----------

#### **wlan_stop**
* This function is turn off the wlan device, and deactivate the SPI interface
----------
#### **wlan_sta_role_up**
*  This command will role up the device in station mode:
1. Adding new STA netif interface to the network stack
2. Adding new WLAN role id by dispatching **CMD_ROLE_START** to Osprey CC33xx device

```
Usage: 
        wlan_sta_role_up [-help] 

Description:
        Role up device as STA.
```
> **Note**: after role is started (device is ON, and all the modules are initialized), we can role up the device as Station
----------
#### **wlan_sta_role_down**
*  This command will role down the device in station mode:
1. If station is connected, so first disconnecting from the AP.
1. Remove STA netif interface from the network stack
2. Remove the STA WLAN role id by dispatching **CMD_ROLE_DISABLE** to Osprey CC33xx device
----------
#### **wlan_Scan**
*  Allows user to retrieve scan results from network processor's.
```
Usage: 
  scan [-help] [-n <number of APs to display>]
  
  -n        Maximum number of Scan results to show
  -help     Display this help.
```

Once **wlan_scan** command has successfully completed, an event `WLAN_EVENT_SCAN_RESULT` and the available devices would be printed to UART console. Please Refer to `WlanEventHandler()` implemented in `network_terminal.c` for more info.

Example:
```
scan -n 20

---------------------------------------------------------------------------------------------
    |               SSID               |       BSSID       | RSSI  | Ch | Hidden | Security |
---------------------------------------------------------------------------------------------
 1  | SSID1                            | 94:10:3e:c6:70:a4 | -26   | 1  | NO     | OPEN     |
 2  | SSID2                            | cc:db:93:43:da:80 | -61   | 1  | NO     | OPEN     |
 3  | SSID3                            | 34:03:de:10:fc:43 | -59   | 1  | NO     | WPA/WPA2 |
---------------------------------------------------------------------------------------------
```
> **Note:** Max number to show is **20**

----------

#### **wlan_connect**:
* Allows user to connect to an AP. Connection is based on the SSID of the AP,and receives the following parameters:
1. *SSID*: The requested SSID of the AP. Application expects this parameter to be inside quotation marks (" /"). 
2. *Security Type*: Device supports four modes: `[OPEN, WPA2]`.  
3. *Password*: If needed, application expects this parameter to appear inside quotation marks ("/").

```
Usage: 
  wlan_connect [-help] [-s <"ssid name">] [-t <security type>] [-p <"password">]
    -s      SSID
    -t      Type of security (security type = [OPEN, WPA/WPA2])
    -p      Password in ascii character 
```

> **Note:** This command is a blocking command and the time out configured by `WLAN_EVENT_TOUT` at `wlan_cmd.c`  
* Expected Host events:  
In connection process to an AP, some async events are expected from the Device side.  
These are the following:
1. `WLAN_EVENT_CONNECT`: This event signals that the Device is connected to an AP. It also prints details about the connected AP, such as SSID and BSSID. Please Refer to `WlanEventHandler()` implemented in `network_terminal.c` for more info.
2. `WLAN_EVENT_DISCONNECT` : In case the device is already connected to an AP, before attempting to connect with the requested AP, the device would disconnect from the old one. Disconnect reason code would than be: `WLAN_DISCONNECT_USER_INITIATED`. Please refer to `WlanEventHandler()` implemented in `network_terminal.c` for more info.  

Example:
```
wlan_connect -s "myhome network" -t WPA2 -p "password" 
```

----------

#### **wlan_disconnect**:
* Allows user to disconnect from the AP (if connected).

* Expected Host events:  
`WLAN_EVENT_DISCONNECT` : It also prints details about the disconnected AP, such as SSID and BSSID. refer to `WlanEventHandler()` implemented in `network_terminal.c` for more info.  

----------
#### **wlan_get_mac**:
* Allows user to get device MAC address.
receives the following parameters:
1. *role type*: 0 - station, 2 - AP 

```
Usage: 
        wlan_get_mac [-help] [-i <RoleType>] 

Description:
        Get MacAddress. role type 0 - STA , 2 - AP 

        -i      RoleType, if role type does not exists error is return

        -help   Display this help
```

> **Note:** for now we only supporte `STA role`

Example:
```
wlan_get_mac -i 0

[MAC ADDRESS] : a8:1b:6a:a9:90:f2
```

----------
#### **wlan_set_mac**:
* Allows user to set device MAC address.
receives the following parameters:
1. *role type*: 0 - station, 2 - AP 
2. *mac address*: new mac address

```
Usage: 
        wlan_set_mac [-help] [-i <RoleType>] [-m <macAdress>]

Description:
        Set MacAddress.role type 0 - STA , 2 - AP 

        -i      RoleType, if role type does not exists error is return

        -m      MacAddress, if MacAdrress does not valid error is return

        -help   Display this help
```

> **Note:** for now we only supporte `STA role`

For applying the chages must to do - `wlan_sta_role_down` than `wlan_sta_role_up`

Example:
```
wlan_set_mac -i 0 -m AA:BB:CC:DD:EE:FF

wlan_sta_role_down

wlan_sta_role_up

wlan_get_mac -i 0

[MAC ADDRESS] : aa:bb:cc:dd:ee:ff
```

----------
#### **wlan_get_ps**:
* Allows user to get current power save mode

```
Usage: 
        wlan_get_ps [-help] 
Description:
        Get current power save mode. 0 - Auto PS , 1 - Active mode, 2 - power save mode
```

> **Note:** The device start with Active mode

----------
#### **wlan_set_ps**:
* Allows user to set device power save mode
* receives the following parameters:
0 - Auto PS , 1 - Active mode, 2 - power save mode

```
Usage:
        wlan_set_ps [-help] [-m <PowerSaveMode>]

Description:
        Set power save mode. 0 - Auto PS , 1 - Active mode, 2 - power save mode

        -m      Mode, if device not started error is return

        -help   Display this help

```

----------

### *Socket Commands*

* [send](#send)
* [recv](#recv)
* [socket_show](#socket_show)
* [kill](#kill)

----------

#### **send**
* This command demonstrate sending data in packets using networking API. 
This command allows user to open a udp or tcp **client** sockets, and send packets in various 
configurations, which include the following:
1. Protocol: TCP/UDP
2. Port Number
3. Blocking vs. Non-blocking option
4. Number of packet to transmit ( 0 - infinty )

```
Usage:
        send [-help] [-c <server ip address>] [-u] [-p <port number>]  [-nb] [-n <number of packets>]  
        
        -c      Run in client mode and connect to mentioned server - IP should be in '.' format 
        -u      Use UDP rather than TCP
        -p      Port number to send/receive data (Default is 5001)
        -nb     Create non-blocking socket rather than blocking
        -n      Number of packets to transmit (Default is 1000)
        -V      Use IPv6 rather than IPv4
```

> For now only support IPv4, and size of packets limit to 800 bytes.

Once all packets have been sent or timeout expired (see `RECEIVE_TIMEOUT` at `networtk_terminal.h`), application would return to main menu. 

Example:
```
send -c 10.0.0.10 -u -p 5001 -n 1500
```

**Infinty option** - number of packets = 0
* This option will excute a new task
* This task will start sending packets in infinty loop 
* For more  information how to manage this option see [kill](#kill) and [socket_show](#socket_show)

> Limit for task the user can create is 4

----------

#### **recv**
* This command demonstrate reception of data in packets using networking API. 

This command allows user to open a udp or tcp **server** sockets and receive packets in various configurations, which include the following:

1. Protocol: TCP/UDP
2. Port Number
3. Blocking vs. Non-blocking option
4. Number of packet to transmit ( 0 - infinty )

```
Usage:
        recv [-help] [-u] [-p <port number>] [-nb] [-n <number of packets>]          

        -u      Use UDP rather than TCP
        -p      Port number to send/receive data (Default is 5001)
        -nb     Create non-blocking socket rather than blocking
        -n      Number of packets to transmit (Default is 1000)
        -V      Use IPv6 rather than IPv4
```

> For now only support IPv4, and size of packets limit to 800 bytes.

Once all packets have been sent or timeout expired (see `RECEIVE_TIMEOUT` at `networtk_terminal.h`), application would return to main menu. 

Example:
```
recv -u -p 5001 -n 1500
```

**Infinty option** - number of packets = 0
* This option will excute a new task
* This task will start receving packets in infinty loop 
* For more information how to manage this option see [kill](#kill) and [socket_show](#socket_show)

> Limit for task the user can create is 4

----------

#### **socket_show**

* This command will show the user the current socket process that running
* Each process as is own id (**0-3**) and is own name

```
Example:
socket_show 

         Process running: 

        ---id 0          UDP client
        ---id 1          UDP server

```

----------

#### **kill**
* This command will allow the user to close spesifc task (by id) socket process that running

```
Usage:
        kill [-help] [-i <id>]

Description:
        kill available running process by id        
```

```
Example:

kill -i 0
kill -i 1
```


### *BLE commands*

- [ble_start](#ble_start)
- [ble_stop](#ble_stop)
- [ble_adv_cfg](#ble_adv_cfg)
- [ble_adv_enable](#ble_adv_enable)
- [ble_scan_cfg](#ble_scan_cfg)
- [ble_scan_enable](#ble_scan_enable)
- [ble_connect](#ble_connect)
- [ble_disconnect](#ble_disconnect)
- [ble_peers](#ble_peers)
- [ble_test_mode](#ble_test_mode)

----------
#### **ble_start**
* This command turns on the ble controller and host and initialize the BLE interface 
----------

#### **ble_stop**
* This function stop the ble controller and host
----------

#### **ble_adv_cfg**
* This function sets Ble Advertise instance configuration
```
Usage:
        ble_adv_cfg [-i <instance>] [-l <legacy [0:extended/1:legacy]>] [-n <interval (ms)>] [-p <primary phy [1:1M/2:2M/3:coded]>] [-s <secondary phy [1:1M/2:2M/3:coded]>]

Description:
        Configure a specific advertise instance.
		Can be used with no parameters.
        Default Parameters: instance 0, extended, 100ms interval, 1M PHY	

Example:
		ble_adv_cfg
		ble_adv_cfg -i 0 -l 0 -n 100 -p 1 -s 1
```
----------

#### **ble_adv_enable**
* This function enables or disables Ble Advertise instance
```
Usage:
        ble_adv_enable [-e <enable [0/1]>] [-i <instance>] [-d <duration (10ms units)>] [-m <max events]

Description:
        Enable or disable a specific advertise instance.
	    Can be called only after a previous ble_adv_cfg command.
		Can be used with no parameters.
        Default Parameters: enable with no expiration and max events 0	

Example:
		ble_adv_enable
		ble_adv_enable -e 1 -i 0 -d 0 -m 0
		ble_adv_enable -e 0 -i 0
```
----------

#### **ble_scan_cfg**
* This function sets Ble scan configuration
```
Usage:
        ble_scan_cfg [-i <scan interval (ms)>] [-w <scan window (ms)>] [-p <phy [ 1:1M/4:coded/5:mixed]>]

Description:
        Configure BLE scan
		Can be used with no parameters.
        Default Parameters: Default Parameters: extended 1M PHY, scan interval of 100ms and scan window of 50ms

Example:
		ble_scan_cfg
		ble_scan_cfg -i 100 -w 50 -p 1
```
----------

#### **ble_scan_enable**
* This function enables or disables Ble Scan
```
Usage:
        ble_scan_enable [-e <enable [0/1]>] [-f <filter duplicate [0/1/2]>] [-p <period (sec)>]

Description:
        Enable or disable Ble scan.
	    Can be called only after a previous ble_scan_cfg command.
		Can be used with no parameters.
        Default Parameters: enable with scan duration of 3s, scan period 0s and no filter duplicate

Example:
		ble_scan_enable
		ble_scan_enable -e 1 -f 0 -p 0
		ble_scan_enable -e 0
		
--------------------------------------------------------------
| ID |        NAME      |    PEER ADDRESS   |  TYPE  |  RSSI |
--------------------------------------------------------------
| 0  | Simple Peripheal | A4:10:3E:C8:70:17 | PUBLIC |  -56  |
| 1  | Simple Central   | A4:2B:83:43:DA:90 | PUBLIC |  -70  | 
| 2  | Basic Peripheral | C0:03:DE:10:FC:43 | RANDOM |  -62  | 
--------------------------------------------------------------
```
----------

#### **ble_connect**
* This function connect to a BLE peer device
```
Usage:
        ble_connect [-b <bd address>] [-t <address type [PUBLIC/RANDOM]

Description:
        Connect to a BLE peer device

Example:
		ble_connect -b A4:38:25:11:53:18 -t PUBLIC
```
----------

#### **ble_disconnect**
* This function disconnect from a BLE peer device
```
Usage:
        ble_disconnect [-b <bd address>] [-t <address type [PUBLIC/RANDOM]

Description:
        Disconnect from a BLE peer device.
		Can be used with no parameters.
		Default action when no parameters is to disconnect all peers

Example:
		ble_disconnect
		ble_disconnect -b A4:38:25:11:53:18 -t PUBLIC
```
----------

#### **ble_peers**
* This function display the list of all connected BLE peer devices
```
Usage:
        ble_peers

Description:
        Display the list of all connected BLE peer devices with their attributes

Example:
		ble_peers
			
-------------------------------------------------------------
| Handle |   PEER ADDRESS    |  TYPE  |  OWN ROLE  | BONDED?|
-------------------------------------------------------------
| 0      | A4:10:3E:C8:70:17 | PUBLIC | PERIPHERAL | YES    |
| 1      | A4:2B:83:43:DA:90 | PUBLIC | CENTRAL    | NO     |
| 2      | C0:03:DE:10:FC:43 | RANDOM | PERIPHERAL | NO     |
-------------------------------------------------------------
```
----------

#### **ble_test_mode**
* This function set the device into test mode (controller only mode)
```
Usage:
        ble_test_mode [-e <enable>]

Description:
        Enter into a controller only )no host) mode running over UART interface for testing purposes

Example:
		ble_test_mode -e 1
		ble_test_mode -e 0
```
----------

## References
* For further information please refer to the user programmers guide: [Getting Startded Guide](../../docs/html/CC33XX/getting-started.html)

