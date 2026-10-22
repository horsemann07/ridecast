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
#include "bsp_config.h"
#include "bsp_cli.h"
#include "embedded_cli.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/*----------------------------------------------------------------------------
 * Private Defines
 *---------------------------------------------------------------------------*/
#define BSP_CLI_PROMPT                bspCONFIG_CLI_PROMPT
#define BSP_CLI_PRINTF_BUF_SIZE       bspCONFIG_CLI_PRINTF_BUF_SIZE
#define BSP_CLI_MAX_ARGS              bspCONFIG_CLI_MAX_ARGS
#define BSP_CLI_RX_BUF_SIZE           bspCONFIG_CLI_RX_BUF_SIZE
#define BSP_CLI_TX_BUF_SIZE           bspCONFIG_CLI_TX_BUF_SIZE
#define BSP_CLI_CMD_BUF_SIZE          bspCONFIG_CLI_CMD_BUF_SIZE
#define BSP_CLI_RING_SIZE             bspCONFIG_CLI_RING_SIZE
#define BSP_CLI_MAX_BINDINGS          bspCONFIG_CLI_MAX_BINDINGS
#define BSP_CLI_MUTEX_TIMEOUT_MS      bspCONFIG_CLI_MUTEX_TIMEOUT_MS
#define BSP_CLI_FW_NAME               bspCONFIG_CLI_FW_NAME
#define BSP_CLI_FW_VERSION            bspCONFIG_CLI_FW_VERSION
#define BSP_CLI_AUTOCOMPLETE          bspCONFIG_CLI_AUTOCOMPLETE
#define BSP_CLI_EMBEDDED_BUFFER_WORDS bspCONFIG_CLI_EMBEDDED_BUFFER_WORDS
/*----------------------------------------------------------------------------
 * Private data types
 *---------------------------------------------------------------------------*/
typedef struct
{
    bspCliCommand_t command;
    CliCommandBinding embeddedBinding;
    struct bspCliContext* owner;
} bspCliBinding_t;

typedef struct
{
    uint32_t rxBytes;
    uint32_t txBytes;
    uint32_t rxOverflowCnt;
    uint32_t commandCount;

} bspCliStats_t;

struct bspCliContext
{
    EmbeddedCli* cli;
    const bspConsoleTransport_t* transport;
    bspMutexHandle_t mutex;
    bool initialized;

    uint8_t rxBuf[BSP_CLI_RX_BUF_SIZE];
    uint8_t txBuf[BSP_CLI_TX_BUF_SIZE];
    uint16_t txLen;

    uint8_t rxRing[BSP_CLI_RING_SIZE];
    volatile uint16_t rxHead;
    volatile uint16_t rxTail;

    uint8_t inputLine[BSP_CLI_CMD_BUF_SIZE];
    uint16_t inputLineLen;
    uint16_t pendingHelpInfo;

    volatile uint32_t rxBytes;
    volatile uint32_t txBytes;
    volatile uint32_t rxOverflowCnt;
    volatile uint32_t commandCount;

    CLI_UINT cliBuffer[BSP_CLI_EMBEDDED_BUFFER_WORDS];
    bspCliBinding_t bindings[BSP_CLI_MAX_BINDINGS];
    uint32_t bindingCount;
};

/*----------------------------------------------------------------------------
 * Static Variables
 *---------------------------------------------------------------------------*/
static bspCliContext_t s_consoleCliContext;

/*----------------------------------------------------------------------------
 * Forward Declarations
 *---------------------------------------------------------------------------*/

static void bspCliWriteChar(EmbeddedCli* cli, char c);
static void bspCliRxCallback(uint8_t* data, size_t len, bsp_err_sts_t status, void* userCtx);
static void bspCliCommandAdapter(EmbeddedCli* cli, char* args, void* context);

static bool bspCliRingPush(bspCliContext_t* ctx, uint8_t byte);
static bool bspCliRingPop(bspCliContext_t* ctx, uint8_t* byte);
static bool bspCliFlushTx(bspCliContext_t* ctx);
static bspCliBinding_t* bspCliFindBinding(bspCliContext_t* ctx, const char* name);
static bsp_err_sts_t bspCliAddCommand(bspCliContext_t* ctx, const bspCliCommand_t* command);
static bsp_err_sts_t bspCliCreateEmbedded(bspCliContext_t* ctx);
static bsp_err_sts_t bspCliRebuildEmbedded(bspCliContext_t* ctx);

/*----------------------------------------------------------------------------
 * Private Functions
 *---------------------------------------------------------------------------*/
static bool bspCliRingPush(bspCliContext_t* ctx, uint8_t byte)
{
    uint16_t next;

    if(ctx == NULL)
    {
        return false;
    }

    next = (uint16_t)((ctx->rxHead + 1U) % BSP_CLI_RING_SIZE);

    if(next == ctx->rxTail)
    {
        ctx->rxOverflowCnt++;
        return false;
    }

    ctx->rxRing[ctx->rxHead] = byte;
    ctx->rxHead              = next;

    return true;
}

/*---------------------------------------------------------------------------*/
static bool bspCliRingPop(bspCliContext_t* ctx, uint8_t* byte)
{
    if((ctx == NULL) || (byte == NULL) || (ctx->rxHead == ctx->rxTail))
    {
        return false;
    }

    *byte       = ctx->rxRing[ctx->rxTail];
    ctx->rxTail = (uint16_t)((ctx->rxTail + 1U) % BSP_CLI_RING_SIZE);

    return true;
}
/*---------------------------------------------------------------------------*/
static bool bspCliFlushTx(bspCliContext_t* ctx)
{
    bsp_err_sts_t status;

    if((ctx == NULL) || (ctx->transport == NULL) ||
       (ctx->transport->write == NULL) || (ctx->txLen == 0U))
    {
        return true;
    }

    status = ctx->transport->write(ctx->transport->context, ctx->txBuf, ctx->txLen);

    if(status != PLT_ERR_STS_OK)
    {
        return false;
    }

    ctx->txBytes += ctx->txLen;
    ctx->txLen = 0U;

    return true;
}

/*----------------------------------------------------------------------------
 * EmbeddedCLI TX Callback
 *---------------------------------------------------------------------------*/
static void bspCliWriteChar(EmbeddedCli* cli, char c)
{
    bspCliContext_t* ctx;

    if(cli == NULL)
    {
        return;
    }

    ctx = (bspCliContext_t*)cli->appContext;

    if(ctx == NULL)
    {
        return;
    }

    if(ctx->txLen >= BSP_CLI_TX_BUF_SIZE)
    {
        if(bspCliFlushTx(ctx) == false)
        {
            return;
        }
    }

    ctx->txBuf[ctx->txLen++] = (uint8_t)c;

    if((c == '\n') || (ctx->txLen >= BSP_CLI_TX_BUF_SIZE))
    {
        (void)bspCliFlushTx(ctx);
    }
}

/*----------------------------------------------------------------------------
 * Transport RX Callback
 *---------------------------------------------------------------------------*/
static void bspCliRxCallback(uint8_t* data, size_t len, bsp_err_sts_t status, void* userCtx)
{
    bspCliContext_t* ctx = (bspCliContext_t*)userCtx;
    size_t i;

    if(ctx == NULL)
    {
        return;
    }

    if((status == PLT_ERR_STS_OK) && (data != NULL))
    {
        for(i = 0U; i < len; i++)
        {
            if(bspCliRingPush(ctx, data[i]))
            {
                ctx->rxBytes++;
            }
        }
    }

    if((ctx->initialized == true) && (ctx->transport != NULL) &&
       (ctx->transport->read_start != NULL))
    {
        (void)ctx->transport->read_start(ctx->transport->context, ctx->rxBuf, BSP_CLI_RX_BUF_SIZE);
    }
}


/*----------------------------------------------------------------------------*/
static bspCliBinding_t* bspCliFindBinding(bspCliContext_t* ctx, const char* name)
{
    uint32_t i;

    if((ctx == NULL) || (name == NULL))
    {
        return NULL;
    }

    for(i = 0U; i < ctx->bindingCount; i++)
    {
        if(strcmp(ctx->bindings[i].command.name, name) == 0)
        {
            return &ctx->bindings[i];
        }
    }

    return NULL;
}
/*----------------------------------------------------------------------------*/
static bool bspCliIsHelpCommand(const char* line, uint16_t length)
{
    uint16_t start = 0U;
    uint16_t end   = length;

    while((start < end) && ((line[start] == ' ') || (line[start] == '\t')))
    {
        start++;
    }

    while((end > start) && ((line[end - 1U] == ' ') || (line[end - 1U] == '\t')))
    {
        end--;
    }

    return ((end - start) == 4U) && (memcmp(&line[start], "help", 4U) == 0);
}
/*----------------------------------------------------------------------------*/
static void bspCliTrackInput(bspCliContext_t* ctx, uint8_t ch)
{
    if((ch == '\r') || (ch == '\n'))
    {
        if(bspCliIsHelpCommand(ctx->inputLine, ctx->inputLineLen))
        {
            ctx->pendingHelpInfo++;
        }

        ctx->inputLineLen = 0U;
        return;
    }

    if((ch == '\b') || (ch == 0x7FU))
    {
        if(ctx->inputLineLen > 0U)
        {
            ctx->inputLineLen--;
        }

        return;
    }

    if((ch == 0x15U) || (ch == 0x1BU)) /* Ctrl-U or Escape */
    {
        ctx->inputLineLen = 0U;
        return;
    }

    if((ch >= 0x20U) && (ctx->inputLineLen < (BSP_CLI_CMD_BUF_SIZE - 1U)))
    {
        ctx->inputLine[ctx->inputLineLen++] = (char)ch;
    }
}
/*----------------------------------------------------------------------------*/
static void bspCliPrintInfo(bspCliContext_t* ctx)
{
    bspCliStats_t stats;

    if(bsp_cli_get_stats(ctx, &stats) != PLT_ERR_STS_OK)
    {
        return;
    }

    bsp_cli_printf(ctx,
                   "\r\nFirmware : %s\r\n"
                   "Version  : %s\r\n"
                   "RX Bytes : %lu\r\n"
                   "TX Bytes : %lu\r\n"
                   "Overflow : %lu\r\n"
                   "Commands : %lu\r\n",
                   BSP_CLI_FW_NAME, BSP_CLI_FW_VERSION, (unsigned long)stats.rxBytes,
                   (unsigned long)stats.txBytes, (unsigned long)stats.rxOverflowCnt,
                   (unsigned long)stats.commandCount);
}
/*----------------------------------------------------------------------------*/
static void bspCliCommandAdapter(EmbeddedCli* cli, char* args, void* context)
{
    bspCliBinding_t* binding = (bspCliBinding_t*)context;
    bspCliContext_t* ctx;
    char* argv[BSP_CLI_MAX_ARGS];
    uint32_t argc = 0U;
    char* cursor;

    (void)cli;

    if((binding == NULL) || (binding->owner == NULL))
    {
        return;
    }

    ctx = binding->owner;
    ctx->commandCount++;

    if(binding->command.handler == NULL)
    {
        return;
    }

    cursor = args;

    while((cursor != NULL) && (*cursor != '\0') && (argc < BSP_CLI_MAX_ARGS))
    {
        while((*cursor == ' ') || (*cursor == '\t'))
        {
            cursor++;
        }

        if(*cursor == '\0')
        {
            break;
        }

        argv[argc++] = cursor;

        while((*cursor != '\0') && (*cursor != ' ') && (*cursor != '\t'))
        {
            cursor++;
        }

        if(*cursor != '\0')
        {
            *cursor = '\0';
            cursor++;
        }
    }

    binding->command.handler(argc, argv);
}
/*----------------------------------------------------------------------------*/
static bsp_err_sts_t bspCliCreateEmbedded(bspCliContext_t* ctx)
{
    EmbeddedCliConfig config;
    uint32_t i;

    (void)memset(&config, 0, sizeof(config));

    config.invitation         = BSP_CLI_PROMPT;
    config.rxBufferSize       = BSP_CLI_RX_BUF_SIZE;
    config.cmdBufferSize      = BSP_CLI_CMD_BUF_SIZE;
    config.historyBufferSize  = BSP_CLI_CMD_BUF_SIZE;
    config.maxBindingCount    = BSP_CLI_MAX_BINDINGS;
    config.cliBuffer          = ctx->cliBuffer;
    config.cliBufferSize      = sizeof(ctx->cliBuffer);
    config.enableAutoComplete = BSP_CLI_AUTOCOMPLETE;

    if(embeddedCliRequiredSize(&config) > sizeof(ctx->cliBuffer))
    {
        return PLT_ERR_STS_NO_MEM;
    }

    ctx->cli = embeddedCliNew(&config);

    if(ctx->cli == NULL)
    {
        return PLT_ERR_STS_FAIL;
    }

    ctx->cli->writeChar  = bspCliWriteChar;
    ctx->cli->appContext = ctx;

    for(i = 0U; i < ctx->bindingCount; i++)
    {
        if(embeddedCliAddBinding(ctx->cli, ctx->bindings[i].embeddedBinding) == false)
        {
            embeddedCliFree(ctx->cli);
            ctx->cli = NULL;
            return PLT_ERR_STS_NO_MEM;
        }
    }

    return PLT_ERR_STS_OK;
}
/*----------------------------------------------------------------------------*/
static bsp_err_sts_t bspCliRebuildEmbedded(bspCliContext_t* ctx)
{
    if(ctx->cli != NULL)
    {
        embeddedCliFree(ctx->cli);
        ctx->cli = NULL;
    }

    return bspCliCreateEmbedded(ctx);
}
/*----------------------------------------------------------------------------*/
static bsp_err_sts_t bspCliAddCommand(bspCliContext_t* ctx, const bspCliCommand_t* command)
{
    bspCliBinding_t* binding;

    if((ctx == NULL) || (command == NULL) || (command->name == NULL))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }


    if(ctx->bindingCount >= BSP_CLI_MAX_BINDINGS)
    {
        return PLT_ERR_STS_NO_MEM;
    }

    if(bspCliFindBinding(ctx, command->name) != NULL)
    {
        return PLT_ERR_STS_ALREADY_EXISTS;
    }

    binding = &ctx->bindings[ctx->bindingCount];
    (void)memset(binding, 0, sizeof(*binding));

    binding->command                      = *command;
    binding->owner                        = ctx;
    binding->embeddedBinding.name         = command->name;
    binding->embeddedBinding.help         = command->description;
    binding->embeddedBinding.tokenizeArgs = false;
    binding->embeddedBinding.context      = binding;
    binding->embeddedBinding.binding      = bspCliCommandAdapter;

    if((ctx->cli != NULL) &&
       (embeddedCliAddBinding(ctx->cli, binding->embeddedBinding) == false))
    {
        return PLT_ERR_STS_NO_MEM;
    }

    ctx->bindingCount++;
    return PLT_ERR_STS_OK;
}

/*----------------------------------------------------------------------------
 * Public Functions
 *---------------------------------------------------------------------------*/
bsp_err_sts_t bsp_cli_register_command(bspCliHandle_t handle, const bspCliCommand_t* command)
{
    /* A handle is required before accessing instance members. */
    if(handle == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    /* Commands can only be added after EmbeddedCLI is created. */
    if(handle->initialized == false)
    {
        return PLT_ERR_STS_NOT_INIT;
    }

    /*
     * bspCliAddCommand() validates the command, prevents duplicate names,
     * stores the BSP command binding, and registers it with EmbeddedCLI.
     */
    return bspCliAddCommand(handle, command);
}
/*----------------------------------------------------------------------------*/
bsp_err_sts_t bsp_cli_unregister_command(bspCliHandle_t handle, const char* name)
{
    bspCliContext_t* ctx = handle;
    uint32_t i;
    bsp_err_sts_t status;

    if((ctx == NULL) || (name == NULL))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_NOT_INIT;
    }

    /* Find the command entry in the BSP command-binding array. */
    for(i = 0U; i < ctx->bindingCount; i++)
    {
        if(strcmp(ctx->bindings[i].command.name, name) == 0)
        {
            /*
             * Remove the entry by shifting all following bindings down by
             * one slot. memmove() is required because source and destination
             * memory regions overlap.
             */
            (void)memmove(&ctx->bindings[i], &ctx->bindings[i + 1U],
                          (ctx->bindingCount - i - 1U) * sizeof(ctx->bindings[0]));

            ctx->bindingCount--;

            /*
             * Binding addresses changed after memmove(). Update each owner
             * and EmbeddedCLI callback context to its new array address.
             */
            for(; i < ctx->bindingCount; i++)
            {
                ctx->bindings[i].owner                   = ctx;
                ctx->bindings[i].embeddedBinding.context = &ctx->bindings[i];
            }

            /* Clear the now-unused final array entry. */
            (void)memset(&ctx->bindings[ctx->bindingCount], 0, sizeof(ctx->bindings[0]));

            /*
             * EmbeddedCLI copies bindings internally and cannot remove one.
             * Recreate it so its internal list exactly matches BSP bindings.
             */
            status = bspCliRebuildEmbedded(ctx);
            return status;
        }
    }

    return PLT_ERR_STS_NOT_FOUND;
}
/*----------------------------------------------------------------------------*/
bsp_err_sts_t bsp_cli_init(bspCliHandle_t handle, const bspConsoleTransport_t* transport)
{
    bspCliContext_t* ctx = handle;
    bsp_err_sts_t status;

    /*
     * write() is mandatory because both EmbeddedCLI output and bsp_cli_printf()
     * use it. A ring size below two cannot distinguish full from empty.
     */
    if((ctx == NULL) || (transport == NULL) || (transport->write == NULL) ||
       (BSP_CLI_RING_SIZE < 2U))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    /* Reinitializing an active instance is harmless. */
    if(ctx->initialized == true)
    {
        return PLT_ERR_STS_OK;
    }

    /* Reset runtime state and save the transport supplied by the application. */
    (void)memset(ctx, 0, sizeof(*ctx));
    ctx->transport = transport;

    /* Create synchronization object used by the CLI implementation. */
    ctx->mutex = bspMutexCreateForCli();

    if(ctx->mutex == NULL)
    {
        return PLT_ERR_STS_NO_MEM;
    }

    /*
     * Create EmbeddedCLI using ctx->cliBuffer. No heap allocation is used
     * when bspCliCreateEmbedded() supplies this static buffer.
     */
    status = bspCliCreateEmbedded(ctx);

    if(status != PLT_ERR_STS_OK)
    {
        if(ctx->cli != NULL)
        {
            embeddedCliFree(ctx->cli);
            ctx->cli = NULL;
        }

        bspMutexDelete(ctx->mutex);
        ctx->mutex = NULL;

        return status;
    }

    /*
     * Mark initialized before enabling receive. An RX callback may occur
     * immediately after read_start() returns.
     */
    ctx->initialized = true;

    /*
     * Register the callback that copies transport RX data into the CLI ring
     * buffer. It does not execute commands in interrupt/callback context.
     */
    if(transport->set_callback != NULL)
    {
        status = transport->set_callback(transport->context, bspCliRxCallback, ctx);

        if(status != PLT_ERR_STS_OK)
        {
            (void)bsp_cli_deinit(ctx);
            return status;
        }
    }

    /*
     * Start the first asynchronous receive. bspCliRxCallback() starts the
     * next receive after each completed transfer.
     */
    if(transport->read_start != NULL)
    {
        status = transport->read_start(transport->context, ctx->rxBuf, BSP_CLI_RX_BUF_SIZE);

        if(status != PLT_ERR_STS_OK)
        {
            (void)bsp_cli_deinit(ctx);
            return status;
        }
    }

    return PLT_ERR_STS_OK;
}

/*----------------------------------------------------------------------------*/
void bsp_cli_process(bspCliHandle_t handle)
{
    uint8_t ch;

    /* Validate that the CLI instance exists and is initialized. */
    if((handle == NULL) || (handle->initialized == false))
    {
        return;
    }

    /*
     * Move all received transport bytes from the BSP ring buffer into
     * EmbeddedCLI's input buffer.
     */
    while(bspCliRingPop(handle, &ch))
    {
        /*
         * Detect a completed "help" line. This does not print anything;
         * it only increments pendingHelpInfo when "help" is entered.
         */
        bspCliTrackInput(handle, ch);

        /* Give the received character to EmbeddedCLI. */
        embeddedCliReceiveChar(handle->cli, (char)ch);
    }

    /*
     * Run EmbeddedCLI's command parser and callbacks.
     *
     * For "help", EmbeddedCLI prints its normal command list here.
     */
    embeddedCliProcess(handle->cli);

    /*
     * Print BSP-specific information after EmbeddedCLI has finished
     * printing help. bspCliPrintInfo() prints firmware name, firmware
     * version, and CLI statistics.
     */
    while(handle->pendingHelpInfo > 0U)
    {
        handle->pendingHelpInfo--;
        bspCliPrintInfo(handle);
    }

    /*
     * Send any characters accumulated by EmbeddedCLI output callbacks
     * to the configured UART/USB/RTT transport.
     */
    (void)bspCliFlushTx(handle);
}

/*----------------------------------------------------------------------------*/
void bsp_cli_printf(bspCliHandle_t handle, const char* fmt, ...)
{
    char buffer[BSP_CLI_PRINTF_BUF_SIZE];
    va_list args;
    int length;

    /* Validate CLI state and format string before formatting output. */
    if((handle == NULL) || (fmt == NULL) || (handle->initialized == false))
    {
        return;
    }

    /* Format user output into a bounded local buffer. */
    va_start(args, fmt);
    length = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    /* No output was generated, or formatting failed. */
    if(length <= 0)
    {
        return;
    }

    /*
     * vsnprintf() returns the required length when output is truncated.
     * Clamp it to the actual buffer length before passing it to transport.
     */
    if((size_t)length >= sizeof(buffer))
    {
        length = (int)(sizeof(buffer) - 1U);
    }

    /* Send output and count only bytes accepted by the transport. */
    if(handle->transport->write(handle->transport->context, (const uint8_t*)buffer,
                                (uint16_t)length) == PLT_ERR_STS_OK)
    {
        handle->txBytes += (uint32_t)length;
    }
}

/*----------------------------------------------------------------------------*/
bsp_err_sts_t bsp_cli_get_stats(bspCliHandle_t handle, bspCliStats_t* stats)
{
    if((handle == NULL) || (stats == NULL))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    /* Copy counters so the caller receives one statistics snapshot. */
    stats->rxBytes       = handle->rxBytes;
    stats->txBytes       = handle->txBytes;
    stats->rxOverflowCnt = handle->rxOverflowCnt;
    stats->commandCount  = handle->commandCount;

    return PLT_ERR_STS_OK;
}

/*----------------------------------------------------------------------------*/
void bsp_cli_reset_stats(bspCliHandle_t handle)
{
    if(handle == NULL)
    {
        return;
    }

    handle->rxBytes       = 0U;
    handle->txBytes       = 0U;
    handle->rxOverflowCnt = 0U;
    handle->commandCount  = 0U;
}

/*----------------------------------------------------------------------------*/
bsp_err_sts_t bsp_cli_deinit(bspCliHandle_t handle)
{
    bspCliContext_t* ctx = handle;

    if(ctx == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    /* Deinitializing an already stopped instance is harmless. */
    if(ctx->initialized == false)
    {
        return PLT_ERR_STS_OK;
    }

    /*
     * Mark stopped before disabling the callback. If an RX callback arrives
     * during shutdown, it will not start another receive operation.
     */
    ctx->initialized = false;

    /* Disconnect the transport RX callback from this CLI context. */
    if((ctx->transport != NULL) && (ctx->transport->set_callback != NULL))
    {
        (void)ctx->transport->set_callback(ctx->transport->context, NULL, NULL);
    }

    /* Attempt to transmit EmbeddedCLI characters still in the TX buffer. */
    (void)bspCliFlushTx(ctx);

    /* Release EmbeddedCLI. Static cliBuffer memory remains inside ctx. */
    if(ctx->cli != NULL)
    {
        embeddedCliFree(ctx->cli);
        ctx->cli = NULL;
    }

    /* Release operating-system synchronization resources. */
    if(ctx->mutex != NULL)
    {
        bspMutexDelete(ctx->mutex);
        ctx->mutex = NULL;
    }

    /* Reset runtime state for a possible later bsp_cli_init() call. */
    ctx->bindingCount    = 0U;
    ctx->rxHead          = 0U;
    ctx->rxTail          = 0U;
    ctx->txLen           = 0U;
    ctx->pendingHelpInfo = 0U;
    ctx->inputLineLen    = 0U;

    return PLT_ERR_STS_OK;
}
/*----------------------------------------------------------------------------*/
bspCliHandle_t bsp_cli_get_console_handle(void)
{
    return &s_consoleCliContext;
}
/*----------------------------------------------------------------------------
 * File End
 *---------------------------------------------------------------------------*/