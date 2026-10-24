/**
 * @file test_cli_accessors.cpp
 * @brief Parameter extraction and application-owned output buffer APIs.
 */
#include "support/cli_test_fixture.hpp"

TEST_GROUP(CliParameters)
{
    RIDECAST_CLI_FIXTURE()
};

/** @test CLI-PARAM-001: Parameters use one-based indexing. */
TEST(CliParameters, ExtractsFirstParameter)
{
    size_t length = 99U;
    const char *result = cli_get_parameter("set level debug", 1U, &length);
    CHECK_TRUE(result != NULL);
    CHECK_EQUAL(5U, length);
    CHECK_EQUAL(0, std::memcmp(result, "level", length));
}

/** @test CLI-PARAM-002: Accessing second and subsequent parameters works. */
TEST(CliParameters, ExtractsSecondParameter)
{
    size_t length = 99U;
    const char *result = cli_get_parameter("set level debug", 2U, &length);
    CHECK_TRUE(result != NULL);
    CHECK_EQUAL(5U, length);
    CHECK_EQUAL(0, std::memcmp(result, "debug", length));
}

/** @test CLI-PARAM-003: Leading/trailing spaces and tabs are skipped. */
TEST(CliParameters, IgnoresWhitespaceBetweenTokens)
{
    size_t length = 0U;
    const char *result = cli_get_parameter(" \tset \tlevel\t debug  ", 2U, &length);
    CHECK_TRUE(result != NULL);
    CHECK_EQUAL(5U, length);
    CHECK_EQUAL(0, std::memcmp(result, "debug", length));
}

/** @test CLI-PARAM-004: A NULL command pointer yields NULL and zero length. */
TEST(CliParameters, RejectsNullCommandLine)
{
    size_t length = 99U;
    POINTERS_EQUAL(NULL, cli_get_parameter(NULL, 1U, &length));
    CHECK_EQUAL(0U, length);
}

/** @test CLI-PARAM-005: A NULL length pointer returns NULL. */
TEST(CliParameters, RejectsNullLengthPointer)
{
    POINTERS_EQUAL(NULL, cli_get_parameter("set value", 1U, NULL));
}

/** @test CLI-PARAM-006: Index zero is invalid. */
TEST(CliParameters, RejectsParameterIndexZero)
{
    size_t length = 99U;
    POINTERS_EQUAL(NULL, cli_get_parameter("set value", 0U, &length));
    CHECK_EQUAL(0U, length);
}

/** @test CLI-PARAM-007: Missing parameter returns NULL and zero length. */
TEST(CliParameters, RejectsParameterBeyondAvailableCount)
{
    size_t length = 99U;
    POINTERS_EQUAL(NULL, cli_get_parameter("set only", 2U, &length));
    CHECK_EQUAL(0U, length);
}

/** @test CLI-PARAM-008: A parameter cannot be read from a command-only line. */
TEST(CliParameters, RejectsCommandWithNoArguments)
{
    size_t length = 99U;
    POINTERS_EQUAL(NULL, cli_get_parameter("help", 1U, &length));
    CHECK_EQUAL(0U, length);
}

/** @test CLI-PARAM-009: Empty and whitespace-only lines return NULL. */
TEST(CliParameters, RejectsEmptyOrWhitespaceOnlyInput)
{
    size_t length = 99U;
    POINTERS_EQUAL(NULL, cli_get_parameter("", 1U, &length));
    CHECK_EQUAL(0U, length);
    POINTERS_EQUAL(NULL, cli_get_parameter("   \t  ", 1U, &length));
    CHECK_EQUAL(0U, length);
}

/** @test CLI-PARAM-010: Original command string is not altered. */
TEST(CliParameters, PreservesInputString)
{
    char line[] = "set speed 100";
    size_t length = 0U;
    const char *result = cli_get_parameter(line, 2U, &length);
    CHECK_TRUE(result != NULL);
    CHECK_EQUAL(3U, length);
    STRCMP_EQUAL("set speed 100", line);
}

/** @test CLI-PARAM-011: Returned value is a substring, not NUL-terminated. */
TEST(CliParameters, ReturnsPointerIntoOriginalCommandLine)
{
    const char line[] = "set level debug";
    size_t length = 0U;
    const char *result = cli_get_parameter(line, 1U, &length);
    POINTERS_EQUAL(&line[4], result);
    CHECK_EQUAL(5U, length);
    CHECK_EQUAL(' ', result[length]);
}

TEST_GROUP(CliOutputAccessors)
{
    RIDECAST_CLI_FIXTURE()
};

/** @test CLI-BUFFER-001: Buffer pointer is NULL before initialization. */
TEST(CliOutputAccessors, PointerIsNullBeforeInit)
{
    POINTERS_EQUAL(NULL, cli_get_output_buffer());
}

/** @test CLI-BUFFER-002: Buffer capacity is zero before initialization. */
TEST(CliOutputAccessors, SizeIsZeroBeforeInit)
{
    CHECK_EQUAL(0U, cli_get_output_buffer_size());
}

/** @test CLI-BUFFER-003: API exposes the exact application-owned buffer. */
TEST(CliOutputAccessors, PointerEqualsConfiguredBuffer)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    POINTERS_EQUAL(rig.output_buffer, cli_get_output_buffer());
}

/** @test CLI-BUFFER-004: Reported capacity includes NUL terminator. */
TEST(CliOutputAccessors, SizeEqualsConfiguredCapacity)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(sizeof(rig.output_buffer), cli_get_output_buffer_size());
}

/** @test CLI-BUFFER-005: Getter results reset after deinitialization. */
TEST(CliOutputAccessors, AccessorsResetAfterDeinit)
{
    CHECK_EQUAL(RET_ERR_STS_OK, rig.initialize());
    CHECK_EQUAL(RET_ERR_STS_OK, cli_deinit());
    POINTERS_EQUAL(NULL, cli_get_output_buffer());
    CHECK_EQUAL(0U, cli_get_output_buffer_size());
}

/** @test CLI-PARAM-012: No available parameters when skipping multiple indexes. */
TEST(CliParameters, RejectsHighIndexWhenNoParametersExist)
{
    size_t length = 99U;
    POINTERS_EQUAL(NULL, cli_get_parameter("set", 3U, &length));
    CHECK_EQUAL(0U, length);
}
