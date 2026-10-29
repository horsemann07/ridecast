## Wi-Fi Provisioning over BLE Example Summary

TI developed a demo to demonstrate how the BLE functionality of the CC33xx can be used to provision it to a Wi-Fi AP of
the user's choice. This works by passing the Wi-Fi AP credentials to the CC33xx via BLE using a phone or a similar device.

The provisioning sequence is shown below where a user scans for BLE devices, connects to the CC33xx BLE peripheral and
passes the Wi-Fi credentials.

## Prerequisites

* Install the 'SimpleLink Connect' application on your phone or device

## Example Usage

* Flash the Ble Wifi Provisioning example as explained in the platforms getting started guide
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

* Once the application has complete it's initialization and the network processor is up,  
the application banner would be displayed, showing version details:
```
***********************************************************************
********************   BLE WiFi Provisioning   ************************
********************   Version x.x.x.x         ************************
***********************************************************************
```
At this point, the application will wait for a BLE connection from the mobile application.
Once a BLE connection is made the user can enter the WLAN credentials (SSID and Password) to be used for the WLAN connection.

## Mobile Application Usage

* Open the application and enable BluetoothLE Scan
* Find the device that advertises as *TI CC3xxx* and connect to it
* Open the device services and enter to the "Wi-Fi Provisioning Over BLE" service
* Enter SSID and select the Security type in the drop list (Open/WPA2). In case WPA2 was selected, enter the Password
* Click on Connect and wait for the status to be updated

## References
* For further information please refer to the user programmers guide: [Getting Startded Guide](../../docs/html/CC33XX/getting-started.html)

