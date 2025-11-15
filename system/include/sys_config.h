


#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif


#include "bsp_config.h"
#include "nal_config.h"
    // appconfig.h


    /* */

// #define SYS_USE_LWIP
#define SYS_USE_CMSIS
// #define SYS_USE_MBEDTLS

/*automatically defined in by espidf build. */
#ifndef ESP_PLATFORM
    #define ESP_PLATFORM
#endif // ESP_PLATFORM
#define ESP_PLATFORM_LWIP
#define ESP_PLATFORM_MBEDTLS
    // #endif


#ifdef __cplusplus
}
#endif // __cplusplus


#endif // SYS_CONFIG_H