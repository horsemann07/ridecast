# RideCast — Phase 1 Development and Validation Plan

**Document:** `PHASE_1_PLAN.md`  
**Project:** RideCast  
**Phase:** 1 — Basic Wi-Fi-to-UART Communication  
**Status:** NOT STARTED  
**Target:** ESP32 / ESP-IDF 5.5  
**Execution Guide:** `PHASE_1_EXECUTION_GUIDE.md`

---

## 1. Phase 1 Mission

Develop and validate the minimum working RideCast application capable of receiving raw binary data from a phone/PC over Wi-Fi TCP and transmitting the same bytes to an external device through UART.

**Required data path:**

```text
Phone / PC
    |
    v
Wi-Fi SoftAP
    |
    v
TCP Server
    |
    v
RideCast Bridge
    |
    v
BSP UART
    |
    v
Cluster MCU / Test Receiver
```

**Only TCP-to-UART forwarding is required.**

The goal is to validate integration, data integrity, lifecycle behavior, and the portability boundary before implementing the full RideCast feature set.

---

## 2. Non-Negotiable Rules for AI Coding

These instructions apply to every AI agent, code-generation tool, and human implementation session.

### Rule 1 — Follow the Plan

- Implement one numbered execution step at a time.
- Do not skip gates.
- Do not start the next step without explicit authorization.
- Do not interpret optional ideas as requirements.
- Do not create future-phase code during Phase 1.

### Rule 2 — Preserve Existing BSP Interfaces

- Read existing BSP headers and implementations before integration.
- Use verified function signatures and return codes.
- Do not invent or replace BSP APIs.
- Do not redesign UART, Wi-Fi, CLI, or logger modules.
- Do not modify existing BSP behavior without a confirmed integration blocker and explicit approval.
- Keep generic BSP naming; `RideCast` belongs only to the application.

### Rule 3 — Maintain Portability

- No direct `esp_wifi_*` calls in portable application modules.
- No direct ESP-IDF UART driver calls in portable application modules.
- No NAL layer.
- Use BSD sockets for application networking.
- Prefer CMSIS-RTOS2 interfaces for application tasks if the existing port supports the required operations.
- Restrict vendor-specific bootstrap work to the platform entry/integration boundary.

### Rule 4 — Control Memory Usage

- Use fixed-size, explicitly bounded data buffers.
- Avoid unnecessary duplicate buffers.
- Do not introduce dynamic queues, caches, registries, or memory pools without a demonstrated need.
- Do not allocate memory for every received TCP message.
- Maintain bounded memory consumption under sustained traffic.

### Rule 5 — Never Hide Failures

- Never return success when initialization failed.
- Never ignore incomplete UART transmissions.
- Never silently discard received application data.
- Never claim a test passed if it was not executed.
- Never replace a failing test with a weaker test.

### Rule 6 — Produce Reviewable Changes

For each execution step:

1. Show the relevant findings.
2. List files being changed.
3. Implement only the approved step.
4. Run available validation.
5. Record actual results.
6. Stop at the gate boundary.

### Rule 7 — Use Clear Documentation

- Keep code compact but readable.
- Preserve existing code formatting conventions.
- Add comments for non-obvious behavior, ownership, synchronization, and failure handling.
- Use Doxygen for public API contracts.
- Separate C function definitions using:

```c
/*------------------------------------------*/
```

- Do not add repetitive comments for obvious statements.
- Do not invent changelog history.

### Rule 8 — Security Boundaries

- Phase 1 plaintext TCP is development-only.
- Use a controlled local development network.
- Do not expose the TCP service to an untrusted network.
- Do not store hardcoded production credentials.
- Do not log Wi-Fi passwords, keys, or sensitive payload contents.
- Keep development, production-testing, and market-release security behavior distinct.
- Do not introduce an unauthenticated production CLI.

---

## 3. Locked Scope

### 3.1 In Scope

| ID | Feature | Requirement |
|---|---|---|
| R-01 | BSP integration | Existing BSP UART and Wi-Fi APIs |
| R-02 | Wi-Fi | SoftAP mode |
| R-03 | Networking | Single-client IPv4 TCP server |
| R-04 | Bridge | Raw TCP-to-UART forwarding |
| R-05 | Binary safety | Preserve byte values and order |
| R-06 | Buffering | Bounded memory |
| R-07 | Errors | Validate, report, and recover where possible |
| R-08 | Reconnection | Accept clients after disconnect |
| R-09 | Logging | Existing BSP logger |
| R-10 | Portability | No vendor Wi-Fi/UART drivers in portable app |
| R-11 | Testing | Unit and hardware integration validation |
| R-12 | Documentation | Record tests, results, and limitations |

### 3.2 Out of Scope

The AI must not implement:

- Bluetooth BLE or Classic
- CAN or USB routing
- Bidirectional UART-to-TCP forwarding
- TLS or AES-GCM
- Navigation or media message parsing
- Custom application message framing
- Multi-client TCP server support
- Web server, HTTP, MQTT, or WebSocket
- OTA updates
- Database, NVS profile storage, or credential management systems
- Runtime per-file logger registration
- New CLI frameworks or commands
- NAL or generic transport-plugin frameworks
- STM32H7 application implementation
- Unrequested refactoring of BSP components

### 3.3 Change Control

Any scope change requires:

1. A documented reason.
2. Impact on files, architecture, RAM, dependencies, and testing.
3. A plan revision approved by the user.
4. Updated acceptance criteria where necessary.

**An AI suggestion alone is not approval.**

---

## 4. Repository Change Boundaries

### 4.1 Preferred Application Files

The proposed Phase 1 application files are:

```text
app/
  app_config/
    app_config.h
    app_wifi_config.h
    app_uart_config.h
    app_network_config.h

  include/
    ridecast_app.h
    ridecast_wifi_service.h
    ridecast_tcp_server.h
    ridecast_bridge.h

  main/
    app_main.c
    CMakeLists.txt

  src/
    ridecast_app.c
    ridecast_wifi_service.c
    ridecast_tcp_server.c
    ridecast_bridge.c
```

Create only the files necessary for the active execution step.

Do not generate all files as empty placeholders.

### 4.2 Build Integration

Changes to the root CMake configuration or application component registration are permitted only when required for the active step.

Modify `bsp/bsp.cmake` or BSP CMake configuration only to repair verified blockers or integrate required existing BSP components.

Avoid unrelated build-system cleanup.

### 4.3 BSP Source Modifications

Default policy: **read-only**.

If a verified BSP defect blocks Phase 1:

- Record the failing operation and evidence.
- Identify the smallest possible fix.
- Obtain approval.
- Apply the change separately from application feature work.
- Re-run BSP and application validation.

---

## 5. Development Baseline

Before implementation, record these values.

| Item | Required information | Current status |
|---|---|---|
| ESP32 target | Exact SoC/module | UNVERIFIED |
| ESP-IDF version | Installed/checked-out version | UNVERIFIED |
| Board | Board model | UNVERIFIED |
| UART instance | Dedicated cluster UART | UNVERIFIED |
| UART TX pin | Board-assigned GPIO | UNVERIFIED |
| UART RX pin | Board-assigned GPIO | UNVERIFIED |
| UART voltage | Compatible electrical levels | UNVERIFIED |
| UART driver API | Exact BSP functions | UNVERIFIED |
| Wi-Fi driver API | Exact BSP functions | UNVERIFIED |
| TCP port | 3333 proposed | PROPOSED |
| Wi-Fi mode | SoftAP | SELECTED |
| Wi-Fi SSID | RideCast-Dev proposed | PROPOSED |
| UART baud | 115200 proposed | PROPOSED |
| External receiver | USB-UART adapter or cluster MCU | UNVERIFIED |
| Baseline build | Clean configure and compile | UNVERIFIED |

**No hardware-dependent implementation may rely on an unverified board pin or peripheral selection.**

The known earlier BSP CMake nesting/source-path issue must be checked against the current repository, not assumed fixed.

---

## 6. Design Constraints

### 6.1 TCP Service

- IPv4 TCP server.
- One active client.
- Port configurable; initial proposed value 3333.
- No TLS in Phase 1.
- No message parsing or text conversion.
- Socket lifetime owned by the TCP service.
- Close the accepted socket on disconnect or failure.
- Continue accepting clients while the service remains enabled.

### 6.2 UART Service

- Existing BSP UART owns the hardware driver.
- One selected UART for cluster communication.
- Initial proposed format: 115200, 8N1.
- Do not share the cluster UART with an interactive console.
- Handle partial transmission correctly.
- Treat persistent UART failure as an explicit bridge fault.

### 6.3 Data Integrity

Required behavior:

- Every accepted byte must be transmitted unchanged.
- Preserve receive-stream byte order.
- Do not append line endings.
- Do not treat zero bytes as string terminators.
- Do not modify or interpret binary payloads.
- Do not claim delivery until the UART API's documented completion semantics are satisfied.

### 6.4 Backpressure

UART is likely slower than Wi-Fi.

The application must not continuously accept unbounded TCP data when UART cannot transmit it.

The implementation must use a bounded forwarding strategy consistent with the actual BSP UART API.

If UART cannot progress within a configured deadline, report the failure and stop or reset the affected client session according to the documented policy.

Do not silently drop received bytes to maintain apparent throughput.

### 6.5 Application State

Keep application state minimal.

Suggested conceptual states:

```text
STOPPED
INITIALIZING
READY
CLIENT_CONNECTED
FAULT
STOPPING
```

These are conceptual states. Do not create an unnecessary state-machine framework unless the implementation requires it.

### 6.6 Task Ownership

Start with the minimum task count necessary.

Prefer one application TCP worker for accept/receive/forward operations if the BSP UART behavior permits it.

Use additional tasks or queues only when a concrete blocking, concurrency, or lifecycle requirement justifies them.

---

## 7. Phase 1 Execution Gates

**Each gate must be completed in order.**

| Gate | Name | Required result |
|---|---|---|
| G0 | Repository and hardware audit | Existing interfaces, build system, and board details verified |
| G1 | Baseline build | Existing firmware configures and compiles |
| G2 | UART validation | Known bytes transmitted using BSP UART |
| G3 | Wi-Fi validation | SoftAP starts and a client connects |
| G4 | TCP validation | Client connects and bounded binary data is received correctly |
| G5 | End-to-end bridge | TCP bytes reach UART unchanged |
| G6 | Robustness validation | Reconnect, boundary, failure, and sustained-transfer tests pass |
| G7 | Final review | Evidence documented; architecture and scope reviewed |

### G0 — Repository Audit

Pass only when:

- Actual folder structure is known.
- BSP UART and Wi-Fi headers are reviewed.
- API signatures and lifetimes are documented.
- Current CMake components are identified.
- Board and UART pin choices are confirmed.
- Necessary build and test tools are available.

No feature implementation is permitted in G0.

### G1 — Baseline Build

Pass only when:

- The checked-in build workflow is understood.
- Clean CMake configuration succeeds.
- Compilation and linking succeed.
- Any baseline fix is documented.
- No application feature has been added as part of baseline repair.

A failed baseline must not be disguised as a successful application build.

### G2 — UART Validation

Pass only when:

- BSP UART initialization returns success.
- Hardware capture confirms transmitted bytes.
- Known binary payloads are unchanged.
- UART errors and timeouts are observable.
- Test code does not bypass BSP UART.

### G3 — Wi-Fi Validation

Pass only when:

- SoftAP starts through BSP Wi-Fi.
- The configured SSID is visible.
- A client connects and obtains network connectivity.
- The service reports the correct lifecycle state.
- Start/stop behavior is tested where the BSP supports it.

### G4 — TCP Validation

Pass only when:

- A TCP server listens on the configured port.
- One client can connect.
- Exact binary bytes are received in correct order.
- Disconnection is detected.
- The server accepts a later client.

A temporary diagnostic receive path is permitted for this gate. Do not add unnecessary permanent protocol features.

### G5 — End-to-End Bridge

Pass only when:

- TCP input passes through the bridge to the BSP UART.
- An independent receiver captures UART bytes.
- The captured bytes exactly match the transmitted input.
- No additional bytes, delimiters, or logging data appear on the cluster UART.
- Partial UART writes and error paths are handled.

### G6 — Robustness Validation

Required tests:

| Test ID | Test | Pass criterion |
|---|---|---|
| T-01 | Single-byte payload | Exact byte match |
| T-02 | Binary payload with `00` and `FF` | Exact byte match |
| T-03 | 256-byte payload | Exact byte match |
| T-04 | 257-byte payload | Exact byte match |
| T-05 | 4 KiB payload | Exact byte match |
| T-06 | 64 KiB sustained payload | Exact byte match |
| T-07 | Ten sequential reconnects | Each client reconnects and transfers successfully |
| T-08 | Unexpected client disconnect | No crash or resource leak; new client accepted |
| T-09 | Concurrent second client | One-client policy enforced without corrupting active transfer |
| T-10 | UART write timeout/failure | Error reported; no false delivery-success claim |
| T-11 | Wi-Fi interruption | Failure handled and documented recovery behavior observed |
| T-12 | Repeated service startup/shutdown | No crash or unreleased resources in supported lifecycle |
| T-13 | Memory observation | No persistent resource growth across repeated cycles |
| T-14 | No console-data interference | Cluster UART contains only expected bridge bytes |

For large transfers, match full captured byte sequences or cryptographic digests **and byte counts**.

A checksum alone is insufficient if the capture contains unexpected additional bytes.

Timeout values must account for UART baud rate and hardware behavior.

Where fault injection cannot be performed on actual hardware, the test must remain UNVERIFIED rather than be recorded as PASS.

### G7 — Final Review

Pass only when:

- All required gates have documented evidence.
- The application builds and links.
- End-to-end UART capture has been validated.
- Unit and integration test results are recorded.
- No out-of-scope features were introduced.
- Memory ownership and cleanup paths were reviewed.
- Open defects and limitations are documented.
- Source and documentation match actual implementation.

---

## 8. Acceptance Criteria

**Phase 1 acceptance requires all of the following:**

**Functional acceptance**

- Wi-Fi SoftAP is operational.
- TCP server accepts a client.
- TCP-to-UART forwarding works.
- Binary payloads are preserved.
- Reconnection works.

**Quality acceptance**

- No known buffer overflow.
- No unbounded RAM growth.
- No silent payload truncation.
- No BSP bypasses.
- No confirmed reproducible crash in required tests.
- Relevant error paths are exercised.

**Architecture acceptance**

- No NAL added.
- No Bluetooth/CAN/USB implementation added.
- No unnecessary platform-specific APIs in portable application modules.
- No duplicate lwIP stack linked.
- No unapproved BSP API modification.
- No new logging registry introduced.

**Evidence acceptance**

- Successful build log.
- Hardware setup record.
- Test input and capture results.
- Failed tests and fixes documented.
- Remaining limitations explicitly stated.

---

## 9. Test Evidence Template

Create a record for each test execution.

| Field | Value |
|---|---|
| Gate | Gx |
| Test ID | T-xx |
| Date/time | |
| Firmware revision | |
| Board | |
| Input payload | |
| Expected result | |
| Actual result | |
| Test method | |
| Evidence file/log | |
| Status | PASS / FAIL / BLOCKED / NOT RUN |
| Notes | |

**PASS** requires actual execution and matching observed results.

**BLOCKED** means the test could not proceed because a prerequisite is missing.

**NOT RUN** is never equivalent to PASS.

---

## 10. Gate Status Tracker

This is the authoritative Phase 1 progress record.

| Gate | Status | Evidence | Approval |
|---|---|---|---|
| G0 | NOT STARTED | — | — |
| G1 | NOT STARTED | — | — |
| G2 | NOT STARTED | — | — |
| G3 | NOT STARTED | — | — |
| G4 | NOT STARTED | — | — |
| G5 | NOT STARTED | — | — |
| G6 | NOT STARTED | — | — |
| G7 | NOT STARTED | — | — |

Allowed statuses:

- NOT STARTED
- IN PROGRESS
- PASS
- FAIL
- BLOCKED

A gate must not be marked PASS without evidence and user approval.

Never modify earlier gate evidence simply to make the tracker appear complete.

---

## 11. AI Completion Report Format

After every implementation step, the AI must report:

**Step:**  
**Gate:**  
**Files read:**  
**Files changed:**  
**Implementation summary:**  
**Build result:** PASS / FAIL / NOT RUN  
**Test result:** PASS / FAIL / BLOCKED / NOT RUN  
**Evidence:**  
**Known problems:**  
**Scope deviations:** NONE / DETAILS  
**Next allowed step:**

The AI must stop after this report.

The AI must not automatically begin the next step.

---

## 12. Phase Completion Rule

Phase 1 is complete only when G0 through G7 are approved as PASS.

Compilation alone does not establish Phase 1 completion.

A successful TCP connection alone does not establish Phase 1 completion.

Only verified **end-to-end binary delivery to UART**, together with the required robustness and architectural checks, establishes acceptance.

**Final status remains NOT COMPLETE until all gates pass.**
