/* ============================================================================
 * File        : cli.h
 * Description : BSP command-line interface public API
 * Author      : Jha Raghav
 * Date        : 2026-10-24
 * ============================================================================
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Description:
 *   Defines the public interface for the reusable BSP command-line
 *   interface (CLI) module, including:
 *
 *   - CLI configuration structures
 *   - CLI command definition structures
 *   - Command callback function types
 *   - Command registration interfaces
 *   - CLI initialization and deinitialization APIs
 *   - Command-line execution APIs
 *   - Input processing interfaces
 *   - CLI polling interfaces
 *   - Command parameter configuration
 *   - Platform-independent error status reporting
 *
 * Integration:
 *   This header exposes the platform-independent BSP CLI API
 *   for application and middleware components.
 *
 *   Applications initialize the CLI using cli_init() and
 *   register supported commands through cli_register_command().
 *
 *   Commands are executed through cli_execute() or the
 *   configured CLI input processing mechanism.
 *
 * Requirements:
 *   - C11 compatible compiler.
 *   - Platform-independent error status definitions.
 *   - Application-provided CLI configuration.
 *   - Valid command callbacks and command definitions.
 *   - Application-managed input and output resources.
 *
 * Notes:
 *   - CLI APIs are designed to remain independent of target hardware.
 *   - Application code owns the configured CLI resources.
 *   - Command registration must follow the CLI lifecycle contract.
 *   - CLI execution is not assumed to be reentrant.
 *   - CLI functions must not be called from interrupt context unless
 *     explicitly documented as ISR-safe.
 *   - Security extensions must preserve the public CLI API wherever
 *     backward compatibility is required.
 *
 * ============================================================================
 * Change Log:
 *
 * Date          Author          Description
 * ------------  --------------  ----------------------------------------------
 * 2026-10-24    Jha Raghav      Standardized CLI public API documentation
 * ============================================================================
 */

#ifndef CLI_H
#define CLI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_err_sts.h"

/*===========================================================================
 * TRANSPORT (UART / USB CDC / RTT / TCP / application console)
 *===========================================================================*/

/**
 * @brief Read at most capacity bytes into buffer.
 * @return Positive byte count, 0 if no bytes available, negative on error.
 * @note May block if desired; cli_poll() inherits this behavior.
 */
typedef int32_t (*cli_transport_read_t)(uint8_t *buffer,
                                        size_t capacity,
                                        void *context);

/**
 * @brief Write exactly length bytes, or return an error.
 * @note Must consume/copy the bytes before returning; buffers are reused.
 */
typedef ret_err_sts_t (*cli_transport_write_t)(const uint8_t *data,
                                                size_t length,
                                                void *context);

typedef struct
{
    cli_transport_read_t  read;   /**< Optional in push-input mode. */
    cli_transport_write_t write;  /**< Required for all modes. */
    void *context;
} cli_transport_t;

/*===========================================================================
 * COMMANDS
 *===========================================================================*/

/**
 * @brief Process a command or produce its next chunk of output.
 *
 * output has output_capacity bytes and must be NUL-terminated by the callback.
 * Return true if another invocation is needed to emit more output, otherwise
 * false. A callback may be called repeatedly with the same command_line.
 * The returned output is written through cli_transport_t.write.
 * @note Never use this callback from an interrupt.
 */
typedef bool (*cli_command_callback_t)(char *output,
                                       size_t output_capacity,
                                       const char *command_line,
                                       void *context);

/**
 * @brief Command definition (same semantics as FreeRTOS+CLI, portable types).
 */
typedef struct
{
    const char *name;             /**< Example: "log". Single token. */
    const char *help;             /**< Example: "log <level>: Set level\r\n". */
    cli_command_callback_t callback;
    int8_t expected_parameters;   /**< 0..INT8_MAX, or -1 for variable count. */
    void *context;                /**< Per-command opaque application context. */
} cli_command_t;

/*===========================================================================
 * CONFIGURATION
 *===========================================================================*/

typedef struct
{
    cli_transport_t transport;

    /* Application-owned buffers; each must remain valid until deinit. */
    char *command_buffer;         /**< Collects a command line. */
    size_t command_buffer_size;   /**< Includes space for terminating NUL. */
    char *output_buffer;          /**< Shared sequential command output buffer. */
    size_t output_buffer_size;    /**< Includes space for terminating NUL. */

    /* Optional read buffer, required only when transport.read is used. */
    uint8_t *read_buffer;
    size_t read_buffer_size;

    bool echo_input;              /**< Echo typed characters / backspace. */
    const char *prompt;           /**< NULL for no prompt, e.g. "ridecast> ". */
} cli_config_t;

/*===========================================================================
 * PUBLIC API
 *===========================================================================*/

/**
 * @brief Attach application-owned resources and initialize the CLI.
 * @return RET_ERR_STS_OK, or an error from err_sts.h.
 * @note Application initializes transport and CMSIS worker independently.
 */
ret_err_sts_t cli_init(const cli_config_t *config);

/**
 * @brief Unbind CLI resources; does not delete application-owned resources.
 * @note Stop incoming input and the CLI worker before calling.
 */
ret_err_sts_t cli_deinit(void);

/**
 * @brief Register command metadata and its handler.
 * @note Register before input processing begins, from task context.
 * @note The implementation may have a configured command-count limit.
 */
ret_err_sts_t cli_register_command(const cli_command_t *command);

/**
 * @brief Feed incoming characters (UART RX DMA/IRQ ring-buffer data, USB, etc.).
 *
 * Handles CR/LF, backspace, line accumulation, command dispatch, and multi-part
 * response writes. Process from a task, never directly from an ISR. On input
 * overflow the current line is rejected; implementation should recover on EOL.
 */
ret_err_sts_t cli_process_input(const uint8_t *data, size_t length);

/**
 * @brief Process incoming console data and execute CLI commands.
 *
 * Reads available data from the configured console transport,
 * processes received commands, and writes generated responses.
 *
 * @note Must be called from task context.
 * @note Not thread-safe. Use a single CLI processing task.
 *
 * @return ret_err_sts_t Execution status.
 */
ret_err_sts_t cli_process_console(void);

/**
 * @brief Execute an already complete, NUL-terminated command line.
 *
 * Sends all generated output chunks using the configured transport.write.
 * Does not require or modify the input command buffer.
 * @note Command line must not be changed while the callback is running.
 */
ret_err_sts_t cli_execute(const char *command_line);

/**
 * @brief Return a 1-based parameter as a non-NUL-terminated substring.
 *
 * A returned pointer refers into command_line. Use *parameter_length;
 * never assume the parameter ends in NUL. Returns NULL if not present.
 */
const char *cli_get_parameter(const char *command_line,
                              size_t parameter_index,
                              size_t *parameter_length);

/**
 * @brief Return the application-owned output buffer (for compatibility).
 * @note Not reentrant; only access from the CLI owner task.
 */
char *cli_get_output_buffer(void);
size_t cli_get_output_buffer_size(void);

#ifdef __cplusplus
}
#endif

#endif /* RIDECAST_CLI_H */
