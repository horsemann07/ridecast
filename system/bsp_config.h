

#ifndef _BSP_CONFIG_H_
#define _BSP_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif


#include <stdint.h>
#include <stddef.h>

    // Configuration parameters for the BSP (Board Support Package).

    /* === Wifi BSP configuration   */

#define bspCONFIG_WIFI_SSID_MAX_LEN       (32)
#define bspCONFIG_WIFI_PASSWORD_MAX_LEN   (64)
#define bspCONFIG_WIFI_IP_ADDR_MAX_LEN    (46)
#define bspCONFIG_WIFI_MAC_ADDR_MAX_LEN   (6)
#define bspCONFIG_WIFI_MAX_SAVED_NETWORKS (5)


#ifdef __cplusplus
}
#endif

#endif /* _BSP_CONFIG_H_ */