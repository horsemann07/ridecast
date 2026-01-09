


idf_component_register(
    SRCS "app_main.c" 
        "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp/Source/cmsis_os2.c"
        "${CMAKE_SOURCE_DIR}/app/demo_config/app_bsp_config.c"
        "${CMAKE_SOURCE_DIR}/app/demo/uart/demo_app_uart_sync.c"
        "${CMAKE_SOURCE_DIR}/app/demo/uart/demo_app_uart_async.c"

        
    INCLUDE_DIRS "." 
        "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp/Include"
        "${CMAKE_SOURCE_DIR}/app/demo_config"
    # REQUIRES 
    #     freertos

    PRIV_REQUIRES
        freertos
        cmsis_esp
        system
        mbedtls
        lwip
        driver
        esp_netif
        esp_wifi
        esp_event
        log
        mbedtls
        lwip
        esp-tls
        esp_common
        bsp
        nal

)