/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef __HAL_UART_H__
#define __HAL_UART_H__

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum hal_uart_parity
{
    HAL_UART_PARITY_NONE = 0,
    HAL_UART_PARITY_ODD = 1,
    HAL_UART_PARITY_EVEN = 2
};

enum hal_uart_flow_ctl
{
    HAL_UART_FLOW_CTL_NONE = 0,
    HAL_UART_FLOW_CTL_RTS_CTS = 1
};


typedef int (*hal_uart_tx_char)(void *);
typedef int (*hal_uart_rx_char)(void *, unsigned char);
typedef void (*hal_uart_tx_done)();


int
hal_uart_config(int port, int32_t baudrate, uint8_t databits, uint8_t stopbits,
                enum hal_uart_parity parity, enum hal_uart_flow_ctl flow_ctl);

int
hal_uart_init_cbs(int port, hal_uart_tx_char tx_func, hal_uart_tx_done tx_done,
                  hal_uart_rx_char rx_func, void *arg);

void
hal_uart_start_tx(int port);


#ifdef __cplusplus
}
#endif

#endif /* __HAL_UART_H__ */
