# RideCast — Cluster-Master Communication Flow

**Status:** Proposed unified application flow  
**Architecture:** Cluster master → RideCast slave → Mobile companion app  
**Transport:** UART (cluster), Wi-Fi and BLE (mobile)  
**Target:** ESP32 initially, portable application design

## 1. Product responsibilities

### Vehicle cluster (Master)

The cluster controls when RideCast is allowed to start, stop, retry or restart wireless connectivity.

It is responsible for:

- Sending START, STOP, STATUS and RESTART commands
- Displaying pairing information
- Displaying connected device details
- Displaying notifications, media and navigation information
- Handling the vehicle's user interface

### RideCast (Slave)

RideCast acts as the wireless communication coprocessor.

It is responsible for:

- Accepting commands from the cluster
- Starting and stopping wireless transports
- Generating and managing pairing sessions
- Authenticating the companion app
- Reporting connection status to the cluster
- Managing disconnection and recovery
- Receiving mobile application information
- Forwarding validated messages to the cluster
- Maintaining hardware-independent application logic

### Mobile companion application

The mobile app is responsible for:

- Discovering RideCast over BLE or Wi-Fi
- Establishing an authenticated session
- Providing its device name and application identity
- Obtaining permitted notification and media information
- Providing navigation information
- Sending structured messages to RideCast

RideCast cannot independently read all phone notifications or map data without the appropriate companion-app or supported mobile operating-system interfaces.

---

## 2. Complete connection sequence

```text
        VEHICLE CLUSTER             RIDECAST                  MOBILE APP
              |                        |                          |
              |                  Power ON                        |
              |                        |                          |
              |                 Initialize UART                  |
              |                 Wi-Fi OFF                        |
              |                 BLE OFF                          |
              |                        |                          |
              |<----- DEVICE_READY ----|                          |
              |                        |                          |
              |------ START ---------->|                          |
              |                        |                          |
              |                 Start Wi-Fi AP                    |
              |                 Start BLE (future)                |
              |                        |                          |
              |                 Create pairing session           |
              |                 Generate 6-digit code            |
              |                        |                          |
              |<---- PAIRING_INFO -----|                          |
              |                        |                          |
       Display pairing code            |                          |
              |                        |---- Advertise/Listen --->|
              |                        |                          |
              |                        |<---- Connect request ----|
              |<--- APP_CONNECTING ----|                          |
              |                        |                          |
              |                        |<--- Pairing proof -------|
              |                        |                          |
              |                 Verify pairing/session           |
              |                        |                          |
              |                        |---- Auth success ------->|
              |<---- APP_CONNECTED ----|                          |
              |      Device name       |                          |
              |      Transport         |                          |
              |                        |                          |
       Display "Connected"             |                          |
              |                        |                          |
              |                        |<--- Notification --------|
              |<--- NOTIFICATION ------|                          |
              |                        |                          |
              |                        |<--- Music metadata ------|
              |<--- MEDIA_INFO --------|                          |
              |                        |                          |
              |                        |<--- Navigation ----------|
              |<--- NAVIGATION --------|                          |
              |                        |                          |
              |                        |                          |
              |                        |<--- Connection lost -----|
              |<--- DISCONNECTED ------|                          |
              |                        |                          |
       Show "Disconnected"             |                          |
              |                        |                          |
              |                 Reconnect window 1               |
              |<----- RETRY(1/5) -----|                          |
              |                        |                          |
              |                       ...                         |
              |                        |                          |
              |                 Reconnect window 5               |
              |<----- RETRY(5/5) -----|                          |
              |                        |                          |
              |                 No successful reconnection       |
              |                        |                          |
              |<--- CONNECTION_FAILED -|                          |
              |                        |                          |
        Show failure                   |                          |
              |                        |                          |
              |                 Wait for cluster command         |
              |                        |                          |
              |------ RESTART -------->|                          |
              |                        |                          |
              |                 Restart pairing/connection       |
```

For Wi-Fi SoftAP, reconnection windows mean RideCast remains available for the phone to reconnect. The ESP32 cannot force a disconnected mobile phone to initiate a connection.

---

## 3. Connection-manager state machine

```text
POWER_ON
    |
    v
IDLE
    |
    | Cluster START
    v
STARTING
    |
    | Wireless services ready
    v
PAIRING
    |
    | Phone transport connected
    v
AUTHENTICATING
    |
    | Authentication successful
    v
CONNECTED
    |
    +-------------------------------+
    |                               |
    | Phone disconnected            | Cluster STOP
    v                               v
RECONNECTING                       STOPPING
    |                               |
    | Success                       v
    +----------> CONNECTED         IDLE
    |
    | Five attempts exhausted
    v
FAILED
    |
    +---- Cluster RESTART ----> STARTING
    |
    +---- Cluster STOP -------> IDLE
```

### State rules

| State | Required behavior |
|---|---|
| IDLE | UART control active; Wi-Fi and BLE stopped |
| STARTING | Start requested transports and report progress |
| PAIRING | Wireless available; pairing code presented to cluster |
| AUTHENTICATING | Transport connected; verify the app before accepting data |
| CONNECTED | Authenticated session; application data permitted |
| RECONNECTING | Inform cluster and execute bounded reconnection windows |
| FAILED | Report retry exhaustion and await cluster instruction |
| STOPPING | Close sessions, stop wireless services and release resources |

**Only an authenticated session may forward mobile application data to the cluster.**

A Wi-Fi association or accepted TCP socket alone must never trigger an `APP_CONNECTED` status.

---

## 4. Cluster UART protocol

Retain the current versioned `rc_protocol` architecture after correcting its implementation inconsistencies.

### Existing wire format

```text
+--------+--------+-----------+----------+--------+---------+--------+
| HEADER | MSG ID | TRANSPORT | SEQUENCE | LENGTH | PAYLOAD | CRC16  |
| 1 byte | 1 byte |  1 byte   | 2 bytes  | 2 bytes| N bytes | 2 bytes|
+--------+--------+-----------+----------+--------+---------+--------+
```

Minimum frame: **9 bytes**.

Protocol rules:

- Use the existing CRC16 approach.
- Use the existing network/big-endian representation for 16-bit fields.
- Define message IDs and payload schemas centrally.
- Validate protocol version, length, message type, transport and CRC.
- Use one UART streaming parser capable of handling split and concatenated frames.
- Match ACK responses to command sequence numbers.
- Reject unsupported commands explicitly.
- Keep messages binary-safe.

### Required command/event semantics

Reuse existing message IDs wherever their semantics match. Add or revise IDs through an approved protocol-version update where necessary.

| Direction | Logical message | Meaning |
|---|---|---|
| Cluster → RideCast | DEVICE_START | Start requested transports |
| Cluster → RideCast | DEVICE_STOP | Stop wireless sessions |
| Cluster → RideCast | DEVICE_RESTART | Reset wireless session lifecycle |
| Cluster → RideCast | GET_STATUS | Retrieve current state |
| RideCast → Cluster | DEVICE_READY | UART control service available |
| RideCast → Cluster | WIRELESS_READY | Wi-Fi/BLE transport started |
| RideCast → Cluster | PAIRING_INFO | Display pairing code and transport |
| RideCast → Cluster | APP_CONNECTING | Candidate mobile app connected |
| RideCast → Cluster | APP_CONNECTED | Mobile app authenticated |
| RideCast → Cluster | APP_DISCONNECTED | Authenticated session ended |
| RideCast → Cluster | RECONNECTING | Retry number and maximum |
| RideCast → Cluster | CONNECTION_FAILED | Retry limit reached |
| RideCast → Cluster | NOTIFICATION_DATA | Phone notification payload |
| RideCast → Cluster | MEDIA_DATA | Music metadata/playback state |
| RideCast → Cluster | NAVIGATION_DATA | Turn-by-turn navigation data |
| Either direction | ACK / ERROR | Command result or rejection |

This is the required logical message set, not a claim that each identifier already exists in `rc_protocol.h`.

### Required status information

The cluster must be able to distinguish:

- Wireless interface ready
- Phone connected to Wi-Fi
- TCP/BLE transport connected
- App authentication in progress
- App authentication succeeded
- App disconnected
- Reconnection attempt number
- Permanent connection failure

Do not collapse these into a generic CONNECTED status.

---

## 5. Pairing and security flow

The pairing process must create a new, temporary authorization context.

1. Cluster sends START.
2. RideCast starts the requested wireless transports.
3. RideCast generates a fresh six-digit code using a cryptographically secure random source.
4. RideCast sends the code to the cluster for display.
5. Phone user enters or confirms the displayed code in the companion app.
6. RideCast verifies the pairing proof within a short timeout and attempt limit.
7. A protected, authenticated session is established.
8. RideCast reports the verified device name and connection status.
9. Cluster displays the connection result.

### Important distinctions

**Wi-Fi credential:** Use a separate valid WPA2/WPA3 password. A six-digit number is not a valid conventional WPA2-Personal passphrase.

**App pairing code:** The six-digit number is a temporary confirmation secret, not a permanent password.

**BLE pairing:** When BLE is implemented, use the Bluetooth stack's supported authenticated pairing/bonding mechanism. Do not substitute plaintext six-digit-code transmission for BLE security.

**TCP security:** Sending the six-digit code or a fixed token over plaintext TCP does not provide production-grade authenticated encryption. TLS and a suitable pairing/key-establishment protocol are required before handling sensitive live phone data in a production product.

After pairing, protect sessions against replay, unauthenticated reconnection and unauthorized additional clients.

---

## 6. Connection and retry policy

### Normal disconnect

1. Detect loss of the authenticated mobile session.
2. Immediately report APP_DISCONNECTED to the cluster.
3. Stop forwarding data associated with the lost session.
4. Preserve the authorized reconnect context only according to security policy.
5. Start reconnect attempt 1.
6. Report each retry attempt and its current count.
7. Continue for at most five bounded attempts.

### Reconnection success

- Reauthenticate or securely resume the mobile session.
- Reset the retry count to zero.
- Notify the cluster with APP_CONNECTED.
- Resume validated message delivery.

### Reconnection failure

After the fifth unsuccessful attempt:

- Report CONNECTION_FAILED.
- Stop the automatic retry process.
- Enter FAILED.
- Await cluster STOP or RESTART.
- Do not loop indefinitely or restart Wi-Fi automatically without cluster authorization.

### Cluster RESTART

A RESTART command means:

- Stop relevant transport sessions.
- Clear expired pairing state.
- Reset retry counters.
- Start a fresh authorized connection cycle.
- Report the new pairing/session status to the cluster.

A full MCU reset is a separate command or policy decision and is not implied by restarting connectivity.

---

## 7. Mobile application data flow

### Supported message classes

**Notification**

```text
Type       : NOTIFICATION
Source     : Application identifier
Title      : Bounded UTF-8 text
Content    : Bounded UTF-8 text
Timestamp  : Optional
```

**Music**

```text
Type       : MEDIA
Title      : Song title
Artist     : Artist name
State      : Playing / Paused / Stopped
Position   : Optional playback position
```

**Navigation**

```text
Type       : NAVIGATION
Instruction: Turn left / Turn right / Continue / etc.
Distance   : Distance to next maneuver
ETA        : Estimated arrival
Coordinates: Optional position
Route ID   : Optional
```

RideCast validates the message envelope and forwards the supported data to the cluster.

The cluster remains responsible for drawing navigation prompts, media interfaces and notifications.

### Map rendering

Phase 1 should use simulated navigation instructions and coordinates.

Full Google Maps or open-source map rendering requires additional design for map data access, licensing, bandwidth, memory, framing and cluster rendering capabilities.

Do not introduce raw map-tile streaming until the cluster's graphics and memory requirements are defined.

---

## 8. Module ownership

The existing source files should be aligned to these responsibilities.

| Module | Required ownership |
|---|---|
| `app_main.c` | Platform bootstrap only |
| `ridecast_main.c` | Application lifecycle and wiring |
| `uart_app.c` | Cluster UART framing, RX dispatch, serialized TX |
| `rc_protocol.h/.c` | Single authoritative frame and message contract |
| `rc_connection.c` | Central connection lifecycle, pairing orchestration and retry policy |
| `rc_wifi.c` | Wi-Fi AP and TCP operations; no independent retry policy |
| `wifi_app.c` | Legacy/alternative Wi-Fi implementation; do not use concurrently with `rc_wifi.c` |
| Future BLE module | BLE transport interface and security integration |
| `rc_config.c` | Application-owned RTOS and peripheral configuration |
| BSP drivers | Platform hardware operations only |

Use one connection manager, one Wi-Fi implementation and one UART protocol.

Do not add a third alternative implementation.

---

## 9. Required corrections before feature expansion

### P0 — Protocol consistency

- [ ] Choose a single definition of `rc_frame_t`.
- [ ] Align `rc_protocol.h` and `rc_protocol.c`.
- [ ] Remove duplicate encode/decode implementations.
- [ ] Replace the obsolete UART frame parser.
- [ ] Correct every old `subcode`/`intf` reference.
- [ ] Test framing, CRC, stream splitting and frame concatenation.

### P1 — Application integration

- [ ] Align application CMake and header dependencies.
- [ ] Align `rc_config.c` with actual task and queue definitions.
- [ ] Confirm the existing BSP UART and Wi-Fi APIs.
- [ ] Confirm ESP-IDF/CMSIS startup behavior.
- [ ] Compile and link the baseline application.

### P2 — Cluster-master lifecycle

- [ ] Keep Wi-Fi/BLE off until START.
- [ ] Implement START, STOP, RESTART and STATUS handling.
- [ ] Implement explicit connection-manager states.
- [ ] Correct the distinction between wireless ready and app authenticated.
- [ ] Route all state changes to the cluster.

### P3 — Pairing

- [ ] Generate fresh six-digit pairing code.
- [ ] Deliver pairing information to cluster.
- [ ] Implement pairing expiration and failure limits.
- [ ] Validate mobile-app session authentication.
- [ ] Report authenticated device identity.
- [ ] Block data before authentication.

### P4 — Reconnection

- [ ] Detect authenticated-session loss.
- [ ] Report disconnect immediately.
- [ ] Implement five bounded reconnection windows.
- [ ] Report each retry.
- [ ] Enter FAILED after exhaustion.
- [ ] Await cluster instruction.
- [ ] Test STOP and RESTART during retry.

### P5 — Application data

- [ ] Define notification payload schema.
- [ ] Define media payload schema.
- [ ] Define navigation payload schema.
- [ ] Validate each received message.
- [ ] Forward messages over UART.
- [ ] Test binary integrity, queue limits and backpressure.
- [ ] Use a phone simulator until a real companion app is available.

### P6 — BLE preparation

- [ ] Define portable BLE initialization API.
- [ ] Define start/stop and pairing interfaces.
- [ ] Define BLE event callbacks.
- [ ] Provide an explicit NOT_SUPPORTED stub.
- [ ] Do not claim BLE functionality before real integration.

---

## 10. Completion criteria

The cluster-master flow is validated only after the following tests pass:

1. Power on RideCast; verify Wi-Fi/BLE remain off.
2. Cluster sends START; requested transport starts.
3. Cluster receives and displays a fresh pairing code.
4. Phone connects but cannot forward application data before authentication.
5. Valid pairing succeeds and RideCast notifies the cluster.
6. Device identity is displayed after authentication.
7. Phone notification reaches the cluster correctly.
8. Music metadata reaches the cluster correctly.
9. Simulated navigation data reaches the cluster correctly.
10. Phone disconnect is reported promptly.
11. Exactly five authorized reconnection windows are attempted.
12. Reconnection success restores the authenticated state.
13. Reconnection failure transitions to FAILED without automatic restart.
14. Cluster RESTART initiates a new authorized connection cycle.
15. Cluster STOP returns RideCast to IDLE with wireless services stopped.

All results require actual build, simulation or hardware-test evidence.

**End of proposed cluster-master flow.**
