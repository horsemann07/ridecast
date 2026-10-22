/*
 * =========================================================================
 * rc_uart.h
 *
 * Description:
 * RideCast UART interface between Host and RideCast.
 * Provides UART configuration, initialization, transmit and receive APIs.
 *
 * Change History:
 * Format:
 * YYYY-MM-DD - Developer - Description
 *
 * 2026-09-20 - J.Raghav - Initial development
 * =========================================================================
 */

#ifndef RC_UART_H
#define RC_UART_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

#include "bsp_uart.h"
#include "bsp_err_sts.h"
#include "rc_config.h"
#include "rc_protocol.h"

#define RC_UART_RX_BUF_LEN (256U)
#define RC_UART_TX_BUF_LEN (RC_FRAME_MAX_LEN)

    /* =========================================================================
     * RX message
     * =========================================================================
     */

    typedef struct
    {
        uint8_t buf[RC_UART_RX_BUF_LEN];
        uint16_t len;
    } rc_uart_rx_msg_t;


    /* =========================================================================
     * TX message
     * =========================================================================
     */

    typedef struct
    {
        uint8_t buf[RC_UART_TX_BUF_LEN];
        uint16_t len;
    } rc_uart_tx_msg_t;
    /* =========================================================================
     * CONFIGURATION
     * ========================================================================= */

    /**
     * @brief Initialize Host communication UART.
     *
     * @return PLT_ERR_STS_OK          UART initialized successfully.
     * @return PLT_ERR_STS_INVALID_PARAM Invalid configuration.
     * @return PLT_ERR_STS_FAIL         UART initialization failed.
     */
    plt_err_sts_t rc_uart_init(void);


    /**
     * @brief Deinitialize Host communication UART.
     *
     * @return PLT_ERR_STS_OK   UART deinitialized successfully.
     * @return PLT_ERR_STS_FAIL UART deinitialization failed.
     */
    plt_err_sts_t rc_uart_deinit(void);


    /* =========================================================================
     * TRANSMIT
     * ========================================================================= */

    /**
     * @brief Transmit data to Host.
     *
     * @param[in] data      Data buffer.
     * @param[in] dataLen   Number of bytes to transmit.
     *
     * @return PLT_ERR_STS_OK          Data transmitted successfully.
     * @return PLT_ERR_STS_INVALID_PARAM Invalid parameter.
     * @return PLT_ERR_STS_FAIL         Transmission failed.
     */
    plt_err_sts_t rc_uart_transmit(const uint8_t* data, uint16_t dataLen);


    /* =========================================================================
     * RECEIVE
     * ========================================================================= */

    /**
     * @brief Receive data from Host.
     *
     * @param[out] data       Receive buffer.
     * @param[in]  dataLen    Maximum number of bytes to receive.
     * @param[out] receivedLen Number of bytes actually received.
     *
     * @return PLT_ERR_STS_OK          Data received successfully.
     * @return PLT_ERR_STS_INVALID_PARAM Invalid parameter.
     * @return PLT_ERR_STS_TIMEOUT      Receive timeout.
     * @return PLT_ERR_STS_FAIL          Reception failed.
     */
    plt_err_sts_t rc_uart_receive(uint8_t* data, uint16_t dataLen, uint16_t* receivedLen);


    /* =========================================================================
     * STATUS
     * ========================================================================= */

    /**
     * @brief Check whether UART is initialized.
     *
     * @return true  UART is initialized.
     * @return false UART is not initialized.
     */
    bool rc_uart_is_initialized(void);


    /* =========================================================================
     * CALLBACK
     * ========================================================================= */

    /**
     * @brief UART receive callback.
     *
     * This callback is used by the BSP UART layer to notify RideCast
     * when data is received.
     *
     * @param[in] data     Received data.
     * @param[in] dataLen  Number of received bytes.
     */
    void rc_uart_rx_callback(const uint8_t* data, uint16_t dataLen);


#ifdef __cplusplus
}
#endif

#endif /* RC_UART_H */