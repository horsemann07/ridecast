# RideCast Host Communication Protocol

## 1. Overview

RideCast is a **transport gateway** between the Host/Cluster MPU/MCU and the Mobile Application.

RideCast provides connectivity through:

* Wi-Fi
* BLE
* Bluetooth Classic

The Host communicates with RideCast through a wired interface such as:

* UART
* SPI

The Mobile Application communicates with RideCast through one of the supported wireless interfaces.

```text
                         MOBILE APPLICATION
                       ┌─────────────────────┐
                       │                     │
                       │ Navigation          │
                       │ Map Data            │
                       │ Notifications       │
                       │ Media               │
                       │ Calling             │
                       │ Other App Data      │
                       │                     │
                       └───────┬─────┬───────┘
                               │     │
                             Wi-Fi   BLE
                               │     │
                               └──┬──┘
                                  │
                         ┌────────▼────────┐
                         │    RIDECAST     │
                         │                 │
                         │ Transport      │
                         │ Gateway        │
                         │                 │
                         │ Wi-Fi          │
                         │ BLE            │
                         │ BT Classic     │
                         │                 │
                         │ Connection     │
                         │ Management     │
                         └────────┬────────┘
                                  │
                              UART / SPI
                                  │
                         ┌────────▼────────┐
                         │      HOST       │
                         │ Cluster MPU/MCU │
                         │                 │
                         │ Application     │
                         │ Logic           │
                         │ Display         │
                         │ Vehicle Data    │
                         └─────────────────┘
```

---

# 2. Design Principle

The primary design principle of the RideCast protocol is:

> **RideCast shall act as a transport gateway and shall not interpret application payload data.**

The Host owns the application-level protocol.

For example, the Host may send:

* Navigation data
* Map data
* Notification data
* Media data
* Calling data
* Any future application-specific data

RideCast does not need to know what the payload represents.

For example:

```text
HOST
 │
 │ SEND_DATA
 │ Transport = Wi-Fi
 │ Payload = application-defined data
 ▼
RIDECAST
 │
 │ Forward payload
 ▼
Wi-Fi
 │
 ▼
MOBILE APP
```

RideCast only determines:

1. Whether the frame is valid.
2. Which transport is requested.
3. Whether the requested transport is available.
4. Whether the payload can be transmitted.
5. The connection status.
6. Whether reconnection is required.

---

# 3. Responsibility Split

## 3.1 Host Responsibilities

The Host/Cluster MPU/MCU is responsible for:

* Application-level protocol.
* Defining the payload format.
* Deciding when data needs to be sent.
* Deciding which transport should be used.
* Starting/stopping wireless interfaces through RideCast commands.
* Configuring Wi-Fi.
* Configuring BLE.
* Configuring Bluetooth Classic.
* Reading wireless configuration.
* Reading RideCast status.
* Handling RideCast connection status.
* Sending application data to the Mobile Application.
* Receiving application data from the Mobile Application.

The Host understands what the payload means.

For example:

```text
Payload = Navigation
Payload = Notification
Payload = Media
Payload = Calling
Payload = Map
```

RideCast does not need to understand these payload types.

---

## 3.2 RideCast Responsibilities

RideCast is responsible for:

* Host communication.
* Frame validation.
* Transport selection.
* Data forwarding.
* Wi-Fi management.
* BLE management.
* Bluetooth Classic management.
* Connection management.
* Reconnection handling.
* Retry handling.
* Wireless status reporting.
* Basic RideCast health monitoring.

RideCast does **not** interpret application payloads.

For example, RideCast should not contain logic such as:

```c
if(message == NAVIGATION)
{
    ...
}
else if(message == NOTIFICATION)
{
    ...
}
```

Instead:

```text
Receive frame
     ↓
Validate frame
     ↓
Read requested transport
     ↓
Forward payload
```

---

## 3.3 Mobile Application Responsibilities

The Mobile Application is responsible for:

* Navigation information.
* Map information.
* Notifications.
* Media information.
* Calling information.
* Application-level protocol.
* Application payload interpretation.

The exact payload format is outside the RideCast transport protocol.

---

# 4. Communication Architecture

There are two logical communication sides.

## Host Side

```text
HOST
  │
  │ UART / SPI
  │
  ▼
RIDECAST
```

This side uses the RideCast Host Communication Protocol.

## Mobile Side

```text
RIDECAST
   │
   ├── Wi-Fi
   ├── BLE
   └── Bluetooth Classic
          │
          ▼
    MOBILE APPLICATION
```

The Host does not directly communicate with the Mobile Application.

RideCast provides the transport bridge.

---

# 5. Transport Selection

The Host specifies the required transport when sending application data.

Supported transports:

```c
typedef enum
{
    eRcTransportWifi      = 0x01U,
    eRcTransportBle       = 0x02U,
    eRcTransportBtClassic = 0x03U,

} rc_transport_t;
```

The transport value tells RideCast where the payload should be transmitted.

Example:

```text
Transport = Wi-Fi
```

means:

```text
HOST → RIDECAST → Wi-Fi → MOBILE APP
```

Example:

```text
Transport = BLE
```

means:

```text
HOST → RIDECAST → BLE → MOBILE APP
```

Example:

```text
Transport = Bluetooth Classic
```

means:

```text
HOST → RIDECAST → BT Classic → MOBILE APP
```

RideCast does not interpret the payload.

---

# 6. Application Payload

The application payload is owned by the Host and Mobile Application.

RideCast treats it as opaque data.

For example:

```text
+------------------------------------------------+
| RideCast Frame                                 |
+------------------------------------------------+
| Transport                                      |
+------------------------------------------------+
| Length                                         |
+------------------------------------------------+
| Application Payload                            |
|                                                |
| Navigation / Notification / Map / Media / ... |
|                                                |
+------------------------------------------------+
```

RideCast shall not parse or modify the application payload.

---

# 7. Frame Format

The Host ↔ RideCast protocol uses the following frame format:

```text
+--------+--------+--------+--------+----------+---------+-------+
| HEADER | MSG ID | SEQ    | ROUTE  | LENGTH   | PAYLOAD | CRC16 |
|  1 B   |  1 B   | 2 B    | 1 B    |  2 B     | N Bytes | 2 B   |
+--------+--------+--------+--------+----------+---------+-------+
```

## Field Description

| Field   |    Size | Description                  |
| ------- | ------: | ---------------------------- |
| HEADER  |  1 byte | Protocol version/type/flags  |
| MSG ID  |  1 byte | Protocol message identifier  |
| SEQ     | 2 bytes | Transaction/sequence number  |
| ROUTE   |  1 byte | Requested wireless transport |
| LENGTH  | 2 bytes | Payload length               |
| PAYLOAD | N bytes | Message-specific data        |
| CRC16   | 2 bytes | Frame integrity check        |

Maximum application payload:

```c
#define RC_MAX_PAYLOAD_LEN    (4096U)
```

---

# 8. Header

The Header contains protocol information.

Recommended structure:

```text
Byte 0

+-----------+-----------+-------------+
| VERSION   | TYPE      | FLAGS       |
+-----------+-----------+-------------+
| bits 7:6  | bits 5:4  | bits 3:0    |
+-----------+-----------+-------------+
```

## TYPE

```c
typedef enum
{
    eRcTypeCmd    = 0x00U,
    eRcTypeStatus = 0x01U,
    eRcTypeData   = 0x02U,
    eRcTypeAck    = 0x03U

} rc_type_t;
```

### CMD

Host → RideCast control command.

Examples:

```text
PING
RESET
WIFI_START
WIFI_STOP
BLE_START
BLE_STOP
```

### STATUS

RideCast → Host status/event.

Examples:

```text
WIFI_CONNECTED
WIFI_DISCONNECTED
BLE_CONNECTED
BLE_DISCONNECTED
RECONNECTING
CONNECTION_FAILED
```

### DATA

Application data transferred between Host and Mobile Application.

RideCast does not interpret the payload.

### ACK

Acknowledgement of a Host/RideCast protocol transaction.

---

# 9. Sequence Number

Each Host/RideCast transaction may contain a sequence number.

```text
HOST
 │
 │ MSG = WIFI_START
 │ SEQ = 100
 ▼
RIDECAST
 │
 │ ACK
 │ SEQ = 100
 ▼
HOST
```

The sequence number allows the Host to associate an acknowledgement with the corresponding command.

Sequence numbers are especially useful when multiple commands are outstanding.

---

# 10. Message IDs

Message IDs identify the operation being performed.

Recommended message groups:

```c
typedef enum
{
    /* =========================================================
     * BASIC
     * ========================================================= */
    RC_MSG_PING              = 0x01U,
    RC_MSG_PONG              = 0x02U,
    RC_MSG_GET_VERSION       = 0x03U,
    RC_MSG_GET_STATUS        = 0x04U,
    RC_MSG_RESET             = 0x05U,

    /* =========================================================
     * WIFI
     * ========================================================= */
    RC_MSG_WIFI_SET_CONFIG   = 0x10U,
    RC_MSG_WIFI_GET_CONFIG   = 0x11U,
    RC_MSG_WIFI_START        = 0x12U,
    RC_MSG_WIFI_STOP         = 0x13U,
    RC_MSG_WIFI_STATUS       = 0x14U,

    /* =========================================================
     * BLE
     * ========================================================= */
    RC_MSG_BLE_SET_CONFIG    = 0x20U,
    RC_MSG_BLE_GET_CONFIG    = 0x21U,
    RC_MSG_BLE_START         = 0x22U,
    RC_MSG_BLE_STOP          = 0x23U,
    RC_MSG_BLE_STATUS        = 0x24U,

    /* =========================================================
     * BLUETOOTH CLASSIC
     * ========================================================= */
    RC_MSG_BT_SET_CONFIG     = 0x30U,
    RC_MSG_BT_GET_CONFIG     = 0x31U,
    RC_MSG_BT_START          = 0x32U,
    RC_MSG_BT_STOP           = 0x33U,
    RC_MSG_BT_STATUS         = 0x34U,

    /* =========================================================
     * APPLICATION DATA
     * ========================================================= */
    RC_MSG_SEND_DATA         = 0x40U,

    /* =========================================================
     * CONNECTION EVENTS
     * ========================================================= */
    RC_MSG_CONNECTED         = 0x50U,
    RC_MSG_DISCONNECTED      = 0x51U,
    RC_MSG_RECONNECTING      = 0x52U,
    RC_MSG_CONNECTION_FAILED = 0x53U,

} rc_msg_id_t;
```

The application payload is carried using:

```text
RC_MSG_SEND_DATA
```

RideCast does not define whether that data is navigation, notification, map data, media, etc.

---

# 11. Wi-Fi Configuration

The Host can configure Wi-Fi parameters.

Example:

```c
typedef enum
{
    eRcWifiModeSta = 0x00U,
    eRcWifiModeAp  = 0x01U,

} rc_wifi_mode_t;
```

Configuration:

```c
typedef struct
{
    rc_wifi_mode_t mode;

    char ssid[32];

    char password[64];

} rc_wifi_config_t;
```

The Host can:

```text
SET_WIFI_CONFIG
GET_WIFI_CONFIG
```

---

# 12. Wi-Fi Mode

The Host controls when Wi-Fi should start and stop.

### Station Mode

```text
HOST
 │
 │ SET_WIFI_CONFIG
 │ Mode = STA
 ▼
RIDECAST
 │
 │ WIFI_START
 ▼
Wi-Fi STA
 │
 ▼
Connect to configured AP
```

### Access Point Mode

```text
HOST
 │
 │ SET_WIFI_CONFIG
 │ Mode = AP
 ▼
RIDECAST
 │
 │ WIFI_START
 ▼
RideCast creates AP
```

RideCast owns the implementation details of the Wi-Fi stack.

---

# 13. BLE Configuration

The Host can configure the BLE device name.

Example:

```c
typedef struct
{
    char device_name[32];

} rc_ble_config_t;
```

Host commands:

```text
SET_BLE_CONFIG
GET_BLE_CONFIG
BLE_START
BLE_STOP
```

When `BLE_START` is received, RideCast handles BLE initialization and advertising.

The Host does not need to manage the BLE stack directly.

---

# 14. Bluetooth Classic Configuration

Bluetooth Classic follows the same concept.

Host commands:

```text
SET_BT_CONFIG
GET_BT_CONFIG
BT_START
BT_STOP
```

RideCast manages the Bluetooth Classic stack and connection lifecycle.

---

# 15. Starting and Stopping Interfaces

The Host controls the desired state of each wireless interface.

Example:

```text
HOST → RIDECAST

BLE_START
```

RideCast:

```text
Initialize BLE
      ↓
Start advertising
      ↓
Wait for connection
```

RideCast reports:

```text
BLE_STARTED
BLE_CONNECTED
```

If the connection is lost:

```text
BLE_DISCONNECTED
```

---

# 16. Connection Management

Connection management is owned by RideCast.

RideCast shall monitor:

* Wi-Fi connection
* BLE connection
* Bluetooth Classic connection

The Host does not need to implement wireless reconnection logic.

---

# 17. Connection State

Typical state flow:

```text
                    START
                      │
                      ▼
                 CONNECTING
                      │
              ┌───────┴────────┐
              │                │
          SUCCESS             FAIL
              │                │
              ▼                ▼
          CONNECTED        RETRYING
              │                │
              │          ┌─────┴─────┐
              │          │           │
              │       SUCCESS       MAX RETRY
              │          │           │
              └──────────┘           ▼
                                  FAILED
```

---

# 18. Reconnection and Retry

When a wireless connection is lost, RideCast may automatically attempt reconnection according to the configured retry policy.

Example:

```text
Connection lost
      ↓
Retry #1
      ↓
Retry #2
      ↓
Retry #3
      ↓
Retry #4
      ↓
Retry #5
      ↓
Connection Failed
```

RideCast shall notify the Host during this process.

Example:

```text
RIDECAST → HOST

RECONNECTING
retry = 2
max_retry = 5
```

If all retries fail:

```text
RIDECAST → HOST

CONNECTION_FAILED
```

The Host may then decide whether to:

* Start another connection attempt.
* Change configuration.
* Stop the interface.
* Wait for a future command.

---

# 19. Connection Status Notifications

RideCast should report important wireless events to the Host.

Example:

```text
WIFI_CONNECTING
WIFI_CONNECTED
WIFI_DISCONNECTED
WIFI_RECONNECTING
WIFI_CONNECTION_FAILED
```

BLE:

```text
BLE_STARTING
BLE_CONNECTED
BLE_DISCONNECTED
BLE_RECONNECTING
BLE_CONNECTION_FAILED
```

Bluetooth Classic:

```text
BT_STARTING
BT_CONNECTED
BT_DISCONNECTED
BT_RECONNECTING
BT_CONNECTION_FAILED
```

---

# 20. Host ↔ RideCast Health Monitoring

Host and RideCast should independently verify that the wired communication link is alive.

The Host periodically sends:

```text
PING
```

RideCast responds:

```text
PONG
```

Example:

```text
HOST                         RIDECAST
 │                              │
 │ -------- PING -------------> │
 │                              │
 │ <--------- PONG ------------ │
 │                              │
```

This verifies:

> Host ↔ RideCast communication is operational.

This is different from wireless connection status.

For example:

```text
Host ↔ RideCast       = Connected
RideCast ↔ Wi-Fi      = Disconnected
```

These are two independent states.

---

# 21. Generic Data Transfer

The Host sends application data using:

```text
RC_MSG_SEND_DATA
```

The frame contains the requested transport.

Example:

```text
Transport = Wi-Fi
Payload   = application data
```

RideCast performs:

```text
Receive
   ↓
Validate
   ↓
Check transport
   ↓
Check connection
   ↓
Transmit payload
```

RideCast does not inspect the payload.

---

# 22. Example: Navigation Data

The Host decides that navigation data must be sent over Wi-Fi.

```text
HOST
 │
 │ RC_MSG_SEND_DATA
 │ Transport = WIFI
 │ Payload = Navigation Data
 ▼
RIDECAST
 │
 │ Does not interpret payload
 │
 ▼
Wi-Fi
 │
 ▼
MOBILE APP
```

The RideCast protocol does not define the navigation payload structure.

---

# 23. Example: Notification Data

The Host decides that notification data should be sent through BLE.

```text
HOST
 │
 │ RC_MSG_SEND_DATA
 │ Transport = BLE
 │ Payload = Notification Data
 ▼
RIDECAST
 │
 │ Does not interpret payload
 │
 ▼
BLE
 │
 ▼
MOBILE APP
```

The notification payload format is owned by the application protocol.

---

# 24. Example: Map Data

Map data may be sent through Wi-Fi because of its potentially larger payload size.

```text
HOST
 │
 │ RC_MSG_SEND_DATA
 │ Transport = WIFI
 │ Payload = Map Data
 ▼
RIDECAST
 │
 │ Forward without interpretation
 ▼
Wi-Fi
 │
 ▼
MOBILE APP
```

RideCast does not need to understand the map format.

---

# 25. Bidirectional Data

Data transfer is bidirectional.

### Host → Mobile Application

```text
HOST
  ↓
RIDECAST
  ↓
BLE / Wi-Fi / BT Classic
  ↓
MOBILE APP
```

### Mobile Application → Host

```text
MOBILE APP
  ↓
BLE / Wi-Fi / BT Classic
  ↓
RIDECAST
  ↓
HOST
```

For data received from the Mobile Application, RideCast forwards the payload to the Host without interpreting its application meaning.

---

# 26. Important Design Rule

The following logic should **not** exist inside RideCast:

```c
if(navData)
{
    ...
}

if(notificationData)
{
    ...
}

if(mapData)
{
    ...
}

if(mediaData)
{
    ...
}
```

Instead:

```c
transport = frame.route;

switch(transport)
{
    case RC_TRANSPORT_WIFI:
        sendWifi(frame.payload, frame.length);
        break;

    case RC_TRANSPORT_BLE:
        sendBle(frame.payload, frame.length);
        break;

    case RC_TRANSPORT_BT_CLASSIC:
        sendBtClassic(frame.payload, frame.length);
        break;

    default:
        /* Invalid transport */
        break;
}
```

This keeps RideCast independent from the application.

---

# 27. Configuration Read/Write

The Host should be able to both set and read configuration.

Example:

```text
HOST → RIDECAST
SET_WIFI_CONFIG

RIDECAST → HOST
ACK
```

Later:

```text
HOST → RIDECAST
GET_WIFI_CONFIG

RIDECAST → HOST
WIFI_CONFIG
```

The same pattern applies to:

```text
Wi-Fi
BLE
Bluetooth Classic
Reconnect configuration
```

---

# 28. Error Handling

RideCast should report transport-level errors.

Examples:

```text
INVALID_COMMAND
INVALID_PARAMETER
INVALID_LENGTH
INVALID_TRANSPORT
TRANSPORT_NOT_READY
TRANSPORT_NOT_CONNECTED
TRANSMIT_FAILED
TIMEOUT
CRC_ERROR
INTERNAL_ERROR
```

RideCast should not generate application-specific errors such as:

```text
INVALID_NAVIGATION_DATA
INVALID_NOTIFICATION
INVALID_MAP
```

because RideCast does not understand those application payloads.

---

# 29. Responsibility Summary

| Function                    | Host | RideCast | Mobile App |
| --------------------------- | :--: | :------: | :--------: |
| Application protocol        |   ✓  |          |      ✓     |
| Navigation interpretation   |   ✓  |          |      ✓     |
| Notification interpretation |   ✓  |          |      ✓     |
| Map interpretation          |   ✓  |          |      ✓     |
| Payload definition          |   ✓  |          |      ✓     |
| Transport selection         |   ✓  |     ✓    |            |
| Wi-Fi management            |      |     ✓    |            |
| BLE management              |      |     ✓    |            |
| BT Classic management       |      |     ✓    |            |
| Connection management       |      |     ✓    |            |
| Reconnection                |      |     ✓    |            |
| Retry                       |      |     ✓    |            |
| Wireless status             |      |     ✓    |            |
| Host/RideCast health        |   ✓  |     ✓    |            |
| Cluster display             |   ✓  |          |            |
| Vehicle information         |   ✓  |          |            |
| Application data routing    |      |     ✓    |            |

---

# 30. Example End-to-End Flow

## Startup

```text
HOST
 │
 │ PING
 ▼
RIDECAST
 │
 │ PONG
 ▼
HOST
```

Host verifies that RideCast is alive.

---

## Start Wi-Fi

```text
HOST
 │
 │ SET_WIFI_CONFIG
 ▼
RIDECAST
 │
 │ ACK
 ▼
HOST
 │
 │ WIFI_START
 ▼
RIDECAST
 │
 │ Connect / Start AP
 ▼
Wi-Fi
 │
 │
 ▼
Mobile Application
```

RideCast reports:

```text
WIFI_CONNECTED
```

---

## Send Application Data

```text
HOST
 │
 │ SEND_DATA
 │ Transport = WIFI
 │ Payload = Application Data
 ▼
RIDECAST
 │
 │ Forward payload
 ▼
Wi-Fi
 │
 ▼
MOBILE APP
```

RideCast does not interpret the payload.

---

## Connection Lost

```text
Wi-Fi Connected
       │
       ▼
Connection Lost
       │
       ▼
RIDECAST
       │
       ├── DISCONNECTED → HOST
       │
       ├── Retry #1
       ├── Retry #2
       ├── Retry #3
       │
       └── Connected
               │
               ▼
        CONNECTED → HOST
```

---

# 31. Protocol Philosophy

The RideCast protocol intentionally follows a **thin gateway architecture**.

```text
HOST
  │
  │ WHAT data?
  │ WHEN?
  │ WHERE?
  ▼
RIDECAST
  │
  │ HOW to transport?
  │ HOW to connect?
  │ HOW to reconnect?
  ▼
MOBILE APP
```

### Host owns:

> **What the data means.**

### RideCast owns:

> **How and where the data is transported.**

### Mobile Application owns:

> **How the application data is consumed.**

This separation allows the RideCast firmware to remain stable even when new application features are introduced.

For example, adding a new application feature such as:

```text
Weather
Charging status
Vehicle diagnostics
Remote control
New notification type
```

does not require RideCast to understand the new payload.

The Host and Mobile Application can define the new application payload while RideCast continues to transport it unchanged.

---

# 32. Key Design Rule

> **RideCast is a transport gateway, not an application protocol processor.**

RideCast shall:

1. Validate Host/RideCast frames.
2. Manage Wi-Fi, BLE and Bluetooth Classic.
3. Maintain wireless connections.
4. Perform retry/reconnection.
5. Report wireless status to Host.
6. Route application payloads to the requested transport.
7. Forward received application payloads to the Host.
8. Keep application payload opaque.

RideCast shall **not**:

1. Parse navigation data.
2. Parse map data.
3. Parse notification data.
4. Parse media data.
5. Parse calling data.
6. Implement application-specific business logic.


# 33. Ridecast folder architecture
```
ridecast/
│
├── include/
│   ├── rc_protocol.h          // Protocol frame format, message IDs, enums, payload structures and protocol APIs
│   ├── conn_manager.h        // Connection state, retry/reconnect configuration and connection management APIs
│   ├── rc_wifi.h              // Wi-Fi configuration, AP/STA control, connection and Wi-Fi status APIs
│   ├── rc_ble.h               // BLE configuration, advertising, connection and BLE status APIs
│   └── rc_bt_classic.h        // Bluetooth Classic configuration, connection and BT status APIs
│
├── src/
│   ├── rc_protocol.c          // Implements frame encode/decode, validation and CRC handling
│   ├── rc_connection.c        // Implements common connection state machine and retry/reconnect handling
│   ├── rc_wifi.c              // Implements actual Wi-Fi start, stop, connect, disconnect and reconnect operations
│   ├── rc_ble.c               // Implements actual BLE start, stop, advertising, connect and reconnect operations
│   └── rc_bt_classic.c        // Implements actual Bluetooth Classic start, stop, connect and reconnect operations
│
├── interface/
│   ├── rc_uart.c              // UART communication between Host and RideCast
│   ├── rc_uart.h              // UART configuration, initialization, transmit and receive APIs
│   ├── rc_spi.c               // SPI communication between Host and RideCast
│   └── rc_spi.h               // SPI configuration, initialization, transmit and receive APIs
│
└── README.md                  // RideCast architecture, protocol description, communication flow and integration guide

```

This keeps the RideCast firmware small, reusable, maintainable and independent of the Mobile Application's feature set.
