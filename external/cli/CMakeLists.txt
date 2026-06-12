# ============================================================================
# RideCast External CLI Module
#
# Purpose:
# - Provide Embedded CLI library to RideCast
# - Support both ESP-IDF (ESP32) and non-ESP (STM32H7) builds
# - Keep source/include paths explicit for MISRA-friendly traceability
# ============================================================================

if(ESP_PLATFORM)
    message(STATUS "[RideCast:CLI] Platform: ESP32 (ESP-IDF)")

    # ESP-IDF component registration
    # This folder is treated as one ESP-IDF component.
    idf_component_register(
        SRCS
            "${CMAKE_CURRENT_LIST_DIR}/embedded-cli/lib/src/embedded_cli.c"
        INCLUDE_DIRS
            "${CMAKE_CURRENT_LIST_DIR}/embedded-cli/lib/include"
    )

    message(STATUS "[RideCast:CLI] ✓ ESP-IDF component registered")

elseif(STM32H7_PLATFORM)
    message(STATUS "[RideCast:CLI] Platform: STM32H7")

    # Non-ESP build: create static library target
    add_library(ridecast_cli STATIC
        "${CMAKE_CURRENT_LIST_DIR}/embedded-cli/lib/src/embedded_cli.c"
    )

    target_include_directories(ridecast_cli PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/embedded-cli/lib/include"
    )

    message(STATUS "[RideCast:CLI] ✓ Static library created: ridecast_cli")

else()
    message(FATAL_ERROR
        "[RideCast:CLI] No valid platform selected. "
        "Set ESP_PLATFORM=ON or STM32H7_PLATFORM=ON"
    )
endif()