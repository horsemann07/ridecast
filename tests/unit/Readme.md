# Logger CppUTest suite

`logger_log_test.cpp` is a host-side suite for `bsp/include/logger/log.c`.
It intentionally tests the public logger contract through CMSIS-RTOS2 symbols.

The unit-test target must provide a small CMSIS fake with these helper
functions (the production logger is not modified):

```c
void logger_test_reset_rtos(void);
void *logger_test_task_queue(void);
void *logger_test_isr_queue(void);
void *logger_test_ready_semaphore(void);
void *logger_test_control_flags(void);
void *logger_test_format_mutex(void);
void *logger_test_worker_thread(void);
void logger_test_run_worker_once(bool isr_first);
void logger_test_enqueue_task(const char *text, log_level_t level);
void logger_test_enqueue_isr(log_isr_value_type_t type,
                             log_isr_value_t value,
                             const char *text);
```

The fake should implement the CMSIS queue, semaphore, mutex, event-flag,
thread-flag, kernel-state, and thread-join calls used by `log.c`. It should
also expose controllable failure results so the remaining negative branches
can be covered: invalid kernel state, wrong queue element size/capacity,
non-empty queues, mutex failure, flag failure, semaphore failure, shutdown
timeout, and thread-join failure.

Typical host compilation is:

```text
c++ -std=c++20 -I<cpputest>/include -Ibsp/include -Iexternal/os/cmsis_esp/Include \
    tests/unit/logger_log_test.cpp bsp/include/logger/log.c <cmsis_fake>.c \
    -lCppUTest -lCppUTestExt
```

The suite covers initialization and validation, runtime control, level
filtering, task and ISR submission, all typed ISR values, prefix/timestamp
formatting, truncation/CRLF safety, queue-full behavior, and deinitialization.

## What each test actually verifies

| Test | Observable behavior under test |
| --- | --- |
| `RejectsNullAndInvalidInitialization` | `log_init()` rejects null and incomplete configurations. |
| `InitializesDefaultsAndRejectsSecondInitialization` | Initialization state, initial level, and duplicate-init protection. |
| `EnableDisableAndLevelValidation` | Runtime enable state and valid/invalid severity levels. |
| `FiltersDisabledAndAboveThresholdMessages` | Filtered records do not reach the transport callback. |
| `FormatsAllPrefixFieldsAndTerminatesWithCrLf` | Sequence number, severity, basename/line, function, message, and CRLF. |
| `FormatsUtcAndLocalTimestampsIncludingClockFailure` | UTC time, fixed local offset, and `[TIME?]` fallback. |
| `SupportsEveryIsrValueRepresentation` | Formatting of every unsigned, signed, hexadecimal, Boolean, and character ISR type. |
| `SetAndGetFormatValidatePointersAndBusyState` | Format API pointer checks and round-trip state. |
| `DropsFullQueueAndTruncatesOversizedTaskMessages` | Queue pressure and bounded output termination. |
| `DeinitializeIsIdempotenceGuardedAndRejectsWorkerContext` | Successful shutdown and not-initialized repeat shutdown. |

The tests do not prove interrupt timing, atomic lock-free guarantees, real
CMSIS scheduling, or hardware transport behavior. Those require integration,
target, or stress tests. The CMSIS fake must additionally inject each
documented RTOS failure result to cover every `log_init()` and `log_deinit()`
error branch.
