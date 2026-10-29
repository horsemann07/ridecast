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
#include <kernel/dpl/AddrTranslateP.h>
#include <stdlib.h>
#include <drivers/hw_include/cslr_soc.h>
#include <drivers/gpio.h>
#include <drivers/sciclient.h>
#include "osi_kernel.h"


/****************************************************************************
 *
 *                      Internal helper functions and macros
 *
 ****************************************************************************/


extern void sleep(uint32_t sec);
uint32_t gWlanENGpioAddr = 0;
uint32_t gWlanIRQGpioAddr = 0;
HwiP_Object gWlanIRQHwiObject;

/*
 * Board info
 */
/* This is based on DMSC board config and core */
#define WLAN_IRQ_GPIO_INTR_NUM      (CSLR_R5FSS0_CORE0_INTR_MAIN_GPIOMUX_INTROUTER0_OUTP_8)

/** \brief bank interrupt source index base */
#define TISCI_BANK_SRC_IDX_BASE_GPIO0       (90U)
#define TISCI_BANK_SRC_IDX_BASE_GPIO1       (90U)
#define TISCI_BANK_SRC_IDX_BASE_MCU_GPIO0   (90U)

static void Sciclient_gpioIrqSet(void);
static void Sciclient_gpioIrqRelease(void);

void Board_gpioInit(void)
{
    Sciclient_gpioIrqSet();
}

void Board_gpioDeinit(void)
{
    Sciclient_gpioIrqRelease();
}

uint32_t Board_getGpioWlanIntrNum(void)
{
    return (WLAN_IRQ_GPIO_INTR_NUM);
}

static void Sciclient_gpioIrqSet(void)
{
    int32_t                             retVal;
    struct tisci_msg_rm_irq_set_req     rmIrqReq;
    struct tisci_msg_rm_irq_set_resp    rmIrqResp;

    rmIrqReq.valid_params           = 0U;
    rmIrqReq.valid_params          |= TISCI_MSG_VALUE_RM_DST_ID_VALID;
    rmIrqReq.valid_params          |= TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID;
    rmIrqReq.global_event           = 0U;
    rmIrqReq.src_id                 = TISCI_DEV_GPIO1;
    rmIrqReq.src_index              = TISCI_BANK_SRC_IDX_BASE_GPIO1 + GPIO_GET_BANK_INDEX(WLAN_IRQ_PIN);
    rmIrqReq.dst_id                 = TISCI_DEV_R5FSS0_CORE0;
    rmIrqReq.dst_host_irq           = Board_getGpioWlanIntrNum();
    rmIrqReq.ia_id                  = 0U;
    rmIrqReq.vint                   = 0U;
    rmIrqReq.vint_status_bit_index  = 0U;
    rmIrqReq.secondary_host         = TISCI_MSG_VALUE_RM_UNUSED_SECONDARY_HOST;

    retVal = Sciclient_rmIrqSet(&rmIrqReq, &rmIrqResp, SystemP_WAIT_FOREVER);
    if(0 != retVal)
    {
        DebugP_log("[Error] Sciclient event config failed!!!\r\n");
        DebugP_assert(FALSE);
    }

    return;
}

static void Sciclient_gpioIrqRelease(void)
{
    struct tisci_msg_rm_irq_release_req rmIrqReq;

    rmIrqReq.valid_params           = 0U;
    rmIrqReq.valid_params          |= TISCI_MSG_VALUE_RM_DST_ID_VALID;
    rmIrqReq.valid_params          |= TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID;
    rmIrqReq.global_event           = 0U;
    rmIrqReq.src_id                 = TISCI_DEV_GPIO1;
    rmIrqReq.src_index              = TISCI_BANK_SRC_IDX_BASE_GPIO1 + GPIO_GET_BANK_INDEX(WLAN_IRQ_PIN);
    rmIrqReq.dst_id                 = TISCI_DEV_R5FSS0_CORE0;
    rmIrqReq.dst_host_irq           = Board_getGpioWlanIntrNum();
    rmIrqReq.ia_id                  = 0U;
    rmIrqReq.vint                   = 0U;
    rmIrqReq.vint_status_bit_index  = 0U;
    rmIrqReq.secondary_host         = TISCI_MSG_VALUE_RM_UNUSED_SECONDARY_HOST;

    Sciclient_rmIrqRelease(&rmIrqReq, SystemP_WAIT_FOREVER);
/*  In case we want to validate the function return value:
    int32_t retVal;
    retVal = Sciclient_rmIrqRelease(&rmIrqReq, SystemP_WAIT_FOREVER);
    if(0 != retVal)
    {
        DebugP_log("[Error] Sciclient event reset failed!!!\r\n");
        DebugP_assert(FALSE);
    }
*/

    return;
}


/*!
    \brief initialize the input GPIO coming from the wifi part
    \param cb - gpio ISR called upon assertion
    \note
        - GPIO input should be triggered either -
          - Falling edge - no need to implement wlan_IRQEnableInt wlan_IRQDisableInt and wlan_IRQClearInt
          - Active low - must implement wlan_IRQEnableInt and wlan_IRQDisableInt
    \return
    \warning
*/

void wlan_IRQInit(void *cb)
{
    HwiP_Params     hwiPrms;
    int retVal;
    uint32_t bankNum;
    bankNum = GPIO_GET_BANK_INDEX(WLAN_IRQ_PIN);
    Sciclient_gpioIrqRelease();
    Sciclient_gpioIrqSet();

    /* Address translate */
    gWlanIRQGpioAddr = (uint32_t) AddrTranslateP_getLocalAddr(WLAN_IRQ_BASE_ADDR);
    /* Setup GPIO for interrupt generation */
    GPIO_setDirMode(gWlanIRQGpioAddr, WLAN_IRQ_PIN, WLAN_IRQ_DIR);
    GPIO_setTrigType(gWlanIRQGpioAddr, WLAN_IRQ_PIN, WLAN_IRQ_TRIG_TYPE);
    GPIO_bankIntrEnable(gWlanIRQGpioAddr, bankNum);

    /* Register pin interrupt */
    HwiP_Params_init(&hwiPrms);
    hwiPrms.intNum   = WLAN_IRQ_GPIO_INTR_NUM;
    hwiPrms.callback = (HwiP_FxnCallback)cb;
    hwiPrms.args     = (void *) WLAN_IRQ_PIN;
    retVal = HwiP_construct(&gWlanIRQHwiObject, &hwiPrms);
    DebugP_assert(retVal == SystemP_SUCCESS );
}

void wlan_IRQInitBeforeHwInit(void *cb)
{
    wlan_IRQInit(cb);
}
/*!
    \brief removes the input GPIO from the system's interrupts
    \note
    \return
    \warning
*/
void wlan_IRQDeinit()
{
    HwiP_destruct(&gWlanIRQHwiObject);
}

/*!
    \brief wlan enable input interrupt
    \note need to implement only if working with active low assertion
    \return
    \warning
*/
void wlan_IRQEnableInt()
{
    HwiP_enableInt(WLAN_IRQ_GPIO_INTR_NUM);
    //this is edge triggered so no need to mask the interrupt
}

/*!
    \brief wlan disable input interrupt
    \note need to implement only if working with active low assertion
    \return
    \warning
*/
void wlan_IRQDisableInt()
{
    HwiP_disableInt(WLAN_IRQ_GPIO_INTR_NUM);
    //this is edge triggered so no need to mask the interrupt
}



/*!
    \brief wlan clear pending interrupt
    \note need to implement only if working with active low assertion
    \return
    \warning
*/
void wlan_IRQClearInt()
{
    //this is edge triggered so no need to mask the interrupt
}



/*!
    \brief use pre - configured output GPIO to turn off the WiFi device
    \note
        - need to use pre configured GPIO - set GPIO output to 0 for off and 1 for on
    \return
    \warning
*/
void wlan_TurnOffWlan()
{
    if(0 == gWlanENGpioAddr)
    {
        /* Get address after translation translate */
        gWlanENGpioAddr = (uint32_t) AddrTranslateP_getLocalAddr(WLAN_EN_BASE_ADDR);
        GPIO_setDirMode(gWlanENGpioAddr, WLAN_EN_PIN, WLAN_EN_DIR);
    }


    GPIO_pinWriteLow(gWlanENGpioAddr, WLAN_EN_PIN);
}

/*!
    \brief use pre - configured output GPIO to turn on the WiFi device
    \note
        - need to use pre configured GPIO - set GPIO output to 0 for off and 1 for on
    \return
    \warning
*/
void wlan_TurnOnWlan()
{
    if(0 == gWlanENGpioAddr)
    {
        /* Get address after translation translate */
        gWlanENGpioAddr = (uint32_t) AddrTranslateP_getLocalAddr(WLAN_EN_BASE_ADDR);
        GPIO_setDirMode(gWlanENGpioAddr, WLAN_EN_PIN, WLAN_EN_DIR);
    }

    if(1 == GPIO_pinOutValueRead(gWlanENGpioAddr, WLAN_EN_PIN))
    {
        wlan_TurnOffWlan();
    }
    osi_Sleep(2);
    GPIO_pinWriteHigh(gWlanENGpioAddr, WLAN_EN_PIN);

    osi_Sleep(2);
}

/*!
    \brief get the state of the pre - configured output GPIO
    \note
    \return 0 or 1 output
    \warning
*/
int wlan_GetStateWlan()
{
    if(0 == gWlanENGpioAddr)
    {
        /* Get address after translation translate */
        gWlanENGpioAddr = (uint32_t) AddrTranslateP_getLocalAddr(WLAN_EN_BASE_ADDR);
        GPIO_setDirMode(gWlanENGpioAddr, WLAN_EN_PIN, WLAN_EN_DIR);
    }
    return GPIO_pinOutValueRead(gWlanENGpioAddr, WLAN_EN_PIN);
}

/*!
    \brief disables the irq on the function IRQ handler
    \note
    \return 0 or 1 output
    \warning
*/
void wlan_IRQDisableOnIRQHandler()
{
   wlan_IRQDisableInt();
}



