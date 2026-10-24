/**
 * @file cli_test_fixture.hpp
 * @brief Fixed-capacity host fakes for the Ridecast CLI public API tests.
 *
 * The code under test is built as C11. Test-side C++ is intentionally kept
 * simple, with explicit bounds checking and no production RTOS dependencies.
 * CppUTest itself is outside the scope of MISRA C compliance analysis.
 */
#ifndef RIDECAST_CLI_TEST_FIXTURE_HPP
#define RIDECAST_CLI_TEST_FIXTURE_HPP

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ret_err_sts.h"
#include "cli.h"
}

/* Optional: only needed when tests explicitly exercise the application logger.
 * No logger is called by the CLI, so this remains disabled by default.
 */
#if defined(RIDECAST_CLI_TEST_USE_LOGGER)
extern "C"
{
#include "bsp_log.h"
}
#endif

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <climits>

namespace ridecast_test
{

static const size_t kCommandBufferCapacity = 18U;
static const size_t kOutputBufferCapacity = 64U;
static const size_t kReadBufferCapacity = 32U;
static const size_t kCapturedOutputCapacity = 8192U;
/* CMake compiles cli.c with these smaller, test-only limits. */
static const unsigned int kTestMaxCommands = 4U;
static const unsigned int kTestMaxChunks = 4U;

struct ConsoleSpy
{
    char transmitted[kCapturedOutputCapacity];
    size_t transmitted_length;
    unsigned int write_calls;
    unsigned int read_calls;
    ret_err_sts_t write_result;
    ret_err_sts_t write_failure_result;
    unsigned int fail_on_write_call;
    bool force_read_result;
    int32_t forced_read_result;
    uint8_t pending_input[128U];
    size_t pending_length;
    size_t pending_offset;

    void reset(void)
    {
        (void)std::memset(this, 0, sizeof(*this));
        write_result = RET_ERR_STS_OK;
        write_failure_result = RET_ERR_STS_FAIL;
    }

    void clear_output(void)
    {
        transmitted_length = 0U;
        transmitted[0] = '\0';
        write_calls = 0U;
    }

    bool contains(const char *text) const
    {
        return (std::strstr(transmitted, text) != NULL);
    }

    bool queue_input(const char *text)
    {
        size_t length;
        if(text == NULL)
        {
            return false;
        }
        length = std::strlen(text);
        if(length > sizeof(pending_input))
        {
            return false;
        }
        if(length != 0U)
        {
            (void)std::memcpy(pending_input, text, length);
        }
        pending_length = length;
        pending_offset = 0U;
        return true;
    }
};

/* @brief Callback simulating a synchronous console transmission. */
static ret_err_sts_t fake_write(const uint8_t *data, size_t length, void *context)
{
    ConsoleSpy *spy = static_cast<ConsoleSpy *>(context);
    if(spy == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }
    ++spy->write_calls;
    if((spy->fail_on_write_call != 0U) &&
       (spy->write_calls == spy->fail_on_write_call))
    {
        return spy->write_failure_result;
    }
    if(spy->write_result != RET_ERR_STS_OK)
    {
        return spy->write_result;
    }
    if((data == NULL) || (length >= (sizeof(spy->transmitted) - spy->transmitted_length)))
    {
        return RET_ERR_STS_BUFFER_OVERFLOW;
    }
    if(length != 0U)
    {
        (void)std::memcpy(&spy->transmitted[spy->transmitted_length], data, length);
    }
    spy->transmitted_length += length;
    spy->transmitted[spy->transmitted_length] = '\0';
    return RET_ERR_STS_OK;
}

/* @brief Callback simulating a UART RX stream, including malformed lengths. */
static int32_t fake_read(uint8_t *data, size_t capacity, void *context)
{
    ConsoleSpy *spy = static_cast<ConsoleSpy *>(context);
    size_t available;
    size_t to_read;
    if(spy == NULL)
    {
        return -1;
    }
    ++spy->read_calls;
    if(spy->force_read_result)
    {
        return spy->forced_read_result;
    }
    if(spy->pending_offset >= spy->pending_length)
    {
        return 0;
    }
    available = spy->pending_length - spy->pending_offset;
    to_read = (available < capacity) ? available : capacity;
    if((to_read > static_cast<size_t>(INT32_MAX)) || (data == NULL))
    {
        return -1;
    }
    if(to_read != 0U)
    {
        (void)std::memcpy(data, &spy->pending_input[spy->pending_offset], to_read);
    }
    spy->pending_offset += to_read;
    return static_cast<int32_t>(to_read);
}

enum CommandBehavior
{
    kSingleOutput,
    kThreeChunks,
    kAlwaysMore,
    kMoreWithoutOutput,
    kNoOutput,
    kUnterminatedOutput
};

struct CommandSpy
{
    CommandBehavior behavior;
    unsigned int calls;
    char last_command[128U];
    size_t last_capacity;

    void reset(void)
    {
        (void)std::memset(this, 0, sizeof(*this));
        behavior = kSingleOutput;
    }
};

/* @brief The command callback used to verify the command execution contract. */
static bool fake_command(char *output,
                         size_t capacity,
                         const char *command_line,
                         void *context)
{
    CommandSpy *spy = static_cast<CommandSpy *>(context);
    if((spy == NULL) || (output == NULL) || (command_line == NULL))
    {
        return false;
    }

    ++spy->calls;
    spy->last_capacity = capacity;
    (void)std::snprintf(spy->last_command, sizeof(spy->last_command), "%s", command_line);

    switch(spy->behavior)
    {
        case kSingleOutput:
            (void)std::snprintf(output, capacity, "pong\r\n");
            return false;

        case kThreeChunks:
            (void)std::snprintf(output, capacity, "part%u\r\n", spy->calls);
            return (spy->calls < 3U);

        case kAlwaysMore:
            (void)std::snprintf(output, capacity, "x");
            return true;

        case kMoreWithoutOutput:
            return true;

        case kNoOutput:
            return false;

        case kUnterminatedOutput:
            if(capacity != 0U)
            {
                (void)std::memset(output, 'X', capacity);
            }
            return false;

        default:
            return false;
    }
}

struct CliTestRig
{
    ConsoleSpy console;
    CommandSpy command_spy;
    char command_buffer[kCommandBufferCapacity];
    char output_buffer[kOutputBufferCapacity];
    uint8_t read_buffer[kReadBufferCapacity];
    cli_config_t config;
    cli_command_t command;

    void reset(void)
    {
        console.reset();
        command_spy.reset();
        (void)std::memset(command_buffer, 0, sizeof(command_buffer));
        (void)std::memset(output_buffer, 0, sizeof(output_buffer));
        (void)std::memset(read_buffer, 0, sizeof(read_buffer));
        (void)std::memset(&config, 0, sizeof(config));
        (void)std::memset(&command, 0, sizeof(command));

        config.transport.read = fake_read;
        config.transport.write = fake_write;
        config.transport.context = &console;
        config.command_buffer = command_buffer;
        config.command_buffer_size = sizeof(command_buffer);
        config.output_buffer = output_buffer;
        config.output_buffer_size = sizeof(output_buffer);
        config.read_buffer = read_buffer;
        config.read_buffer_size = sizeof(read_buffer);
        config.echo_input = false;
        config.prompt = "> ";

        command.name = "ping";
        command.help = "ping: Check responsiveness\r\n";
        command.callback = fake_command;
        command.expected_parameters = 0;
        command.context = &command_spy;
    }

    ret_err_sts_t initialize(void)
    {
        return cli_init(&config);
    }

    ret_err_sts_t register_ping(void)
    {
        return cli_register_command(&command);
    }

    ret_err_sts_t feed(const char *text)
    {
        return cli_process_input(reinterpret_cast<const uint8_t *>(text), std::strlen(text));
    }
};

} /* namespace ridecast_test */

/* Keep global CLI reset deterministic between independent test cases. */
#define RIDECAST_CLI_FIXTURE()                  \
    ridecast_test::CliTestRig rig;              \
    void setup(void)                            \
    {                                           \
        (void)cli_deinit();                     \
        rig.reset();                            \
    }                                           \
    void teardown(void)                         \
    {                                           \
        (void)cli_deinit();                     \
    }

#endif /* RIDECAST_CLI_TEST_FIXTURE_HPP */
