# RideCast BSP (Board Support Package)

A portable, SDK-agnostic Board Support Package. The application talks to **one stable API**
(`bspUartXxx`, `bspWifiXxx`, `bspBleXxx`, ...). Each vendor SDK (ESP-IDF, and later STM32 HAL,
Nordic, Zephyr, etc.) is hidden behind a **port layer**.

> Status: UART and Wi-Fi in progress, BLE planned. Built-in CLI/console, logging, OSAL and error system.

---

## 1. Goals

| Goal | How it is achieved |
|------|--------------------|
| Support any SDK | Public headers never include vendor headers. Vendor code lives only in `srcs/port/<sdk>/` |
| App never changes when hardware changes | App includes only `include/*.h` and `include/core/*.h` |
| Consistent API across peripherals | Same handle / config / callback / status / error pattern for every module |
| Testable and debuggable | Built-in logging, CLI and console over a pluggable transport |
| OS independent | All RTOS calls go through OSAL (`bsp_osal.h`) |

---

## 2. Layered Architecture

```
+--------------------------------------------------------------+
|                        APPLICATION                           |
|   (ridecast app: services, state machines, business logic)   |
+------------------------------+-------------------------------+
                               |  includes only public headers
                               v
+--------------------------------------------------------------+
|                       BSP PUBLIC API                         |
|   include/bsp_uart.h  include/bsp_wifi.h  include/bsp_ble.h  |
+------------------------------+-------------------------------+
                               |
        +----------------------+-----------------------+
        v                                              v
+-----------------------------+      +--------------------------------+
|        BSP CORE             |      |        BSP CONFIG              |
|  srcs/core/                 |      |  bsp_config/                   |
|  err_sts, log, osal,        |      |  bsp_config.h  (feature flags) |
|  console, cli               |      |  bsp_cfg_map.* (board pin/     |
|                             |      |   instance mapping)            |
+--------------+--------------+      +----------------+---------------+
               |                                      |
               +------------------+-------------------+
                                  v
+--------------------------------------------------------------+
|                    PORT LAYER (per SDK)                      |
|   srcs/port/esp32x/bsp_uart.c   srcs/port/esp32x/bsp_wifi.c  |
|   srcs/port/<other_sdk>/...                                  |
+------------------------------+-------------------------------+
                               v
+--------------------------------------------------------------+
|              VENDOR SDK / HAL  (ESP-IDF, STM32 HAL, ...)     |
+--------------------------------------------------------------+
```

### Dependency rules (strict)

1. Application -> BSP public headers only. Never include anything from `srcs/`.
2. Public headers never include vendor SDK headers.
3. Port layer may include vendor SDK headers, core headers and public headers.
4. Core never depends on a port. It depends only on OSAL and public headers.
5. Nothing in BSP depends on the application. The app plugs in via callbacks and `user_template/`.

---

## 3. Directory Layout

```
bsp/
├── bsp.cmake                     Build integration (CMake)
├── README.md                     This file
├── README_CONSOLE_ARCHITECTURE.md  Console/CLI deep-dive
│
├── bsp_config/                   Build-time and board-level configuration
│   ├── bsp_config.h              Feature enable/disable flags, sizes, defaults
│   ├── bsp_cfg_map.h/.c          Maps logical instances to physical resources
│   │                             (e.g. BSP_UART_CONSOLE -> UART0, TX pin, RX pin)
│
├── include/                      PUBLIC API (the only headers the app sees)
│   ├── bsp_uart.h
│   ├── bsp_wifi.h
│   └── core/
│       ├── bsp_core.h            Init/deinit, version, common types
│       ├── ret_err_sts.h         ret_err_sts_t error/status codes
│       ├── bsp_log.h             Logging macros
│       ├── bsp_osal.h            OS abstraction (task, mutex, queue, delay...)
│       ├── bsp_os_map.h          Maps OSAL to the chosen RTOS
│       ├── bsp_console.h         Console (line input, output)
│       ├── bsp_console_transport.h  Transport interface for console
│       └── bsp_cli.h             Command line interpreter
│
├── srcs/
│   ├── core/                     SDK-independent implementation
│   │   ├── bsp_err_sts.c         Error-to-string, helpers
│   │   ├── bsp_log.c
│   │   ├── bsp_osal.c
│   │   ├── bsp_console.c
│   │   └── bsp_cli.c
│   └── port/
│       └── esp32x/               ESP-IDF port
│           ├── bsp_uart.c
│           └── bsp_wifi.c
│
└── user_template/                Copy into the APP and customize
    ├── bsp_console_uart_transport.c
    └── bsp_console_uart_transport.h
```

---

## 4. Module Design Pattern

Every peripheral module (UART, Wi-Fi, BLE, later SPI/I2C/GPIO...) follows the **same shape**.
This is what makes the BSP predictable.

### 4.1 Handle-based, opaque instance

```c
bspUartHandle_t* handle;         /* created/initialized by Init, passed to every call */
```

The handle hides vendor state. The app never touches its internals.

### 4.2 Standard API groups

| Group | Purpose | Example (UART) |
|-------|---------|----------------|
| Lifecycle | Init / Deinit / Open / Close | `bspUartInit`, `bspUartDeinit` |
| Data | Read / Write, sync and async | `bspUartWrite`, `bspUartReadAsync` |
| Callback | Register event handlers | `bspUartSetRxCallback`, `bspUartSetTxCallback` |
| Control | Cancel, flush, wait | `bspUartCancelRx`, `bspUartFlushTx`, `bspUartWaitTxDone` |
| Status | Query state | `bspUartGetStatus`, `bspUartIsRxBusy`, `bspUartGetRxCount` |
| Configuration | Get/Set runtime config | `bspUartSetConfig`, `bspUartSetBaudrate` |

### 4.3 Conventions

| Item | Convention |
|------|-----------|
| Function names | `bsp<Module><Action>` in camelCase, e.g. `bspUartSetBaudrate` |
| Types | `bsp<Module><Name>_t`, e.g. `bspUartConfig_t`, `bspUartHandle_t` |
| Return value | Always `ret_err_sts_t` (never raw vendor error codes) |
| Output values | Returned through `[out]` pointer parameters |
| Callbacks | `(event data..., void* userContext)`, may run in ISR context |
| Doxygen | Every public function has `@brief`, `@param[in/out]`, `@retval` |
| Params | `handle` first, then inputs, then outputs |

### 4.4 Error handling

All APIs return `ret_err_sts_t` defined in `bsp_core/bsp_err_sts.h`.
Port code must **translate** SDK errors (e.g. `esp_err_t`) into `ret_err_sts_t`.
Typical codes: `BSP_OK`, `RET_ERR_INVALID_PARAM`, `RET_ERR_NOT_INIT`, `RET_ERR_BUSY`,
`RET_ERR_TIMEOUT`, `RET_ERR_NO_MEM`, `RET_ERR_NOT_SUPPORTED`.
(Adjust to match the real enum.)

---

## 5. Core Services

### 5.1 OSAL (`bsp_osal.h`, `bsp_os_map.h`)
Wraps RTOS primitives: tasks, mutexes, semaphores, queues, timers, delay, tick.
`bsp_os_map.h` selects the backend (FreeRTOS today; Zephyr/bare-metal possible).
Port and core code must use OSAL, never call `xTaskCreate` or similar directly.

### 5.2 Logging (`bsp_log.h`)
Level-based macros (error/warn/info/debug/verbose). Output is routed through the console
or a backend selected in `bsp_config.h`. Levels can be compiled out to save flash.

### 5.3 Console + CLI (`bsp_console.h`, `bsp_cli.h`)
See `README_CONSOLE_ARCHITECTURE.md` for details.

```
   Terminal (PC)
        ^
        | bytes
+-------+--------------------+
|  Console Transport         |  <- user_template/bsp_console_uart_transport.*
|  (UART / USB / BLE / TCP)  |     implements bsp_console_transport.h
+-------+--------------------+
        |
+-------+--------------------+
|  bsp_console  (line edit,  |
|  history, echo)            |
+-------+--------------------+
        |
+-------+--------------------+
|  bsp_cli (command table,   |
|  parse, dispatch)          |
+----------------------------+
```

The **transport is pluggable**: the console does not know it is running over UART.
The app (or `user_template`) supplies a transport that implements the interface
in `bsp_console_transport.h`. This allows the CLI over UART, BLE, TCP or USB with no core change.

### 5.4 Configuration (`bsp_config/`)
- `bsp_config.h`: compile-time switches, e.g. enable UART, WiFi, BLE, console, log level, buffer sizes.
- `bsp_cfg_map.h/.c`: board mapping table. Logical instance IDs to physical resources (port number, pins, default baud).
  Change boards by editing this file only.

---

## 6. Port Layer

Each SDK gets a folder under `srcs/port/<sdk>/` implementing the **same public headers**.

```
include/bsp_uart.h     <- contract (what)
srcs/port/esp32x/bsp_uart.c   <- ESP-IDF implementation (how)
srcs/port/stm32/bsp_uart.c    <- future
```

The build system selects exactly one port folder (see `bsp.cmake`).

### Adding a new SDK port

1. Create `srcs/port/<sdk>/`.
2. Implement every function declared in each public header you want to support.
3. Convert vendor errors to `ret_err_sts_t`.
4. Use OSAL for locking/wait/notify.
5. Return `RET_ERR_NOT_SUPPORTED` for functions the hardware cannot do.
6. Add the port to `bsp.cmake` and a `bsp_os_map.h` entry if the RTOS differs.
7. Do not modify public headers to fit one SDK. If the API is insufficient, extend it in a backward-compatible way.

### Adding a new peripheral (e.g. BLE, SPI)

1. Add `include/bsp_<module>.h` following section 4 (handle, groups, naming, Doxygen).
2. Add feature flag in `bsp_config.h`.
3. Add instance mapping in `bsp_cfg_map.*` if needed.
4. Implement `srcs/port/<sdk>/bsp_<module>.c`.
5. Optionally register CLI commands (see 8).

---

## 7. Application Architecture

The application sits on top of the BSP and should be layered too.

```
+--------------------------------------------------------+
|  APP ENTRY   main / app_main: init BSP, start services |
+--------------------------------------------------------+
|  APP SERVICES                                          |
|   connectivity_service (WiFi/BLE state machine)        |
|   comm_service         (protocol over UART/BLE/WiFi)   |
|   config_service       (settings, NVS)                 |
|   cli_commands         (app-specific CLI commands)     |
+--------------------------------------------------------+
|  APP DRIVERS / ADAPTERS                                |
|   console_uart_transport (from user_template)          |
|   device drivers built on BSP APIs                     |
+--------------------------------------------------------+
|  BSP  (this repository)                                |
+--------------------------------------------------------+
```

### App rules

1. Include only `bsp_*.h` public headers; no vendor SDK calls in app logic.
   (Exception: SDK glue such as `app_main` entry, if unavoidable.)
2. Use **callbacks** for async events; keep ISR-context callbacks tiny. Push events into a queue
   (via OSAL) and handle them in a task.
3. One owner per peripheral handle. Wrap shared handles with a service.
4. Check every `ret_err_sts_t`. Log failures using `bsp_log`.
5. Startup order: `bspCoreInit` -> console transport -> `bspConsoleInit` -> CLI -> peripherals -> app services.
6. Provide app-specific CLI commands in the app, never inside BSP core.

### Typical startup

```c
void app_main(void)
{
    bspCoreInit();                     /* OSAL, log, error system */

    consoleUartTransportInit();        /* from user_template      */
    bspConsoleInit(consoleUartTransport());
    bspCliInit();

    bspUartInit(&uart, &cfg);          /* peripherals             */
    bspWifiInit(&wifi, &wifiCfg);

    appServicesStart();                /* tasks, state machines   */
}
```

(Function names above are illustrative; match your real init APIs.)

---

## 8. CLI Extension

Modules can expose debug commands, e.g.:

```
uart status
uart baud 115200
wifi scan
wifi connect <ssid> <pass>
ble adv start
```

Register commands through `bsp_cli.h`. Keep BSP-level commands generic (hardware
diagnostics). Keep product-specific commands in the app.

---

## 9. Threading and Callback Model

| Item | Rule |
|------|------|
| Callback context | May be ISR or driver task. Do not block, do not allocate, do not log heavily |
| Thread safety | Each module protects its state with an OSAL mutex; APIs are safe to call from tasks, not from ISR unless documented |
| Blocking calls | Only calls with `timeout_ms` (e.g. `bspUartWaitTxDone`) block |
| `userContext` | Opaque pointer given at registration, returned unchanged in the callback |

---

## 10. Guidelines for AI Assistants (Copilot, Claude, ChatGPT, etc.)

Read this section before generating or modifying code in this repository.

### Must do
- Follow the layering in section 2 and the dependency rules.
- Follow naming: functions `bsp<Module><Action>`, types `bsp<Module><Name>_t`.
- Every function returns `ret_err_sts_t`; outputs go through pointer params.
- Validate `handle` and pointer args first; return `RET_ERR_INVALID_PARAM` on NULL.
- Add Doxygen (`@brief`, `@param[in]/[out]`, `@retval`) to every public function.
- Put SDK-specific code **only** under `srcs/port/<sdk>/`.
- Use OSAL for tasks, mutex, queues, delays.
- Use `bsp_log` macros, not `printf` or `ESP_LOGx`, in core and public-facing code.
- Gate optional modules with flags from `bsp_config.h`.
- When adding a module, mirror the API groups in section 4.2.

### Must not do
- Do not include vendor headers (`driver/uart.h`, `esp_wifi.h`, `stm32*_hal.h`) in `include/`.
- Do not include port files from core, or app code from BSP.
- Do not return vendor error types or magic numbers.
- Do not hardcode pins or instance numbers; use `bsp_cfg_map`.
- Do not change existing public signatures without updating all ports and docs.
- Do not put product-specific logic into BSP.

### Where things go

| Task | Location |
|------|----------|
| New public API | `include/bsp_<module>.h` |
| New error code | `bsp_core/bsp_err_sts.h` and `srcs/core/bsp_err_sts.c` |
| ESP32 implementation | `srcs/port/esp32x/bsp_<module>.c` |
| Board pin mapping | `bsp_config/bsp_cfg_map.*` |
| Feature flag / buffer size | `bsp_config/bsp_config.h` |
| New console transport | App side, based on `user_template/` |
| App logic | Outside the `bsp/` folder |

### Prompt template for AI

```
Repository: RideCast BSP (layered, SDK-agnostic, see README.md).
Task: <describe>
Constraints: follow section 2 dependency rules, section 4 naming/API pattern,
return ret_err_sts_t, use OSAL and bsp_log, add Doxygen.
Touch only: <files>
```

---

## 11. Roadmap

| Module | Status |
|--------|--------|
| Core (err, log, osal) | Implemented |
| Console + CLI | Implemented |
| UART (ESP32) | Implemented / in progress |
| Wi-Fi (ESP32) | In progress |
| BLE | Planned |
| GPIO / SPI / I2C | Future |
| Additional SDK ports (STM32, Nordic, Zephyr) | Future |

---

## 12. Quick Glossary

| Term | Meaning |
|------|---------|
| BSP | Board Support Package: hardware abstraction API |
| Port | SDK-specific implementation of the BSP API |
| OSAL | OS Abstraction Layer |
| Transport | Byte-level link the console runs on (UART, USB, BLE, TCP) |
| Handle | Opaque pointer representing one peripheral instance |
