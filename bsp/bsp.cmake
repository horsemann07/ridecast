# =========================================================================
# File        : bsp.cmake
# Description : BSP build configuration and platform port selection
# Author      : Jha Raghav
# Date        : 2026-01-01
# =========================================================================
# Copyright (c) <2026> <Navyantra Tech Private Limited>
#
# This file configures the BSP module, including common BSP
# sources, platform-specific sources, include paths, and target
# registration.
# =========================================================================

# =========================================================================
# Changlog:
#
# <YYYY-MM-DD>   <Name>         <Description>
# 2026-01-01    Jha Raghav      Initial version
# =========================================================================
message(STATUS "[RideCast:BSP] Configuring BSP modules...")

# =========================================================================
# BSP CONFIGURATION SOURCES
# =========================================================================
# Platform-independent BSP configuration and compile-time settings.
# Platform-specific configuration can be selected through the BSP
# configuration layer without modifying the common BSP sources.
# =========================================================================
set(BSP_CFG_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/bsp_config/bsp_cfg_map.c"
)

# =========================================================================
# COMMON BSP SOURCES
# =========================================================================
# Core BSP modules shared across all supported board/platform ports.
#
# Platform-specific hardware implementation must be kept under:
# srcs/port/<platform>/
#
# These modules provide common BSP functionality such as:
# - Error/status handling
# - Logging
# - CLI interface
# - Console interface
# =========================================================================
set(BSP_COMMON_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/srcs/core/bsp_err_sts.c"
    "${CMAKE_CURRENT_LIST_DIR}/srcs/core/bsp_log.c"
    "${CMAKE_CURRENT_LIST_DIR}/srcs/core/bsp_cli.c"
    "${CMAKE_CURRENT_LIST_DIR}/srcs/core/bsp_console.c"
)

# =========================================================================
# BSP INCLUDE DIRECTORIES
# =========================================================================
# Include paths required by the BSP configuration, core modules, and
# public BSP interfaces.
#
# bsp_config -> BSP configuration headers
# include/core -> Common/core BSP headers
# include -> Public BSP API headers
# =========================================================================
set(BSP_INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/bsp_config"
    "${CMAKE_CURRENT_LIST_DIR}/include/core"
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

# =========================================================================
# BSP Platform Port Selection
# =========================================================================
# Select the platform-specific BSP port folder based on the active platform.
#
# ESPx_PLATFORM      -> ESP32-specific BSP implementation
# STM32H7x_PLATFORM  -> STM32H7-specific BSP implementation
# =========================================================================
if(ESPx_PLATFORM)
    set(BOARD_PORT_FOLDER "esp32x")
elseif(STM32H7x_PLATFORM)
    set(BOARD_PORT_FOLDER "stm32h7x")
endif()

# =========================================================================
# BSP Platform-Specific Sources
# =========================================================================
# Add BSP source files that are implemented differently for each platform.
# Common BSP functionality is maintained separately in BSP_COMMON_SRCS.
# =========================================================================
set(BSP_PORT_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/srcs/port/${BOARD_PORT_FOLDER}/bsp_uart.c"
    "${CMAKE_CURRENT_LIST_DIR}/srcs/port/${BOARD_PORT_FOLDER}/bsp_wifi.c"
)

# =========================================================================
# BSP Target Registration
# =========================================================================
# Register the BSP using the build system supported by the selected
# platform.
#
# ESP32:
# Uses ESP-IDF component registration and declares platform-specific
# dependencies required by the BSP.
#
# STM32H7:
# Builds the BSP as a static library and exposes the BSP include paths
# to dependent targets.
# =========================================================================
if(ESPx_PLATFORM)
    idf_component_register(
        SRCS
        ${BSP_CFG_SRCS}
        ${BSP_COMMON_SRCS}
        ${BSP_PORT_SRCS}

        INCLUDE_DIRS
        ${BSP_INCLUDE_DIRS}

        PRIV_REQUIRES
        system
        cmsis_esp
        freertos
        cli
        driver
        esp_wifi
        esp_event
        esp_netif
        lwip
        log
        nvs_flash
    )

elseif(STM32H7x_PLATFORM)
    add_library(bsp STATIC
        ${BSP_COMMON_SRCS}
        ${BSP_PORT_SRCS}
        ${BSP_CFG_SRCS}
    )

    target_include_directories(bsp PUBLIC
        ${BSP_INCLUDE_DIRS}
    )

else()
    # Fail the configuration early if no supported BSP platform is selected.
    message(FATAL_ERROR
        "[RideCast:BSP] No valid platform selected. "
        "Set ESPx_PLATFORM=ON or STM32H7x_PLATFORM=ON"
    )
endif()

# =========================================================================
# File End
# =========================================================================