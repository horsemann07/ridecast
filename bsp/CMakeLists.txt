
message(STATUS "Configuring bsp modules...")

set(bsp_common
    "${CMAKE_CURRENT_LIST_DIR}/srcs/bsp_log.c"
    "${CMAKE_CURRENT_LIST_DIR}/srcs/bsp_err_sts.c"
)

set(include
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

if(ESP_PLATFORM)
    set(bsp_port
        "${CMAKE_CURRENT_LIST_DIR}/port/esp32/bsp_uart.c"
        "${CMAKE_CURRENT_LIST_DIR}/port/esp32/bsp_wifi.c"
    )



    # # This folder acts as the ESP-IDF component for CLI
    idf_component_register(
        SRCS
            "${bsp_common}"
            "${bsp_port}"
        
        INCLUDE_DIRS
            "${include}"

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
        cli
        nvs_flash
    )
endif()
