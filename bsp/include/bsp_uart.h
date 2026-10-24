/* ============================================================================
 * File        : bsp_uart.h
 * Description : Platform-independent BSP UART public interface
 * Author      : Jha Raghav
 * Date        : 2026-10-24
 * ============================================================================
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Description:
 *   Defines a portable UART interface shared by ESP32 and STM32H7 ports.
 *   The public API contains no ESP-IDF, STM32 HAL, FreeRTOS, or CMSIS types.
 *
 * Integration:
 *   Include bsp_uart.h from application and middleware code. Link exactly
 *   one target-specific implementation (ESP32 or STM32H7).
 *
 * Requirements:
 *   C11, stdint.h, stddef.h, stdbool.h, and BSP status definitions.
 *
 * Change Log (newest first):
 * Date          Author          Description
 * ------------  --------------  ----------------------------------------------
 * 2026-10-24    Jha Raghav      Aligned portable API with ESP-IDF implementation
 * 2026-10-23    Jha Raghav      UART API draft (original date to be confirmed)
 * ============================================================================ */

#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bsp_config.h"
#include "bsp_err_sts.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Baud rate conveniences; arbitrary nonzero baud rates are also allowed. */
#define BSP_UART_BAUD_1200     (1200UL)
#define BSP_UART_BAUD_9600     (9600UL)
#define BSP_UART_BAUD_19200    (19200UL)
#define BSP_UART_BAUD_38400    (38400UL)
#define BSP_UART_BAUD_57600    (57600UL)
#define BSP_UART_BAUD_115200   (115200UL)
#define BSP_UART_BAUD_230400   (230400UL)
#define BSP_UART_BAUD_460800   (460800UL)
#define BSP_UART_BAUD_921600   (921600UL)

/** Platform-neutral sentinel meaning 'retain the existing pin mapping'.
 *  Board setup must configure valid TX/RX pins before the first use. */
#define BSP_UART_PIN_NO_CHANGE UINT32_MAX

/* Compatibility aliases for pre-existing generic BSP config symbols. */
#ifndef bspCONFIG_UART_RXTX_BUFFER_SIZE
    #define bspCONFIG_UART_RXTX_BUFFER_SIZE (1024U)
#endif
#ifndef bspCONFIG_UART_ENABLE_ASYNC
    #define bspCONFIG_UART_ENABLE_ASYNC (1U)
#endif
#define BSP_UART_RXTX_BUFFER_SIZE (bspCONFIG_UART_RXTX_BUFFER_SIZE)
#define BSP_UART_ENABLE_ASYNC     (bspCONFIG_UART_ENABLE_ASYNC)

    /** Port values are logical identifiers. The selected target port validates
     *  the actual hardware UART count at runtime. */
    typedef enum
    {
        eBspUartPort0 = 0U,
        eBspUartPort1,
        eBspUartPort2,
        eBspUartPort3,
        eBspUartPort4,
        eBspUartPort5,
        eBspUartPort6,
        eBspUartPort7
    } bspUartPort_t;

    typedef uint8_t bspUartOwner_t;
    typedef uint32_t bspUartBaudrate_t;
    /** Target-specific pin identifier: native GPIO number on ESP32, board pin
     *  mapping identifier on STM32H7. This type never exposes a HAL pointer. */
    typedef uint32_t bspUartPin_t;

    typedef enum
    {
        eBspUartParityNone = 0U,
        eBspUartParityOdd,
        eBspUartParityEven
    } bspUartParity_t;

    typedef enum
    {
        eBspUartStopBitsOne = 0U,
        eBspUartStopBitsTwo
    } bspUartStopBits_t;

    typedef enum
    {
        eBspUartModeTx = 0U,
        eBspUartModeRx,
        eBspUartModeTxRx
    } bspUartMode_t;

    typedef enum
    {
        eBspUartWordLength5 = 5U,
        eBspUartWordLength6,
        eBspUartWordLength7,
        eBspUartWordLength8
    } bspUartWordLength_t;

    typedef enum
    {
        eBspUartDmaDisabled = 0U,
        eBspUartDmaTx,
        eBspUartDmaRx,
        eBspUartDmaTxRx
    } bspUartDmaMode_t;

    typedef enum
    {
        eBspUartFlowControlNone = 0U,
        eBspUartFlowControlRtsCts
    } bspUartFlowControl_t;

    /** Reserved for future optional operations. Prefer dedicated BSP APIs. */
    typedef enum
    {
        eBspUartIoctlReserved = 0U
    } bspUartIoctlRequest_t;

    /** UART configuration. Pins use BSP_UART_PIN_NO_CHANGE to retain mapping.
     *  fifoSize and oversampling are optional board-dependent tuning fields;
     *  unsupported nonzero values return RET_ERR_STS_NOT_SUPPORTED.
     *  dmaMode is optional and may be unsupported on some targets.
     *  rxThreshold == 0 selects the target's driver default.
     */
    typedef struct
    {
        bspUartPort_t port;
        bspUartOwner_t owner;
        bspUartBaudrate_t baudrate;
        bspUartWordLength_t wordLength;
        bspUartParity_t parity;
        bspUartStopBits_t stopBits;
        bspUartMode_t mode;
        bspUartPin_t uartRxPin;
        bspUartPin_t uartTxPin;
        bspUartPin_t uartRtsPin;
        bspUartPin_t uartCtsPin;
        uint16_t fifoSize;
        uint16_t rxThreshold;
        bspUartFlowControl_t flowControl;
        bspUartDmaMode_t dmaMode;
        uint8_t oversampling;
        bool invertTx;
        bool invertRx;
    } bspUartConfig_t;

    /** Counters report accepted TX bytes and received RX bytes, not guaranteed
     *  physical wire completion. Busy flags describe current driver activity. */
    typedef struct
    {
        bool initialized;
        bool rxBusy;
        bool txBusy;
        size_t rxCount;
        size_t txCount;
        uint32_t rxErrorCount;
        uint32_t txErrorCount;
    } bspUartStatus_t;

    /** RX completion callback runs in UART worker TASK context (not an ISR).
     *  Cancellation may invoke it synchronously in the cancelling caller's
     *  task context. Never rely on a particular task identity.
     *  Callback is invoked with the UART state mutex unlocked.
     *  data is the caller-provided buffer and remains owned by the caller.
     *  data_len is the number received, including partial data on cancellation.
     */
    typedef void (*bspUartRxCallback_t)(ret_err_sts_t status,
                                        uint8_t* data,
                                        size_t data_len,
                                        void* userContext);

    /** Reserved completion callback API. The ESP-IDF port currently returns
     *  RET_ERR_STS_NOT_SUPPORTED when registering a TX callback, because
     * copying data into the driver ring is not physical TX completion. */
    typedef void (*bspUartTxCallback_t)(ret_err_sts_t status, size_t data_len, void* userContext);

    /** Application-owned handle. Initialize it to zero, then fill config.
     *  Keep the handle at a stable address until bspUartDeInit() succeeds.
     *  Do not modify config between init and deinit except through BSP APIs.
     *  Only one handle may own any hardware port at a time. */
    typedef struct
    {
        bspUartConfig_t config;
    } bspUartHandle_t;

    /** All APIs are task-context only. Init/deinit must not race with other calls.
     *  DeInit aborts a pending RX with RET_ERR_STS_CANCELLED; the callback may
     *  run in the deinitializing task and must not recursively call DeInit. */
    ret_err_sts_t bspUartInit(bspUartHandle_t* handle);
    ret_err_sts_t bspUartDeInit(bspUartHandle_t* handle);

    /** SendSync enqueues data, then waits for hardware TX completion. The
     *  underlying ESP-IDF enqueue call can itself block, independent of timeout. */
    ret_err_sts_t
    bspUartSendSync(bspUartHandle_t* handle, const uint8_t* data, size_t length, uint32_t timeout_ms);
    /** Receive up to buffer_len bytes; timeout with no data returns OK,
     * rx_len=0. Do not mix synchronous and asynchronous RX on the same UART. */
    ret_err_sts_t bspUartReceiveSync(bspUartHandle_t* handle,
                                     uint8_t* buffer,
                                     size_t buffer_len,
                                     size_t* rx_len,
                                     uint32_t timeout_ms);

    /** Arm exactly one asynchronous RX operation. The ESP-IDF port completes
     *  upon collecting length bytes; callback executes in a worker task.
     *  Call bspUartCancelRx() if the requested length will not arrive.
     *  The caller must keep buffer valid until completion/cancellation.
     */
    ret_err_sts_t bspUartReadAsync(bspUartHandle_t* handle, uint8_t* buffer, size_t length);
    /** Queue TX data into the driver's ring. May block if the ring is full.
     *  Does not invoke a physical TX-completion callback. */
    ret_err_sts_t bspUartSendAsync(bspUartHandle_t* handle, const uint8_t* buffer, size_t length);
    ret_err_sts_t bspUartSetRxCallback(bspUartHandle_t* handle,
                                       bspUartRxCallback_t callback,
                                       void* userContext);
    ret_err_sts_t bspUartSetTxCallback(bspUartHandle_t* handle,
                                       bspUartTxCallback_t callback,
                                       void* userContext);
    ret_err_sts_t bspUartCancelRx(bspUartHandle_t* handle);
    ret_err_sts_t bspUartWaitTxDone(bspUartHandle_t* handle, uint32_t timeout_ms);
    ret_err_sts_t bspUartFlushRx(bspUartHandle_t* handle);
    /** On ESP-IDF, hardware TX flushing is not provided by uart_flush();
     *  returns RET_ERR_STS_NOT_SUPPORTED. */
    ret_err_sts_t bspUartFlushTx(bspUartHandle_t* handle);
    ret_err_sts_t bspUartGetStatus(bspUartHandle_t* handle, bspUartStatus_t* status);
    ret_err_sts_t bspUartIsRxBusy(bspUartHandle_t* handle, bool* busy);
    ret_err_sts_t bspUartIsTxBusy(bspUartHandle_t* handle, bool* busy);
    ret_err_sts_t bspUartGetRxCount(bspUartHandle_t* handle, size_t* rx_count);
    ret_err_sts_t bspUartGetTxCount(bspUartHandle_t* handle, size_t* tx_count);
    ret_err_sts_t bspUartSetConfig(bspUartHandle_t* handle, const bspUartConfig_t* config);
    ret_err_sts_t bspUartGetConfig(bspUartHandle_t* handle, bspUartConfig_t* config);
    ret_err_sts_t bspUartSetBaudrate(bspUartHandle_t* handle, bspUartBaudrate_t baudrate);
    ret_err_sts_t bspUartGetBaudrate(bspUartHandle_t* handle, bspUartBaudrate_t* baudrate);
    ret_err_sts_t
    bspUartIoctl(bspUartHandle_t* handle, bspUartIoctlRequest_t req, void* arg);

#ifdef __cplusplus
}
#endif
#endif /* BSP_UART_H */
