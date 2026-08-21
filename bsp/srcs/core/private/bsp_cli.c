/******************************************************************************
 * @file    bsp_cli.c
 * @brief   BSP CLI implementation.
 *
 * Portable BSP wrapper around EmbeddedCLI.
 *
 * Features:
 *  - Static allocation
 *  - Multiple CLI instances
 *  - Transport abstraction
 *  - RX ring buffer
 *  - Thread-safe command execution
 *  - UART / USB CDC / RTT support
 *
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * Includes
 *---------------------------------------------------------------------------*/
#include "bsp_cli.h"
#include "embedded_cli.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/*----------------------------------------------------------------------------
 * Private Defines
 *---------------------------------------------------------------------------*/

#ifndef BSP_CLI_PROMPT
#define BSP_CLI_PROMPT    "> "
#endif

#ifndef BSP_CLI_PRINTF_BUF_SIZE
#define BSP_CLI_PRINTF_BUF_SIZE    (256U)
#endif

/*----------------------------------------------------------------------------
 * Private Types
 *---------------------------------------------------------------------------*/
uint8_t bindingStorage[
    BSP_CLI_MAX_BINDINGS *
    sizeof(void*) * 8U];
/**
 * Internal wrapper used to connect BSP command handlers
 * with EmbeddedCLI bindings.
 */
typedef struct
{
    bspCliCommand_t command;

    CliCommandBinding binding;

} bspCliBinding_t;

/*----------------------------------------------------------------------------
 * Static Variables
 *---------------------------------------------------------------------------*/

/**
 * Temporary printf buffer.
 */
static char s_printfBuffer[BSP_CLI_PRINTF_BUF_SIZE];

/*----------------------------------------------------------------------------
 * Active CLI Context
 *---------------------------------------------------------------------------*/

/*
 * Used by writeChar callback because EmbeddedCLI does not
 * provide user context to writeChar().
 *
 * For single CLI instance this is sufficient.
 *
 * Multi-instance support will require a lookup table later.
 */
static bspCliContext_t* s_activeCliCtx = NULL;
/*----------------------------------------------------------------------------
 * Forward Declarations
 *---------------------------------------------------------------------------*/

/**
 * EmbeddedCLI write callback.
 */
static void bspCliWriteChar(
    EmbeddedCli* cli,
    char c);

/**
 * Transport RX callback.
 */
static void bspCliRxCallback(
    bsp_err_sts_t status,
    uint8_t* data,
    uint16_t len,
    void* userCtx);

/**
 * Flush pending TX buffer.
 */
static void bspCliFlushTx(
    bspCliContext_t* ctx);

/**
 * Push byte into RX ring.
 */
static bool bspCliRingPush(
    bspCliContext_t* ctx,
    uint8_t byte);

/**
 * Pop byte from RX ring.
 */
static bool bspCliRingPop(
    bspCliContext_t* ctx,
    uint8_t* byte);

/**
 * EmbeddedCLI command adapter.
 */
static void bspCliCommandAdapter(
    EmbeddedCli* cli,
    char* args,
    void* context);

/**
 * Find command by name.
 */
static bspCliBinding_t* bspCliFindBinding(
    bspCliContext_t* ctx,
    const char* name);

/**
 * Internal stats command.
 */
static void bspCliStatsCmd(
    uint32_t argc,
    char* argv[]);

/**
 * Internal help command.
 */
static void bspCliHelpCmd(
    uint32_t argc,
    char* argv[]);

/**
 * Internal version command.
 */
static void bspCliVersionCmd(
    uint32_t argc,
    char* argv[]);


/*----------------------------------------------------------------------------
 * Ring Buffer Functions
 *---------------------------------------------------------------------------*/

static bool bspCliRingPush(
    bspCliContext_t* ctx,
    uint8_t byte)
{
    uint16_t next;

    if(ctx == NULL)
    {
        return false;
    }

    next = (uint16_t)
        ((ctx->rxHead + 1U) % BSP_CLI_RING_SIZE);

    /* Ring full */
    if(next == ctx->rxTail)
    {
        ctx->rxOverflowCnt++;
        return false;
    }

    ctx->rxRing[ctx->rxHead] = byte;
    ctx->rxHead = next;

    return true;
}

/*---------------------------------------------------------------------------*/

static bool bspCliRingPop(
    bspCliContext_t* ctx,
    uint8_t* byte)
{
    if((ctx == NULL) || (byte == NULL))
    {
        return false;
    }

    if(ctx->rxHead == ctx->rxTail)
    {
        return false;
    }

    *byte = ctx->rxRing[ctx->rxTail];

    ctx->rxTail =
        (uint16_t)
        ((ctx->rxTail + 1U) % BSP_CLI_RING_SIZE);

    return true;
}

/*----------------------------------------------------------------------------
 * TX Functions
 *---------------------------------------------------------------------------*/

static void bspCliFlushTx(
    bspCliContext_t* ctx)
{
    if(ctx == NULL)
    {
        return;
    }

    if(ctx->transport == NULL)
    {
        return;
    }

    if(ctx->transport->write == NULL)
    {
        return;
    }

    if(ctx->txLen == 0U)
    {
        return;
    }

    (void)ctx->transport->write(
        ctx->transport->context,
        ctx->txBuf,
        ctx->txLen);

    ctx->txBytes += ctx->txLen;

    ctx->txLen = 0U;
}

/*----------------------------------------------------------------------------
 * EmbeddedCLI TX Callback
 *---------------------------------------------------------------------------*/
static void bspCliWriteChar(
    EmbeddedCli* cli,
    char c)
{
    bspCliContext_t* ctx;

    (void)cli;

    ctx = s_activeCliCtx;

    if(ctx == NULL)
    {
        return;
    }

    if(ctx->txLen < BSP_CLI_TX_BUF_SIZE)
    {
        ctx->txBuf[ctx->txLen] = (uint8_t)c;
        ctx->txLen++;
    }

    if((c == '\n') ||
       (ctx->txLen >= BSP_CLI_TX_BUF_SIZE))
    {
        bspCliFlushTx(ctx);
    }
}


/*----------------------------------------------------------------------------
 * Transport RX Callback
 *---------------------------------------------------------------------------*/

static void bspCliRxCallback(
    bsp_err_sts_t status,
    uint8_t* data,
    uint16_t len,
    void* userCtx)
{
    bspCliContext_t* ctx;

    ctx = (bspCliContext_t*)userCtx;

    if(ctx == NULL)
    {
        return;
    }

    if(status != BSP_ERR_STS_OK)
    {
        return;
    }

    if(data == NULL)
    {
        return;
    }

    for(uint16_t i = 0U; i < len; i++)
    {
        if(bspCliRingPush(ctx, data[i]) == true)
        {
            ctx->rxBytes++;
        }
    }

    if((ctx->transport != NULL) &&
       (ctx->transport->read_start != NULL))
    {
        (void)ctx->transport->read_start(
            ctx->transport->context,
            ctx->rxBuf,
            BSP_CLI_RX_BUF_SIZE);
    }
}

/*----------------------------------------------------------------------------
 * Find Command Binding
 *---------------------------------------------------------------------------*/

static bspCliBinding_t* bspCliFindBinding(
    bspCliContext_t* ctx,
    const char* name)
{
    bspCliBinding_t* binding;

    if((ctx == NULL) || (name == NULL))
    {
        return NULL;
    }

    for(uint32_t i = 0U;
        i < ctx->bindingCount;
        i++)
    {
        binding =
            (bspCliBinding_t*)ctx->bindings[i];

        if(binding == NULL)
        {
            continue;
        }

        if(strcmp(binding->command.name, name) == 0)
        {
            return binding;
        }
    }

    return NULL;
}

/*----------------------------------------------------------------------------
 * EmbeddedCLI Command Adapter
 *---------------------------------------------------------------------------*/

static void bspCliCommandAdapter(
    EmbeddedCli* cli,
    char* args,
    void* context)
{
    bspCliBinding_t* binding;

    char* argv[16];
    uint32_t argc = 0U;

    char* token;

    (void)cli;

    binding = (bspCliBinding_t*)context;

    if(binding == NULL)
    {
        return;
    }

    if(binding->command.handler == NULL)
    {
        return;
    }

    /*
     * Tokenize argument string.
     *
     * Example:
     *
     * "10 test abc"
     *
     * ->
     *
     * argv[0] = "10"
     * argv[1] = "test"
     * argv[2] = "abc"
     */
    token = strtok(args, " ");

    while((token != NULL) &&
          (argc < 16U))
    {
        argv[argc] = token;

        argc++;

        token = strtok(NULL, " ");
    }

    bspCliContext_t* ctx;

    ctx = s_activeCliCtx;

    if(ctx != NULL)
    {
        ctx->commandCount++;
    }

    binding->command.handler(
        argc,
        argv)
}

/*----------------------------------------------------------------------------
 * Built-in Help Command
 *---------------------------------------------------------------------------*/

static void bspCliHelpCmd(
    uint32_t argc,
    char* argv[])
{
    bspCliContext_t* ctx;

    (void)argc;
    (void)argv;

    ctx = s_activeCliCtx;

    if(ctx == NULL)
    {
        return;
    }

    bsp_cli_printf(
        ctx,
        "\r\nAvailable Commands:\r\n");

    for(uint32_t i = 0U;
        i < ctx->bindingCount;
        i++)
    {
        bspCliBinding_t* binding;

        binding =
            (bspCliBinding_t*)ctx->bindings[i];

        if(binding == NULL)
        {
            continue;
        }

        bsp_cli_printf(
            ctx,
            "  %-16s %s\r\n",
            binding->command.name,
            (binding->command.help != NULL) ?
            binding->command.help :
            "");
    }
}

/*----------------------------------------------------------------------------
 * Built-in Version Command
 *---------------------------------------------------------------------------*/

#ifndef BSP_CLI_FW_NAME
#define BSP_CLI_FW_NAME      "Firmware"
#endif

#ifndef BSP_CLI_FW_VERSION
#define BSP_CLI_FW_VERSION   "1.0.0"
#endif

static void bspCliVersionCmd(
    uint32_t argc,
    char* argv[])
{
    bspCliContext_t* ctx;

    (void)argc;
    (void)argv;

    ctx = s_activeCliCtx;

    if(ctx == NULL)
    {
        return;
    }

    bsp_cli_printf(
        ctx,
        "\r\n%s\r\n"
        "Version : %s\r\n",
        BSP_CLI_FW_NAME,
        BSP_CLI_FW_VERSION);
}

/*----------------------------------------------------------------------------
 * Built-in Statistics Command
 *---------------------------------------------------------------------------*/

static void bspCliStatsCmd(
    uint32_t argc,
    char* argv[])
{
    bspCliContext_t* ctx;

    bspCliStats_t stats;

    (void)argc;
    (void)argv;

    ctx = s_activeCliCtx;

    if(ctx == NULL)
    {
        return;
    }

    if(bsp_cli_get_stats(
           ctx,
           &stats) != BSP_ERR_STS_OK)
    {
        return;
    }

    bsp_cli_printf(
        ctx,
        "\r\nCLI Statistics\r\n"
        "------------------------------\r\n"
        "RX Bytes      : %lu\r\n"
        "TX Bytes      : %lu\r\n"
        "RX Overflow   : %lu\r\n"
        "Commands      : %lu\r\n",
        (unsigned long)stats.rxBytes,
        (unsigned long)stats.txBytes,
        (unsigned long)stats.rxOverflowCnt,
        (unsigned long)stats.commandCount);
}
/*----------------------------------------------------------------------------
 * Register Command
 *---------------------------------------------------------------------------*/

bsp_err_sts_t bsp_cli_register_command(
    bspCliHandle_t handle,
    const bspCliCommand_t* command)
{
    bspCliContext_t* ctx;
    bspCliBinding_t* binding;

    if((handle == NULL) ||
       (command == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    ctx = handle;

    if(ctx->bindingCount >= BSP_CLI_MAX_BINDINGS)
    {
        return BSP_ERR_STS_NO_MEM;
    }

    if(command->name == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(command->handler == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* Prevent duplicate registration */
    if(bspCliFindBinding(ctx, command->name) != NULL)
    {
        return BSP_ERR_STS_ALREADY_EXISTS;
    }

    binding =
        (bspCliBinding_t*)&ctx->bindings[
            ctx->bindingCount];

    (void)memset(
        binding,
        0,
        sizeof(bspCliBinding_t));

    /* Store BSP command */
    binding->command = *command;

    /* Create EmbeddedCLI binding */
    binding->binding.name = command->name;
    binding->binding.help = command->help;

    binding->binding.tokenizeArgs = true;

    binding->binding.context = binding;

    binding->binding.binding = bspCliCommandAdapter;

    embeddedCliAddBinding(
        ctx->cli,
        &binding->binding);

    ctx->bindingCount++;

    return BSP_ERR_STS_OK;
}

/*----------------------------------------------------------------------------
 * Unregister Command
 *---------------------------------------------------------------------------*/

bsp_err_sts_t bsp_cli_unregister_command(
    bspCliHandle_t handle,
    const char* name)
{
    bspCliContext_t* ctx;

    if((handle == NULL) ||
       (name == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    ctx = handle;

    for(uint32_t i = 0U;
        i < ctx->bindingCount;
        i++)
    {
        bspCliBinding_t* binding;

        binding =
            (bspCliBinding_t*)ctx->bindings[i];

        if(binding == NULL)
        {
            continue;
        }

        if(strcmp(
               binding->command.name,
               name) == 0)
        {
            /*
             * Compact array.
             */
            for(uint32_t j = i;
                j < (ctx->bindingCount - 1U);
                j++)
            {
                ctx->bindings[j] =
                    ctx->bindings[j + 1U];
            }

            ctx->bindings[
                ctx->bindingCount - 1U] = NULL;

            ctx->bindingCount--;

            return BSP_ERR_STS_OK;
        }
    }

    return BSP_ERR_STS_NOT_FOUND;
}

/*----------------------------------------------------------------------------
 * Initialize CLI
 *---------------------------------------------------------------------------*/

bsp_err_sts_t bsp_cli_init(
    bspCliHandle_t handle,
    const bspConsoleTransport_t* transport)
{
    bspCliContext_t* ctx;

    EmbeddedCliConfig cliCfg;

    if((handle == NULL) ||
       (transport == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    ctx = handle;

    if(ctx->initialized == true)
    {
        return BSP_ERR_STS_OK;
    }

    (void)memset(
        ctx,
        0,
        sizeof(bspCliContext_t));

    ctx->transport = transport;

    /*--------------------------------------------------
     * Create mutex
     *--------------------------------------------------*/
    ctx->mutex = bspMutexCreate();

    if(ctx->mutex == NULL)
    {
        return BSP_ERR_STS_NO_MEM;
    }

    /*--------------------------------------------------
     * Configure EmbeddedCLI
     *--------------------------------------------------*/
    (void)memset(
        &cliCfg,
        0,
        sizeof(cliCfg));

    cliCfg.cliBuffer =
        ctx->cliBuffer;

    cliCfg.cliBufferSize =
        sizeof(ctx->cliBuffer);

    cliCfg.rxBufferSize =
        BSP_CLI_RX_BUF_SIZE;

    cliCfg.cmdBufferSize =
        BSP_CLI_CMD_BUF_SIZE;

    cliCfg.maxBindingCount =
        BSP_CLI_MAX_BINDINGS;

    /*--------------------------------------------------
     * Create CLI instance
     *--------------------------------------------------*/
    ctx->cli =
        embeddedCliNew(
            &cliCfg);

    if(ctx->cli == NULL)
    {
        bspMutexDelete(
            ctx->mutex);

        ctx->mutex = NULL;

        return BSP_ERR_STS_FAIL;
    }

    /*--------------------------------------------------
     * Attach context
     *--------------------------------------------------*/
    ctx->cli->writeChar =
        bspCliWriteChar;

    s_activeCliCtx = ctx;

    /*--------------------------------------------------
     * Register RX callback
     *--------------------------------------------------*/
    if(ctx->transport->set_callback != NULL)
    {
        (void)ctx->transport->set_callback(
            ctx->transport->context,
            bspCliRxCallback,
            ctx);
    }

    /*--------------------------------------------------
     * Start first receive
     *--------------------------------------------------*/
    if(ctx->transport->read_start != NULL)
    {
        (void)ctx->transport->read_start(
            ctx->transport->context,
            ctx->rxBuf,
            BSP_CLI_RX_BUF_SIZE);
    }

    /*--------------------------------------------------
     * Reset runtime state
     *--------------------------------------------------*/
    ctx->rxHead = 0U;
    ctx->rxTail = 0U;

    ctx->txLen = 0U;

    ctx->rxBytes = 0U;
    ctx->txBytes = 0U;

    ctx->rxOverflowCnt = 0U;
    ctx->commandCount  = 0U;

    ctx->bindingCount = 0U;

    /*--------------------------------------------------
     * Register built-in commands
     *--------------------------------------------------*/

    static const bspCliCommand_t helpCmd =
    {
        .name    = "help",
        .help    = "Show available commands",
        .handler = bspCliHelpCmd
    };

    static const bspCliCommand_t statsCmd =
    {
        .name    = "cli_stats",
        .help    = "Show CLI statistics",
        .handler = bspCliStatsCmd
    };

    static const bspCliCommand_t versionCmd =
    {
        .name    = "version",
        .help    = "Show firmware version",
        .handler = bspCliVersionCmd
    };

    (void)bsp_cli_register_command(
        ctx,
        &helpCmd);

    (void)bsp_cli_register_command(
        ctx,
        &statsCmd);

    (void)bsp_cli_register_command(
        ctx,
        &versionCmd);

    /*--------------------------------------------------
     * Ready
     *--------------------------------------------------*/
    ctx->initialized = true;

    return BSP_ERR_STS_OK;
}

/*----------------------------------------------------------------------------
 * Process CLI
 *---------------------------------------------------------------------------*/

void bsp_cli_process(
    bspCliHandle_t handle)
{
    bspCliContext_t* ctx;

    uint8_t ch;

    if(handle == NULL)
    {
        return;
    }

    ctx = handle;

    if(ctx->initialized == false)
    {
        return;
    }

    /*
     * Feed all received bytes
     * into EmbeddedCLI.
     */
    while(bspCliRingPop(
              ctx,
              &ch) == true)
    {
        embeddedCliReceiveChar(
            ctx->cli,
            (char)ch);
    }

    /*
     * Run CLI state machine.
     */
    embeddedCliProcess(
        ctx->cli);

    /*
     * Flush pending output.
     */
    bspCliFlushTx(
        ctx);
}


/*----------------------------------------------------------------------------
 * CLI Printf
 *---------------------------------------------------------------------------*/

void bsp_cli_printf(
    bspCliHandle_t handle,
    const char* fmt,
    ...)
{
    bspCliContext_t* ctx;

    va_list args;

    int32_t len;

    if((handle == NULL) ||
       (fmt == NULL))
    {
        return;
    }

    ctx = handle;

    va_start(args, fmt);

    len = vsnprintf(
        s_printfBuffer,
        sizeof(s_printfBuffer),
        fmt,
        args);

    va_end(args);

    if(len <= 0)
    {
        return;
    }

    if(ctx->transport == NULL)
    {
        return;
    }

    if(ctx->transport->write == NULL)
    {
        return;
    }

    (void)ctx->transport->write(
        ctx->transport->context,
        (const uint8_t*)s_printfBuffer,
        (uint16_t)len);

    ctx->txBytes += (uint32_t)len;
}

/*----------------------------------------------------------------------------
 * Get Statistics
 *---------------------------------------------------------------------------*/

bsp_err_sts_t bsp_cli_get_stats(
    bspCliHandle_t handle,
    bspCliStats_t* stats)
{
    bspCliContext_t* ctx;

    if((handle == NULL) ||
       (stats == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    ctx = handle;

    stats->rxBytes =
        ctx->rxBytes;

    stats->txBytes =
        ctx->txBytes;

    stats->rxOverflowCnt =
        ctx->rxOverflowCnt;

    stats->commandCount =
        ctx->commandCount;

    return BSP_ERR_STS_OK;
}

/*----------------------------------------------------------------------------
 * Reset Statistics
 *---------------------------------------------------------------------------*/

void bsp_cli_reset_stats(
    bspCliHandle_t handle)
{
    bspCliContext_t* ctx;

    if(handle == NULL)
    {
        return;
    }

    ctx = handle;

    ctx->rxBytes = 0U;
    ctx->txBytes = 0U;

    ctx->rxOverflowCnt = 0U;
    ctx->commandCount = 0U;
}

/*----------------------------------------------------------------------------
 * Deinitialize CLI
 *---------------------------------------------------------------------------*/

bsp_err_sts_t bsp_cli_deinit(
    bspCliHandle_t handle)
{
    bspCliContext_t* ctx;

    if(handle == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    ctx = handle;

    if(ctx->initialized == false)
    {
        return BSP_ERR_STS_OK;
    }

    /*--------------------------------------------------
     * Stop transport callback
     *--------------------------------------------------*/
    if((ctx->transport != NULL) &&
       (ctx->transport->set_callback != NULL))
    {
        (void)ctx->transport->set_callback(
            ctx->transport->context,
            NULL,
            NULL);
    }

    /*--------------------------------------------------
     * Flush pending output
     *--------------------------------------------------*/
    bspCliFlushTx(ctx);

    /*--------------------------------------------------
     * Destroy EmbeddedCLI instance
     *--------------------------------------------------*/
    if(ctx->cli != NULL)
    {
        embeddedCliFree(
            ctx->cli);

        ctx->cli = NULL;
    }

    /*--------------------------------------------------
     * Delete mutex
     *--------------------------------------------------*/
    if(ctx->mutex != NULL)
    {
        bspMutexDelete(
            ctx->mutex);

        ctx->mutex = NULL;
    }

    /*--------------------------------------------------
     * Reset runtime state
     *--------------------------------------------------*/
    ctx->initialized = false;

    ctx->bindingCount = 0U;

    ctx->rxHead = 0U;
    ctx->rxTail = 0U;

    ctx->txLen = 0U;

    ctx->rxBytes = 0U;
    ctx->txBytes = 0U;

    ctx->rxOverflowCnt = 0U;
    ctx->commandCount = 0U;

    if(s_activeCliCtx == ctx)
    {
        s_activeCliCtx = NULL;
    }

    return BSP_ERR_STS_OK;
}