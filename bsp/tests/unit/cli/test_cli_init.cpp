/**
 * @file test_cli_init.cpp
 * @brief CppUTest cases for CLI initialization and deinitialization.
 *
 * Test conventions: one behavior per test; explicit boundary values;
 * no ISR calls or concurrent access because the CLI contract excludes them.
 */
#include "support/cli_test_fixture.hpp"

using ridecast_test::kReadBufferCapacity;

TEST_GROUP(CliInitialization)
{
    RIDECAST_CLI_FIXTURE()
};

/** @test CLI-INIT-001: Reject a NULL configuration without dereference. */
TEST(CliInitialization, RejectsNullConfiguration)
{
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, cli_init(NULL));
}

/** @test CLI-INIT-002: Prompt is emitted once on successful initialization. */
TEST(CliInitialization, InitializesResourcesAndWritesPrompt)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    STRCMP_EQUAL("> ", rig.console.transmitted);
    CHECK_EQUAL(1U, rig.console.write_calls);
    CHECK_EQUAL('\0', rig.command_buffer[0]);
    CHECK_EQUAL('\0', rig.output_buffer[0]);
}

/** @test CLI-INIT-003: Already initialized state is reported. */
TEST(CliInitialization, RejectsDuplicateInitialization)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_ALREADY_INIT, rig.initialize());
}

/** @test CLI-INIT-004: A transport write callback is mandatory. */
TEST(CliInitialization, RejectsMissingWriteCallback)
{
    rig.config.transport.write = NULL;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
}

/** @test CLI-INIT-005: A command buffer pointer is mandatory. */
TEST(CliInitialization, RejectsMissingCommandBuffer)
{
    rig.config.command_buffer = NULL;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
}

/** @test CLI-INIT-006: Buffer capacity must reserve a NUL byte. */
TEST(CliInitialization, RejectsCommandBufferSizesZeroAndOne)
{
    rig.config.command_buffer_size = 0U;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
    rig.config.command_buffer_size = 1U;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
}

/** @test CLI-INIT-007: An output buffer pointer is mandatory. */
TEST(CliInitialization, RejectsMissingOutputBuffer)
{
    rig.config.output_buffer = NULL;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
}

/** @test CLI-INIT-008: Output capacity must reserve a NUL byte. */
TEST(CliInitialization, RejectsOutputBufferSizesZeroAndOne)
{
    rig.config.output_buffer_size = 0U;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
    rig.config.output_buffer_size = 1U;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
}

/** @test CLI-INIT-009: Polling requires a valid read buffer. */
TEST(CliInitialization, RejectsReadCallbackWithoutReadBuffer)
{
    rig.config.read_buffer = NULL;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
}

/** @test CLI-INIT-010: Zero read capacity is invalid when read callback exists. */
TEST(CliInitialization, RejectsZeroReadBufferCapacity)
{
    rig.config.read_buffer_size = 0U;
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
}

/** @test CLI-INIT-011: Read callback's int32 return limits its capacity. */
TEST(CliInitialization, RejectsReadBufferCapacityAboveInt32Max)
{
    if(sizeof(size_t) > sizeof(int32_t))
    {
        rig.config.read_buffer_size = static_cast<size_t>(INT32_MAX) + 1U;
        CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
    }
}

/** @test CLI-INIT-012: Push-input mode requires no read callback/buffer. */
TEST(CliInitialization, AllowsPushInputMode)
{
    rig.config.transport.read = NULL;
    rig.config.read_buffer = NULL;
    rig.config.read_buffer_size = 0U;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    STRCMP_EQUAL("> ", rig.console.transmitted);
}

/** @test CLI-INIT-013: Prompt is optional. */
TEST(CliInitialization, InitializesWithoutPrompt)
{
    rig.config.prompt = NULL;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(0U, rig.console.write_calls);
}

/** @test CLI-INIT-014: The same buffer cannot back input and output. */
TEST(CliInitialization, RejectsSameCommandAndOutputBuffer)
{
    rig.config.output_buffer = rig.command_buffer;
    rig.config.output_buffer_size = sizeof(rig.command_buffer);
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
}

/** @test CLI-INIT-015: An RX buffer cannot alias command storage. */
TEST(CliInitialization, RejectsReadBufferAliasingCommandBuffer)
{
    rig.config.read_buffer = reinterpret_cast<uint8_t *>(rig.command_buffer);
    rig.config.read_buffer_size = sizeof(rig.command_buffer);
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
}

/** @test CLI-INIT-016: An RX buffer cannot alias output storage. */
TEST(CliInitialization, RejectsReadBufferAliasingOutputBuffer)
{
    rig.config.read_buffer = reinterpret_cast<uint8_t *>(rig.output_buffer);
    rig.config.read_buffer_size = sizeof(rig.output_buffer);
    CHECK_EQUAL(RET_ERR_STS_INVALID_PARAM, rig.initialize());
}

/** @test CLI-INIT-017: Failed initial prompt write rolls back module state. */
TEST(CliInitialization, PromptFailureAllowsRetry)
{
    rig.console.write_result = RET_ERR_STS_FAIL;
    CHECK_EQUAL(RET_ERR_STS_FAIL, rig.initialize());
    POINTERS_EQUAL(NULL, cli_get_output_buffer());
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, cli_deinit());
    rig.console.write_result = RET_ERR_STS_OK;
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
}

/** @test CLI-INIT-018: Application-owned buffers are cleared on initialization. */
TEST(CliInitialization, InitializesCommandAndOutputStrings)
{
    rig.command_buffer[0] = 'A';
    rig.output_buffer[0] = 'B';
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL('\0', rig.command_buffer[0]);
    CHECK_EQUAL('\0', rig.output_buffer[0]);
}

TEST_GROUP(CliDeinitialization)
{
    RIDECAST_CLI_FIXTURE()
};

/** @test CLI-DEINIT-001: Calling deinit without init is deterministic. */
TEST(CliDeinitialization, RejectsDeinitBeforeInitialization)
{
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, cli_deinit());
}

/** @test CLI-DEINIT-002: Deinitialization clears the CLI state. */
TEST(CliDeinitialization, ClearsStateAndAllowsReinitialization)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, cli_deinit());
    POINTERS_EQUAL(NULL, cli_get_output_buffer());
    CHECK_EQUAL(0U, cli_get_output_buffer_size());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
}

/** @test CLI-DEINIT-003: All registered command references are removed. */
TEST(CliDeinitialization, RemovesRegisteredCommands)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.register_ping());
    CHECK_EQUAL(RET_ERR_STS_OK, cli_deinit());
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.console.clear_output();
    CHECK_EQUAL(RET_ERR_STS_OK, cli_execute("ping"));
    CHECK_TRUE(rig.console.contains("not recognised"));
}

/** @test CLI-DEINIT-004: Deinit does not free or overwrite owned buffers. */
TEST(CliDeinitialization, DoesNotDestroyApplicationOwnedBuffers)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    rig.command_buffer[0] = 'C';
    rig.output_buffer[0] = 'D';
    CHECK_EQUAL(RET_ERR_STS_OK, cli_deinit());
    CHECK_EQUAL('C', rig.command_buffer[0]);
    CHECK_EQUAL('D', rig.output_buffer[0]);
    CHECK_EQUAL(kReadBufferCapacity, sizeof(rig.read_buffer));
}

/** @test CLI-DEINIT-005: Repeated deinit cannot succeed twice. */
TEST(CliDeinitialization, DoubleDeinitReturnsNotInitialized)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, cli_deinit());
    CHECK_EQUAL(RET_ERR_STS_NOT_INITIALIZED, cli_deinit());
}
