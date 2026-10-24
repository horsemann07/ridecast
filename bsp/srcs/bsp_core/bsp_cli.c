/* ============================================================================
 * File        : cli.c
 * Description : BSP command-line interface implementation
 * Author      : Jha Raghav
 * Date        : 2026-10-24
 * ============================================================================
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Description:
 *   Implements the reusable, platform-independent command-line
 *   interface (CLI) module, including:
 *
 *   - CLI initialization and deinitialization
 *   - Command registration and command lookup
 *   - Command-line input processing
 *   - Command parsing and argument extraction
 *   - Registered command callback execution
 *   - Command parameter validation
 *   - CLI input buffer management
 *   - CLI command execution status handling
 *   - Application-provided input and output interfaces
 *   - Polling-based CLI processing support
 *
 * Integration:
 *   This module is part of the reusable BSP CLI.
 *
 *   The application must provide the required configuration
 *   and buffer resources through cli_config_t before calling
 *   cli_init().
 *
 *   Application commands are registered using
 *   cli_register_command().
 *
 *   Commands may be executed through cli_execute() or
 *   processed through the configured CLI input mechanism.
 *
 * Requirements:
 *   - C11 compatible compiler.
 *   - BSP CLI public API definitions.
 *   - Platform-independent error status definitions.
 *   - Application-provided CLI configuration and buffers.
 *   - Application-provided command callback implementations.
 *
 * Notes:
 *   - The CLI core must remain independent of ESP-IDF and STM32 HAL.
 *   - CLI command processing must follow its documented
 *     execution-context and reentrancy restrictions.
 *   - Command callbacks must validate application-specific
 *     arguments and execution conditions.
 *   - Authentication and authorization must be enforced before
 *     executing protected commands when CLI security is enabled.
 *   - The CLI core must not directly depend on hardware UART APIs.
 *
 * ============================================================================
 * Change Log:
 *
 * Date          Author          Description
 * ------------  --------------  ----------------------------------------------
 * 2026-10-24    Jha Raghav      Standardized CLI implementation documentation
 * ============================================================================
 */
#include "bsp_cli.h"

#include <limits.h>
#include <string.h>
/* ============================================================================
 * CLI Compile-Time Configuration
 * ============================================================================ */

/**
 * @brief Maximum number of commands supported by the CLI.
 *
 * @details
 * Defines the maximum number of command descriptors that can be
 * registered with the CLI module.
 *
 * The command registry uses a statically allocated array of pointers.
 * Increasing this value increases the registry's RAM consumption.
 *
 * @note Applications may override this macro at compile time.
 *
 * @warning The configured value must be greater than zero.
 */
#ifndef configCLI_MAX_COMMANDS
    #define configCLI_MAX_COMMANDS (24U)
#endif

/* -------------------------------------------------------------------------- */

/**
 * @brief Maximum number of CLI output chunks.
 *
 * @details
 * Defines the maximum number of output chunks that may be processed
 * during a bounded CLI output operation.
 *
 * This configuration prevents unrestricted output processing when
 * command callbacks generate multiple output chunks.
 *
 * @note Applications may override this macro at compile time.
 *
 * @warning The configured value must be greater than zero.
 */
#ifndef configCLI_MAX_OUTPUT_CHUNKS
    #define configCLI_MAX_OUTPUT_CHUNKS (256U)
#endif

/* ============================================================================
 * CLI Compile-Time Configuration Validation
 * ============================================================================ */

/**
 * @brief Validate the maximum command registry capacity.
 *
 * @details
 * At least one command slot must be available for command registration.
 */
#if (configCLI_MAX_COMMANDS < 1U)
    #error "configCLI_MAX_COMMANDS must be at least 1"
#endif

/* -------------------------------------------------------------------------- */

/**
 * @brief Validate the maximum output chunk count.
 *
 * @details
 * At least one output chunk must be supported by the CLI.
 */
#if (configCLI_MAX_OUTPUT_CHUNKS < 1U)
    #error "configCLI_MAX_OUTPUT_CHUNKS must be at least 1"
#endif

/* ============================================================================
 * CLI ASCII Character Definitions
 * ============================================================================ */

/**
 * @brief ASCII backspace character.
 *
 * Used to remove the previous character from the CLI input line.
 */
#define CLI_ASCII_BS  (0x08U)

/**
 * @brief ASCII delete character.
 *
 * Used to recognize the delete key during CLI input processing.
 */
#define CLI_ASCII_DEL (0x7FU)

/* ============================================================================
 * CLI Internal Response Strings
 * ============================================================================ */

/**
 * @brief Standard CLI newline sequence.
 *
 * Uses carriage return followed by line feed.
 */
static const char s_newline[] = "\r\n";

/**
 * @brief Terminal erase sequence.
 *
 * Moves the cursor backward, replaces the character with a space,
 * and moves the cursor backward again.
 */
static const char s_erase[] = "\b \b";

/**
 * @brief Response for an unrecognized CLI command.
 */
static const char s_unknown[] =
"Command not recognised. Type 'help' for available commands.\r\n";

/**
 * @brief Response for invalid command parameters.
 */
static const char s_wrong_args[] =
"Incorrect command parameter(s). Type 'help' for usage.\r\n";

/**
 * @brief Response when the CLI input exceeds its configured capacity.
 */
static const char s_line_too_long[] = "Input too long; command discarded.\r\n";

/**
 * @brief Introduction displayed by the built-in help command.
 */
static const char s_help_intro[] = "help: List available commands\r\n";

/* ============================================================================
 * CLI Internal State
 * ============================================================================ */

/**
 * @brief Internal runtime state of the CLI module.
 *
 * @details
 * Maintains the validated application configuration, registered
 * commands, command iteration state, and input processing flags.
 *
 * The command registry has a fixed capacity determined by
 * configCLI_MAX_COMMANDS.
 *
 * @note
 * This structure is private to the CLI implementation.
 *
 * @warning
 * CLI initialization, command registration, and input processing
 * must follow the documented threading and reentrancy restrictions.
 */
typedef struct
{
    cli_config_t config;
    const cli_command_t* commands[configCLI_MAX_COMMANDS];
    size_t command_count;
    size_t command_index;
    bool initialized;
    bool discarding_line;
    uint8_t previous_eol;
} cli_state_t;

/* ============================================================================
 * CLI Internal State Instance
 * ============================================================================ */

/**
 * @brief Private runtime state of the CLI module.
 *
 * @details
 * Static storage duration ensures that all members are
 * zero-initialized before application startup.
 *
 * The state is configured during cli_init() and reset during
 * cli_deinit().
 */
static cli_state_t s_cli;

/* -------------------------------------------------------------------------- */
/* PRIVATE FUNCTION DECLARATIONS                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Check whether a character is CLI whitespace.
 */
static bool cli_space(char c);

/**
 * @brief Write data to the configured CLI transport.
 */
static ret_err_sts_t cli_write(const void* data, size_t length);

/**
 * @brief Write a NUL-terminated string to the CLI transport.
 */
static ret_err_sts_t cli_write_string(const char* str);

/**
 * @brief Write the configured CLI prompt.
 */
static ret_err_sts_t cli_prompt(void);

/**
 * @brief Skip leading CLI whitespace characters.
 */
static const char* cli_skip_space(const char* p);

/**
 * @brief Count command parameters excluding the command name.
 */
static size_t cli_count_params(const char* command_line);

/**
 * @brief Check whether a command line matches a command name.
 */
static bool cli_matches(const char* line, const char* name);

/**
 * @brief Write the help text for registered CLI commands.
 */
static ret_err_sts_t cli_write_help(void);

/**
 * @brief Execute a CLI command and transmit its output.
 */
static ret_err_sts_t cli_run_command(const cli_command_t* command, const char* command_line);

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTION DEFINATIONS                                               */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/* CLI Initialization                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the command-line interface module.
 *
 * @details
 * Validates the application-provided CLI configuration,
 * initializes the internal module state, and prepares
 * the command and output buffers.
 *
 * The configured CLI prompt is transmitted after
 * successful initialization.
 *
 * If prompt transmission fails, the internal module
 * state is cleared and the initialization fails.
 *
 * @param[in] config
 * Pointer to the application-provided CLI configuration.
 *
 * @retval RET_ERR_STS_OK
 * CLI initialized successfully.
 *
 * @retval RET_ERR_STS_INVALID_PARAM
 * Invalid configuration, buffer size, or buffer pointer.
 *
 * @retval RET_ERR_STS_ALREADY_INIT
 * CLI module is already initialized.
 *
 * @return
 * Transport error status if prompt transmission fails.
 *
 * @pre
 * Application-owned buffers must remain valid throughout
 * the lifetime of the initialized CLI module.
 *
 * @note
 * A read buffer is required only when the transport
 * provides a read callback.
 *
 * @par MISRA C Considerations
 * - Configuration pointers are validated before use.
 * - Buffer capacities are checked before accessing buffers.
 * - Required callbacks are validated before invocation.
 * - Explicit pointer conversion is used for buffer comparison.
 * - Module state is initialized deterministically.
 * - No dynamic memory allocation is performed.
 */
ret_err_sts_t cli_init(const cli_config_t* config)
{
    ret_err_sts_t status;

    /* Validate the configuration structure pointer. */
    if(config == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Prevent repeated initialization of the CLI module. */
    if(s_cli.initialized)
    {
        return RET_ERR_STS_ALREADY_INIT;
    }

    /*
     * Validate required transport callbacks, command buffers,
     * output buffers, and optional input buffer configuration.
     */
    if((config->transport.write == NULL) || (config->command_buffer == NULL) ||
       (config->command_buffer_size < 2U) || (config->output_buffer == NULL) ||
       (config->output_buffer_size < 2U) ||
       ((config->transport.read != NULL) &&
        ((config->read_buffer == NULL) || (config->read_buffer_size == 0U) ||
         (config->read_buffer_size > (size_t)INT32_MAX))))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Reject identical buffer starting addresses to prevent
     * direct buffer reuse between CLI operations.
     */
    if((config->command_buffer == config->output_buffer) ||
       (config->read_buffer == (uint8_t*)config->command_buffer) ||
       (config->read_buffer == (uint8_t*)config->output_buffer))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Reset the internal CLI state before configuration. */
    (void)memset(&s_cli, 0, sizeof(s_cli));

    /* Copy the application-provided configuration. */
    s_cli.config = *config;

    /* Initialize command and output buffers as empty strings. */
    config->command_buffer[0] = '\0';
    config->output_buffer[0]  = '\0';

    /* Mark the CLI module as initialized. */
    s_cli.initialized = true;

    /* Transmit the configured CLI prompt. */
    status = cli_prompt();

    /* Roll back initialization if prompt transmission fails. */
    if(status != RET_ERR_STS_OK)
    {
        (void)memset(&s_cli, 0, sizeof(s_cli));
    }

    return status;
}

/* -------------------------------------------------------------------------- */
/* CLI Deinitialization                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Deinitialize the command-line interface module.
 *
 * @details
 * Clears the internal CLI state and removes all
 * registered command references and configuration data.
 *
 * Application-owned transport resources and buffers
 * are not deleted or released.
 *
 * @retval RET_ERR_STS_OK
 * CLI deinitialized successfully.
 *
 * @retval RET_ERR_STS_NOT_INITIALIZED
 * CLI module was not initialized.
 *
 * @note
 * The application remains responsible for managing
 * transport resources and allocated buffers.
 *
 * @par MISRA C Considerations
 * - Initialization state is checked before deinitialization.
 * - Internal state is reset deterministically.
 * - No dynamic memory deallocation is performed.
 */
ret_err_sts_t cli_deinit(void)
{
    /* Reject deinitialization of an inactive CLI module. */
    if(!s_cli.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /* Clear the internal configuration and command registry. */
    (void)memset(&s_cli, 0, sizeof(s_cli));

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */
/* CLI Command Registration                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Register a command with the CLI module.
 *
 * @details
 * Validates the supplied command descriptor and
 * registers it in the internal command table.
 *
 * The command name must:
 * - Be nonempty.
 * - Contain only printable non-whitespace ASCII characters.
 * - Not match the reserved "help" command.
 * - Be unique among registered commands.
 *
 * A negative expected parameter count of -1 indicates
 * that the command accepts a variable number of parameters.
 *
 * @param[in] command
 * Pointer to the command descriptor to register.
 *
 * @retval RET_ERR_STS_OK
 * Command registered successfully.
 *
 * @retval RET_ERR_STS_NOT_INITIALIZED
 * CLI module is not initialized.
 *
 * @retval RET_ERR_STS_INVALID_PARAM
 * Invalid command descriptor, callback, name,
 * or expected parameter count.
 *
 * @retval RET_ERR_STS_ALREADY_EXISTS
 * Command name is already registered or reserved.
 *
 * @retval RET_ERR_STS_BUFFER_FULL
 * Maximum number of registered commands reached.
 *
 * @pre
 * The command descriptor, name, help string, and
 * callback must remain valid while registered.
 *
 * @note
 * Command descriptors are stored by pointer
 * and are not copied into internal storage.
 *
 * @par MISRA C Considerations
 * - Input pointers are validated before use.
 * - Command name characters are explicitly validated.
 * - Command registry traversal is bounded.
 * - Duplicate commands are rejected.
 * - Registry capacity is checked before insertion.
 * - No dynamic memory allocation is performed.
 */
ret_err_sts_t cli_register_command(const cli_command_t* command)
{
    const char* p;
    size_t i;

    /* Verify that the CLI module is initialized. */
    if(!s_cli.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /* Validate required command descriptor fields. */
    if((command == NULL) || (command->name == NULL) ||
       (command->name[0] == '\0') || (command->help == NULL) ||
       (command->callback == NULL) || (command->expected_parameters < -1))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /*
     * Validate that the command name contains only
     * printable, non-whitespace ASCII characters.
     */
    for(p = command->name; *p != '\0'; ++p)
    {
        if((*p < '!') || (*p > '~') || cli_space(*p))
        {
            return RET_ERR_STS_INVALID_PARAM;
        }
    }

    /* Prevent registration of the reserved help command. */
    if(strcmp(command->name, "help") == 0)
    {
        return RET_ERR_STS_ALREADY_EXISTS;
    }

    /* Check whether the command name is already registered. */
    for(i = 0U; i < s_cli.command_count; ++i)
    {
        if(strcmp(s_cli.commands[i]->name, command->name) == 0)
        {
            return RET_ERR_STS_ALREADY_EXISTS;
        }
    }

    /* Ensure the command registry has available capacity. */
    if(s_cli.command_count >= configCLI_MAX_COMMANDS)
    {
        return RET_ERR_STS_BUFFER_FULL;
    }

    /* Store the command descriptor in the registry. */
    s_cli.commands[s_cli.command_count] = command;

    /* Update the number of registered commands. */
    ++s_cli.command_count;

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */
/* CLI Command Execution                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Parse and execute a CLI command line.
 *
 * @details
 * Processes the supplied command line by:
 * - Skipping leading whitespace.
 * - Ignoring empty command lines.
 * - Handling the built-in help command.
 * - Searching for a registered command.
 * - Validating the expected parameter count.
 * - Executing the matching command callback.
 *
 * An error message is transmitted when the command
 * is unknown or its parameter count is incorrect.
 *
 * @param[in] command_line
 * Pointer to the NUL-terminated command line.
 *
 * @retval RET_ERR_STS_OK
 * Command processed successfully, or an empty
 * command line was received.
 *
 * @retval RET_ERR_STS_NOT_INITIALIZED
 * CLI module is not initialized.
 *
 * @retval RET_ERR_STS_INVALID_PARAM
 * Command line pointer is NULL.
 *
 * @return
 * Status returned by command execution or
 * the configured CLI transport.
 *
 * @note
 * Commands with expected_parameters equal to -1
 * bypass the fixed parameter count validation.
 *
 * @note
 * An unknown command or incorrect parameter count
 * may still return RET_ERR_STS_OK if the associated
 * error message is transmitted successfully.
 *
 * @par MISRA C Considerations
 * - Input pointer is validated before use.
 * - Command registry traversal is bounded.
 * - Parameter counts use size_t.
 * - Signed parameter counts are checked before conversion.
 * - Command execution status is propagated.
 * - No dynamic memory allocation is performed.
 */
ret_err_sts_t cli_execute(const char* command_line)
{
    const char* line;
    const cli_command_t* cmd;
    size_t i;

    /* Verify that the CLI module is initialized. */
    if(!s_cli.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /* Validate the supplied command line pointer. */
    if(command_line == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Skip whitespace preceding the command name. */
    line = cli_skip_space(command_line);

    /* Ignore empty or whitespace-only command lines. */
    if(*line == '\0')
    {
        return RET_ERR_STS_OK;
    }

    /* Process the built-in help command. */
    if(cli_matches(line, "help"))
    {
        /* The built-in help command accepts no parameters. */
        if(cli_count_params(line) != 0U)
        {
            return cli_write_string(s_wrong_args);
        }

        /* Transmit help information for registered commands. */
        return cli_write_help();
    }

    /* Search the registered command table. */
    for(i = 0U; i < s_cli.command_count; ++i)
    {
        /* Retrieve the current registered command. */
        cmd = s_cli.commands[i];

        /* Check whether the command name matches. */
        if(cli_matches(line, cmd->name))
        {
            /*
             * Validate the parameter count for commands
             * that require a fixed number of parameters.
             */
            if((cmd->expected_parameters >= 0) &&
               (cli_count_params(line) != (size_t)cmd->expected_parameters))
            {
                return cli_write_string(s_wrong_args);
            }

            /* Execute the matching registered command. */
            return cli_run_command(cmd, line);
        }
    }

    /* Report an unknown command when no match is found. */
    return cli_write_string(s_unknown);
}

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* CLI Input Processing                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Process received CLI input characters.
 *
 * @details
 * Processes a sequence of input bytes received from the
 * application-provided CLI transport.
 *
 * The function supports:
 * - Command-line character accumulation.
 * - Optional character echo.
 * - Backspace and delete handling.
 * - CR and LF command termination.
 * - Suppression of consecutive CRLF or LFCR pairs.
 * - Command execution after line termination.
 * - Detection and rejection of oversized command lines.
 * - Automatic prompt output after command processing.
 *
 * If the command buffer becomes full, the entire input
 * line is discarded until the next line terminator.
 *
 * @param[in] data
 * Pointer to the received input data.
 *
 * @param[in] length
 * Number of input bytes to process.
 *
 * @retval RET_ERR_STS_OK
 * Input processed successfully.
 *
 * @retval RET_ERR_STS_NOT_INITIALIZED
 * CLI module is not initialized.
 *
 * @retval RET_ERR_STS_INVALID_PARAM
 * Input data pointer is NULL while length is nonzero.
 *
 * @retval RET_ERR_STS_BUFFER_OVERFLOW
 * Received command line exceeded the configured buffer.
 *
 * @return
 * First error encountered during input processing,
 * command execution, or transport output.
 *
 * @note
 * A NULL data pointer is permitted when length is zero.
 *
 * @note
 * CRLF and LFCR pairs are suppressed even when their
 * characters arrive in separate calls.
 *
 * @note
 * Processing continues after an error, and the first
 * encountered error status is returned.
 *
 * @par MISRA C Considerations
 * - Input pointers are validated before dereferencing.
 * - Array access is bounded by the supplied input length.
 * - Command buffer capacity is checked before insertion.
 * - Buffer indices use size_t.
 * - NUL termination is maintained after modifications.
 * - Error status is preserved across input processing.
 * - No dynamic memory allocation is performed.
 */
ret_err_sts_t cli_process_input(const uint8_t* data, size_t length)
{
    size_t i;
    uint8_t c;
    ret_err_sts_t status;
    ret_err_sts_t first_error = RET_ERR_STS_OK;

    /* Verify that the CLI module is initialized. */
    if(!s_cli.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /* Validate the input pointer for nonzero data length. */
    if((data == NULL) && (length != 0U))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Process each received input character. */
    for(i = 0U; i < length; ++i)
    {
        /* Retrieve the current input character. */
        c = data[i];

        /* Reset the operation status for the current character. */
        status = RET_ERR_STS_OK;

        /* Check whether the character terminates a command line. */
        if((c == '\r') || (c == '\n'))
        {
            /*
             * Suppress consecutive CRLF or LFCR pairs,
             * including pairs received across separate calls.
             */
            if(((s_cli.previous_eol == '\r') && (c == '\n')) ||
               ((s_cli.previous_eol == '\n') && (c == '\r')))
            {
                s_cli.previous_eol = 0U;
                continue;
            }

            /* Record the current line termination character. */
            s_cli.previous_eol = c;

            /* Echo the configured newline when echo is enabled. */
            if(s_cli.config.echo_input)
            {
                status = cli_write_string(s_newline);
            }

            /* Process the completed command line if output succeeded. */
            if(status == RET_ERR_STS_OK)
            {
                if(s_cli.discarding_line)
                {
                    /* Report that the received command was too long. */
                    status = cli_write_string(s_line_too_long);
                }
                else
                {
                    /* Execute the completed command line. */
                    status = cli_execute(s_cli.config.command_buffer);
                }
            }

            /* Reset the command buffer for the next input line. */
            s_cli.command_index            = 0U;
            s_cli.config.command_buffer[0] = '\0';

            /* Preserve the buffer overflow status when applicable. */
            if(s_cli.discarding_line && (first_error == RET_ERR_STS_OK))
            {
                first_error = RET_ERR_STS_BUFFER_OVERFLOW;
            }

            /* Clear the oversized-line discard state. */
            s_cli.discarding_line = false;

            /* Preserve the first command-processing error. */
            if((status != RET_ERR_STS_OK) && (first_error == RET_ERR_STS_OK))
            {
                first_error = status;
            }

            /* Display the prompt for the next command. */
            status = cli_prompt();
        }
        else
        {
            /* Reset line termination tracking for normal input. */
            s_cli.previous_eol = 0U;

            /* Handle backspace and delete characters. */
            if((c == CLI_ASCII_BS) || (c == CLI_ASCII_DEL))
            {
                /*
                 * Remove the previous character only when
                 * the current command line is not discarded.
                 */
                if((!s_cli.discarding_line) && (s_cli.command_index != 0U))
                {
                    /* Move the command buffer index backward. */
                    --s_cli.command_index;

                    /* Maintain NUL termination after deletion. */
                    s_cli.config.command_buffer[s_cli.command_index] = '\0';

                    /* Echo the terminal erase sequence if enabled. */
                    if(s_cli.config.echo_input)
                    {
                        status = cli_write(s_erase, sizeof(s_erase) - 1U);
                    }
                }
            }
            else if((c >= ' ') && (c <= '~'))
            {
                /* Process printable ASCII input characters. */
                if(!s_cli.discarding_line)
                {
                    /*
                     * Reserve one byte for the terminating
                     * NUL character in the command buffer.
                     */
                    if(s_cli.command_index < (s_cli.config.command_buffer_size - 1U))
                    {
                        /* Append the received character. */
                        s_cli.config.command_buffer[s_cli.command_index] = (char)c;

                        /* Advance the command buffer index. */
                        ++s_cli.command_index;

                        /* Maintain NUL termination. */
                        s_cli.config.command_buffer[s_cli.command_index] = '\0';

                        /* Echo the received character if enabled. */
                        if(s_cli.config.echo_input)
                        {
                            status = cli_write(&c, 1U);
                        }
                    }
                    else
                    {
                        /*
                         * Reject the entire oversized command
                         * rather than executing a truncated prefix.
                         */
                        s_cli.discarding_line = true;
                    }
                }
            }
            else
            {
                /* Ignore unsupported input characters. */
            }
        }

        /* Preserve the first error encountered during processing. */
        if((status != RET_ERR_STS_OK) && (first_error == RET_ERR_STS_OK))
        {
            first_error = status;
        }
    }

    return first_error;
}

/* -------------------------------------------------------------------------- */
/* CLI Transport Polling                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Poll the configured CLI transport for input data.
 *
 * @details
 * Invokes the application-provided transport read callback
 * to retrieve available input bytes.
 *
 * Successfully received data is forwarded to
 * cli_process_input() for command-line processing.
 *
 * A negative read result is treated as a transport
 * receive failure.
 *
 * A positive read result exceeding the configured
 * receive buffer capacity is rejected.
 *
 * @retval RET_ERR_STS_OK
 * Input polling and processing completed successfully.
 *
 * @retval RET_ERR_STS_NOT_INITIALIZED
 * CLI module is not initialized.
 *
 * @retval RET_ERR_STS_INVALID_STATE
 * Transport read callback or receive buffer is unavailable.
 *
 * @retval RET_ERR_STS_RX_FAILED
 * Transport read callback reported a receive failure.
 *
 * @retval RET_ERR_STS_INVALID_LENGTH
 * Transport returned a length greater than the receive
 * buffer capacity.
 *
 * @return
 * Status returned by cli_process_input().
 *
 * @note
 * The transport read callback is expected to return
 * the number of received bytes or a negative error value.
 *
 * @par MISRA C Considerations
 * - Initialization state is checked before processing.
 * - Callback and buffer pointers are validated before use.
 * - Signed read result is checked before unsigned conversion.
 * - Received length is validated against buffer capacity.
 * - Transport errors are propagated explicitly.
 */
ret_err_sts_t cli_process_console(void)
{
    int32_t length;

    /* Verify that the CLI module is initialized. */
    if(!s_cli.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    /* Validate the configured receive transport resources. */
    if((s_cli.config.transport.read == NULL) ||
       (s_cli.config.read_buffer == NULL) || (s_cli.config.read_buffer_size == 0U))
    {
        return RET_ERR_STS_INVALID_STATE;
    }

    /* Read available input data from the configured transport. */
    length = s_cli.config.transport.read(s_cli.config.read_buffer, s_cli.config.read_buffer_size,
                                         s_cli.config.transport.context);

    /* Reject a transport-reported receive failure. */
    if(length < 0)
    {
        return RET_ERR_STS_RX_FAILED;
    }

    /* Ensure the received length does not exceed buffer capacity. */
    if((size_t)length > s_cli.config.read_buffer_size)
    {
        return RET_ERR_STS_INVALID_LENGTH;
    }

    /* Process the received input bytes. */
    return cli_process_input(s_cli.config.read_buffer, (size_t)length);
}

/* -------------------------------------------------------------------------- */
/* CLI Parameter Extraction                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieve a parameter from a CLI command line.
 *
 * @details
 * Locates the requested whitespace-separated parameter
 * in a NUL-terminated command line.
 *
 * The command name is treated as the first token
 * and is excluded from parameter indexing.
 *
 * Parameter indexing begins at one.
 *
 * For example:
 *
 * @code
 * command_line = "set value 100"
 *
 * parameter_index = 1 -> "value"
 * parameter_index = 2 -> "100"
 * @endcode
 *
 * The function returns a pointer into the original
 * command line without copying or modifying its contents.
 *
 * @param[in] command_line
 * Pointer to the NUL-terminated command line.
 *
 * @param[in] parameter_index
 * One-based index of the requested parameter.
 *
 * @param[out] parameter_length
 * Pointer to the variable receiving the parameter length.
 *
 * @return
 * Pointer to the first character of the requested parameter,
 * or NULL if the parameter is unavailable or input is invalid.
 *
 * @note
 * The returned parameter is not separately NUL-terminated.
 * Use parameter_length to determine its valid character range.
 *
 * @note
 * When parameter_length is non-NULL, it is initialized
 * to zero before parameter processing.
 *
 * @par MISRA C Considerations
 * - Input pointers are validated before dereferencing.
 * - Parameter indexing is explicitly one-based.
 * - String traversal stops at the NUL terminator.
 * - Input string remains unchanged.
 * - No dynamic memory allocation is performed.
 */
const char* cli_get_parameter(const char* command_line, size_t parameter_index, size_t* parameter_length)
{
    size_t i;
    const char* p;
    const char* start;

    /* Initialize the returned parameter length when possible. */
    if(parameter_length != NULL)
    {
        *parameter_length = 0U;
    }

    /* Validate the command line, output pointer, and index. */
    if((command_line == NULL) || (parameter_length == NULL) || (parameter_index == 0U))
    {
        return NULL;
    }

    /* Skip whitespace preceding the command name. */
    p = cli_skip_space(command_line);

    /* Advance beyond the command name token. */
    while((*p != '\0') && (!cli_space(*p)))
    {
        ++p;
    }

    /* Skip whitespace preceding the first parameter. */
    p = cli_skip_space(p);

    /* Advance to the requested one-based parameter index. */
    for(i = 1U; i < parameter_index; ++i)
    {
        /* Reject requests beyond the available parameters. */
        if(*p == '\0')
        {
            return NULL;
        }

        /* Skip the current parameter token. */
        while((*p != '\0') && (!cli_space(*p)))
        {
            ++p;
        }

        /* Skip whitespace before the next parameter. */
        p = cli_skip_space(p);
    }

    /* Reject the request if the parameter does not exist. */
    if(*p == '\0')
    {
        return NULL;
    }

    /* Record the beginning of the requested parameter. */
    start = p;

    /* Advance to the end of the parameter token. */
    while((*p != '\0') && (!cli_space(*p)))
    {
        ++p;
    }

    /* Calculate the requested parameter length. */
    *parameter_length = (size_t)(p - start);

    /* Return a pointer into the original command line. */
    return start;
}

/* -------------------------------------------------------------------------- */
/* CLI Output Buffer Access                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Get the configured CLI output buffer.
 *
 * @details
 * Returns the application-provided output buffer
 * associated with the initialized CLI module.
 *
 * @return
 * Pointer to the configured output buffer,
 * or NULL if the CLI module is not initialized.
 *
 * @note
 * The returned buffer remains owned by the application.
 *
 * @par MISRA C Considerations
 * - Initialization state is checked before access.
 * - No pointer conversion is required.
 * - No dynamic memory allocation is performed.
 */
char* cli_get_output_buffer(void)
{
    char* buffer = NULL;

    /* Return the output buffer only when CLI is initialized. */
    if(s_cli.initialized)
    {
        buffer = s_cli.config.output_buffer;
    }

    return buffer;
}

/* -------------------------------------------------------------------------- */
/* CLI Output Buffer Capacity                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Get the configured CLI output buffer capacity.
 *
 * @details
 * Returns the total size of the application-provided
 * CLI output buffer in bytes.
 *
 * @return
 * Configured output buffer capacity in bytes,
 * or zero when the CLI module is not initialized.
 *
 * @note
 * The returned capacity includes space for
 * the terminating NUL character.
 *
 * @par MISRA C Considerations
 * - Initialization state is checked before access.
 * - Uses size_t for buffer capacity.
 * - Returns zero when the module is inactive.
 * - No shared state is modified.
 */
size_t cli_get_output_buffer_size(void)
{
    size_t size = 0U;

    /* Return the configured capacity only when initialized. */
    if(s_cli.initialized)
    {
        size = s_cli.config.output_buffer_size;
    }

    return size;
}

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PRIVATE FUNCTION DEFINATIONS                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Check whether a character is CLI whitespace.
 *
 * @details
 * Determines whether the supplied character is a space
 * or horizontal tab.
 *
 * @param[in] c
 * Character to validate.
 *
 * @retval true
 * Character is a space or horizontal tab.
 *
 * @retval false
 * Character is not recognized as CLI whitespace.
 *
 * @par MISRA C Considerations
 * - Uses explicit character comparisons.
 * - Has no side effects.
 * - Returns a Boolean result.
 */
static bool cli_space(char c)
{
    /* Recognize spaces and horizontal tabs as CLI separators. */
    return ((c == ' ') || (c == '\t'));
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Write data to the configured CLI transport.
 *
 * @details
 * Forwards the specified data buffer to the
 * application-provided CLI transport callback.
 *
 * Zero-length writes are treated as successful
 * without invoking the transport.
 *
 * @param[in] data
 * Pointer to the data buffer to transmit.
 *
 * @param[in] length
 * Number of bytes to transmit.
 *
 * @return
 * Transport operation status.
 *
 * @retval RET_ERR_STS_OK
 * No data needs to be transmitted, or the transport
 * reports successful completion.
 *
 * @pre
 * The CLI transport callback must be configured.
 *
 * @warning
 * For a nonzero length, data must point to a readable
 * buffer containing at least length bytes.
 *
 * @par MISRA C Considerations
 * - Zero-length operations are handled explicitly.
 * - Uses an explicit pointer conversion.
 * - Propagates the transport callback status.
 * - Performs no dynamic memory allocation.
 */
static ret_err_sts_t cli_write(const void* data, size_t length)
{
    /* Avoid invoking the transport for an empty buffer. */
    if(length == 0U)
    {
        return RET_ERR_STS_OK;
    }

    /* Forward the data to the application transport callback. */
    return s_cli.config.transport.write((const uint8_t*)data, length,
                                        s_cli.config.transport.context);
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Write a NUL-terminated string to the CLI transport.
 *
 * @details
 * Determines the string length and forwards the
 * string data to the configured CLI transport.
 *
 * The terminating NUL character is not transmitted.
 *
 * @param[in] str
 * Pointer to a NUL-terminated string.
 *
 * @return
 * Status returned by cli_write().
 *
 * @pre
 * str must point to a valid NUL-terminated string.
 *
 * @par MISRA C Considerations
 * - Uses size_t for string length.
 * - Does not modify the input string.
 * - Propagates the transport operation status.
 */
static ret_err_sts_t cli_write_string(const char* str)
{
    size_t length;

    /* Determine the number of characters to transmit. */
    length = strlen(str);

    /* Transmit the string without its NUL terminator. */
    return cli_write(str, length);
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Write the configured CLI prompt.
 *
 * @details
 * Checks whether the application has configured
 * a CLI prompt string.
 *
 * If a prompt is available, it is transmitted using
 * the configured CLI transport.
 *
 * If no prompt is configured, the operation succeeds
 * without transmitting any data.
 *
 * @return
 * Status of the prompt write operation.
 *
 * @retval RET_ERR_STS_OK
 * Prompt transmitted successfully or no prompt configured.
 *
 * @par MISRA C Considerations
 * - Optional pointer is checked before use.
 * - No dynamic memory allocation is performed.
 * - Transport errors are propagated.
 */
static ret_err_sts_t cli_prompt(void)
{
    ret_err_sts_t status = RET_ERR_STS_OK;

    /* Transmit the prompt only when it is configured. */
    if(s_cli.config.prompt != NULL)
    {
        status = cli_write_string(s_cli.config.prompt);
    }

    return status;
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Skip leading whitespace in a CLI string.
 *
 * @details
 * Advances the supplied string pointer past
 * consecutive spaces and horizontal tabs.
 *
 * Processing stops at the first non-whitespace
 * character or the terminating NUL character.
 *
 * @param[in] p
 * Pointer to the input string.
 *
 * @return
 * Pointer to the first non-whitespace character
 * or the terminating NUL character.
 *
 * @pre
 * p must point to a valid NUL-terminated string.
 *
 * @par MISRA C Considerations
 * - Input data is not modified.
 * - Pointer advancement follows whitespace validation.
 * - No additional memory is allocated.
 */
static const char* cli_skip_space(const char* p)
{
    /* Advance past all leading CLI whitespace characters. */
    while(cli_space(*p))
    {
        ++p;
    }

    /* Return the first non-whitespace character. */
    return p;
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Count parameters in a CLI command line.
 *
 * @details
 * Parses the supplied command line and counts
 * whitespace-separated parameters.
 *
 * The first token is treated as the command name
 * and is excluded from the parameter count.
 *
 * Spaces and horizontal tabs are recognized
 * as parameter separators.
 *
 * @param[in] command_line
 * Pointer to the NUL-terminated command line.
 *
 * @return
 * Number of parameters following the command name.
 *
 * @pre
 * command_line must point to a valid NUL-terminated string.
 *
 * @note
 * Quoted strings are not treated as single parameters.
 *
 * @par MISRA C Considerations
 * - Uses size_t for the parameter counter.
 * - Input string remains unchanged.
 * - String traversal stops at the NUL terminator.
 * - No dynamic memory allocation is performed.
 */
static size_t cli_count_params(const char* command_line)
{
    size_t count = 0U;
    const char* p;

    /* Skip whitespace before the command name. */
    p = cli_skip_space(command_line);

    /* Skip the first token, which represents the command name. */
    while((*p != '\0') && (!cli_space(*p)))
    {
        ++p;
    }

    /* Skip whitespace following the command name. */
    p = cli_skip_space(p);

    /* Count each remaining whitespace-separated parameter. */
    while(*p != '\0')
    {
        /* A new parameter begins at the current position. */
        ++count;

        /* Advance to the end of the current parameter. */
        while((*p != '\0') && (!cli_space(*p)))
        {
            ++p;
        }

        /* Skip separators before the next parameter. */
        p = cli_skip_space(p);
    }

    return count;
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Check whether a command line matches a command name.
 *
 * @details
 * Compares the beginning of a command line with
 * the supplied registered command name.
 *
 * A match is accepted only when the command name
 * is followed by:
 * - The terminating NUL character.
 * - A space.
 * - A horizontal tab.
 *
 * This prevents a shorter command name from
 * incorrectly matching a longer command token.
 *
 * @param[in] line
 * Pointer to the command line.
 *
 * @param[in] name
 * Pointer to the registered command name.
 *
 * @retval true
 * Command name matches the first command-line token.
 *
 * @retval false
 * Command name does not match.
 *
 * @pre
 * Both pointers must reference valid NUL-terminated strings.
 *
 * @par MISRA C Considerations
 * - Uses size_t for string length.
 * - Performs bounded string comparison.
 * - Validates the command token boundary.
 * - Does not modify input strings.
 */
static bool cli_matches(const char* line, const char* name)
{
    size_t name_length;
    bool matches;

    /* Determine the registered command name length. */
    name_length = strlen(name);

    /* Compare the command line against the registered name. */
    matches = (strncmp(line, name, name_length) == 0);

    /* Ensure the matched name ends at a valid token boundary. */
    if(matches)
    {
        matches = ((line[name_length] == '\0') || cli_space(line[name_length]));
    }

    return matches;
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Write help information for registered CLI commands.
 *
 * @details
 * Transmits the general CLI help introduction,
 * followed by the help string of each registered command.
 *
 * If a command help string does not end with a
 * newline character, the configured newline string
 * is appended automatically.
 *
 * Processing stops immediately if a transport
 * write operation fails.
 *
 * @return
 * Status of the help output operation.
 *
 * @retval RET_ERR_STS_OK
 * All help information transmitted successfully.
 *
 * @note
 * Registered command entries and help strings
 * must remain valid during this operation.
 *
 * @par MISRA C Considerations
 * - Array traversal is bounded by command_count.
 * - Transport return values are checked.
 * - String length is checked before indexing.
 * - Prevents unsigned underflow for empty help strings.
 */
static ret_err_sts_t cli_write_help(void)
{
    size_t i;
    size_t length;
    const cli_command_t* command;
    ret_err_sts_t status;

    /* Transmit the general CLI help introduction. */
    status = cli_write_string(s_help_intro);

    if(status != RET_ERR_STS_OK)
    {
        return status;
    }

    /* Process each registered CLI command. */
    for(i = 0U; i < s_cli.command_count; ++i)
    {
        /* Retrieve the current registered command. */
        command = s_cli.commands[i];

        /* Transmit the command-specific help text. */
        status = cli_write_string(command->help);

        if(status != RET_ERR_STS_OK)
        {
            return status;
        }

        /* Determine whether the help text includes a newline. */
        length = strlen(command->help);

        /*
         * Append a newline when the help string is empty
         * or does not already end with one.
         */
        if((length == 0U) || (command->help[length - 1U] != '\n'))
        {
            status = cli_write_string(s_newline);

            if(status != RET_ERR_STS_OK)
            {
                return status;
            }
        }
    }

    return RET_ERR_STS_OK;
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Execute a registered CLI command.
 *
 * @details
 * Invokes the registered command callback and
 * transmits the generated output through the
 * configured CLI transport.
 *
 * The callback may generate output in multiple chunks.
 * Execution continues while the callback returns true.
 *
 * To prevent infinite execution, the number of
 * callback iterations is limited by configCLI_MAX_OUTPUT_CHUNKS.
 *
 * The output buffer is cleared before each callback
 * and explicitly NUL-terminated afterward.
 *
 * @param[in] command
 * Pointer to the registered CLI command descriptor.
 *
 * @param[in] command_line
 * Pointer to the complete NUL-terminated command line.
 *
 * @retval RET_ERR_STS_OK
 * Command completed successfully.
 *
 * @retval RET_ERR_STS_INVALID_STATE
 * Callback requested additional output without progress,
 * or exceeded the permitted number of output chunks.
 *
 * @return
 * Transport error status if output transmission fails.
 *
 * @pre
 * command and command_line must be valid.
 *
 * @pre
 * The command callback must be configured.
 *
 * @pre
 * The configured output buffer must be valid and
 * output_buffer_size must be greater than zero.
 *
 * @warning
 * The callback must not write beyond the configured
 * output buffer capacity.
 *
 * @note
 * A true callback return value indicates that
 * additional output chunks are available.
 *
 * @par MISRA C Considerations
 * - Callback execution count is bounded.
 * - Output buffer is initialized before use.
 * - NUL termination is enforced after callback execution.
 * - Transport return values are explicitly checked.
 * - Empty output with pending work is rejected.
 * - No dynamic memory allocation is performed.
 */
static ret_err_sts_t cli_run_command(const cli_command_t* command, const char* command_line)
{
    size_t iteration;
    size_t length;
    bool more;
    ret_err_sts_t status;

    /* Execute the command callback with a bounded iteration count. */
    for(iteration = 0U; iteration < configCLI_MAX_OUTPUT_CHUNKS; ++iteration)
    {
        /* Initialize the output buffer before callback execution. */
        s_cli.config.output_buffer[0] = '\0';

        /* Request the next output chunk from the command callback. */
        more = command->callback(s_cli.config.output_buffer, s_cli.config.output_buffer_size,
                                 command_line, command->context);

        /*
         * Enforce NUL termination even when the callback
         * fails to terminate its output correctly.
         */
        s_cli.config.output_buffer[s_cli.config.output_buffer_size - 1U] = '\0';

        /* Determine the generated output length. */
        length = strlen(s_cli.config.output_buffer);

        /* Transmit the generated command output. */
        status = cli_write(s_cli.config.output_buffer, length);

        if(status != RET_ERR_STS_OK)
        {
            return status;
        }

        /* Stop processing when the callback has completed. */
        if(!more)
        {
            return RET_ERR_STS_OK;
        }

        /*
         * Prevent repeated callback execution when
         * no output progress has been made.
         */
        if(length == 0U)
        {
            return RET_ERR_STS_INVALID_STATE;
        }
    }

    /* Reject commands that exceed the allowed output iterations. */
    return RET_ERR_STS_INVALID_STATE;
}

/* -------------------------------------------------------------------------- */
