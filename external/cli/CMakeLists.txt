

if(ESP_BOARD)
    message(STATUS "Configuring ESP-IDF external modules...")

    # # This folder acts as the ESP-IDF component for CLI
    idf_component_register(
        SRCS
        "${CMAKE_CURRENT_LIST_DIR}/embedded-cli/lib/src/embedded_cli.c"

        INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/embedded-cli/lib/include"
    )
endif()
