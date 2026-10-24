/**
 * @file test_cli_input.cpp
 * @brief Input state-machine, bounds, and transport read tests.
 *
 * All test inputs are finite; the production input API is task-context only.
 */
#include "support/cli_test_fixture.hpp"

TEST_GROUP(CliInput)
{
    RIDECAST_CLI_FIXTURE()
};

/** @test CLI-INPUT-001: Processing data before initialization fails. */
TEST(CliInput, RejectsInputBeforeInitialization)
{
    static const uint8_t input[] = { 'a' };
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, cli_process_input(input, sizeof(input)));
}

/** @test CLI-INPUT-002: NULL pointer with nonzero length is invalid. */
TEST(CliInput, RejectsNullInputWhenLengthIsNonzero)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, cli_process_input(NULL, 1U));
}

/** @test CLI-INPUT-003: A NULL pointer with zero length is a no-op. */
TEST(CliInput, AcceptsNullInputWhenLengthIsZero)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_process_input(NULL, 0U));
    CHECK_EQUAL(0U, rig.console.write_calls);
}

/** @test CLI-INPUT-004: Command is accumulated across separate RX calls. */
TEST(CliInput, AccumulatesPartialInput)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("pi"));
    CHECK_EQUAL(0U, rig.command_spy.calls);
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("ng\r"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
    CHECK_TRUE(rig.console.contains("pong"));
}

/** @test CLI-INPUT-005: A carriage return dispatches one command. */
TEST(CliInput, ExecutesOnCarriageReturn)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("ping\r"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-INPUT-006: A line feed dispatches one command. */
TEST(CliInput, ExecutesOnLineFeed)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("ping\n"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-INPUT-007: CRLF is one terminator, including across RX chunks. */
TEST(CliInput, SuppressesSplitCrLfPair)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("ping\r"));
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("\n"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
    STRCMP_EQUAL("pong\r\n> ", rig.console.transmitted);
}

/** @test CLI-INPUT-008: LFCR is one terminator, including across RX chunks. */
TEST(CliInput, SuppressesSplitLfCrPair)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("ping\n"));
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("\r"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
    STRCMP_EQUAL("pong\r\n> ", rig.console.transmitted);
}

/** @test CLI-INPUT-009: Two full lines in a single frame execute twice. */
TEST(CliInput, ProcessesMultipleCommandsInOneInputFrame)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("ping\nping\n"));
    CHECK_EQUAL(2U, rig.command_spy.calls);
}

/** @test CLI-INPUT-010: Backspace removes exactly one input character. */
TEST(CliInput, HandlesBackspace)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("pigg\b\bng\n"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-INPUT-011: ASCII DEL has the same editing effect as backspace. */
TEST(CliInput, HandlesAsciiDelete)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("pigg\x7f\x7f" "ng\n"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-INPUT-012: Backspace at index zero does not underflow. */
TEST(CliInput, IgnoresBackspaceAtStartOfLine)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("\b\x7f" "ping\n"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-INPUT-013: Echo echoes printable bytes and erases one byte. */
TEST(CliInput, EchoesPrintableCharactersAndBackspace)
{
    rig.config.echo_input = true;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("pinX\bg\r"));
    CHECK_TRUE(rig.console.contains("pinX\b \bg\r\n"));
    CHECK_TRUE(rig.console.contains("pong\r\n"));
}

/** @test CLI-INPUT-014: Disabled echo never sends typed characters or EOL. */
TEST(CliInput, DoesNotEchoWhenDisabled)
{
    rig.config.echo_input = false;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("ping\r"));
    STRCMP_EQUAL("pong\r\n> ", rig.console.transmitted);
}

/** @test CLI-INPUT-015: Unsupported controls do not enter the command buffer. */
TEST(CliInput, IgnoresUnsupportedControlBytes)
{
    static const uint8_t bytes[] = { 'p', 0x01U, 'i', 0x1BU, 'n', 'g', '\n' };
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, cli_process_input(bytes, sizeof(bytes)));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-INPUT-016: Exactly capacity - 1 printable bytes fit. */
TEST(CliInput, AcceptsCommandAtMaximumPermittedLength)
{
    rig.command.name = "abcdefghijklmnopq"; /* 17 characters. */
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("abcdefghijklmnopq\n"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-INPUT-017: Any additional printable byte discards whole line. */
TEST(CliInput, RejectsOversizedLineWithoutRunningTruncatedPrefix)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_BUFFER_OVERFLOW,
                rig.feed("ping               more text\n"));
    CHECK_EQUAL(0U, rig.command_spy.calls);
    CHECK_TRUE(rig.console.contains("Input too long"));
}

/** @test CLI-INPUT-018: Oversized-line state is retained between reads. */
TEST(CliInput, RecoversAfterDiscardingOverlongLine)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("this_command_is_far_too_long"));
    CHECK_EQUAL(RET_ERR_STS_BUFFER_OVERFLOW, rig.feed("\n"));
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("ping\n"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-INPUT-019: Input processing propagates transport errors. */
TEST(CliInput, PropagatesEchoWriteFailure)
{
    rig.config.echo_input = true;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.clear_output();
    rig.console.write_result = RET_ERR_STS_FAIL;
    CHECK_EQUAL(RET_ERR_STS_FAIL, rig.feed("a"));
}

/** @test CLI-INPUT-020: The first error wins when processing multiple bytes. */
TEST(CliInput, KeepsFirstFailureAcrossInputFrame)
{
    rig.config.echo_input = true;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.clear_output();
    rig.console.fail_on_write_call = 1U;
    CHECK_EQUAL(RET_ERR_STS_FAIL, rig.feed("ab"));
    CHECK_EQUAL(2U, rig.console.write_calls);
}

/** @test CLI-INPUT-021: Prompt output failure must be returned to the caller. */
TEST(CliInput, PropagatesPromptFailureAfterInput)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.clear_output();
    rig.console.fail_on_write_call = 1U;
    CHECK_EQUAL(RET_ERR_STS_FAIL, rig.feed("\n"));
}

/** @test CLI-INPUT-022: Next line remains valid after command callback error. */
TEST(CliInput, ClearsInputStateWhenCommandExecutionFails)
{
    rig.command_spy.behavior = ridecast_test::kMoreWithoutOutput;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_INVALID_STATE, rig.feed("ping\n"));
    rig.command_spy.behavior = ridecast_test::kSingleOutput;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.feed("ping\n"));
    CHECK_EQUAL(2U, rig.command_spy.calls);
}

TEST_GROUP(CliConsoleProcessing)
{
    RIDECAST_CLI_FIXTURE()
};

/** @test CLI-CONSOLE-001: Console processing requires initialization. */
TEST(CliConsoleProcessing, RejectsBeforeInitialization)
{
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, cli_process_console());
}

/** @test CLI-CONSOLE-002: Push-only mode cannot poll a missing read callback. */
TEST(CliConsoleProcessing, RejectsMissingReadCallback)
{
    rig.config.transport.read = NULL;
    rig.config.read_buffer = NULL;
    rig.config.read_buffer_size = 0U;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_INVALID_STATE, cli_process_console());
}

/** @test CLI-CONSOLE-003: Zero received bytes are a successful no-op. */
TEST(CliConsoleProcessing, ReturnsOkWhenReadHasNoData)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_process_console());
    CHECK_EQUAL(0U, rig.console.write_calls);
    CHECK_EQUAL(1U, rig.console.read_calls);
}

/** @test CLI-CONSOLE-004: Negative transport read result maps to RX_FAILED. */
TEST(CliConsoleProcessing, ConvertsNegativeReadToRxFailed)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.force_read_result = true;
    rig.console.forced_read_result = -5;
    CHECK_EQUAL(RET_ERR_STS_RX_FAILED, cli_process_console());
}

/** @test CLI-CONSOLE-005: Oversized reported RX length is rejected. */
TEST(CliConsoleProcessing, RejectsReportedLengthGreaterThanReadBuffer)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.force_read_result = true;
    rig.console.forced_read_result = static_cast<int32_t>(sizeof(rig.read_buffer) + 1U);
    CHECK_EQUAL(RET_ERR_STS_INVALID_LENGTH, cli_process_console());
}

/** @test CLI-CONSOLE-006: A valid RX frame dispatches the command. */
TEST(CliConsoleProcessing, ReadsAndProcessesCommand)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_TRUE(rig.console.queue_input("ping\r\n"));
    CHECK_EQUAL(RET_ERR_STS_OK, cli_process_console());
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-CONSOLE-007: RX frames larger than read buffer are accumulated. */
TEST(CliConsoleProcessing, ReassemblesCommandAcrossTransportReads)
{
    rig.config.read_buffer_size = 2U;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_TRUE(rig.console.queue_input("ping\n"));
    CHECK_EQUAL(RET_ERR_STS_OK, cli_process_console());
    CHECK_EQUAL(RET_ERR_STS_OK, cli_process_console());
    CHECK_EQUAL(0U, rig.command_spy.calls);
    CHECK_EQUAL(RET_ERR_STS_OK, cli_process_console());
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-CONSOLE-008: Processing errors propagate through the console API. */
TEST(CliConsoleProcessing, PropagatesReceivedCommandFailure)
{
    rig.command_spy.behavior = ridecast_test::kMoreWithoutOutput;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_TRUE(rig.console.queue_input("ping\r"));
    CHECK_EQUAL(RET_ERR_STS_INVALID_STATE, cli_process_console());
}
