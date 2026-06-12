# ============================================================================
# RideCast External OS Abstraction
#
# Purpose:
# - Select OS integration layer by target platform
# - ESP32   -> CMSIS-ESP wrapper
# - STM32H7 -> CMSIS + CMSIS-FreeRTOS
# ============================================================================

message(STATUS "[RideCast:OS] Configuring external/os...")

if(ESP_PLATFORM)
    message(STATUS "[RideCast:OS] Platform: ESP32 (ESP-IDF)")
    message(STATUS "[RideCast:OS] Using cmsis_esp")

    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmsis_esp/CMakeLists.txt")
        add_subdirectory(cmsis_esp)
    else()
        message(FATAL_ERROR "[RideCast:OS] Missing: external/os/cmsis_esp/CMakeLists.txt")
    endif()

elseif(STM32H7_PLATFORM)
    message(STATUS "[RideCast:OS] Platform: STM32H7")
    message(STATUS "[RideCast:OS] Using cmsis + cmsis_freertos")

    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmsis/CMakeLists.txt")
        add_subdirectory(cmsis)
    else()
        message(FATAL_ERROR "[RideCast:OS] Missing: external/os/cmsis/CMakeLists.txt")
    endif()

    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmsis_freertos/CMakeLists.txt")
        add_subdirectory(cmsis_freertos)
    else()
        message(FATAL_ERROR "[RideCast:OS] Missing: external/os/cmsis_freertos/CMakeLists.txt")
    endif()

else()
    message(FATAL_ERROR
        "[RideCast:OS] No valid platform selected. "
        "Set ESP_PLATFORM=ON or STM32H7_PLATFORM=ON"
    )
endif()

message(STATUS "[RideCast:OS] ✓ external/os configured")