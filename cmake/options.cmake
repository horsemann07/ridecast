# ============================================================================
# File        : options.cmake
# Description : Centralized application and BSP build options
# Author      : Jha Raghav
# Date        : 2026-01-01
# ============================================================================
# Copyright (c) 2026 Navyantra Tech Private Limited
#
# Description:
#   Defines configurable CMake build options for:
#
#   - Target platform selection
#   - BSP core modules
#   - Platform-specific BSP drivers
#   - Logger features
#   - Host unit and integration tests
#
# Integration:
#   Included by the root CMakeLists.txt before project()
#   and before ESP-IDF component discovery.
#
# Requirements:
#   - CMake version 3.16 or later.
#   - Options are defined using the CMake cache.
#   - BSP configuration remains application-independent.
#
# ============================================================================
# Change Log:
#
# Date          Author          Description
# ------------  --------------  ----------------------------------------------
# 2026-01-01    Jha Raghav      Initial version
# ============================================================================


include_guard(GLOBAL)


# ============================================================================
# 1. TARGET PLATFORM OPTIONS
# ============================================================================

option(ESPx_PLATFORM
    "Build firmware for ESP32 using ESP-IDF"
    ON
)

option(STM32H7x_PLATFORM
    "Build firmware for STM32H7"
    OFF
)


# ============================================================================
# 2. BSP CORE MODULE OPTIONS
# ============================================================================

option(BSP_BUILD_CLI
    "Build portable BSP CLI module"
    ON
)

option(BSP_BUILD_LOGGER
    "Build asynchronous CMSIS-RTOS2 logger"
    ON
)


# ============================================================================
# 3. BSP PLATFORM DRIVER OPTIONS
# ============================================================================

option(BSP_BUILD_PORT
    "Build platform-specific BSP drivers"
    ON
)


# ============================================================================
# 5. BSP TEST OPTIONS
# ============================================================================

option(BSP_BUILD_TESTS
    "Build BSP host unit and integration tests"
    OFF
)


# ============================================================================
# 6. CONFIGURATION VALIDATION
# ============================================================================

if(ESPx_PLATFORM AND STM32H7x_PLATFORM)
    message(FATAL_ERROR
        "[Build] Only one embedded platform may be selected."
    )
endif()

if(BSP_LOG_COLOR_SUPPORT AND NOT BSP_BUILD_LOGGER)
    message(WARNING
        "[Build] BSP_LOG_COLOR_SUPPORT is enabled, "
        "but BSP_BUILD_LOGGER is disabled."
    )
endif()

if(BSP_BUILD_TESTS AND
   (ESPx_PLATFORM OR STM32H7x_PLATFORM))
    message(FATAL_ERROR
        "[Build] BSP host tests require both embedded "
        "platform options to be OFF."
    )
endif()


# ============================================================================
# File End
# ============================================================================