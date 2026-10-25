# RideCast — Phase 1 Step-by-Step Execution Guide

**Document:** `PHASE_1_EXECUTION_GUIDE.md`  
**Project:** RideCast  
**Phase:** 1 — Wi-Fi-to-UART Communication  
**Initial Status:** NOT STARTED

**Required reading:**

1. `README.md`
2. `PHASE_1_PLAN.md`
3. This execution guide

---

## 1. AI Coding Instructions

### 1.1 Mandatory Rules

Before editing code, the AI must:

1. Read the three project documents.
2. Identify the current execution step.
3. Inspect the existing files required by that step.
4. Confirm actual API signatures and dependencies.
5. Identify which files need modification.
6. Implement only the requested step.
7. Run applicable validation.
8. Report actual results.
9. Stop and request authorization before proceeding.

**Do not implement multiple steps in a single response unless the user explicitly changes this rule.**

### 1.2 Never Assume

Do not assume:

- Hardware pin assignments
- ESP32 target variant
- BSP function signatures
- BSP initialization order
- UART callback or blocking behavior
- Wi-Fi lifecycle behavior
- Existing application files
- Valid return-status names
- Build dependencies
- Test outcomes

Verify these from the repository, hardware setup, or user-provided evidence.

### 1.3 Code Preservation

Do not rewrite existing BSP drivers to make application integration easier.

Prefer adapting the application to the verified BSP interface.

A BSP defect must be reported and approved before modification.

### 1.4 Required AI Response Format

At the end of each step, report:

```text
STEP:
GATE:

FILES INSPECTED:
FILES MODIFIED:

WORK COMPLETED:

BUILD:
PASS / FAIL / NOT RUN

TESTS:
PASS / FAIL / BLOCKED / NOT RUN

EVIDENCE:

OPEN ISSUES:

SCOPE DEVIATION:
NONE / DETAILS

NEXT STEP:
WAITING FOR USER APPROVAL
```

Do not report hardware tests as passed based only on source-code review.

---

## 2. Execution Overview

Execute these steps in order:

```text
STEP 00 - Repository and hardware audit
    |
    v
STEP 01 - Baseline CMake/build validation
    |
    v
STEP 02 - BSP UART smoke test
    |
    v
STEP 03 - BSP Wi-Fi SoftAP smoke test
    |
    v
STEP 04 - Application skeleton and lifecycle
    |
    v
STEP 05 - TCP server implementation
    |
    v
STEP 06 - TCP-to-UART bridge
    |
    v
STEP 07 - Integration and robustness testing
    |
    v
STEP 08 - Final review and Phase 1 sign-off
```

Steps 04 and 05 jointly satisfy the TCP readiness gate G4. The mapping is:

| Step | Gate |
|---|---|
| 00 | G0 |
| 01 | G1 |
| 02 | G2 |
| 03 | G3 |
| 04–05 | G4 |
| 06 | G5 |
| 07 | G6 |
| 08 | G7 |

---

## STEP 00 — Repository and Hardware Audit

**Gate:** G0  
**Implementation permitted:** NO

### Objective

Understand the real repository and hardware before generating application code.

### AI Tasks

Inspect:

```text
CMakeLists.txt
build.py
bsp/CMakeLists.txt
bsp/bsp.cmake
bsp/include/
bsp/srcs/
bsp/bsp_config/
app/
```

Locate and read the actual:

- `bsp_uart.h`
- `bsp_uart.c`
- `bsp_wifi.h`
- `bsp_wifi.c`
- BSP logger headers
- BSP CLI headers
- Shared return-status header
- CMSIS-RTOS2 adaptation configuration

Record:

- Exact initialization APIs
- Configuration types
- Public UART send/write APIs
- Blocking or asynchronous write semantics
- UART completion and timeout behavior
- Wi-Fi SoftAP configuration APIs
- Wi-Fi start/stop APIs
- Wi-Fi readiness/event mechanism
- Deinitialization and resource ownership
- Relevant CMake components
- ESP-IDF component dependencies

### Hardware Checklist

- [ ] Exact ESP32 target identified
- [ ] Board model identified
- [ ] Dedicated cluster UART identified
- [ ] TX GPIO confirmed
- [ ] RX GPIO confirmed
- [ ] Logic voltage confirmed
- [ ] UART receiver/test adapter available
- [ ] Development console available separately
- [ ] Wi-Fi-capable PC or phone available

### Required Deliverable

Produce a concise repository audit report.

Example:

```text
BSP Wi-Fi header:
<verified path>

BSP UART header:
<verified path>

UART initialization API:
<exact signature>

UART transmission API:
<exact signature>

Wi-Fi SoftAP initialization/start APIs:
<exact signatures>

ESP32 board:
<actual model>

UART TX / RX:
<confirmed GPIOs>

Build entry:
<verified command>

Unresolved issues:
<list>
```

### Gate G0 Pass Condition

All required BSP interfaces and hardware details are known.

No unapproved code modification has occurred.

**STOP after presenting the audit.**

---

## STEP 01 — Baseline CMake and Build Validation

**Gate:** G1

### Objective

Establish a known working build before adding RideCast features.

### AI Tasks

1. Read the current CMake configuration.
2. Check BSP source paths and component dependencies.
3. Confirm the application entry-point configuration.
4. Inspect the supported flags in `build.py`.
5. Run the existing build.
6. Record all build failures.

### Windows Commands

From the repository root:

```powershell
python.exe .\build.py -h
python.exe .\build.py -b
```

Use other build flags only after verifying them through the script.

### Important

A previous version of `bsp/bsp.cmake` contained an unmatched `foreach()` block and incorrect source-path handling.

Verify whether the current repository still has those problems.

Do not apply old patches blindly.

### Allowed Modifications

Only verified baseline build blockers, with user approval.

Examples:

- Incorrect BSP source paths
- Invalid CMake nesting
- Missing required component declarations
- Incorrect source registration

### Not Allowed

- Adding Wi-Fi-to-UART bridge logic
- Replacing BSP implementations
- Unrelated CMake restructuring
- Adding Bluetooth, TLS, or application frameworks

### Validation

- [ ] CMake configuration succeeds
- [ ] All required components are discovered
- [ ] C compilation succeeds
- [ ] Linking succeeds
- [ ] Build output is recorded
- [ ] No unapproved source changes

### Gate G1 Pass Condition

A reproducible firmware build succeeds.

**STOP after recording results.**

---

## STEP 02 — BSP UART Smoke Test

**Gate:** G2

### Objective

Validate the BSP UART independently of Wi-Fi and TCP.

### Hardware Connection

Use a dedicated UART-to-USB test adapter or an appropriate cluster test receiver.

For an ESP32-to-USB-UART one-way test:

```text
ESP32 UART TX ------> USB-UART RX
ESP32 GND ----------> USB-UART GND
```

Use compatible logic levels. Do not connect 5 V logic directly to a 3.3 V GPIO. Use the board's verified GPIO assignments and avoid boot-strapping pin conflicts.

### AI Tasks

1. Read the existing BSP UART API.
2. Create the smallest possible test integration.
3. Configure the selected UART through BSP APIs.
4. Transmit a known test sequence.
5. Check all returned statuses.
6. Do not bypass the BSP driver.

### Test Sequence

Transmit these 12 bytes:

```text
00 01 52 49 44 45 43 41 53 54 FE FF
```

The sequence deliberately contains:

- Zero byte `00`
- Binary byte `01`
- ASCII bytes
- High-value bytes `FE` and `FF`

The output must be exactly 12 bytes.

### PC Capture

If the test machine uses Python and `pyserial`, an example capture command is:

```powershell
python -m pip install pyserial
```

In a dedicated terminal, adapt the COM port and baud rate:

```powershell
python -c "import serial; s=serial.Serial('COM6',115200,timeout=15); data=s.read(12); print(data.hex(' ')); s.close()"
```

Start capture before triggering the UART transmission.

The COM port is only an example; use the actual attached adapter.

### Validation

- [ ] UART initialization succeeds
- [ ] Exact 12 bytes captured
- [ ] No missing bytes
- [ ] No added log messages on the data UART
- [ ] Error reporting works
- [ ] UART test exits or cleans up correctly

### Gate G2 Pass Condition

Independent hardware capture matches the exact transmitted sequence.

**STOP after presenting capture evidence.**

---

## STEP 03 — BSP Wi-Fi SoftAP Smoke Test

**Gate:** G3

### Objective

Validate Wi-Fi SoftAP startup without a TCP server.

### AI Tasks

1. Read actual BSP Wi-Fi APIs.
2. Determine any required network-stack and platform initialization.
3. Configure the BSP for SoftAP mode.
4. Start the Wi-Fi service.
5. Observe and report its actual readiness.
6. Keep application code independent of `esp_wifi.h`.

### Development Settings

```text
Mode     : SoftAP
SSID     : RideCast-Dev
Security : WPA2 or supported stronger mode
Password : Configured locally for development
```

Do not store production credentials in source code.

### Test Procedure

1. Flash the application.
2. Start the device.
3. Scan for Wi-Fi networks from a phone or PC.
4. Confirm `RideCast-Dev` is visible.
5. Connect using the configured development credential.
6. Confirm the client establishes network connectivity.
7. Record the device's actual network address and the client address.
8. Observe disconnect and reconnect behavior.

Do not assume a particular SoftAP IP address unless confirmed by the runtime configuration.

### Validation

- [ ] Wi-Fi initialization succeeds
- [ ] SoftAP starts
- [ ] SSID is visible
- [ ] Phone/PC connects
- [ ] Network addressing works
- [ ] Readiness is accurately reported
- [ ] Start/stop behavior tested where supported

### Gate G3 Pass Condition

An external device can connect to the ESP32 SoftAP.

**STOP after presenting evidence.**

---

## STEP 04 — Application Skeleton and Lifecycle

**Gate:** G4 (Part 1)

### Objective

Create only the application structure required for Phase 1.

### Files to Introduce as Needed

```text
app/main/app_main.c

app/include/ridecast_app.h
app/src/ridecast_app.c

app/app_config/app_config.h
app/app_config/app_wifi_config.h
app/app_config/app_uart_config.h
app/app_config/app_network_config.h
```

Avoid creating unnecessary source files.

### Application Startup

```text
app_main()
    |
    v
Platform bootstrap
    |
    v
Application initialization
    |
    v
BSP logger readiness
    |
    v
BSP UART initialization
    |
    v
BSP Wi-Fi initialization and startup
    |
    v
Application ready
```

Resolve the precise order from the actual BSP contracts.

### AI Tasks

1. Implement a minimal application entry point.
2. Add a portable application lifecycle API.
3. Move configuration values out of implementation files.
4. Use the shared BSP return-status type.
5. Implement startup error handling.
6. Release initialized resources in the correct order on failure.
7. Do not add TCP-to-UART forwarding yet.

### Validation

- [ ] Application component registers correctly
- [ ] Firmware compiles and links
- [ ] Application initializes existing BSP services
- [ ] Startup failures propagate
- [ ] No direct ESP-IDF Wi-Fi/UART drivers in portable application code
- [ ] No additional middleware

### Completion Condition

The minimal application lifecycle builds and runs with the existing validated BSP functionality.

This does not yet close G4.

**STOP and request approval for STEP 05.**

---

## STEP 05 — TCP Server

**Gate:** G4 (Part 2)

### Objective

Receive raw binary data from a connected phone/PC over TCP.

### Files

```text
app/include/ridecast_tcp_server.h
app/src/ridecast_tcp_server.c
```

Update application startup and CMake registration only as required.

### Required Behavior

1. Wait until Wi-Fi is ready.
2. Create a TCP socket.
3. Bind to the configured port.
4. Listen for connections.
5. Accept one client.
6. Receive binary data into a bounded buffer.
7. Detect disconnect or socket failure.
8. Close the accepted socket.
9. Return to listening.
10. Close the listening socket during service shutdown.

### Constraints

- No UART forwarding yet.
- No TLS.
- No HTTP.
- No custom message framing.
- No multi-client service.
- No dynamically allocated buffer per message.
- No assumption that one TCP read equals one application message.

### Data Reception

Start with a configurable 256-byte receive buffer.

The receive loop must handle:

- Positive byte count: valid received bytes
- Zero: peer closed the connection
- Negative result: socket error or an appropriate retryable condition

Use the socket implementation's documented behavior for error classification.

### Test

Connect a PC to the device's SoftAP.

From PowerShell, replace `<DEVICE_IP>` with the observed device address:

```powershell
python -c "import socket; s=socket.create_connection(('<DEVICE_IP>',3333),timeout=5); s.sendall(bytes.fromhex('00 01 52 49 44 45 43 41 53 54 FE FF')); s.close()"
```

For G4, verify that the application receives the complete binary sequence using a test-only observer, controlled instrumentation, or an appropriate receive-test harness.

Do not print sensitive payload data in production logs.

### Validation

- [ ] Server starts only after network readiness
- [ ] TCP client connects
- [ ] Binary data is received correctly
- [ ] Short reads handled correctly
- [ ] Client disconnect detected
- [ ] Client reconnect succeeds
- [ ] Listening socket remains usable
- [ ] Resources close on service shutdown

### Gate G4 Pass Condition

The TCP server receives complete binary input and survives disconnect/reconnect.

**STOP after providing results.**

---

## STEP 06 — TCP-to-UART Bridge

**Gate:** G5

### Objective

Implement the first complete RideCast data path.

### Files

```text
app/include/ridecast_bridge.h
app/src/ridecast_bridge.c
```

Update the TCP server and application lifecycle only where needed.

### Data Flow

```text
TCP recv()
    |
    v
Validate received length
    |
    v
RideCast bridge
    |
    v
BSP UART transmit
    |
    v
Wait/confirm progress as required
    |
    v
Continue until complete
```

### Mandatory Rules

- Forward binary bytes unchanged.
- Do not convert to text.
- Do not append a terminator.
- Do not add framing.
- Do not silently discard bytes.
- Do not claim successful transmission after a partial write.
- Do not add an unnecessary bridge queue.
- Do not call ESP-IDF UART APIs directly.
- Do not modify BSP UART behavior to simplify the bridge.

### Partial UART Write Handling

The implementation must use the actual BSP UART completion semantics.

If the BSP reports a number of bytes transmitted, update the remaining byte count accordingly.

If the BSP uses an asynchronous queue or callback, wait for or otherwise track completion according to its documented contract.

**Never assume a returned success code means all physical UART bytes have finished transmitting without verifying the API contract.**

### Backpressure

The bridge must remain bounded when TCP produces data faster than the UART can transmit it.

Use the simplest safe strategy supported by the existing BSP.

If the UART cannot transmit within the configured deadline:

1. Record the error.
2. Stop accepting further data for the affected session.
3. Release or recover resources according to the service contract.
4. Do not report undelivered bytes as successful.

### End-to-End Test

Connect:

```text
Phone / PC
    |
    | Wi-Fi TCP
    v
ESP32 RideCast
    |
    | UART TX
    v
USB-UART Receiver
    |
    v
PC Binary Capture
```

Use the same 12-byte sequence:

```text
00 01 52 49 44 45 43 41 53 54 FE FF
```

A correct capture must match the bytes and byte count exactly.

### Validation

- [ ] TCP input reaches bridge
- [ ] Bridge uses BSP UART
- [ ] Correct byte count transmitted
- [ ] Byte order preserved
- [ ] No newline or null terminator added
- [ ] Partial writes handled
- [ ] UART errors handled
- [ ] No logs contaminate data UART

### Gate G5 Pass Condition

The exact binary sequence received through TCP is observed on the physical UART.

**STOP after presenting captured data and results.**

---

## STEP 07 — Integration and Robustness Testing

**Gate:** G6

### Objective

Validate that the integrated bridge behaves correctly under normal and failure conditions.

### 7.1 Required Test Matrix

| ID | Test case | Required result |
|---|---|---|
| T-01 | 1 byte | Exact match |
| T-02 | Binary bytes including `00`, `FE`, `FF` | Exact match |
| T-03 | 256 bytes | Exact match |
| T-04 | 257 bytes | Exact match |
| T-05 | 4096 bytes | Exact match |
| T-06 | 65536 bytes | Exact match |
| T-07 | 10 client reconnect cycles | All succeed |
| T-08 | Unexpected disconnect | Server recovers |
| T-09 | Second simultaneous client | One-client policy preserved |
| T-10 | UART failure / timeout | Explicit failure handling |
| T-11 | Wi-Fi interruption | Defined recovery |
| T-12 | Repeated start/stop | Resources released |
| T-13 | Repeated load test | No persistent memory growth |
| T-14 | UART console interference | No unintended bytes |

### 7.2 Test Payload Generation

For deterministic binary tests, the following Python expression generates a repeatable sequence:

```python
payload = bytes(index & 0xFF for index in range(4096))
```

This contains all possible byte values and repeats them.

For 64 KiB:

```python
payload = bytes(index & 0xFF for index in range(65536))
```

Use a test sender and an independent UART capture to compare the full payload.

### 7.3 Test Method

For each binary transfer:

1. Start independent UART capture.
2. Establish the TCP connection.
3. Send the defined payload.
4. Wait for expected UART delivery or a bounded timeout.
5. Stop the capture.
6. Compare actual byte count.
7. Compare the entire byte sequence.
8. Save the outcome and evidence.

Do not validate only through a success message from the device.

For sustained transfers, use a timeout appropriate to the UART baud rate and test overhead. At 115200 baud with 8N1, transmission of 65536 bytes has a theoretical serialization time of approximately 5.7 seconds, excluding software overhead.

### 7.4 Reconnection Test

Repeat ten times:

1. Connect TCP client.
2. Send a known payload.
3. Validate UART output.
4. Close TCP client.
5. Wait for server to return to listening.
6. Reconnect.

Any failed cycle makes the test FAIL.

### 7.5 Error Handling Test

Exercise or simulate:

- Invalid socket state
- Socket closure
- Wi-Fi loss
- UART busy state
- UART transmission timeout
- Unexpected client disconnect

Use controlled test doubles for failures that cannot be safely induced on hardware.

Clearly distinguish host-test results from physical hardware validation.

### 7.6 Resource Monitoring

Record resource usage:

- Before starting the TCP service
- After the first connection
- After repeated connections
- After sustained transfer
- After stopping the service

Prefer existing available diagnostics. Do not introduce a custom monitoring framework.

### Gate G6 Pass Condition

All required test cases pass with evidence.

If a test cannot be performed, mark it BLOCKED or NOT RUN. Do not waive it silently.

**STOP after presenting the test report.**

---

## STEP 08 — Final Review and Phase 1 Sign-Off

**Gate:** G7

### Objective

Confirm the implementation satisfies the Phase 1 plan without architectural drift.

### Functional Review

- [ ] Wi-Fi SoftAP works
- [ ] TCP server works
- [ ] UART works
- [ ] TCP-to-UART bridge works
- [ ] Binary integrity validated
- [ ] Reconnection validated
- [ ] Failure handling validated

### Architecture Review

- [ ] Application uses BSP Wi-Fi APIs
- [ ] Application uses BSP UART APIs
- [ ] No NAL added
- [ ] No duplicate lwIP implementation
- [ ] No unapproved BSP API change
- [ ] No Bluetooth/CAN/USB features added
- [ ] No premature TLS implementation
- [ ] No unnecessary buffer or task allocation
- [ ] No runtime file logger registry
- [ ] No unapproved development CLI extension

### Build Review

- [ ] Clean configuration succeeds
- [ ] Compilation succeeds
- [ ] Linking succeeds
- [ ] Firmware runs on hardware
- [ ] Relevant warnings are reviewed
- [ ] Test artifacts correspond to the tested revision

### Documentation Review

- [ ] README matches actual implemented capabilities
- [ ] Phase 1 gate tracker updated
- [ ] Test evidence recorded
- [ ] Hardware details recorded
- [ ] Known limitations listed
- [ ] Unverified capabilities are not presented as complete

### Final Deliverable

Produce a Phase 1 validation report:

```text
PROJECT:
RideCast

PHASE:
1 - Wi-Fi-to-UART Communication

FIRMWARE REVISION:
<revision>

HARDWARE:
<board and wiring>

BUILD:
PASS / FAIL

UART:
PASS / FAIL

WI-FI:
PASS / FAIL

TCP:
PASS / FAIL

END-TO-END BRIDGE:
PASS / FAIL

ROBUSTNESS:
PASS / FAIL / INCOMPLETE

ARCHITECTURE REVIEW:
PASS / FAIL

OPEN DEFECTS:
<list>

FINAL PHASE STATUS:
PASS / FAIL / BLOCKED
```

### Gate G7 Pass Condition

All required Phase 1 acceptance conditions have passed and the user approves the final report.

Only then may Phase 1 be marked COMPLETE.

---

## 3. How to Work With an AI Coding Assistant

### 3.1 Start of a Session

Use this prompt:

> Read `README.md`, `PHASE_1_PLAN.md`, and `PHASE_1_EXECUTION_GUIDE.md` completely. Follow their architecture and scope restrictions. Determine the current approved step from the gate tracker. Do not modify any files yet. Tell me what you need to inspect and which exact step you will perform.

### 3.2 Execute a Step

Use this prompt:

> Implement only STEP XX from `PHASE_1_EXECUTION_GUIDE.md`. Inspect existing APIs before coding. Do not change unrelated files or add future features. Run the required validation, present the evidence, and stop. Do not proceed to the next step without my approval.

Replace `XX` with the actual approved step number.

### 3.3 Handle a Failure

Use this prompt:

> The current step has failed. Analyze only the reported failure and its direct dependencies. Identify the root cause and propose the smallest safe fix. Do not refactor unrelated code or advance the plan. After applying an approved fix, rerun the failed validation and report the result.

### 3.4 Review a Completed Step

Use this prompt:

> Review the changes against the active step, the locked Phase 1 scope, and the repository conventions. List every modified file, any unapproved change, outstanding validation, and the evidence required to close the gate. Do not implement additional features.

### 3.5 Approve Progression

Use this prompt:

> I approve Gate Gx based on the recorded evidence. Update the gate tracker and proceed only to the preparation or implementation of STEP XX. Maintain the same scope restrictions and stop at the next gate.

Never approve a gate based solely on an AI statement that the code looks correct.

---

## 4. Progress Management

At the start of every coding session:

1. Check the current repository revision.
2. Read the gate tracker.
3. Identify the most recent approved gate.
4. Identify the active step.
5. Verify any outstanding failures.
6. Work only on that step.

The gate tracker in `PHASE_1_PLAN.md` is the single source of truth for implementation progress.

Do not maintain conflicting completion statuses in separate documents.

The AI may propose a tracker update, but only after providing evidence and obtaining approval.

---

## 5. When the AI Must Stop

Stop immediately when:

- A required BSP API cannot be found.
- The actual API differs from an assumed signature.
- The current hardware configuration is unknown.
- A build fails outside the active step's scope.
- A test fails unexpectedly.
- A proposed fix changes public BSP behavior.
- A new third-party dependency appears necessary.
- A new architecture layer is proposed.
- A test requires hardware that is not available.
- The implementation would require an unapproved scope change.

The next response must identify the blocker and the minimum decision needed to continue.

Do not invent temporary success paths.

---

## 6. Completion Policy

Phase 1 must remain **INCOMPLETE** until:

- The full TCP-to-UART path has been tested on real hardware.
- Binary data integrity is verified.
- Failure and reconnection tests have passed.
- Resource management is reviewed.
- Build and test evidence are preserved.
- All gates are explicitly approved.

A successful compilation is one milestone, not completion.

**End of Phase 1 execution guide.**
