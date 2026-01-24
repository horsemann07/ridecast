

set(srcs
    "${CMAKE_CURRENT_LIST_DIR}/srcs/nal_core.c"
    "${CMAKE_CURRENT_LIST_DIR}/srcs/nal_tls.c"
    "${CMAKE_CURRENT_LIST_DIR}/srcs/nal_crypto.c"
)

set(include
    "${CMAKE_CURRENT_LIST_DIR}/include")

    
if(ESP_PLATFORM)
    message(STATUS "Configuring ESP-IDF external modules...")

    # # This folder acts as the ESP-IDF component for CLI
    idf_component_register(
        SRCS 
            "${srcs}"
            "${CMAKE_CURRENT_LIST_DIR}/port/nal_platform_esp32.c"
        
        INCLUDE_DIRS
            "${include}"

        PRIV_REQUIRES 
            freertos 
            cmsis_esp 
            system 
            mbedtls 
            lwip
            esp_netif
            esp_event
            bsp
    )
endif()
