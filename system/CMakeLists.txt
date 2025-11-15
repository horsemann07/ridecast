

if(ESP_PLATFORM)
    message(STATUS "Loading component: system")

    # # This folder acts as the ESP-IDF component for CLI
    idf_component_register(
        INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include"
    )
endif()
