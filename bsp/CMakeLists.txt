

if(ESP_PLATFORM)
    message(STATUS "Configuring bsp modules...")

    # # This folder acts as the ESP-IDF component for CLI
    idf_component_register(
        SRCS
        "${CMAKE_CURRENT_LIST_DIR}/srcs/cli_logger.c"
        "${CMAKE_CURRENT_LIST_DIR}/srcs/err_status.c"

        "${CMAKE_CURRENT_LIST_DIR}/port/esp32/bsp_uart.c"
        "${CMAKE_CURRENT_LIST_DIR}/port/esp32/bsp_wifi.c"

        INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/include"

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
    )
endif()
