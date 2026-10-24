
/**
 * @file application_cli.c
 * @brief Application-side CLI integration using the board UART driver.
 *
 * @details
 * Demonstrates how to initialize the CLI module, register
 * application commands, and process UART input from an
 * application-created task.
 *
 * The CLI module does not directly access UART hardware.
 * All UART operations are performed through application-
 * provided transport callbacks.
 */

/* -------------------------------------------------------------------------- */
/* Header Files                                                               */
/* -------------------------------------------------------------------------- */

#include "bsp_cli.h"

#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Application CLI Buffer Configuration                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Maximum application CLI command buffer capacity.
 */
#ifndef configCLI_COMMAND_BUFFER_SIZE
    #define configCLI_COMMAND_BUFFER_SIZE 96U
#endif

/**
 * @brief Maximum application CLI output buffer capacity.
 */
#ifndef configCLI_OUTPUT_BUFFER_SIZE
    #define configCLI_OUTPUT_BUFFER_SIZE 192U
#endif

/**
 * @brief Maximum application CLI receive buffer capacity.
 */
#ifndef configCLI_READ_BUFFER_SIZE
    #define configCLI_READ_BUFFER_SIZE 32U
#endif

/* -------------------------------------------------------------------------- */
/* Application CLI Buffers                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Buffer used to accumulate incoming CLI command lines.
 */
static char command_buffer[configCLI_COMMAND_BUFFER_SIZE];

/**
 * @brief Buffer used to store generated CLI command output.
 */
static char output_buffer[configCLI_OUTPUT_BUFFER_SIZE];

/**
 * @brief Buffer used to receive input bytes from the UART driver.
 */
static uint8_t read_buffer[configCLI_READ_BUFFER_SIZE];


/* -------------------------------------------------------------------------- */
/* Board UART Driver Declarations                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read input data from the board UART driver.
 *
 * @param[out] data
 * Destination buffer for received bytes.
 *
 * @param[in] capacity
 * Maximum number of bytes that may be received.
 *
 * @return
 * Number of received bytes, or a negative value on failure.
 */
extern int32_t board_uart_read(uint8_t* data, size_t capacity);

/**
 * @brief Write data using the board UART driver.
 *
 * @param[in] data
 * Pointer to the data to transmit.
 *
 * @param[in] length
 * Number of bytes to transmit.
 *
 * @return
 * UART transmission status.
 */
extern ret_err_sts_t board_uart_write(const uint8_t* data, size_t length);

/* -------------------------------------------------------------------------- */
/* Internal Function Declarations                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read CLI input from the application UART driver.
 */
static int32_t cli_uart_read(uint8_t* data, size_t capacity, void* ctx);

/**
 * @brief Write CLI output through the application UART driver.
 */
static ret_err_sts_t cli_uart_write(const uint8_t* data, size_t length, void* ctx);

/**
 * @brief Execute the CLI ping command.
 */
static bool command_ping(char* out, size_t out_size, const char* command_line, void* context);

/* -------------------------------------------------------------------------- */
/* Application API Declarations                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the application CLI interface.
 */
ret_err_sts_t application_cli_init(void);

/**
 * @brief Application-owned CLI processing task.
 */
void application_cli_task(void* arg);

/* -------------------------------------------------------------------------- */
/* CLI Command Registration Data                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief CLI command descriptor for the ping command.
 *
 * @details
 * Registers the "ping" command, which returns "pong"
 * to verify that the CLI interface is responsive.
 *
 * The command accepts no parameters.
 */
static const cli_command_t ping_cmd = { .name = "ping",
                                        .help =
                                        "ping: Check CLI responsiveness\r\n",
                                        .callback            = command_ping,
                                        .expected_parameters = 0,
                                        .context             = NULL };

/* -------------------------------------------------------------------------- */
/* CLI UART Read Callback                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read CLI input from the application UART driver.
 *
 * @details
 * Adapts the board-specific UART receive function
 * to the transport callback interface expected
 * by the CLI module.
 *
 * @param[out] data
 * Destination buffer for received UART data.
 *
 * @param[in] capacity
 * Maximum number of bytes available in the buffer.
 *
 * @param[in] ctx
 * Application-defined transport context.
 * Unused in this implementation.
 *
 * @return
 * Number of bytes received, or a negative error value.
 *
 * @par MISRA C Considerations
 * - Unused callback context is explicitly discarded.
 * - Buffer capacity is passed to the UART driver.
 * - No dynamic memory allocation is performed.
 */
static int32_t cli_uart_read(uint8_t* data, size_t capacity, void* ctx)
{
    /* Mark the unused transport context explicitly. */
    (void)ctx;

    /* Forward the receive request to the board UART driver. */
    return board_uart_read(data, capacity);
}

/* -------------------------------------------------------------------------- */
/* CLI UART Write Callback                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Write CLI output through the application UART driver.
 *
 * @details
 * Adapts the board-specific UART transmit function
 * to the transport callback interface expected
 * by the CLI module.
 *
 * @param[in] data
 * Pointer to the output data to transmit.
 *
 * @param[in] length
 * Number of bytes to transmit.
 *
 * @param[in] ctx
 * Application-defined transport context.
 * Unused in this implementation.
 *
 * @return
 * Status returned by the board UART transmit function.
 *
 * @warning
 * The UART driver must complete transmission or
 * copy the supplied data before returning.
 *
 * @par MISRA C Considerations
 * - Unused callback context is explicitly discarded.
 * - Transport status is propagated.
 * - No dynamic memory allocation is performed.
 */
static ret_err_sts_t cli_uart_write(const uint8_t* data, size_t length, void* ctx)
{
    /* Mark the unused transport context explicitly. */
    (void)ctx;

    /*
     * Transmit or copy the supplied data before returning,
     * because the CLI may reuse its output buffer.
     */
    return board_uart_write(data, length);
}

/* -------------------------------------------------------------------------- */
/* CLI Ping Command Callback                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Execute the CLI ping command.
 *
 * @details
 * Writes the response "pong" into the CLI output buffer.
 *
 * The callback returns false to indicate that no
 * additional output chunks are required.
 *
 * @param[out] out
 * Pointer to the command output buffer.
 *
 * @param[in] out_size
 * Total capacity of the output buffer in bytes.
 *
 * @param[in] command_line
 * Pointer to the complete CLI command line.
 * Unused by this command.
 *
 * @param[in] context
 * Application-defined command context.
 * Unused by this command.
 *
 * @retval false
 * Command output is complete.
 *
 * @pre
 * out must point to a writable buffer.
 *
 * @pre
 * out_size must be greater than zero.
 *
 * @par MISRA C Considerations
 * - Unused parameters are explicitly discarded.
 * - Output formatting is bounded by buffer capacity.
 * - The formatting result is intentionally discarded.
 * - No dynamic memory allocation is performed.
 */
static bool command_ping(char* out, size_t out_size, const char* command_line, void* context)
{
    /* Mark unused command callback parameters. */
    (void)command_line;
    (void)context;

    /* Generate the ping response within the output buffer. */
    (void)snprintf(out, out_size, "pong\r\n");

    /* Indicate that no additional output chunks are required. */
    return false;
}

/* -------------------------------------------------------------------------- */
/* Application CLI Initialization                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the application CLI interface.
 *
 * @details
 * Configures the CLI module with the board UART
 * transport callbacks and application-owned buffers.
 *
 * After successful initialization, the ping command
 * is registered with the CLI command registry.
 *
 * @retval RET_ERR_STS_OK
 * CLI initialized and ping command registered successfully.
 *
 * @return
 * Error status returned by cli_init() or
 * cli_register_command().
 *
 * @note
 * The CLI configuration is copied internally by cli_init().
 *
 * @note
 * The application-owned buffers have static storage duration
 * and remain valid throughout CLI operation.
 *
 * @par MISRA C Considerations
 * - Configuration structure is explicitly initialized.
 * - Buffer capacities are derived using sizeof.
 * - Callback pointers are assigned explicitly.
 * - Initialization and registration results are checked.
 * - No dynamic memory allocation is performed.
 */
ret_err_sts_t application_cli_init(void)
{
    ret_err_sts_t status;

    /* Configure the CLI transport and application-owned buffers. */
    cli_config_t config = { .transport = { .read    = cli_uart_read,
                                           .write   = cli_uart_write,
                                           .context = NULL },

                            .command_buffer      = command_buffer,
                            .command_buffer_size = sizeof(command_buffer),

                            .output_buffer      = output_buffer,
                            .output_buffer_size = sizeof(output_buffer),

                            .read_buffer      = read_buffer,
                            .read_buffer_size = sizeof(read_buffer),

                            .echo_input = true,
                            .prompt     = "ridecast> " };

    /* Initialize the CLI module using the supplied configuration. */
    status = cli_init(&config);

    /* Register the ping command only after successful initialization. */
    if(status == RET_ERR_STS_OK)
    {
        status = cli_register_command(&ping_cmd);
    }

    return status;
}

/* -------------------------------------------------------------------------- */
/* Application CLI Task                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Application-owned CLI processing task.
 *
 * @details
 * Continuously polls the configured UART transport
 * for received CLI input.
 *
 * Received characters are processed by the CLI module,
 * which executes registered commands when a complete
 * command line is received.
 *
 * @param[in] arg
 * Application-defined task argument.
 * Unused in this implementation.
 *
 * @return
 * None.
 *
 * @pre
 * application_cli_init() must complete successfully
 * before this task begins execution.
 *
 * @warning
 * This function must execute in task context,
 * not interrupt context.
 *
 * @note
 * If the UART receive function is nonblocking,
 * the application should provide an appropriate
 * task delay to avoid continuous CPU usage.
 *
 * @par MISRA C Considerations
 * - Unused task argument is explicitly discarded.
 * - CLI polling return status is intentionally discarded.
 * - No dynamic memory allocation is performed.
 */
void application_cli_task(void* arg)
{
    /* Mark the unused application task argument. */
    (void)arg;

    /* Continuously process incoming CLI data. */
    for(;;)
    {
        /*
         * Poll the configured UART transport.
         * The UART read operation may block depending
         * on the board-specific driver implementation.
         */
        ret_err_sts_t status = cli_process_console();

        if(status != RET_ERR_STS_OK)
        {
            /* Handle or count transport errors. */
            osDelay(10U);
        }

        /*
         * If UART receive is nonblocking,
         * add a small delay on every iteration.
         */
    }
}

/* -------------------------------------------------------------------------- */
