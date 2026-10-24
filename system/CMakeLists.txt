# ============================================================================
# File        : CMakeLists.txt
# Description : System component configuration for ESP-IDF
# Author      : Jha Raghav
# Date        : 2026-10-23
# Current Dir : system/CMakeLists.txt
# ============================================================================
#
# Copyright (c) 2026 Navyantra Tech Private Limited
#
# Description:
#   Configures the reusable system component for the ESP-IDF platform.
#
# Responsibilities:
#   - Register system implementation sources.
#   - Export system public include directories.
#   - Declare required ESP-IDF component dependencies.
#   - Support ESP-IDF component dependency discovery.
#   - Avoid reliance on top-level application CMake variables.
#
# ============================================================================
# Change Log:
#
# Date          Author          Description
# ------------  --------------  ----------------------------------------------
# 2026-10-23    Jha Raghav      Fixed ESP-IDF component discovery compatibility
# 2026-10-23    Jha Raghav      Removed dependency on ESPx_PLATFORM variable
# 2026-01-01    Jha Raghav      Initial version
# ============================================================================

# ============================================================================
# 1. ESP-IDF BUILD SYSTEM VALIDATION
# ============================================================================

if(NOT COMMAND idf_component_register)
    message(FATAL_ERROR
        "[SYSTEM] This component requires the ESP-IDF build system.\n"
        "         Build the ESP32 project using idf.py."
    )
endif()

message(STATUS "[SYSTEM] Configuring system component")

# ============================================================================
# 2. COMPONENT DIRECTORY CONFIGURATION
# ============================================================================

set(SYSTEM_ROOT_DIR
    "${CMAKE_CURRENT_LIST_DIR}"
)

set(SYSTEM_INCLUDE_DIR
    "${SYSTEM_ROOT_DIR}/include"
)

set(SYSTEM_SOURCE_DIR
    "${SYSTEM_ROOT_DIR}/src"
)

# ============================================================================
# 3. SYSTEM SOURCE FILES
# ============================================================================
# Explicit source registration only.
# Do not use file(GLOB) for source discovery.
# ============================================================================

set(SYSTEM_SRCS
    # "${SYSTEM_SOURCE_DIR}/system.c"
)

# ============================================================================
# 4. PUBLIC INCLUDE DIRECTORIES
# ============================================================================

set(SYSTEM_PUBLIC_INCLUDES
    "${SYSTEM_INCLUDE_DIR}"
)

# ============================================================================
# 5. PRIVATE INCLUDE DIRECTORIES
# ============================================================================

set(SYSTEM_PRIVATE_INCLUDES
)

# ============================================================================
# 6. PUBLIC COMPONENT DEPENDENCIES
# ============================================================================
# Add dependencies here when corresponding types or headers are
# exposed through system public headers.
# ============================================================================

set(SYSTEM_PUBLIC_REQUIRES
)

# ============================================================================
# 7. PRIVATE COMPONENT DEPENDENCIES
# ============================================================================

set(SYSTEM_PRIVATE_REQUIRES
    freertos
    cmsis_esp
    mbedtls
    lwip
    driver
    esp_netif
    esp_wifi
    esp_event
    log
    esp-tls
    esp_common
)

# ============================================================================
# 8. ESP-IDF COMPONENT REGISTRATION
# ============================================================================

idf_component_register(
    SRCS
        ${SYSTEM_SRCS}

    INCLUDE_DIRS
        ${SYSTEM_PUBLIC_INCLUDES}

    PRIV_INCLUDE_DIRS
        ${SYSTEM_PRIVATE_INCLUDES}

    REQUIRES
        ${SYSTEM_PUBLIC_REQUIRES}

    PRIV_REQUIRES
        ${SYSTEM_PRIVATE_REQUIRES}
)


# ============================================================================
# File End
# ============================================================================