/**
 * @file test_cli_commands.cpp
 * @brief Command registration and command execution boundary tests.
 */
#include "support/cli_test_fixture.hpp"

using ridecast_test::kTestMaxCommands;
using ridecast_test::kTestMaxChunks;
using ridecast_test::kSingleOutput;
using ridecast_test::kThreeChunks;
using ridecast_test::kAlwaysMore;
using ridecast_test::kMoreWithoutOutput;
using ridecast_test::kNoOutput;
using ridecast_test::kUnterminatedOutput;

TEST_GROUP(CliRegistration)
{
    RIDECAST_CLI_FIXTURE()
};

/** @test CLI-REG-001: Commands cannot be registered before initialization. */
TEST(CliRegistration, RejectsRegistrationBeforeInit)
{
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, rig.register_ping());
}

/** @test CLI-REG-002: NULL descriptor shall be rejected. */
TEST(CliRegistration, RejectsNullCommand)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, cli_register_command(NULL));
}

/** @test CLI-REG-003: Empty or NULL command name shall be rejected. */
TEST(CliRegistration, RejectsMissingCommandName)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command.name = NULL;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.register_ping());
    rig.command.name = "";
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.register_ping());
}

/** @test CLI-REG-004: Command description and callback are required. */
TEST(CliRegistration, RejectsNullHelpOrCallback)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command.help = NULL;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.register_ping());
    rig.command.help = "ping: help\r\n";
    rig.command.callback = NULL;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.register_ping());
}

/** @test CLI-REG-005: Command names cannot contain whitespace or control characters. */
TEST(CliRegistration, RejectsInvalidCommandNameCharacters)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command.name = "has space";
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.register_ping());
    rig.command.name = "has\ttab";
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.register_ping());
    rig.command.name = "line\nfeed";
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.register_ping());
    rig.command.name = "nonascii\x7f";
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.register_ping());
}

/** @test CLI-REG-006: The built-in help name is reserved. */
TEST(CliRegistration, RejectsReservedHelpCommand)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command.name = "help";
    CHECK_EQUAL(RET_ERR_STS_ALREADY_EXISTS, rig.register_ping());
}

/** @test CLI-REG-007: Duplicate names are rejected independent of pointer identity. */
TEST(CliRegistration, RejectsDuplicateName)
{
    cli_command_t another;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    another = rig.command;
    CHECK_EQUAL(RET_ERR_STS_ALREADY_EXISTS, cli_register_command(&another));
}

/** @test CLI-REG-008: -1 is accepted for variable parameters; -2 is invalid. */
TEST(CliRegistration, ValidatesExpectedParameterCount)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command.expected_parameters = -2;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.register_ping());
    rig.command.expected_parameters = -1;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
}

/** @test CLI-REG-009: The fixed table rejects entries past its capacity. */
TEST(CliRegistration, RegistryCapacityIsBounded)
{
    cli_command_t entries[5U];
    static const char * const names[5U] = { "a", "b", "c", "d", "e" };
    size_t index;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());

    for(index = 0U; index < kTestMaxCommands + 1U; ++index)
    {
        entries[index] = rig.command;
        entries[index].name = names[index];
        if(index < kTestMaxCommands)
        {
            CHECK_EQUAL(RET_ERR_STS_OK, cli_register_command(&entries[index]));
        }
        else
        {
            CHECK_EQUAL(RET_ERR_STS_BUFFER_FULL, cli_register_command(&entries[index]));
        }
    }
}

/** @test CLI-REG-010: Different case constitutes a different command name. */
TEST(CliRegistration, CommandNamesAreCaseSensitive)
{
    cli_command_t uppercase;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    uppercase = rig.command;
    uppercase.name = "PING";
    CHECK_EQUAL(RET_ERR_STS_OK, cli_register_command(&uppercase));
}

TEST_GROUP(CliExecution)
{
    RIDECAST_CLI_FIXTURE()
};

/** @test CLI-EXEC-001: Command execution requires initialization. */
TEST(CliExecution, RejectsExecutionBeforeInitialization)
{
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, cli_execute("ping"));
}

/** @test CLI-EXEC-002: NULL input is invalid. */
TEST(CliExecution, RejectsNullCommandLine)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, cli_execute(NULL));
}

/** @test CLI-EXEC-003: Blank input does not produce transport traffic. */
TEST(CliExecution, IgnoresBlankAndWhitespaceOnlyLines)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute(""));
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("  \t  "));
    CHECK_EQUAL(0U, rig.console.write_calls);
}

/** @test CLI-EXEC-004: Executing a registered command calls its callback. */
TEST(CliExecution, ExecutesCommandAndWritesOutput)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("ping"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
    STRCMP_EQUAL("pong\r\n", rig.console.transmitted);
    CHECK_EQUAL(ridecast_test::kOutputBufferCapacity, rig.command_spy.last_capacity);
}

/** @test CLI-EXEC-005: Leading whitespace is accepted and command line forwarded. */
TEST(CliExecution, SkipsLeadingWhitespace)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute(" \tping"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
    STRCMP_EQUAL("ping", rig.command_spy.last_command);
}

/** @test CLI-EXEC-006: Command matching shall use complete tokens. */
TEST(CliExecution, PrefixIsNotAcceptedAsCommand)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("pingpong"));
    CHECK_EQUAL(0U, rig.command_spy.calls);
    CHECK_TRUE(rig.console.contains("not recognised"));
}

/** @test CLI-EXEC-007: Unknown commands report a message without processing error. */
TEST(CliExecution, UnknownCommandReportsFriendlyError)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("invalid"));
    CHECK_TRUE(rig.console.contains("not recognised"));
}

/** @test CLI-EXEC-008: Fixed parameter count prevents callback invocation. */
TEST(CliExecution, RejectsWrongParameterCount)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("ping extra"));
    CHECK_EQUAL(0U, rig.command_spy.calls);
    CHECK_TRUE(rig.console.contains("Incorrect command parameter"));
}

/** @test CLI-EXEC-009: Exact parameter counts handle multiple separators. */
TEST(CliExecution, AcceptsCorrectParameterCount)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command.expected_parameters = 2;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("ping a \t b   "));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-EXEC-010: Variable argument count bypasses validation. */
TEST(CliExecution, AllowsVariableParameterCount)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command.expected_parameters = -1;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("ping one two three"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-EXEC-011: Built-in help enumerates registered commands. */
TEST(CliExecution, HelpIncludesBuiltInAndRegisteredCommands)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("help"));
    CHECK_TRUE(rig.console.contains("help: List available commands"));
    CHECK_TRUE(rig.console.contains("ping: Check responsiveness"));
}

/** @test CLI-EXEC-012: Help refuses extra parameters. */
TEST(CliExecution, HelpRejectsArguments)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("help extra"));
    CHECK_TRUE(rig.console.contains("Incorrect command parameter"));
}

/** @test CLI-EXEC-013: Help emits a newline for a help string lacking one. */
TEST(CliExecution, HelpAddsNewlineWhenMissing)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command.help = "ping: no newline";
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("help"));
    CHECK_TRUE(rig.console.contains("ping: no newline\r\n"));
}

/** @test CLI-EXEC-014: Empty help strings are handled without underflow. */
TEST(CliExecution, HelpHandlesEmptyHelpDescription)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command.help = "";
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("help"));
    CHECK_TRUE(rig.console.contains("help: List available commands\r\n\r\n"));
}

/** @test CLI-EXEC-015: A command can produce multiple output chunks. */
TEST(CliExecution, SupportsMultiChunkOutput)
{
    rig.command_spy.behavior = kThreeChunks;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("ping"));
    CHECK_EQUAL(3U, rig.command_spy.calls);
    STRCMP_EQUAL("part1\r\npart2\r\npart3\r\n", rig.console.transmitted);
}

/** @test CLI-EXEC-016: Repeated callback requests with empty output are rejected. */
TEST(CliExecution, RejectsNoProgressWhenMoreOutputRequested)
{
    rig.command_spy.behavior = kMoreWithoutOutput;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_INVALID_STATE, cli_execute("ping"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
    CHECK_EQUAL(0U, rig.console.write_calls);
}

/** @test CLI-EXEC-017: Infinite more-output callbacks are bounded. */
TEST(CliExecution, LimitsMaximumOutputChunks)
{
    rig.command_spy.behavior = kAlwaysMore;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_INVALID_STATE, cli_execute("ping"));
    CHECK_EQUAL(kTestMaxChunks, rig.command_spy.calls);
    CHECK_EQUAL(kTestMaxChunks, rig.console.write_calls);
}

/** @test CLI-EXEC-018: A callback can finish without writing data. */
TEST(CliExecution, PermitsSuccessfulEmptyOutput)
{
    rig.command_spy.behavior = kNoOutput;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("ping"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
    CHECK_EQUAL(0U, rig.console.write_calls);
}

/** @test CLI-EXEC-019: The final output byte is forced to NUL. */
TEST(CliExecution, SafelyTerminatesCallbackOutput)
{
    rig.command_spy.behavior = kUnterminatedOutput;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("ping"));
    CHECK_EQUAL(ridecast_test::kOutputBufferCapacity - 1U, rig.console.transmitted_length);
    CHECK_EQUAL('\0', rig.output_buffer[sizeof(rig.output_buffer) - 1U]);
}

/** @test CLI-EXEC-020: Command write failures are propagated. */
TEST(CliExecution, PropagatesWriteFailure)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    rig.console.write_result = RET_ERR_STS_FAIL;
    CHECK_EQUAL(RET_ERR_STS_FAIL, cli_execute("ping"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
}

/** @test CLI-EXEC-021: Help transport failures propagate immediately. */
TEST(CliExecution, PropagatesHelpWriteFailure)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    rig.console.fail_on_write_call = 2U;
    CHECK_EQUAL(RET_ERR_STS_FAIL, cli_execute("help"));
    CHECK_EQUAL(2U, rig.console.write_calls);
}

/** @test CLI-EXEC-022: Each registration stores a callback-specific context. */
TEST(CliExecution, ForwardsCommandContext)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("ping"));
    CHECK_EQUAL(1U, rig.command_spy.calls);
    STRCMP_EQUAL("ping", rig.command_spy.last_command);
}

/** @test CLI-EXEC-023: Failure writing the help introduction is returned. */
TEST(CliExecution, PropagatesHelpIntroductionFailure)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.clear_output();
    rig.console.fail_on_write_call = 1U;
    CHECK_EQUAL(RET_ERR_STS_FAIL, cli_execute("help"));
    CHECK_EQUAL(1U, rig.console.write_calls);
}

/** @test CLI-EXEC-024: Failure writing an appended newline is returned. */
TEST(CliExecution, PropagatesHelpNewlineFailure)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command.help = "ping: no final newline";
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    rig.console.clear_output();
    rig.console.fail_on_write_call = 3U;
    CHECK_EQUAL(RET_ERR_STS_FAIL, cli_execute("help"));
    CHECK_EQUAL(3U, rig.console.write_calls);
}
