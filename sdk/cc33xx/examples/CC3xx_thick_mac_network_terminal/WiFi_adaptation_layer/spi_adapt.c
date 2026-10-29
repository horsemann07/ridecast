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
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "ti_drivers_config.h"
#include "ti_drivers_open_close.h"
#include "ti_board_open_close.h"



uint32_t bytesToWord32s(uint32_t bytesLen)
{
    uint32_t reminder = 1;
    if((bytesLen & 0x3) == 0)
    {
        reminder = 0;
    }

    return ((bytesLen >> 2) + reminder);
}


/*!
    \brief read SPI data as master

    \This function assumes that the SPI channel is already initialized and ready for use.
    \The caller must ensure that the SPI channel is properly initialized before calling this function,
    \and that it is properly deinitialized after use.

    \param data - buffer for output data
    \param length - length in bytes to read
    \return 0 - success, -1 - failed
    \note
    \warning
*/
int spi_ReadSync(uint8_t *data, uint32_t length)
{

    MCSPI_Transaction   spiTransaction;
    int32_t             transferOK;

    memset(&spiTransaction,0,sizeof(MCSPI_Transaction));


    spiTransaction.channel  = gConfigMcspi0ChCfg[0].chNum;
    spiTransaction.count    = bytesToWord32s(length);
    spiTransaction.txBuf    = NULL;
    spiTransaction.rxBuf    = data;
    spiTransaction.args     = NULL;
    spiTransaction.dataSize = 32;
    transferOK = MCSPI_transfer(gMcspiHandle[CONFIG_MCSPI0], &spiTransaction);
    return transferOK;


} /* spi_ReadSync() */


/*!
    \brief write SPI data as master

    \This function assumes that the SPI channel is already initialized and ready for use.
    \The caller must ensure that the SPI channel is properly initialized before calling this function,
    \and that it is properly deinitialized after use.

    \param data - input buffer
    \param length - length in bytes to read
    \return 0 - success, -1 - failed
    \note
    \warning
*/
int spi_WriteSync(uint8_t *data, uint32_t length)
{
    MCSPI_Transaction   spiTransaction;
    int32_t             transferOK;
    uint32_t            offset = 0;

    memset(&spiTransaction,0,sizeof(MCSPI_Transaction));
    spiTransaction.channel  = gConfigMcspi0ChCfg[0].chNum;
    spiTransaction.count    = bytesToWord32s(length);
    spiTransaction.txBuf    = data + offset;
    spiTransaction.rxBuf    = NULL;
    spiTransaction.args     = NULL;
    spiTransaction.dataSize = 32;
    transferOK = MCSPI_transfer(gMcspiHandle[CONFIG_MCSPI0], &spiTransaction);

    return transferOK;

} /* spi_WriteSync() */


