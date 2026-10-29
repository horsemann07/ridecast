/*
 * Copyright (c) 2024, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// Standard includes
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <osi_kernel.h>
#include <uart_hci.h>

/* Driver configuration */
#include "ti_drivers_config.h"
#include "ti_drivers_open_close.h"


//*****************************************************************************
//                          LOCAL DEFINES
//*****************************************************************************
int uart_Read(uint8_t* buf, uint16_t len);
int uart_Write(uint8_t* buf, uint16_t len);

const hciTransport_t uartForHci =
{
    &uart_Read,
    &uart_Write
};

//*****************************************************************************
//                          LOCAL VARIABLES
//*****************************************************************************
UART_Handle uartHciHandle = 0;


//*****************************************************************************
//                          LOCAL FUNCTIONS
//*****************************************************************************


//*****************************************************************************
//                          GLOBAL VARIABLES
//*****************************************************************************
static UART_Transaction uartHciTrans;

//*****************************************************************************
//                          API FUNCTIONS
//*****************************************************************************
//*****************************************************************************
//
//! UartHciOpen
//!
//! This function
//!        1. Configures the CONFIG_UART_BLE to be used.
//!
//!        Note the CONFIG_UART_BLE is configured in the SySconfig
//! \param  none
//!
//! \return none
//
//*****************************************************************************
void UartHciOpen(void)
{
    UART_close(gUartHandle[CONFIG_UART_BLE]);
    gUartHandle[CONFIG_UART_BLE] = UART_open(CONFIG_UART_BLE, &gUartParams[CONFIG_UART_BLE]);

    if (NULL != gUartHandle[CONFIG_UART_BLE])
    {
        Report("\n\rHCI UART Baudrate :: %d\n\r",gUartParams[CONFIG_UART_BLE].baudRate);
        uartHciHandle = gUartHandle[CONFIG_UART_BLE];
    }
    else
    {
        UART_close(gUartHandle[CONFIG_UART_BLE]);
        Report("\r\nUART open failed for instance %d", CONFIG_UART_BLE);
    }
}

//*****************************************************************************
//
//! UartHciClose
//!
//! This function closes the CONFIG_UART_BLE.
//! Note the UART2_2 is configured in the SySconfig
//!
//! \param  none
//!
//! \return none
//
//*****************************************************************************
void UartHciClose(void)
{
    UART_readCancel(uartHciHandle, &uartHciTrans);
    UART_writeCancel(uartHciHandle, &uartHciTrans);
    UART_close(uartHciHandle);
}

//*****************************************************************************
//
//! uart_Read
//!
//! This function reads to a buffer with a given length from CONFIG_UART_BLE.
//!
//! \param  buf - buffer to read
//!         len - len to read
//!
//! \return none
//
//*****************************************************************************
int uart_Read(uint8_t* buf, uint16_t len)
{
    int rc = 0;

    UART_Transaction_init(&uartHciTrans);
    uartHciTrans.buf = buf;
    uartHciTrans.count = len;

    rc = UART_read(uartHciHandle, &uartHciTrans);

    return rc;
}

//*****************************************************************************
//
//! uart_Write
//!
//! This function writes buffer with a given length to CONFIG_UART_BLE.
//!
//! \param  buf - buffer to write
//!         len - len to write
//!
//! \return none
//
//*****************************************************************************
int uart_Write(uint8_t* buf, uint16_t len)
{
    int rc = 0;

    UART_Transaction_init(&uartHciTrans);
    uartHciTrans.buf = buf;
    uartHciTrans.count = len;

    rc = UART_write(uartHciHandle, &uartHciTrans);

    return rc;
}
