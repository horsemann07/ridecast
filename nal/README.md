# Network Abstraction Layer (NAL)

This document gives a high-level overview of the Network Abstraction Layer (NAL) used in this project: what it is, why it exists, how it's designed, and where it fits in your system. It intentionally avoids per-function API documentation — consult the header files for exact prototypes and return codes.

## What is NAL?

NAL (Network Abstraction Layer) is a small, portable C component that encapsulates network transport details behind a consistent interface. It provides both synchronous and asynchronous connection and I/O capabilities over plain TCP and optional TLS-over-TCP transports.

The main purpose of NAL is to decouple higher-level code (application logic, protocol layers) from platform-specific socket or TLS stacks (BSD sockets, lwIP, mbedTLS, platform SDKs). That makes the rest of the codebase easier to port, test, and reason about.


```

/nal
├── nal.h          ← public API
├── nal_core.c     ← scheme-agnostic TCP logic
├── nal_tls.c      ← TLS glue (scheme-specific)
├── nal_crypto.c   ← generic crypto helpers
├── port/
│   ├── esp32/
│   │   └── nal_platform_esp32.c
│   ├── stm32/
│   │   └── nal_platform_lwip.c



```
## Why use NAL?

- Portability: write protocol code once and run it on desktop (POSIX) and embedded platforms (lwIP/ESP-IDF) with minimal changes.
- Transport abstraction: switch or toggle transport modes (plain TCP vs TLS) without changing callers.
- Async convenience: built-in asynchronous worker support lets code react to network events (connected, data received, data sent, disconnected, errors) via callbacks, without every consumer having to implement their own event loop or threading.
- Consistent error reporting: NAL centralizes network-related error/status values so callers can handle failures uniformly.

## Design overview (high-level)

- Two modes of operation:
  - Synchronous: standard connect/send/recv/close semantics for blocking flows or when callers want full control.
  - Asynchronous: NAL can spawn a worker thread per connection that performs connect (if requested) and then monitors the socket for read/write readiness and connection state, delivering events to a caller-provided callback.

- Transport schemes supported:
  - Plain TCP
  - TLS-over-TCP (optional; provided when a TLS backend such as mbedTLS is enabled in the build)

- Threading and concurrency:
  - The async mode uses a single worker thread per connection which performs the I/O loop and runs callbacks in the worker context. Callers should avoid long-blocking logic inside callbacks or dispatch the work to another thread/context if needed.
  - A lightweight synchronization primitive protects async context state (a mutex and a join/exit semaphore). The implementation is CMSIS-RTOS2-centric but a POSIX shim is included for host testing.

- Simplicity-first async design:
  - The current async implementation supports one outstanding queued send per connection (copy-on-send). This keeps the state machine simple and safe across platforms. If you require higher throughput or many queued writes, consider adding a bounded FIFO send queue.

## Portability and integration notes

- Platform bindings:
  - NAL is written in portable C and uses a small set of platform primitives (sockets or lwIP, optional mbedTLS, and CMSIS-RTOS2 primitives for threading). The demo code contains a POSIX-to-CMSIS shim used to run the async path on desktop for testing.

- Build-time options:
  - Features such as TLS and the RTOS integration are controlled by compile-time macros. Review the project build/system configuration to enable or disable those features for your target.

- Threading model expectations:
  - When running in embedded environments using CMSIS/FreeRTOS, ensure the RTOS kernel is initialized before starting NAL async operations.
  - Callbacks execute in the worker thread. If your callback interacts with platform-only APIs that are not reentrant or must be called from a main thread, dispatch accordingly.

## Testing and demo

- The repository includes a small POSIX demo that exercises the async connect and I/O path using a local TCP echo server. This is handy to test behavior on a development machine before deploying to the target hardware.
  - Demo files live in the `demo/` folder (a POSIX CMSIS shim, an echo server, and a demo main that drives an async connect to localhost).

## Common usage patterns (non-API guidance)

- Quick integration pattern:
  1. Initialize your platform network stack (sockets or lwIP) and, if required, TLS resources.
  2. Create or initialize a transport handle that NAL will use.
  3. Choose synchronous or asynchronous operation depending on your application's concurrency model.
  4. For async mode, provide a callback to receive events and ensure the callback returns quickly (or forwards work).

- Error handling:
  - Handle connection errors and timeouts at the call site consistently. Use the centralized error codes where appropriate. For async flows, pay attention to the connect-failed and error events.

## Limitations & future improvements

- Single outstanding async send: the current simple design allows only one queued async send per connection. Consider migrating to a bounded FIFO to support concurrent send requests and better throughput.
- Callbacks in worker context: for safety and to avoid user code blocking the I/O thread, callback dispatching could be moved to a separate dispatcher thread or posted to a thread-safe event queue.
- Non-blocking sockets: converting sockets to non-blocking mode (and handling EAGAIN/EWOULDBLOCK) would improve responsiveness in mixed workloads.

## Where to look next in the codebase

- Implementation: `nal/include/nal_network.c` (contains both sync and async logic).
- Types and public headers: search for `nalHandle_t`, `nalEvent_t`, and related symbols in the `nal` folder to discover the public-facing declarations.
- Demo: `demo/` contains a POSIX shim and a small echo-server-based test harness to run the async code on a desktop.

## Contributing and changes

- If you improve or extend the async behavior (queued sends, offloaded callback dispatcher, non-blocking IO), add tests and update this README with the rationale and a short migration note so future users know why the change was made.

---