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
#include <bus_if.h>
#include "sdio_adapt.h"
#include "macro_utils.h"


/******************************************************************************
                      INTERNAL FUNCTION 
******************************************************************************/

uint32_t bus_AlignLength(uint32_t length)
{
    return ALIGN_TO_BLOCK_SIZE(length);
}

int bus_sendInitCommand(uint32_t config_param, uint8_t fbrw)
{
    // marking the parameters as unused to bypass 'unused parameter' warning
    (void)config_param;
    (void)fbrw;

    sdioAdapt_Init();
    /* to be coded when bus_sendInitCommand available for sdio*/
    return TRUE;
}


int bus_sendReadCommand(uint32_t address, uint8_t * data, uint32_t length, uint8_t isFixed,uint32_t flags)
{
    // marking the parameter as unused to bypass 'unused parameter' warning
    (void)flags;
    
    sdioAdapt_read(address, data, length);
    return TRUE;
}


int bus_sendWriteCommand(uint32_t address, uint8_t * data, uint32_t length, uint8_t isFixed)
{
    SDIO_PRINT("\n\rsdioAdapt_write");
    sdioAdapt_write(address, data, length);
    return TRUE;
}

int bus_Send_deInit()
{
    return(sdioAdapt_deInit());
}

