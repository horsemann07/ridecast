# ============================================================================
# File        : CMakeLists.txt
# Description : RideCast application main component configuration
# Author      : Jha Raghav
# Date        : 2026-01-01
# Current Dir : ridecast/app/main/CMakeLists.txt
# ============================================================================
# Copyright (c) 2026 Navyantra Tech Private Limited
#
# Description:
#   Configures the RideCast application component for ESP-IDF.
#
#   Responsibilities:
#   - Register application configuration sources.
#   - Register application implementation sources.
#   - Register application interface sources.
#   - Register application entry point.
#   - Configure application include directories.
#   - Declare ESP-IDF component dependencies.
#   - Integrate the reusable BSP component.
#
#   BSP functionality:
#   - CLI
#   - Asynchronous logging
#   - Platform-specific drivers
#
# ============================================================================
# Change Log:
#
# Date          Author          Description
# ------------  --------------  ----------------------------------------------
# 2026-10-23    Jha Raghav      Updated application sources and directories
# 2026-10-23    Jha Raghav      Integrated BSP-owned CLI and logger
# 2026-01-01    Jha Raghav      Initial version
# ============================================================================

cmake_minimum_required(VERSION 3.16)

# ============================================================================
# APPLICATION DIRECTORY CONFIGURATION
# ============================================================================

# Absolute path of the current ESP-IDF component.
set(RIDECAST_APP_MAIN_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Parent directory containing all application modules.
get_filename_component(
    RIDECAST_APP_DIR
    "${RIDECAST_APP_MAIN_DIR}/.."
    ABSOLUTE
)

set(RIDECAST_APP_CONFIG_DIR
    "${RIDECAST_APP_DIR}/app_config"
)

set(RIDECAST_APP_SRCS_DIR
    "${RIDECAST_APP_DIR}/srcs"
)


# ============================================================================
# APPLICATION CONFIGURATION SOURCES
# ============================================================================

set(RIDECAST_APP_CONFIG_DIR_INCS
    "${RIDECAST_APP_CONFIG_DIR}/incs"
)

set(RIDECAST_APP_CONFIG_DIR_SRCS
    "${RIDECAST_APP_CONFIG_DIR}/srcs/rc_os_map.c"
    "${RIDECAST_APP_CONFIG_DIR}/srcs/rc_peripheral_map.c"
)

# ============================================================================
# APPLICATION IMPLEMENTATION SOURCES
# ============================================================================
#
# These files are retained from the existing application configuration.
# Only files that exist will be registered.
#
# Add other application implementation files here as needed.
# ============================================================================

set(RIDECAST_APP_DIR_INCS
    "${RIDECAST_APP_DIR}/incs"
)


set(RIDECAST_APP_SRCS "")

set(RIDECAST_APP_CANDIDATE_SRCS
    "${RIDECAST_APP_SRCS_DIR}/uart_app.c"
    "${RIDECAST_APP_SRCS_DIR}/wifi_app.c"
    "${RIDECAST_APP_SRCS_DIR}/conn_manager.c"
    "${RIDECAST_APP_SRCS_DIR}/ridecast_main.c"
)

foreach(APP_SOURCE IN LISTS RIDECAST_APP_CANDIDATE_SRCS)
    if(EXISTS "${APP_SOURCE}")
        list(APPEND RIDECAST_APP_SRCS
            "${APP_SOURCE}"
        )
    else()
        message(STATUS
            "[RideCast:APP] Source not present: ${APP_SOURCE}"
        )
    endif()
endforeach()


# ============================================================================
# APPLICATION ENTRY POINT
# ============================================================================

set(RIDECAST_APP_ENTRY_SRCS
    "${RIDECAST_APP_MAIN_DIR}/app_main.c"
)


# ============================================================================
# APPLICATION INCLUDE DIRECTORIES
# ============================================================================

set(RIDECAST_APP_INCLUDE_DIRS
    "${RIDECAST_APP_CONFIG_DIR_INCS}"
    "${RIDECAST_APP_DIR_INCS}"
    "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp/Include"
    "${CMAKE_BINARY_DIR}/version"
)


# ============================================================================
# 9. APPLICATION SOURCE VALIDATION
# ============================================================================

set(RIDECAST_REQUIRED_SRCS
    ${RIDECAST_APP_CONFIG_DIR_SRCS}
    ${RIDECAST_APP_ENTRY_SRCS}
)

foreach(APP_SOURCE IN LISTS RIDECAST_REQUIRED_SRCS)
    if(NOT EXISTS "${APP_SOURCE}")
        message(FATAL_ERROR
            "[RideCast:APP] Required source file not found: "
            "${APP_SOURCE}"
        )
    endif()
endforeach()


# ============================================================================
# COMPLETE APPLICATION SOURCE LIST
# ============================================================================

set(RIDECAST_ALL_APP_SRCS
    ${RIDECAST_APP_CONFIG_DIR_SRCS}
    ${RIDECAST_APP_SRCS}
    ${RIDECAST_APP_INTERFACE_SRCS}
    ${RIDECAST_APP_ENTRY_SRCS}
    ${RIDECAST_APP_TEST_SRCS}
)

# ============================================================================
# ESP-IDF COMPONENT REGISTRATION
# ============================================================================
#
# CLI and logger are provided by the BSP component.
#
# CMSIS-RTOS2 is provided by the cmsis_esp component.
# Do not compile cmsis_os2.c again in this component.
# ============================================================================

idf_component_register(
    SRCS
        ${RIDECAST_ALL_APP_SRCS}

    INCLUDE_DIRS
        ${RIDECAST_APP_INCLUDE_DIRS}

    PRIV_REQUIRES
        freertos
        cmsis_esp
        system
        lwip
        driver
        esp_netif
        esp_wifi
        esp_event
        log
        esp-tls
        esp_common
        bsp
        nvs_flash
        backoffAlgorithmWrapper
)

# ============================================================================
# File End
# ============================================================================