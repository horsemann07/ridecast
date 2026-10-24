# ============================================================================
# File        : CMakeLists.txt
# Description : External OS abstraction configuration
# Author      : Jha Raghav
# Date        : 2026-10-23
# ============================================================================
#
# Copyright (c) 2026 Navyantra Tech Private Limited
#
# ============================================================================
# Change Log:
#
# Date          Author          Description
# ------------  --------------  ----------------------------------------------
# 2026-10-23    Jha Raghav      Updated ESP-IDF component discovery mechanism
# 2026-10-23    Jha Raghav      Aligned STM32H7 platform configuration naming
# 2026-01-01    Jha Raghav      Initial version
# ============================================================================

message(STATUS "[RideCast:OS] Configuring external OS abstraction")

# ============================================================================
# 1. ESP32 PLATFORM
# ============================================================================

if(ESPx_PLATFORM)

    message(STATUS "[RideCast:OS] Platform: ESP32")
    message(STATUS "[RideCast:OS] OS abstraction: cmsis_esp")

    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmsis_esp/CMakeLists.txt")
        message(FATAL_ERROR "[RideCast:OS] Missing cmsis_esp component")
    endif()

    # ESP-IDF registers cmsis_esp through EXTRA_COMPONENT_DIRS
    # in the top-level CMakeLists.txt before project().

# ============================================================================
# 2. STM32H7 PLATFORM
# ============================================================================

elseif(STM32H7x_PLATFORM)

    message(STATUS "[RideCast:OS] Platform: STM32H7")
    message(STATUS "[RideCast:OS] OS abstraction: CMSIS-FreeRTOS")

    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmsis/CMakeLists.txt")
        message(FATAL_ERROR
            "[RideCast:OS] Missing CMSIS component"
        )
    endif()

    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmsis_freertos/CMakeLists.txt")
        message(FATAL_ERROR
            "[RideCast:OS] Missing CMSIS-FreeRTOS component"
        )
    endif()

    add_subdirectory(cmsis)
    add_subdirectory(cmsis_freertos)

# ============================================================================
# 3. INVALID PLATFORM
# ============================================================================

else()

    message(FATAL_ERROR
        "[RideCast:OS] No valid platform selected.\n"
        "Enable ESPx_PLATFORM or STM32H7x_PLATFORM."
    )

endif()

message(STATUS "[RideCast:OS] External OS configuration complete")