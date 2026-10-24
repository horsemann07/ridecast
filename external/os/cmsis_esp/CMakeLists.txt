# ============================================================================
# File        : CMakeLists.txt
# Description : CMSIS-RTOS2 adaptation layer for ESP-IDF
# Author      : Jha Raghav
# Date        : 2026-10-23
# Current Dir : external/os/cmsis_esp/CMakeLists.txt
# ============================================================================
#
# Copyright (c) 2026 Navyantra Tech Private Limited
#
# Description:
#   Registers the CMSIS-RTOS2 adaptation layer as an ESP-IDF component.
#
# ============================================================================
# Change Log:
#
# Date          Author          Description
# ------------  --------------  ----------------------------------------------
# 2026-10-23    Jha Raghav      Standardized inline set() and message() commands
# 2026-10-23    Jha Raghav      Removed unnecessary CMake status messages
# 2026-10-23    Jha Raghav      Removed ESPx_PLATFORM registration guard
# 2026-01-01    Jha Raghav      Initial version
# ============================================================================

# ============================================================================
# 1. ESP-IDF BUILD SYSTEM VALIDATION
# ============================================================================

if(NOT COMMAND idf_component_register)
    message(FATAL_ERROR "[cmsis_esp] ESP-IDF component registration is unavailable. Build through ESP-IDF.")
endif()

# ============================================================================
# 2. COMPONENT DIRECTORY CONFIGURATION
# ============================================================================

set(CMSIS_ESP_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(CMSIS_ESP_SOURCE_DIR "${CMSIS_ESP_ROOT_DIR}/Source")
set(CMSIS_ESP_INCLUDE_DIR "${CMSIS_ESP_ROOT_DIR}/Include")

# ============================================================================
# 3. SOURCE FILE CONFIGURATION
# ============================================================================

set(CMSIS_ESP_SRCS
    "${CMSIS_ESP_SOURCE_DIR}/cmsis_os2.c"
)

# ============================================================================
# 4. PUBLIC INCLUDE DIRECTORIES
# ============================================================================

set(CMSIS_ESP_PUBLIC_INCLUDES
    "${CMSIS_ESP_INCLUDE_DIR}"
)

# ============================================================================
# 5. SOURCE VALIDATION
# ============================================================================

foreach(CMSIS_ESP_SOURCE IN LISTS CMSIS_ESP_SRCS)
    if(NOT EXISTS "${CMSIS_ESP_SOURCE}")
        message(FATAL_ERROR "[cmsis_esp] Required source file not found: ${CMSIS_ESP_SOURCE}")
    endif()
endforeach()

# ============================================================================
# 6. ESP-IDF COMPONENT REGISTRATION
# ============================================================================

idf_component_register(
    SRCS ${CMSIS_ESP_SRCS}
    INCLUDE_DIRS ${CMSIS_ESP_PUBLIC_INCLUDES}
    REQUIRES freertos
    PRIV_REQUIRES esp_timer
)

# ============================================================================
# File End
# ============================================================================