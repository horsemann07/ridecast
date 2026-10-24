# Ridecast CLI — CppUTest Unit Test Suite

**Scope:** All nine public functions in the user's latest `cli.c`:

1. `cli_init()`
2. `cli_deinit()`
3. `cli_register_command()`
4. `cli_execute()`
5. `cli_process_input()`
6. `cli_process_console()`
7. `cli_get_parameter()`
8. `cli_get_output_buffer()`
9. `cli_get_output_buffer_size()`

There are **104 independently named CppUTest cases**, each with a `CLI-...` test identifier and a Doxygen-style comment explaining its purpose. See [`TEST_MATRIX.md`](TEST_MATRIX.md) or [`TEST_MATRIX.csv`](TEST_MATRIX.csv) for the complete case-by-case traceability list.

## Directory layout

```text
ridecast_cli_cpputest/
├── CMakeLists.txt                  # Host C11/C++11 build and CTest registration
├── include/cli.h                   # Public header aligned with latest cli.c
├── platform/ret_err_sts.h          # Copy of the user's original error enum header
├── src/cli.c                       # Exact, UNMODIFIED latest uploaded cli.c
├── tests/
│   ├── test_main.cpp               # CppUTest runner
│   ├── test_cli_init.cpp           # Initialization/deinit tests
│   ├── test_cli_commands.cpp       # Command registration and execution
│   ├── test_cli_input.cpp          # RX/input handling and console processing
│   ├── test_cli_accessors.cpp      # Parameters and output buffer accessors
│   └── support/cli_test_fixture.hpp # Fixed-capacity transport and callback fakes
├── TEST_MATRIX.md
├── TEST_MATRIX.csv
└── README.md
```

## Important header alignment

The **uploaded `cli.c` was not modified**. The previous `cli.h` still declared `cli_poll()` and included `err_sts.h`; the newly uploaded `cli.c` implements **`cli_process_console()`**. This package updates only the header to declare `cli_process_console()` and include **`ret_err_sts.h`**, as requested.

Apply the same two changes to the Ridecast production header if you are not using the included header. Do not link an old `cli_poll()` declaration against this latest `cli.c`.

## Prerequisites

- C11 compiler and C++11 compiler (GCC/Clang are suitable for host tests)
- CMake 3.16+
- **Official CppUTest** development headers and compiled library, installed separately

### Compile and run with CMake

```sh
cmake -S . -B build -DCPPUTEST_ROOT=/path/to/cpputest/install
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Alternatively, if CppUTest is on standard include/library paths:

```sh
cmake -S . -B build
cmake --build build --parallel
./build/ridecast_cli_tests -v
```

The suite builds **the CLI module as C11** and **the tests as C++11**. The test binary does not require FreeRTOS, CMSIS-RTOS2, a UART, or the logger.

## Compile-time boundary test configuration

This suite intentionally compiles `cli.c` with smaller test-only values:

```c
CLI_MAX_COMMANDS=4
CLI_MAX_OUTPUT_CHUNKS=4
```

That makes capacity and infinite-output prevention easy to exercise, without needing 24 command entries or 256 output chunks. Production may continue to use its existing defaults. If changing these test-only values, update the corresponding constants in `tests/support/cli_test_fixture.hpp`.

## Fake transport behavior

The tests supply a fixed-capacity, application-style `cli_transport_t` implementation that can:

- Capture transmitted bytes and write counts.
- Provide incoming RX data in multiple chunks.
- Return `0` for no data, a negative RX error, or an incorrectly large read length.
- Fail a selected write operation to verify error propagation.
- Verify synchronous copy semantics and NUL termination.

The command callback fake can generate a normal response, three output chunks, empty output, infinite requests for more chunks, and an unterminated output buffer.

No host heap allocation is introduced into **the CLI production code**; the CppUTest framework may allocate memory internally.

## Status types and optional logging

Unit test files include `ret_err_sts.h` and `cli.h`. The supplied platform status header is copied from the user's earlier status definitions, so tests compare against actual `RET_ERR_STS_*` names (not a miniature imitation enum).

**`bsp_log.h` is not required to unit-test this CLI:** `cli.c` never calls the logger. If you explicitly want logging in your tests, the fixture supports a conditional include:

```cpp
#if defined(RIDECAST_CLI_TEST_USE_LOGGER)
extern "C" {
#include "bsp_log.h"
}
#endif
```

Only enable that macro if your full logging header and its dependencies are on the compiler include path. Do not route test assertions through asynchronous logging; let CppUTest report test failures directly.

## MISRA-oriented test conventions

The tests are written **with MISRA-related considerations in mind**:

- Public API parameters, `NULL`, zero lengths, minimum buffer sizes, and invalid states are tested before valid operation.
- Boundary tests check NUL-termination, command length, array limits, signed read error values, and fixed registry capacities.
- Return codes are checked against `ret_err_sts_t` rather than being ignored in assertions.
- Test inputs and transport output buffers have fixed capacity, with explicit bounds checks.
- Each test has a short descriptive name and a requirement-style ID in its comment.
- Real source (`cli.c`) is compiled as C11 rather than compiling it as C++.
- No logger, ISR, task, or UART dependency is needed in the host unit tests.

**This is not a claim of MISRA C:2012 / MISRA C:2023 compliance or certification.** CppUTest tests are written in C++, the production source has not been checked using a licensed MISRA checker, and complete compliance also depends on your project rule set, deviations, static analysis and target environment. These tests are evidence for behavior, not a substitute for MISRA static analysis.

## Validation performed for this package

The execution environment did **not** have the official CppUTest library installed. To validate immediately, the same 104 `TEST_GROUP`/`TEST` bodies were **compiled and executed under a lightweight temporary API-compatible local test harness** (not included in this package):

- GCC strict C11 build of the unmodified CLI source: passed (`-Wall -Wextra -Werror -pedantic`).
- G++ strict C++11 build of all CppUTest-style test files: passed using the local harness.
- **104 test cases executed, 104 passed**, using the local harness.
- AddressSanitizer and UndefinedBehaviorSanitizer test execution: passed using the local harness.
- GNU gcov on `cli.c` in that test run: **100% of 228 instrumented source lines executed**, and **95.13% of 226 branches taken at least once** (the instrumentation reports 100% of branches evaluated).
- CMake build and CTest discovery: passed using the same local harness substitute.

**The tests still need to be run against the official CppUTest installation**, and the source still needs target-specific testing on STM32 or other boards. The coverage figures apply to this host test configuration only.

## Limitations and follow-up cases

The source intentionally supports one CLI owner task only. Therefore, tests do not call CLI APIs concurrently or from ISR context: those calls violate documented preconditions.

`cli_init()` rejects buffers with identical starting addresses, but the existing implementation does **not fully reject partially overlapping buffer ranges** with different starting addresses. This is a source-code limitation that requires a deliberate production change and a regression test; it is not silently fixed in this test package.

The C implementation also assumes command strings supplied to `cli_execute()` and `cli_get_parameter()` are valid NUL-terminated strings. Passing invalid memory or a nonterminated string is outside the documented API preconditions and is not exercised as a normal unit test.

If your project's exact `ret_err_sts.h` has been updated since the shared version, replace `platform/ret_err_sts.h` with your authoritative header before integrating into production CI.
