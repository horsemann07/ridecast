

if(ESP_PLATFORM)
    message(STATUS "Configuring ESP-IDF external modules...")

    # # This folder acts as the ESP-IDF component for CLI
    idf_component_register(
        SRCS "${CMAKE_CURRENT_LIST_DIR}/srcs/nal_network.c"
        INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/include"
        PRIV_REQUIRES freertos cmsis_esp system mbedtls lwip
    )
endif()
