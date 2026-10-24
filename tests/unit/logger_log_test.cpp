/**
 * @file logger_log_test.cpp
 * @brief Host-side CppUTest verification of the logger public API.
 *
 * Build this translation unit together with log.c, CppUTest, and the fake
 * CMSIS-RTOS2 implementation supplied by the unit-test target.  The fake
 * implementation is deliberately injected through the CMSIS symbols rather
 * than by changing production logger code.
 *
 * @note This is C++ test code. MISRA-C:2012 applies to the production C
 *       module; this test boundary uses checked results and bounded copies.
 */
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

// clang-format off
extern "C"
{
#include "bsp_log.h"
}

namespace
{

struct CapturedWrite
{
    char data[512];
    size_t length;
    unsigned calls;
};

CapturedWrite g_write;
uint64_t g_utc;
bool g_clock_ok;
int32_t g_offset;
bool g_initialized;

/** @brief Capture transport output using a bounded copy. */
ret_err_sts_t capture_write(const uint8_t* data, size_t length, void*)
{
    g_write.length = (length < sizeof(g_write.data)) ? length : sizeof(g_write.data) - 1U;
    memcpy(g_write.data, data, g_write.length);
    g_write.data[g_write.length] = '\0';
    ++g_write.calls;
    return RET_ERR_STS_OK;
}

/** @brief Return a deterministic test UTC timestamp. */
bool fake_clock(uint64_t* value, void*)
{
    if(!g_clock_ok)
    {
        return false;
    }
    *value = g_utc;
    return true;
}

/* The test target provides these helpers around its CMSIS queue fakes. */
extern "C" void logger_test_reset_rtos(void);
extern "C" void* logger_test_task_queue(void);
extern "C" void* logger_test_isr_queue(void);
extern "C" void* logger_test_ready_semaphore(void);
extern "C" void* logger_test_control_flags(void);
extern "C" void* logger_test_format_mutex(void);
extern "C" void* logger_test_worker_thread(void);
extern "C" void logger_test_run_worker_once(bool isr_first);
extern "C" void logger_test_enqueue_task(const char* text, log_level_t level);
extern "C" void
logger_test_enqueue_isr(log_isr_value_type_t type, log_isr_value_t value, const char* text);

/** @brief Build a valid application-owned logger configuration. */
log_task_config_t make_config()
{
    log_task_config_t c{};
    c.worker_thread          = (osThreadId_t)logger_test_worker_thread();
    c.task_queue             = (osMessageQueueId_t)logger_test_task_queue();
    c.isr_queue              = (osMessageQueueId_t)logger_test_isr_queue();
    c.ready_sem              = (osSemaphoreId_t)logger_test_ready_semaphore();
    c.control_flags          = (osEventFlagsId_t)logger_test_control_flags();
    c.format_mutex           = (osMutexId_t)logger_test_format_mutex();
    c.initial_level          = LOG_LEVEL_DEBUG;
    c.write                  = capture_write;
    c.get_utc_seconds        = fake_clock;
    c.utc_offset_minutes     = 330;
    c.shutdown_timeout_ticks = 10U;
    return c;
}

void init_logger(log_format_config_t format = {})
{
    log_task_config_t c = make_config();
    c.format            = format;
    CHECK_EQUAL(RET_ERR_STS_OK, log_init(&c));
    g_initialized = true;
    CHECK_TRUE(log_enable(true) == RET_ERR_STS_OK);
}

/** @brief Process one task record and compare captured transport output. */
void expect_output(const char* expected)
{
    logger_test_run_worker_once(false);
    STRCMP_EQUAL(expected, g_write.data);
}

} // namespace

TEST_GROUP(LoggerLog)
{
    /** @brief Reset fake RTOS state and deterministic callback state. */
    void setup() override
    {
        logger_test_reset_rtos();
        memset(&g_write, 0, sizeof(g_write));
        g_utc         = 0U;
        g_clock_ok    = true;
        g_offset      = 330;
        g_initialized = false;
    }

    /** @brief Deinitialize the logger after each initialized test. */
    void teardown() override
    {
        if(g_initialized)
        {
            CHECK_EQUAL(RET_ERR_STS_OK, log_deinit());
        }
    }
};

/** @brief Verify null and incomplete configurations are rejected. */
TEST(LoggerLog, RejectsNullAndInvalidInitialization)
{
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, log_init(nullptr));

    log_task_config_t c = make_config();
    c.write             = nullptr;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, log_init(&c));

    c            = make_config();
    c.task_queue = nullptr;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, log_init(&c));
}

/** @brief Verify initial state and duplicate initialization handling. */
TEST(LoggerLog, InitializesDefaultsAndRejectsSecondInitialization)
{
    log_task_config_t c = make_config();
    CHECK_EQUAL(RET_ERR_STS_OK, log_init(&c));
    g_initialized = true;
    CHECK_EQUAL(RET_ERR_STS_ALREADY_INIT, log_init(&c));
    CHECK_FALSE(log_is_enabled());
    CHECK_EQUAL(LOG_LEVEL_DEBUG, log_get_level());
}

/** @brief Verify enable state and severity range validation. */
TEST(LoggerLog, EnableDisableAndLevelValidation)
{
    init_logger();
    CHECK_TRUE(log_is_enabled());
    CHECK_EQUAL(RET_ERR_STS_OK, log_enable(false));
    CHECK_FALSE(log_is_enabled());
    CHECK_EQUAL(RET_ERR_STS_OK, log_enable(true));
    CHECK_EQUAL(RET_ERR_STS_OK, log_set_level(LOG_LEVEL_ERROR));
    CHECK_EQUAL(LOG_LEVEL_ERROR, log_get_level());
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, log_set_level((log_level_t)99));
}

/** @brief Verify disabled and over-threshold records are not queued. */
TEST(LoggerLog, FiltersDisabledAndAboveThresholdMessages)
{
    init_logger();
    log_set_level(LOG_LEVEL_WARN);
    log_printf(LOG_LEVEL_INFO, "app.c", 4U, "f", "%s", "filtered");
    CHECK_EQUAL(0U, g_write.calls);
    log_enable(false);
    log_printf(LOG_LEVEL_ERROR, "app.c", 5U, "f", "%s", "disabled");
    CHECK_EQUAL(0U, g_write.calls);
}

/** @brief Verify complete prefix composition and sequence numbering. */
TEST(LoggerLog, FormatsAllPrefixFieldsAndTerminatesWithCrLf)
{
    log_format_config_t f{ true, true, true, true, LOG_TIME_NONE };
    init_logger(f);
    log_printf(LOG_LEVEL_ERROR, "/src/app.c", 42U, "run", "hello %d", 7);
    expect_output("*000001 [E] app.c:42 run() hello 7\r\n");
    log_printf(LOG_LEVEL_ERROR, "app.c", 43U, "run", "next");
    expect_output("*000002 [E] app.c:43 run() next\r\n");
}

/** @brief Verify UTC, local offset, and unavailable-clock formatting. */
TEST(LoggerLog, FormatsUtcAndLocalTimestampsIncludingClockFailure)
{
    log_format_config_t f{ false, false, false, false, LOG_TIME_UTC };
    init_logger(f);
    g_utc = 45296U; // 12:34:56 UTC
    log_printf(LOG_LEVEL_INFO, "x", 1U, nullptr, "utc");
    expect_output("[12:34:56 UTC] utc\r\n");

    f.time_mode = LOG_TIME_LOCAL_24H;
    CHECK_EQUAL(RET_ERR_STS_OK, log_set_format(&f));
    log_printf(LOG_LEVEL_INFO, "x", 1U, nullptr, "local");
    expect_output("[18:04:56 +05:30] local\r\n");

    g_clock_ok = false;
    log_printf(LOG_LEVEL_INFO, "x", 1U, nullptr, "unknown");
    expect_output("[TIME?] unknown\r\n");
}

/** @brief Verify every supported typed ISR value is rendered correctly. */
TEST(LoggerLog, SupportsEveryIsrValueRepresentation)
{
    log_format_config_t f{};
    init_logger(f);
    struct Case
    {
        log_isr_value_type_t type;
        log_isr_value_t value;
        const char* suffix;
    };
    Case cases[] = { { LOG_ISR_VALUE_U8, { .u8 = 8U }, ": 8" },
                     { LOG_ISR_VALUE_U16, { .u16 = 16U }, ": 16" },
                     { LOG_ISR_VALUE_U32, { .u32 = 32U }, ": 32" },
                     { LOG_ISR_VALUE_U64, { .u64 = 64U }, ": 64" },
                     { LOG_ISR_VALUE_I8, { .i8 = -8 }, ": -8" },
                     { LOG_ISR_VALUE_I16, { .i16 = -16 }, ": -16" },
                     { LOG_ISR_VALUE_I32, { .i32 = -32 }, ": -32" },
                     { LOG_ISR_VALUE_I64, { .i64 = -64 }, ": -64" },
                     { LOG_ISR_VALUE_HEX8, { .u8 = 0xABU }, ": 0xAB" },
                     { LOG_ISR_VALUE_HEX16, { .u16 = 0xABCDU }, ": 0xABCD" },
                     { LOG_ISR_VALUE_HEX32, { .u32 = 0xABCDEF01U }, ": 0xABCDEF01" },
                     { LOG_ISR_VALUE_HEX64, { .u64 = UINT64_C(0x0123456789ABCDEF) }, ": 0x0123456789ABCDEF" },
                     { LOG_ISR_VALUE_BOOL, { .boolean = true }, ": true" },
                     { LOG_ISR_VALUE_CHAR, { .character = 'Z' }, ": Z" } };
    for(const Case& c : cases)
    {
        logger_test_enqueue_isr(c.type, c.value, "value");
        logger_test_run_worker_once(true);
        CHECK_TRUE(strstr(g_write.data, c.suffix) != nullptr);
    }
}

/** @brief Verify format getters/setters and null-argument handling. */
TEST(LoggerLog, SetAndGetFormatValidatePointersAndBusyState)
{
    log_format_config_t f{};
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, log_get_format(&f));
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, log_set_format(&f));
    init_logger();
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, log_get_format(nullptr));
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, log_set_format(nullptr));
    f.level = true;
    CHECK_EQUAL(RET_ERR_STS_OK, log_set_format(&f));
    log_format_config_t actual{};
    CHECK_EQUAL(RET_ERR_STS_OK, log_get_format(&actual));
    CHECK_TRUE(actual.level);
}

/** @brief Verify queue pressure and bounded task output formatting. */
TEST(LoggerLog, DropsFullQueueAndTruncatesOversizedTaskMessages)
{
    init_logger();
    for(unsigned i = 0U; i < 20U; ++i)
    {
        logger_test_enqueue_task("queue", LOG_LEVEL_INFO);
    }
    logger_test_run_worker_once(false);
    CHECK_TRUE(g_write.calls <= 20U);
    char oversized[512];
    memset(oversized, 'x', sizeof(oversized) - 1U);
    oversized[sizeof(oversized) - 1U] = '\0';
    log_printf(LOG_LEVEL_INFO, "x", 1U, nullptr, "%s", oversized);
    logger_test_run_worker_once(false);
    CHECK_TRUE(g_write.length < 320U);
    CHECK_TRUE(g_write.data[g_write.length - 2U] == '\r');
    CHECK_TRUE(g_write.data[g_write.length - 1U] == '\n');
}

/** @brief Verify shutdown succeeds once and rejects repeated shutdown. */
TEST(LoggerLog, DeinitializeIsIdempotenceGuardedAndRejectsWorkerContext)
{
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, log_deinit());
    init_logger();
    CHECK_EQUAL(RET_ERR_STS_OK, log_deinit());
    g_initialized = false;
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, log_deinit());
}

/**
 * @brief Execute all registered logger CppUTest cases.
 *
 * The application or external test harness owns the process entry point and
 * calls this function directly.
 *
 * @param[in] argc Number of command-line arguments supplied by the caller.
 * @param[in] argv Command-line arguments supplied by the caller.
 * @return CppUTest process status; zero indicates success.
 */
int logger_main_start(int argc, char** argv)
{
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
