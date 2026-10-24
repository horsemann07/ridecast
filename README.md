# RideCast

**Portable Wi-Fi and Serial Communication Platform**

## Overview

RideCast is an embedded communication platform that connects a mobile device (phone or PC) to a vehicle cluster MCU/MPU.

The project is designed around a reusable, hardware-independent Board Support Package (BSP), portable application modules, and configurable communication services.

The initial development target is **ESP32 with ESP-IDF 5.5**, with future support planned for STM32H7 and other MCU platforms.

The first implementation focuses on **Wi-Fi communication and UART data forwarding**. Bluetooth, CAN, USB, encryption, and more advanced routing can be integrated incrementally.

### Design Goals

- Keep the RideCast application independent of vendor-specific SDK APIs wherever practical.
- Reuse BSP interfaces for Wi-Fi, UART, logging, CLI, and other hardware services.
- Use lwIP/BSD sockets for TCP/IP communication.
- Use mbedTLS for future secure application communication.
- Use CMSIS-RTOS2 interfaces where practical for portable application tasks and synchronization.
- Keep application configuration separate from reusable BSP configuration.
- Avoid unnecessary middleware and abstraction layers.
- Support incremental development, testing, and platform migration.

---

## System Architecture

The Network Abstraction Layer (NAL) from the original design has been removed.

The application uses BSD-compatible socket APIs directly for networking and mbedTLS directly for secure communication when enabled. Vendor-specific wireless initialization and hardware drivers remain behind BSP interfaces.

```text
+-------------------------------------------------------+
|           APPLICATION CONFIGURATION LAYER             |
|-------------------------------------------------------|
| Wi-Fi Settings | TCP Port | UART Configuration        |
| Task Settings | Buffer Sizes | Security Settings      |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|                  RIDECAST APPLICATION                 |
|-------------------------------------------------------|
| Application Startup                                   |
| Wi-Fi Connection Management                           |
| TCP Server / Client Session Management                |
| Wi-Fi-to-UART Data Bridge                              |
| Application State Machine                              |
| Diagnostics and Command Handlers                      |
+-------------------------------------------------------+
             |                      |
             v                      v
+--------------------------+  +-------------------------+
|    NETWORK LIBRARIES     |  |      REUSABLE BSP       |
|--------------------------|  |-------------------------|
| BSD Socket APIs          |  | bsp_wifi.h              |
| lwIP                     |  | bsp_uart.h              |
| mbedTLS (optional)       |  | BSP Logger              |
|                          |  | BSP CLI                 |
|                          |  | BSP Configuration       |
+--------------------------+  +-------------------------+
             |                      |
             +-----------+----------+
                         |
                         v
+-------------------------------------------------------+
|                 PLATFORM INTEGRATION                  |
|-------------------------------------------------------|
| ESP32: ESP-IDF / esp_netif / Wi-Fi Driver / FreeRTOS   |
| STM32: HAL / Network Driver / RTOS (Future)            |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|                        HARDWARE                       |
|-------------------------------------------------------|
| ESP32 / STM32 MCU | Wi-Fi | UART | Cluster MCU / MPU   |
+-------------------------------------------------------+
```

### Architectural Boundaries

**Application layer**

Contains RideCast-specific communication logic, connection state management, data forwarding, and application configuration. Portable modules must avoid direct dependencies on `esp_wifi.h`, `driver/uart.h`, and other ESP-IDF hardware APIs.

**Reusable BSP**

Provides hardware-independent public APIs and platform-specific implementations. BSP modules must use generic names and must not be prefixed with `ridecast`.

**Network libraries**

Application networking uses BSD socket operations such as `socket()`, `bind()`, `listen()`, `accept()`, `recv()`, and `send()`.

Application-level TLS and cryptography will use mbedTLS when enabled.

**Platform integration**

The ESP32 implementation connects the BSP and network libraries to ESP-IDF. Future MCU ports may use different vendor SDKs without requiring changes to the core application logic.

---

## Project Folder Structure

The following is the **proposed application structure**, aligned with the newer reusable BSP layout. Exact source locations and CMake registrations must be matched to the current repository.

```text
ridecast/
|
+-- app/
|   +-- app_config/
|   |   +-- app_config.h
|   |   +-- app_wifi_config.h
|   |   +-- app_uart_config.h
|   |   +-- app_network_config.h
|   |
|   +-- include/
|   |   +-- ridecast_app.h
|   |   +-- ridecast_bridge.h
|   |   +-- ridecast_tcp_server.h
|   |   +-- ridecast_wifi_service.h
|   |
|   +-- main/
|   |   +-- app_main.c
|   |   +-- CMakeLists.txt
|   |
|   +-- src/
|       +-- ridecast_app.c
|       +-- ridecast_bridge.c
|       +-- ridecast_tcp_server.c
|       +-- ridecast_wifi_service.c
|
+-- bsp/
|   +-- bsp_config/
|   |
|   +-- include/
|   |   +-- bsp_core/
|   |   +-- bsp_uart.h
|   |   +-- bsp_wifi.h
|   |
|   +-- srcs/
|   |   +-- bsp_core/
|   |   |   +-- cli/
|   |   |   +-- logger/
|   |   |
|   |   +-- port/
|   |       +-- esp32x/
|   |       |   +-- bsp_uart.c
|   |       |   +-- bsp_wifi.c
|   |       |
|   |       +-- stm32h7x/
|   |
|   +-- bsp.cmake
|   +-- CMakeLists.txt
|
+-- external/
|   +-- component/
|   |   +-- lwip/
|   |   +-- mbedtls/
|   |
|   +-- os/
|       +-- cmsis/
|       +-- cmsis_esp/
|
+-- sdk/
|   +-- esp/
|   +-- stm/
|
+-- tests/
|   +-- unit/
|   +-- integration/
|
+-- docs/
|   +-- images/
|   +-- doxygen/
|
+-- cmake/
+-- build.py
+-- CMakeLists.txt
+-- README.md
```

The `external/component/` directories are intended for independently managed third-party dependencies and ports. They must not cause duplicate TCP/IP or TLS implementations to be linked into a single firmware image.

---

## Phase 1: Wi-Fi-to-UART Communication

### Objective

Establish a basic data communication path from a phone or PC to the vehicle cluster MCU using Wi-Fi and UART.

**Primary data flow:**

```text
Phone / PC
    |
    | Wi-Fi
    v
ESP32 Wi-Fi Interface
    |
    | TCP Connection
    v
RideCast TCP Server
    |
    | Received Byte Stream
    v
RideCast Bridge
    |
    | BSP UART API
    v
ESP32 UART Peripheral
    |
    | TX / RX Pins
    v
Vehicle Cluster MCU / MPU
```

The initial milestone is one-way forwarding from TCP to UART. Bidirectional forwarding can be enabled after the basic path is validated.

### Initial Operating Mode

For simple phone-to-device testing, start in **Wi-Fi SoftAP mode**.

Example development configuration:

| Parameter | Initial value |
|---|---|
| Wi-Fi mode | SoftAP |
| SSID | RideCast-Dev |
| Wi-Fi password | Development-only, configurable |
| Transport | TCP |
| TCP port | 3333 |
| UART | Board-configured cluster UART |
| UART baud rate | 115200 initially |
| UART format | 8 data bits, no parity, 1 stop bit |
| Payload format | Raw binary bytes |
| Application TLS | Not enabled in initial prototype |

These are development defaults, not hardcoded production requirements.

### Startup Sequence

```text
app_main()
    |
    v
Initialize required platform services
    |
    v
Initialize BSP logger
    |
    v
Initialize BSP UART
    |
    v
Initialize BSP Wi-Fi
    |
    v
Configure and start Wi-Fi SoftAP
    |
    v
Wait until the Wi-Fi interface is ready
    |
    v
Start TCP server
    |
    v
Accept client connection
    |
    v
Receive TCP data
    |
    v
Forward data through BSP UART
```

Initialization must check and handle each returned status. Failed startup should release already-acquired resources where supported.

### TCP-to-UART Forwarding

The TCP server receives a bounded chunk of data and passes it to the bridge.

The bridge sends the bytes through the existing BSP UART interface.

```text
TCP recv()
    |
    v
Check connection and received length
    |
    v
Forward received bytes to UART
    |
    v
Handle partial writes, busy states, and timeouts
    |
    v
Receive the next chunk
```

TCP is a byte stream. A single `recv()` call is not guaranteed to correspond to one message sent by the phone.

The initial prototype forwards raw bytes. A framed protocol with message length, type, checksum, and optional authentication will be introduced when application-level message boundaries are required.

### Data Buffering

The first implementation should avoid creating multiple large buffers.

Recommended starting approach:

- Use one bounded receive buffer owned by the TCP worker.
- Forward data using the BSP UART write mechanism.
- Handle UART backpressure without unbounded queue growth.
- Keep the buffer size configurable.
- Measure throughput before introducing additional queues or buffers.

A dedicated bridge queue is optional and should only be added if the blocking behavior or performance of the existing UART implementation requires it.

### Connection Handling

The TCP server must handle:

- Client connection and disconnection.
- Partial TCP receives and sends.
- Interrupted or failed socket operations.
- UART initialization or transmission failures.
- TCP socket cleanup and recovery.
- Wi-Fi stop/restart events.
- Proper cancellation during application shutdown.

For the initial implementation, a single connected TCP client is sufficient.

---

## Application Modules

### `ridecast_app.c`

Responsible for the application lifecycle.

Responsibilities:

- Initialize application services.
- Coordinate initialization order.
- Maintain top-level application state.
- Start and stop communication services.
- Handle initialization failures and controlled shutdown.

Suggested public interface:

```c
ret_err_sts_t ridecast_app_init(void);

ret_err_sts_t ridecast_app_start(void);

ret_err_sts_t ridecast_app_stop(void);

ret_err_sts_t ridecast_app_deinit(void);
```

These are proposed application interfaces, not existing verified BSP APIs.

### `ridecast_wifi_service.c`

Responsible for Wi-Fi application configuration and lifecycle.

Responsibilities:

- Configure the Wi-Fi mode.
- Start the Wi-Fi interface through BSP APIs.
- Observe Wi-Fi state or events.
- Report readiness to the application.
- Handle connection-related recovery.

The module must not directly access ESP-IDF Wi-Fi driver interfaces.

### `ridecast_tcp_server.c`

Responsible for TCP/IP communication using BSD sockets.

Responsibilities:

- Create and bind the TCP socket.
- Listen for incoming clients.
- Accept a client connection.
- Receive data using `recv()`.
- Pass received bytes to the bridge.
- Clean up sockets on disconnect or failure.

No separate NAL module is required.

### `ridecast_bridge.c`

Responsible for forwarding data between communication endpoints.

Phase 1:

```text
TCP -> UART
```

Phase 2:

```text
TCP <-> UART
```

Future support may include UART, CAN, USB, Bluetooth, and configurable endpoint routing.

The bridge should depend on portable application and BSP interfaces, not ESP-IDF hardware-driver functions.

### `app_main.c`

Contains the ESP-IDF-specific entry point:

```c
void app_main(void)
{
    /* Initialize and start the RideCast application. */
}
```

This is the platform bootstrap only. The core application startup logic belongs in `ridecast_app.c`.

Future platforms can provide a different entry point while reusing the same application modules.

---

## CLI and Diagnostics

### Native ESP-IDF CLI Support

ESP-IDF provides the `esp_console` component, including command registration, interactive shell support, argument handling, command history, and UART/USB console integrations.

### RideCast CLI Strategy

RideCast already includes a reusable BSP CLI. It will remain the preferred command interface for cross-platform compatibility.

The ESP-IDF console may be used as an optional development integration, but application commands should not depend directly on ESP-IDF command structures.

Proposed development commands:

```text
> help

> system status

> wifi status

> wifi start

> wifi stop

> tcp status

> bridge status

> uart status
```

Commands should report actual state from the application and BSP interfaces. They must not fabricate connection states or success results.

### UART Resource Separation

The interactive console must not interfere with the UART used for vehicle-cluster communication.

Use a dedicated development UART or a supported USB console when available.

On production builds, CLI access must follow the secure-session and authentication policy. Diagnostic and debug interfaces must not expose sensitive data.

---

## Logging

The application will use the reusable BSP logger.

Expected severity levels:

```text
ERROR
WARNING
INFO
DEBUG
```

The initial default level is `INFO`, subject to the actual logger configuration.

Runtime per-file discovery and per-file enable/disable functionality is deferred.

Logs should cover:

- Startup and initialization failures.
- Wi-Fi state changes.
- TCP connection and disconnection.
- UART transmission errors.
- Bridge buffer or throughput problems.
- Resource cleanup and recovery failures.

Passwords, cryptographic keys, authentication tokens, and sensitive application payloads must not be logged.

Development and production-testing builds may retain bounded pre-authentication logs for later transmission after an authenticated secure session is established. Market-release handling follows the separate security build profile.

---

## Networking and TLS

### TCP/IP

The application uses BSD-compatible socket APIs, allowing most network communication logic to be reused across ESP32 and other systems.

On ESP32, Wi-Fi and network interface integration are supplied by the ESP-IDF platform.

On other targets, the platform must provide a compatible network stack and socket implementation.

### lwIP Integration

The project may maintain a separate upstream lwIP dependency for future platform ports.

**ESP32 builds must use one compatible lwIP integration.**

ESP-IDF's Wi-Fi and network interface components are integrated with its lwIP port. A second independently linked upstream lwIP stack must not be added without a specifically engineered replacement and compatibility plan.

The application remains portable by using standard networking interfaces rather than depending on ESP32-specific network internals.

### mbedTLS Integration

Application-level TLS will use mbedTLS.

The application should keep TLS session configuration, connection handling, certificate validation, and cryptographic policy outside BSP hardware drivers.

For ESP32, the selected mbedTLS build must be compatible with ESP-IDF components and dependencies. Avoid linking duplicate or incompatible mbedTLS library versions.

### Security Roadmap

**Phase 1 — Development prototype**

Wi-Fi SoftAP and plain TCP-to-UART forwarding for controlled local testing.

**Phase 2 — Secure communication**

TLS authentication and encrypted TCP communication, with provision for certificate management.

**Phase 3 — Production communication**

Authenticated client sessions, replay protection where appropriate, message validation, secure credential storage, and release-build security controls.

A working TCP connection alone is not proof of a secure communication channel.

---

## BSP Portability

The BSP is a reusable component, not a RideCast-specific library.

Its public headers should expose vendor-independent types and operations wherever practical.

Examples:

```c
#include "bsp_wifi.h"
#include "bsp_uart.h"
```

The ESP32 implementation remains in the ESP-specific BSP port directory. Future STM32H7 drivers can implement the same interfaces.

Application code must use the existing BSP function signatures and return-status definitions. The app should not redefine UART or Wi-Fi handles or duplicate driver logic.

---

## Build and Configuration

### Initial Platform

- ESP32
- ESP-IDF 5.5
- C11
- CMSIS-RTOS2 adaptation where required
- Existing BSP UART and Wi-Fi ports
- lwIP-compatible BSD sockets

### Windows Build

From the project root:

```powershell
python.exe .\build.py -h
python.exe .\build.py -b
```

Additional build and clean options should be taken from the current `build.py` help output.

### CMake Integration

The root project must register:

- The BSP component and enabled platform port.
- The application entry component.
- The application source modules.
- The appropriate lwIP and networking dependencies.
- mbedTLS when application TLS support is enabled.
- Any required CMSIS-RTOS2 adaptation dependencies.

The BSP should continue to use generic names in its CMake targets and configuration options.

---

## Development Milestones

### Milestone 1 — BSP Integration

- [ ] Confirm ESP32 BSP UART initialization and transmission.
- [ ] Confirm BSP Wi-Fi SoftAP startup.
- [ ] Confirm logger initialization and operation.
- [ ] Confirm the application links against the existing BSP component.

### Milestone 2 — Basic TCP Communication

- [ ] Start Wi-Fi SoftAP.
- [ ] Connect a phone or PC.
- [ ] Start a TCP server.
- [ ] Accept one TCP client.
- [ ] Receive a test message.

### Milestone 3 — TCP-to-UART Bridge

- [ ] Initialize the cluster UART.
- [ ] Forward TCP bytes to UART.
- [ ] Handle partial UART transmission and backpressure.
- [ ] Validate binary payload forwarding.
- [ ] Recover correctly after client disconnect.

### Milestone 4 — Bidirectional Bridge

- [ ] Read bytes from the UART.
- [ ] Forward UART data to the TCP client.
- [ ] Manage data flow in both directions.
- [ ] Validate simultaneous receive and transmit operation.

### Milestone 5 — Robustness

- [ ] Test repeated connect/disconnect cycles.
- [ ] Test long-duration transfers.
- [ ] Measure throughput and latency.
- [ ] Measure RAM and queue usage.
- [ ] Test Wi-Fi interruption and recovery.
- [ ] Test shutdown during active transfer.

### Milestone 6 — Security

- [ ] Introduce application TLS.
- [ ] Validate certificates and peer identity.
- [ ] Add client authentication and authorization.
- [ ] Protect credentials and session state.
- [ ] Validate secure-session and release-build policies.

These are planned milestones; completion must be confirmed by builds and tests.

---

## First Integration Test

### Required Hardware

- An ESP32 development board.
- A phone or PC with Wi-Fi.
- A USB-to-UART adapter or vehicle-cluster test MCU.
- A suitable shared-ground UART connection.
- Logic-level compatibility between connected UART devices.

### Test Procedure

1. Build and flash the firmware.
2. Connect the phone/PC to the development Wi-Fi access point.
3. Connect to the configured TCP server port.
4. Send a known byte sequence.
5. Observe the same byte sequence on the designated cluster UART.
6. Verify the transmitted length and ordering.
7. Disconnect and reconnect the TCP client.
8. Confirm the system continues operating correctly.

Example transmitted payload:

```text
52 49 44 45 43 41 53 54
```

Expected UART output:

```text
52 49 44 45 43 41 53 54
```

This represents the ASCII text `RIDECAST`.

The example tests raw data forwarding only; it does not validate a complete vehicle-cluster communication protocol.

---

## Future Development

Planned extensions include:

- Bidirectional TCP/UART communication.
- Binary message framing and routing.
- Wi-Fi Station mode and network configuration.
- Bluetooth BLE/Classic integration where hardware supports it.
- CAN and USB communication endpoints.
- Higher-throughput buffering strategies.
- TLS and AES-GCM security mechanisms.
- Secure device pairing and authentication.
- Performance metrics and diagnostics.
- Host-based unit testing.
- Hardware-in-the-loop integration tests.
- STM32H7 platform support.

---

## Documentation

API documentation will use Doxygen and concise MISRA C:2012-oriented documentation conventions.

Documentation must distinguish implementation behavior, design proposals, and verified test results.

No documentation convention alone establishes MISRA compliance.

---

## Project Status

**Current focus:** Implement the first RideCast application flow using the existing BSP Wi-Fi and UART interfaces.

**Target:** Receive TCP data over Wi-Fi and forward it to the cluster UART.

**Architecture:** Portable application, reusable BSP, direct BSD-socket networking, and optional mbedTLS.

**Current limitation:** The application integration and hardware test remain to be completed and verified.

---

## License

Refer to the repository's `LICENSE` file for the applicable license terms.
