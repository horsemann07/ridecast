/**
 * ============================================================================
 * @file    ridecast_main.c
 * @brief   RideCast application-level coordination.
 *
 * @details
 * The application modules independently initialize through their
 * respective RTOS thread lifecycle callbacks.
 *
 * MODULE OWNERSHIP:
 *
 *   uart_app.c:
 *       UART initialization, RX/TX, protocol dispatch.
 *
 *   conn_manager.c:
 *       Connection states, START/STOP and retry policies.
 *
 *   wifi_app.c:
 *       Wi-Fi initialization, AP/TCP and Wi-Fi commands.
 *
 *   ble_app.c:
 *       BLE initialization and communication.
 *
 *   bt_classic_app.c:
 *       Bluetooth Classic initialization and communication.
 *
 *   ridecast_main.c:
 *       Application-level coordination only.
 *
 * @warning
 *   Wireless interfaces remain OFF until the vehicle cluster
 *   authorizes START.
 *
 *   eRcMsgGetStatus is temporarily reused for device readiness.
 *   A dedicated DEVICE_READY protocol definition is still required.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Removed module initialization, deinitialization and
 *             transport-specific implementation.
 * 2026-05-23  Initial development - J.Raghav.
 * ============================================================================
 */

#include <stdint.h>

#include "app_config.h"
#include "app_logging.h"
#include "ridecast_main.h"
#include "uart_app.h"
#include "rc_protocol.h"

/* =========================================================================
 * PUBLIC APIs
 * ========================================================================= */


/* =========================================================================
 * FILE END
 * ========================================================================= */