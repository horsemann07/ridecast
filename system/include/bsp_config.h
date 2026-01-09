

#ifndef __BSP_CONFIG_H__
#define __BSP_CONFIG_H__


#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C"
{
#endif


// Configuration parameters for the BSP (Board Support Package).

/* ===== UART BSP configuration ===========  */
#define bspCONFIG_UART_RXTX_BUFFER_SIZE ((uint32_t)(256U))


/* === Wifi BSP configuration   */

#define bspCONFIG_WIFI_SSID_MAX_LEN       (32)
#define bspCONFIG_WIFI_PASSWORD_MAX_LEN   (64)
#define bspCONFIG_WIFI_IP_ADDR_MAX_LEN    (46)
#define bspCONFIG_WIFI_MAC_ADDR_MAX_LEN   (6)
#define bspCONFIG_WIFI_MAX_SAVED_NETWORKS (5)


#ifdef __cplusplus
}
#endif

#endif /* __BSP_CONFIG_H__ */