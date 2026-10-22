# BSP Console Architecture and End-to-End Flows

This document explains how the BSP console stack works from application startup to transport shutdown. It focuses on the runtime behavior of the current implementation and on the public API boundaries that are meant to stay simple for application code.

## 1. Public API contract

The application-facing design should stay small:

- Use `bsp_console.h` as the main startup surface for application code.
- Keep `bsp_log.h` and `bsp_cli.h` available for advanced internal use in implementation files.
- Do not force every application file to include low-level module headers unless it truly needs them.
- Prefer default behavior at init time and change runtime behavior with setters such as `bsp_log_enable()` and `bsp_log_set_level()`.

In other words, the public app contract is:

```c
#include "bsp_console.h"

bspConsoleConfig_t config =
{
    .logTransport = consoleTransport,
    .cliTransport = consoleTransport
};

(void)bsp_console_init(&config);
(void)bsp_log_enable(true);
(void)bsp_log_set_level(BSP_LOG_INFO);
```

The important point is that the application should normally only need the console lifecycle API. Detailed logger or CLI calls remain available where they are actually needed in the implementation.

## 2. Layer view

```text
+------------------------------------------------------------------+
| Application                                                      |
|                                                                  |
|  bsp_console_init() / bsp_console_process() / bsp_console_deinit()|
|  bsp_cli_register_command()                                     |
|  BSP_LOGI()/BSP_LOGE()/BSP_LOG*_ISR                              |
+------------------------------------------------------------------+
                              |
                              v
+------------------------------------------------------------------+
| Console coordinator: bsp_console.c                              |
|                                                                  |
| Initializes logger and optional CLI; owns startup order and       |
| service-call ordering.                                           |
+------------------------------------------------------------------+
                |                                 |
                v                                 v
+-------------------------------+   +-------------------------------+
| Logger: bsp_log.c             |   | CLI: bsp_cli.c                 |
|                               |   |                               |
| filtering, mutex, formatting, |   | RX ring, EmbeddedCLI,          |
| ISR queue, transport writes   |   | command bindings, TX buffer    |
+-------------------------------+   +-------------------------------+
                |                                 |
                +---------------+-----------------+
                                v
+------------------------------------------------------------------+
| Common transport interface: bsp_console_transport.h              |
| write(), read_start(), set_callback(), context                   |
+------------------------------------------------------------------+
                                |
                                v
+------------------------------------------------------------------+
| Concrete adapter: UART, USB CDC, RTT, etc.                      |
+------------------------------------------------------------------+
                                |
                                v
| Physical peripheral / driver                                     |
+------------------------------------------------------------------+
```

The logger and CLI are independent subsystems. The console coordinator connects them to a transport and enforces the processing order.

## 3. Config model and runtime behavior

The current public config structure is intentionally small:

```c
typedef struct
{
    const bspConsoleTransport_t* logTransport;

#if (BSP_CLI_ENABLE == 1U)
    const bspConsoleTransport_t* cliTransport;
#endif
} bspConsoleConfig_t;
```

This means:

- `logTransport` is required because the logger must exist even if it starts disabled.
- `cliTransport` is used only when CLI is compiled in and enabled during initialization.
- log level and enable state are not passed as big config fields; they are controlled at runtime using:

```c
(void)bsp_log_enable(true);
(void)bsp_log_enable(false);
(void)bsp_log_set_level(BSP_LOG_INFO);
```

This is the intended pattern: keep the startup API minimal and let the runtime APIs handle dynamic configuration.

## 4. Transport interface

Every backend used by logger or CLI implements this generic transport API:

```c
typedef struct bspConsoleTransport
{
    bsp_err_sts_t (*write)(void* context,
                           const uint8_t* data,
                           uint16_t len);

    bsp_err_sts_t (*read_start)(void* context,
                                uint8_t* buffer,
                                uint16_t len);

    bsp_err_sts_t (*set_callback)(void* context,
                                  bspConsoleRxCallback_t callback,
                                  void* userCtx);

    void* context;
} bspConsoleTransport_t;
```

`write()` is required by the logger. The CLI additionally needs `read_start()` and `set_callback()` to receive input.

The transport adapter owns the physical UART/USB handle and is responsible for serializing writes when logger and CLI share the same backend.

## 5. Startup flow

Initialize the physical transport first, then create the console.

```c
#include "bsp_console.h"
#include "bsp_console_uart_transport.h"

void app_console_start(void)
{
    bsp_err_sts_t status;
    const bspConsoleTransport_t* transport;
    bspConsoleConfig_t config;

    status = bsp_console_uart_transport_init(BSP_UART_OWNER_CONSOLE);
    if(status != PLT_ERR_STS_OK)
    {
        return;
    }

    transport = bsp_console_uart_transport_get(BSP_UART_OWNER_CONSOLE);
    if(transport == NULL)
    {
        return;
    }

    config.logTransport = transport;
    config.cliTransport = transport;

    status = bsp_console_init(&config);
    if(status != PLT_ERR_STS_OK)
    {
        (void)bsp_console_uart_transport_deinit(BSP_UART_OWNER_CONSOLE);
        return;
    }

    (void)bsp_log_enable(true);
    (void)bsp_log_set_level(BSP_LOG_INFO);
}
```

The internal order is:

```text
Application
  |
  +--> bsp_console_uart_transport_init(owner)
  |
  +--> transport = bsp_console_uart_transport_get(owner)
  |
  +--> bsp_console_init(&config)
          |
          +--> bsp_log_init(logTransport)
          |       +--> create logger mutex
          |       +--> create ISR log queue
          |       +--> save transport
          |
          +--> bsp_log_set_level(BSP_LOG_INFO)
          +--> bsp_log_enable(true)
          |
          +--> bsp_cli_init(..., cliTransport)
                  +--> create CLI state
                  +--> attach RX callback
                  +--> start first receive
```

If any later init step fails, the earlier logger resources are torn down. The physical transport remains owned by the application and is deinitialized by the application.

## 6. Console task and processing order

The application owns the scheduling. Exactly one normal task or main-loop path should call:

```c
void app_console_task(void)
{
    for(;;)
    {
        bsp_console_process();
        osDelay(1U);
    }
}
```

`bsp_console_process()` is sequential and must not be called from ISR context. The actual order in the implementation is:

```text
bsp_console_process()
  |
  +--> bsp_cli_process()    // if CLI was initialized
  |      +--> consume RX bytes
  |      +--> parse command lines
  |      +--> run command handler
  |      +--> flush CLI output
  |
  +--> bsp_log_process_isr()
         +--> flush queued ISR log records
         +--> format and write them in task context
```

This is the important rule:

- CLI runs first on the current console cycle.
- Then the deferred ISR log queue is processed.
- Normal log writes from a command handler happen immediately during that command handler, not later in the queue flush.

## 7. One `bsp_console_process()` call: exact flow for `test_gpio 1`

A command such as `test_gpio 1` follows this path:

```text
User enters: test_gpio 1\r
  |
  v
UART RX callback -> CLI RX ring
  |
  v
bsp_console_process()
  |
  +--> bsp_cli_process()
  |      |
  |      +--> parse command text
  |      +--> call testGpioCommand(1, ["1"])
  |      |      |
  |      |      +--> perform GPIO action
  |      |      +--> return PLT_ERR_STS_OK / error
  |      |      +--> BSP_LOGI()/BSP_LOGE() writes immediately
  |      |      +--> optional bsp_cli_printf() writes response
  |      |
  |      +--> command handler returns
  |      +--> flush CLI TX buffer
  |
  +--> bsp_log_process_isr()
         +--> flush any queued ISR records, if any
```

There are two different logging cases:

### A. Normal log from the handler

```c
status = bspGpioToggle(gpioNumber);

if(status == PLT_ERR_STS_OK)
{
    BSP_LOGI("GPIO operation successful");
}
else
{
    BSP_LOGE("GPIO operation failed: %d", (int)status);
}
```

This log is emitted immediately when the handler calls the logging macro. It is not delayed until the later `bsp_log_process_isr()` call.

### B. ISR log generated while the handler is running

```c
BSP_LOGI_ISR("GPIO interrupt completed");
```

This kind of log is queued in ISR context and flushed later when `bsp_console_process()` reaches the logger flush step. That means the ISR log may appear after the handler has already returned, but still in the same outer console cycle.

The practical order is:

```text
1. CLI receives the command
2. Handler executes GPIO operation
3. Handler emits normal log immediately
4. Handler returns
5. CLI output is flushed
6. ISR log queue is flushed by bsp_log_process_isr()
7. bsp_console_process() returns
```

## 8. CLI input flow

The CLI process path is:

```text
Terminal
  |
  +--> UART interrupt / DMA completion
          |
          +--> bspConsoleUartRxAdapter()
                  |
                  +--> bspCliRxCallback()
                          +--> push bytes into CLI RX ring
                          +--> rearm read operation

Console task
  |
  +--> bsp_console_process()
          |
          +--> bsp_cli_process()
                  +--> EmbeddedCLI parses completed line
                  +--> command handler executes in task context
```

The RX callback only copies bytes and rearms the receive. It does not parse commands or call application logic. That keeps interrupt-time work bounded.

CLI registration is done after the console is initialized:

```c
static void appStatusCommand(uint32_t argc, char* argv[])
{
    (void)argc;
    (void)argv;

    BSP_LOGI("status command requested");
}

static const bspCliCommand_t appStatusCommandDefinition =
{
    .name        = "status",
    .description = "Show application status",
    .handler     = appStatusCommand
};

(void)bsp_cli_register_command(bsp_cli_get_console_handle(),
                               &appStatusCommandDefinition);
```

The command handler runs in the same task that calls `bsp_console_process()`. Long work should be queued to another application task instead of blocking the console task.

## 9. Log flow

Normal logging is intended for task or main-loop context:

```c
BSP_LOGI("Connected to vehicle: %s", vehicleName);
```

The path is:

```text
Application task
  |
  +--> BSP_LOGI(...)
          |
          +--> bsp_log()
                  +--> validate enabled/level
                  +--> take mutex
                  +--> format log line
                  +--> transport->write()
                  +--> unlock mutex
```

Runtime controls are done after init:

```c
(void)bsp_log_enable(false);
(void)bsp_log_enable(true);
(void)bsp_log_set_level(BSP_LOG_DEBUG);
```

ISR logging is separate:

```text
ISR
  |
  +--> BSP_LOGI_ISR(...)
          |
          +--> queue fixed-size record

Console task
  |
  +--> bsp_log_process_isr()
          +--> format queued record
          +--> transport->write()
```

The ISR path does not call the transport directly and does not block. If the ISR queue is full, the message is dropped.

## 10. Use cases

### Use case A: logger only

```c
bspConsoleConfig_t config =
{
    .logTransport = logTransport,
    .cliTransport = NULL
};

(void)bsp_console_init(&config);
```

This is valid when the app only needs diagnostics. The logger still exists and can later be enabled or level-adjusted via runtime APIs.

### Use case B: logger + CLI on same UART

```c
bspConsoleConfig_t config =
{
    .logTransport = consoleTransport,
    .cliTransport = consoleTransport
};

(void)bsp_console_init(&config);
```

The same transport is shared by both subsystems, with the UI and logger writing to the same UART. The transport implementation must serialize concurrent writes.

### Use case C: logger + CLI on different UARTs

```c
(void)bsp_console_uart_transport_init(BSP_UART_OWNER_LOG);
(void)bsp_console_uart_transport_init(BSP_UART_OWNER_CLI);

bspConsoleConfig_t config =
{
    .logTransport = bsp_console_uart_transport_get(BSP_UART_OWNER_LOG),
    .cliTransport = bsp_console_uart_transport_get(BSP_UART_OWNER_CLI)
};

(void)bsp_console_init(&config);
```

This is the cleanest setup when input and logs should not interfere with each other.

### Use case D: CLI compiled out

When `BSP_CLI_ENABLE == 0U`, CLI processing is not compiled into the build. In that case the API does not support a runtime CLI path, and the CLI transport field is not used.

### Use case E: log enabled later

```c
bspConsoleConfig_t config =
{
    .logTransport = logTransport,
    .cliTransport = cliTransport
};

(void)bsp_console_init(&config);
(void)bsp_log_enable(true);
(void)bsp_log_set_level(BSP_LOG_WARN);
```

The logger remains present and holds its resources even while disabled. The runtime setter turns output back on later.

## 11. Shutdown flow

```c
(void)bsp_console_deinit();
(void)bsp_console_uart_transport_deinit(BSP_UART_OWNER_CONSOLE);
```

The actual shutdown sequence is:

```text
bsp_console_deinit()
  |
  +--> stop console processing
  +--> bsp_cli_deinit()  // if CLI was active
  +--> bsp_log_deinit()

Application
  |
  +--> deinitialize the physical transport
```

The CLI is stopped before the logger is torn down. The application owns the underlying transport hardware and must deinitialize it after the console coordinator is shut down.

## 12. Ownership and rules

| Situation | Expected behavior |
| --- | --- |
| `logTransport == NULL` | `bsp_console_init()` returns `PLT_ERR_STS_INVALID_PARAM` |
| CLI initialized without transport | if CLI is compiled in, init fails and logger resources are unwound |
| reinitialize an active console | returns `PLT_ERR_STS_ALREADY_INIT` |
| ISR queue full | ISR log message is dropped |
| deinit before init | returns `PLT_ERR_STS_NOT_INIT` |

Ownership is intentionally split:

- Application owns the task and calls `bsp_console_process()`.
- Transport adapter owns the physical UART/USB/RTT handle and callback translation.
- Logger owns its mutex, formatting, and ISR queue.
- CLI owns its RX ring and command state.
- The application keeps command names valid while commands are registered.

## 13. Recommended integration pattern

For most products, the recommended pattern is:

1. Configure the underlying transport.
2. Initialize the transport object once.
3. Set `logTransport` and, if needed, `cliTransport` in `bspConsoleConfig_t`.
4. Call `bsp_console_init()`.
5. Register command handlers after successful init.
6. Call `bsp_console_process()` from a single task/main loop.
7. Use runtime setters for logging enable/level changes instead of large startup config payloads.
8. Call `bsp_console_deinit()` before closing the physical driver.

## 14. API boundary recommendation

The public boundary should stay simple:

- Application code: include only `bsp_console.h`.
- Internal implementation files: may include `bsp_log.h` or `bsp_cli.h` when they need module-specific access.
- Do not expose every internal detail through the public app API if the app only needs startup and task scheduling.

This keeps the user-facing API clean while still allowing implementation code to access the full stack where needed.

## 15. Implementation note

The logger must not directly depend on CLI-specific structures, and the CLI must not be initialized from ISR context. The console coordinator is the boundary that owns sequencing, while the logger and CLI remain independent subsystems.
