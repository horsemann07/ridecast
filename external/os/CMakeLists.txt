if(ESP_PLATFORM)
    message(STATUS "Using CMSIS-ESP for ESP32")
    add_subdirectory(cmsis_esp)

else()
    message(STATUS "Using CMSIS + FreeRTOS")
    add_subdirectory(cmsis)
    add_subdirectory(cmsis_freertos)
endif()
