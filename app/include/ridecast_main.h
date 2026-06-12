/**
 * @file    ridecast_main.h
 * @brief   RideCast application entry point API.
 */

#ifndef RIDECAST_MAIN_H
#define RIDECAST_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_err_sts.h"

/**
 * @brief  Initialise all RideCast subsystems.
 *         Call once after RTOS scheduler has started.
 * @return BSP_ERR_STS_OK on success, error code on failure.
 */
bsp_err_sts_t ridecast_init(void);

/**
 * @brief  Deinitialise all subsystems (OTA / shutdown / fault recovery).
 */
bsp_err_sts_t ridecast_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* RIDECAST_MAIN_H */