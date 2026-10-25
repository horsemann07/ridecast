
# ============================================================================
# File        : bsp.cmake
# Description : BSP build configuration and platform port selection
# Author      : Jha Raghav
# Date        : 2026-01-01
# ============================================================================
# Copyright (c) 2026 Navyantra Tech Private Limited
#
# Description:
#   Configures the reusable Board Support Package (BSP), including:
#
#   - Platform-independent BSP modules
#   - Platform-specific hardware drivers
#   - Public BSP header directories
#   - ESP-IDF component registration
#   - STM32 static library registration
#   - Optional unit and integration tests
#
# Integration:
#   This file must be included from bsp/CMakeLists.txt.
#   It is not intended to be configured as a standalone CMake project.
#
# Requirements:
#   - Parent project must enable the C language.
#   - Exactly one supported target platform must be selected for
#     non-ESP-IDF builds.
#   - Platform SDK, HAL, and CMSIS dependencies must be supplied
#     by the parent project or registered platform components.
#   - Host unit tests require C++ and CppUTest.
#
# ============================================================================
# Change Log:
#
# Date          Author          Description
# ------------  --------------  ----------------------------------------------
# 2026-01-01    Jha Raghav      Initial version
# 2026-10-23    Jha Raghav      Modular BSP sources and test configuration
# ============================================================================


# ============================================================================
# 1. BSP BUILD OPTIONS
# ============================================================================
# These options allow the parent application to select which BSP modules
# are included in the final firmware image.
#
# All options use the BSP_ prefix to keep the BSP application-independent.
#
# BSP_BUILD_CLI:
#   Enable the platform-independent CLI implementation.
#
# BSP_BUILD_LOGGER:
#   Enable the asynchronous CMSIS-RTOS2 logger implementation.
#
# BSP_BUILD_PORT:
#   Enable platform-specific hardware driver implementations.
#
# BSP_BUILD_TESTS:
#   Enable host-based BSP unit and integration tests.
#   This option must not be enabled for an ESP-IDF firmware build.
# ============================================================================

option(BSP_BUILD_CLI "Build the platform-independent BSP CLI module" ON)
option(BSP_BUILD_LOGGER "Build the asynchronous CMSIS-RTOS2 logger module" ON)
option(BSP_BUILD_PORT "Build platform-specific BSP hardware drivers" ON)
option(BSP_BUILD_TESTS "Enable BSP host unit and integration tests" OFF)

# ============================================================================
# 2. BSP DIRECTORY CONFIGURATION
# ============================================================================
# Define absolute paths relative to the location of this CMake file.
#
# CMAKE_CURRENT_LIST_DIR is preferred because bsp.cmake may be included
# from another CMakeLists.txt.
#
# Directory structure:
#
# bsp/
# ├── bsp_config/
# ├── include/
# │   ├── bsp_core/
# |   │   ├── bsp_cli.h
# |   │   ├── bsp_log.h
# |   │   ├── bsp_defs.h
# |   │   └── bsp_err_sts.h
# ├── srcs/
# │   ├── bsp_core/
# |   │   ├── bsp_cli.c
# |   │   └── bsp_log.c
# │   └── port/
# └── tests/
#     ├── unit/
#     └── integration/
# ============================================================================

set(BSP_ROOT        "${CMAKE_CURRENT_LIST_DIR}")
set(BSP_INC_DIR     "${BSP_ROOT}/include")
set(BSP_CORE_DIR    "${BSP_ROOT}/include/bsp_core")
set(BSP_CFG_DIR     "${BSP_ROOT}/bsp_config")
set(BSP_SRC_DIR     "${BSP_ROOT}/srcs")
set(BSP_TEST_DIR    "${BSP_ROOT}/tests")


# ============================================================================
# 3. BSP PLATFORM VALIDATION AND SELECTION
# ============================================================================
# Exactly one supported platform must be enabled.
#
# ESPx_PLATFORM:
#   Selects the ESP32 platform implementation using ESP-IDF.
#
# STM32H7x_PLATFORM:
#   Selects the STM32H7 platform implementation using the STM32 HAL
#   and CMSIS-RTOS2.
#
# BOARD_PORT_FOLDER identifies the corresponding source directory under:
#
#   bsp/srcs/port/<platform>/
# ============================================================================

if(COMMAND idf_component_register)

    # ESP-IDF's component requirement scan evaluates this file in a
    # separate CMake scope, where application options may not be defined.
    set(BSP_IS_ESP_IDF TRUE)
    set(BOARD_PORT_FOLDER "esp32x")

elseif(ESPx_PLATFORM AND STM32H7x_PLATFORM)

    message(FATAL_ERROR
        "[BSP] Multiple target platforms selected. "
        "Enable only one of ESPx_PLATFORM or STM32H7x_PLATFORM."
    )

elseif(ESPx_PLATFORM)

    message(FATAL_ERROR
        "[BSP] ESPx_PLATFORM is enabled, but ESP-IDF component "
        "registration is unavailable."
    )

elseif(STM32H7x_PLATFORM)

    set(BSP_IS_ESP_IDF FALSE)
    set(BOARD_PORT_FOLDER "stm32h7x")

else()

    message(FATAL_ERROR
        "[BSP] No supported target platform selected. "
        "Enable ESPx_PLATFORM or STM32H7x_PLATFORM."
    )

endif()


# ============================================================================
# 4. BSP PUBLIC INCLUDE DIRECTORIES
# ============================================================================
# These directories are exposed to targets consuming the BSP.
#
# bsp_core:
#   Provides module-qualified includes:
#
#       #include "bsp_cli.h"
#       #include "bsp_log.h"
#       #include "bsp_err_sts.h"
#
# include:
#   Provides common BSP interfaces:
#
#       #include "bsp_uart.h"
#       #include "bsp_wifi.h"
#       #include "bsp_defs.h"
#       #include "bsp_os_map.h"
#
# bsp_config:
#   Provides configuration headers:
#
#       #include "bsp_config.h"
#       #include "bsp_cfg_map.h"
#
# Only the parent bsp_core directory is added. Individual module directories
# are intentionally not added to avoid ambiguous header lookup.
# ============================================================================

set(BSP_INCLUDE_DIRS "")

list(APPEND BSP_INCLUDE_DIRS
    "${BSP_CORE_DIR}"
    "${BSP_INC_DIR}"
    "${BSP_CFG_DIR}"
)


# ============================================================================
# 5. PLATFORM-INDEPENDENT BSP SOURCES
# ============================================================================
set(BSP_COMMON_SRCS "")
set(BSP_CORE_SRC "${BSP_SRC_DIR}/bsp_core")

# BSP resource configuration is always required.
list(APPEND BSP_COMMON_SRCS
    "${BSP_CFG_DIR}/bsp_cfg_map.c"
)

if(BSP_BUILD_CLI)
    list(APPEND BSP_COMMON_SRCS
        "${BSP_CORE_SRC}/bsp_cli.c"
    )
endif()

if(BSP_BUILD_LOGGER)
    list(APPEND BSP_COMMON_SRCS
        "${BSP_CORE_SRC}/bsp_log.c"
    )
endif()
# ============================================================================
# 6. PLATFORM-SPECIFIC BSP SOURCES
# ============================================================================

set(BSP_PORT_SRCS "")

if(BSP_BUILD_PORT)
    set(BSP_PORT_DIR "${BSP_SRC_DIR}/port/${BOARD_PORT_FOLDER}")

    if(NOT IS_DIRECTORY "${BSP_PORT_DIR}")
        message(FATAL_ERROR
            "[BSP] Platform port directory not found: ${BSP_PORT_DIR}"
        )
    endif()

    foreach(_bsp_driver IN ITEMS
        bsp_uart.c
        bsp_wifi.c
    )
        if(EXISTS "${BSP_PORT_DIR}/${_bsp_driver}")
            list(APPEND BSP_PORT_SRCS
                "${BSP_PORT_DIR}/${_bsp_driver}"
            )
        endif()
    endforeach()

    if(NOT BSP_PORT_SRCS)
        message(FATAL_ERROR
            "[BSP] No supported platform driver sources found in "
            "${BSP_PORT_DIR}. Enable BSP_BUILD_PORT or add supported drivers."
        )
    endif()
endif()

# ============================================================================
# 7. ESP32 PLATFORM TARGET REGISTRATION
# ============================================================================
# ESP32 builds use the ESP-IDF component system.
#
# idf_component_register():
#   Registers the BSP source files, public include directories, and
#   ESP-IDF component dependencies.
#
# COMPONENT_LIB:
#   The library target generated by ESP-IDF.
#
# No additional standalone BSP static library is created for ESP-IDF.
#
# Dependencies:
#   The list below retains the current BSP platform dependencies.
#   These should be reviewed whenever the enabled BSP drivers change.
#
# Note:
#   The local CLI is compiled directly from bsp_core/cli/cli.c.
#   An external ESP-IDF component named "cli" is not required.
# ============================================================================

if(BSP_IS_ESP_IDF)

    # ESP-IDF source-based component registration requires source files.
    if(NOT BSP_COMMON_SRCS AND NOT BSP_PORT_SRCS)

        message(FATAL_ERROR
            "[BSP] No BSP source files selected. "
            "Enable BSP_BUILD_CLI, BSP_BUILD_LOGGER, or BSP_BUILD_PORT."
        )

    endif()

    # Register BSP as an ESP-IDF component.
    idf_component_register(
        SRCS
            ${BSP_COMMON_SRCS}
            ${BSP_PORT_SRCS}

        INCLUDE_DIRS
            ${BSP_INCLUDE_DIRS}

        PRIV_REQUIRES
            system
            cmsis_esp
            freertos
            driver
            esp_wifi
            esp_event
            esp_netif
            lwip
            log
            nvs_flash
    )

    # Require ISO C11 for BSP source compilation.
    target_compile_features(${COMPONENT_LIB}
        PUBLIC
            c_std_11
    )


# ============================================================================
# 8. STM32H7 PLATFORM TARGET REGISTRATION
# ============================================================================
# STM32H7 builds use a standard CMake library target.
#
# When BSP sources are enabled:
#   A static library named bsp is created.
#
# When all BSP source modules are disabled:
#   An INTERFACE target is created to expose the BSP headers.
#
# The bsp::core alias provides a consistent target name for applications.
#
# Required CMSIS-RTOS2 and STM32 HAL dependencies must be linked by the
# parent firmware project.
# ============================================================================

elseif(STM32H7x_PLATFORM)

    if(BSP_COMMON_SRCS OR BSP_PORT_SRCS)

        # Create the compiled BSP library.
        add_library(bsp STATIC
            ${BSP_COMMON_SRCS}
            ${BSP_PORT_SRCS}
        )

        # Expose BSP headers to application targets.
        target_include_directories(bsp
            PUBLIC
                ${BSP_INCLUDE_DIRS}
        )

        # Require ISO C11 for BSP compilation.
        target_compile_features(bsp
            PUBLIC
                c_std_11
        )

    else()

        # Header-only BSP configuration.
        add_library(bsp INTERFACE)

        target_include_directories(bsp
            INTERFACE
                ${BSP_INCLUDE_DIRS}
        )

    endif()

    # Provide a namespaced BSP target for consuming applications.
    add_library(bsp::core ALIAS bsp)

endif()


# ============================================================================
# 9. BSP HOST UNIT AND INTEGRATION TESTS
# ============================================================================
# BSP tests are optional and disabled by default.
#
# Test directory structure:
#
# bsp/tests/
# ├── CMakeLists.txt
# ├── unit/
# │   ├── cli/
# │   ├── logger/
# │   └── ret_err_sts/
# └── integration/
#
# Unit tests:
#   Validate individual BSP modules using CppUTest and mocked dependencies.
#
# Integration tests:
#   Validate interactions between BSP modules and transport interfaces.
#
# Build requirements:
#   - Host compiler with C and C++ support
#   - CppUTest framework
#   - Suitable host-side RTOS/HAL mocks where required
#
# Tests must be configured with a host toolchain, separately from ESP-IDF
# firmware builds.
# ============================================================================

if(BSP_BUILD_TESTS)

    # Prevent native host tests from being added to an ESP-IDF firmware build.
    if(BSP_IS_ESP_IDF)

        message(FATAL_ERROR
            "[BSP] Host tests cannot be built as an ESP-IDF component. "
            "Configure BSP tests using a separate host CMake build."
        )

    endif()

    # CppUTest requires the C++ language to be enabled.
    if(NOT CMAKE_CXX_COMPILER_LOADED)

        message(FATAL_ERROR
            "[BSP] C++ is not enabled. "
            "Use project(<name> LANGUAGES C CXX) in the parent project."
        )

    endif()

    # Enable CTest test registration.
    include(CTest)
    enable_testing()

    # Register BSP unit and integration test directories.
    add_subdirectory(
        "${BSP_TEST_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/bsp_tests"
    )

endif()

# ============================================================================
# File End
# ============================================================================
