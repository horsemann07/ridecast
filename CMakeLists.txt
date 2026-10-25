
# ============================================================================
# File        : CMakeLists.txt
# Description : RideCast top-level application build configuration
# Author      : Jha Raghav
# Date        : 2026-01-01
# ============================================================================
# Copyright (c) 2026 Navyantra Tech Private Limited
#
# Description:
#   Configures the RideCast embedded application, including:
#
#   - Application metadata and version information
#   - Git-based firmware version generation
#   - Target platform selection
#   - Reusable BSP module configuration
#   - ESP-IDF SDK initialization
#   - External component registration
#   - STM32H7 and host build integration
#   - Compiler diagnostic configuration
#   - Build output directories
#   - Optional host unit and integration tests
#   - Build configuration summary
#
# Integration:
#   This is the root CMakeLists.txt for the RideCast application.
#
#   ESP32:
#     Uses ESP-IDF component registration.
#
#   STM32H7:
#     Uses standard CMake targets and the application-supplied
#     STM32 toolchain, HAL, and CMSIS dependencies.
#
#   Host:
#     Supports native development and CppUTest-based testing.
#
# Requirements:
#   - CMake version 3.16 or later.
#   - ESP-IDF SDK for ESP32 firmware builds.
#   - STM32 toolchain and HAL for STM32H7 firmware builds.
#   - C and C++ host compilers for native testing.
#   - CppUTest for BSP unit and integration tests.
#
# Notes:
#   - RideCast is the application name.
#   - BSP modules use application-independent BSP naming.
#   - Compiler warnings support code-quality analysis but
#     do not establish MISRA C:2012 compliance.
#
# ============================================================================
# Change Log:
#
# Date          Author          Description
# ------------  --------------  ----------------------------------------------
# 2026-10-23    Jha Raghav      Modular BSP and platform build configuration
# 2026-01-01    Jha Raghav      Initial version
# ============================================================================


# ============================================================================
# 1. CMAKE MINIMUM VERSION
# ============================================================================
# Define the minimum supported CMake version.
#
# Do not call project() here because ESP-IDF requires its project.cmake
# to be included before the project() command.
# ============================================================================
cmake_minimum_required(VERSION 3.16)


# ============================================================================
# 2. APPLICATION METADATA
# ============================================================================
set(RC_PROJECT_NAME        "ridecast")
set(RC_PROJECT_VERSION     "0.1.0")
set(RC_PROJECT_DESCRIPTION
    "RideCast - Embedded WiFi/BLE/BT Navigation System"
)

# Root directory of the application.
set(RC_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

# ============================================================================
# 3. CMAKE MODULE DIRECTORIES
# ============================================================================
# Register the directory containing application-specific CMake modules.
#
# These modules may provide:
#   - Version generation
#   - Toolchain configuration
#   - Additional build utilities
# ============================================================================
list(APPEND CMAKE_MODULE_PATH
    "${RC_ROOT_DIR}/cmake"
)


# ============================================================================
# 4. APPLICATION VERSION CONFIGURATION
# ============================================================================
# Provide fallback application version information.
# ============================================================================
set(APP_VERSION_STRING      "0.0.0.0")
set(APP_SHORT_COMMIT_STRING "dev-unknown")
set(APP_RELEASE_DATE        "unknown")

set(_rc_version_script
    "${RC_ROOT_DIR}/cmake/generate_version.cmake"
)

if(EXISTS "${_rc_version_script}")
    include("${_rc_version_script}")
    # Synchronize CMake's version only for numeric version strings.
    if(APP_VERSION_STRING MATCHES
       "^[0-9]+(\\.[0-9]+)?(\\.[0-9]+)?(\\.[0-9]+)?$")

        set(RC_PROJECT_VERSION "${APP_VERSION_STRING}")

    endif()

    message(STATUS
        "[RideCast] Application version generator loaded"
    )
else()
    message(WARNING
        "[RideCast] Version generator not found. "
        "Using fallback version information."
    )
endif()


# ============================================================================
# 5. TARGET PLATFORM SELECTION
# ============================================================================
# Select the target platform before project() is executed.
#
# ESPx_PLATFORM:
#   Build firmware using the ESP-IDF build system.
#
# STM32H7x_PLATFORM:
#   Build firmware using the STM32 toolchain and HAL.
#
# When both options are OFF:
#   Configure a native host or simulation build.
#
# Exactly one embedded platform may be enabled.
# ============================================================================

option(ESPx_PLATFORM
"Build RideCast firmware for ESP32 using ESP-IDF"
    ON
)

option(STM32H7x_PLATFORM
    "Build RideCast firmware for STM32H7"
    OFF
)


# ============================================================================
# 6. PLATFORM VALIDATION
# ============================================================================
# Prevent conflicting embedded platform selections.
#
# RC_PLATFORM_LABEL is used only for diagnostic messages.
# ============================================================================

if(ESPx_PLATFORM AND STM32H7x_PLATFORM)
    message(FATAL_ERROR
        "[RideCast] Multiple embedded platforms selected. "
        "Enable only ESPx_PLATFORM or STM32H7x_PLATFORM."
    )
elseif(ESPx_PLATFORM)
    set(RC_PLATFORM_LABEL "ESP32 (ESP-IDF)")
elseif(STM32H7x_PLATFORM)
    set(RC_PLATFORM_LABEL "STM32H7")
else()
    set(RC_PLATFORM_LABEL "Generic Host / Simulation")
endif()


# ============================================================================
# 7. BSP BUILD CONFIGURATION
# ============================================================================
# Configure reusable BSP modules at the application level.
#
# These are CMake cache options and are accessible throughout the build,
# including bsp/bsp.cmake.
#
# BSP_BUILD_CLI:
#   Build the portable CLI implementation.
#
# BSP_BUILD_LOGGER:
#   Build the asynchronous CMSIS-RTOS2 logger.
#
# BSP_BUILD_PORT:
#   Build platform-specific BSP hardware drivers.
#
# BSP_BUILD_TESTS:
#   Enable native host unit and integration tests.
#
# Options must be defined before ESP-IDF component discovery or
# add_subdirectory(bsp) so the selected values are available.
# ============================================================================

option(BSP_BUILD_CLI
    "Build the platform-independent BSP CLI module"
    ON
)

option(BSP_BUILD_LOGGER
    "Build the asynchronous CMSIS-RTOS2 logger module"
    ON
)

option(BSP_BUILD_PORT
    "Build platform-specific BSP hardware drivers"
    ON
)

option(BSP_BUILD_TESTS
    "Enable BSP host unit and integration tests"
    OFF
)


# ============================================================================
# 8. BSP CONFIGURATION VALIDATION
# ============================================================================
# BSP host tests must use a native host toolchain.
#
# They are not included in ESP-IDF or STM32 firmware builds.
# Firmware builds must therefore disable BSP_BUILD_TESTS.
# ============================================================================

if(BSP_BUILD_TESTS AND
   (ESPx_PLATFORM OR STM32H7x_PLATFORM))
    message(FATAL_ERROR
        "[RideCast] BSP_BUILD_TESTS requires a native host build. "
        "Disable ESPx_PLATFORM and STM32H7x_PLATFORM."
    )
endif()


# ============================================================================
# 9. EARLY PROJECT INFORMATION
# ============================================================================
# Display application and platform information before SDK initialization.
#
# This ensures that configuration logs contain useful project information
# even when a toolchain or SDK cannot be loaded.
# ============================================================================
message(STATUS "[RideCast] ========================================")
message(STATUS "[RideCast] Project   : ${RC_PROJECT_NAME}")
message(STATUS "[RideCast] Description: ${RC_PROJECT_DESCRIPTION}")
message(STATUS "[RideCast] Version   : ${APP_VERSION_STRING}")
message(STATUS "[RideCast] Commit    : ${APP_SHORT_COMMIT_STRING}")
message(STATUS "[RideCast] Date      : ${APP_RELEASE_DATE}")
message(STATUS "[RideCast] Platform  : ${RC_PLATFORM_LABEL}")
message(STATUS "[RideCast] ========================================")


# ============================================================================
# 10. ESP-IDF SDK CONFIGURATION
# ============================================================================
# Configure the ESP-IDF SDK before project().
#
# The application uses a locally maintained ESP-IDF SDK:
#
#   sdk/esp/
#
# IDF_PATH is exposed as a CMake cache variable so it can be overridden
# when necessary.
#
# This section is executed only for ESP32 firmware builds.
# ============================================================================
if(ESPx_PLATFORM)

    # ========================================================================
    # ESP-IDF SDK PATH CONFIGURATION
    # ========================================================================

    set(IDF_PATH
        "${RC_ROOT_DIR}/sdk/esp"
        CACHE PATH
        "Path to the ESP-IDF SDK"
    )

    # Normalize the selected SDK path.
    file(TO_CMAKE_PATH
        "${IDF_PATH}"
        RIDECAST_SELECTED_IDF_PATH
    )

    # ========================================================================
    # ESP-IDF ENVIRONMENT VALIDATION
    # ========================================================================

    if(DEFINED ENV{IDF_PATH} AND NOT "$ENV{IDF_PATH}" STREQUAL "")

        # Normalize the environment SDK path.
        file(TO_CMAKE_PATH
            "$ENV{IDF_PATH}"
            RIDECAST_ENV_IDF_PATH
        )

        # Compare normalized paths.
        if(NOT RIDECAST_ENV_IDF_PATH STREQUAL RIDECAST_SELECTED_IDF_PATH)

            message(WARNING
                "[RideCast] Environment IDF_PATH differs from "
                "the selected SDK path.\n"
                "Environment: ${RIDECAST_ENV_IDF_PATH}\n"
                "Selected   : ${RIDECAST_SELECTED_IDF_PATH}"
            )

        endif()

    endif()

    # ========================================================================
    # ESP-IDF SDK VALIDATION
    # ========================================================================

    if(NOT EXISTS "${RIDECAST_SELECTED_IDF_PATH}/tools/cmake/project.cmake")

        message(FATAL_ERROR
            "[RideCast] ESP-IDF SDK not found at: "
            "${RIDECAST_SELECTED_IDF_PATH}\n"
            "Initialize the SDK submodule or configure IDF_PATH."
        )

    endif()

    # ========================================================================
    # ESP-IDF CMAKE PACKAGE SEARCH PATH
    # ========================================================================

    list(APPEND CMAKE_PREFIX_PATH
        "${RIDECAST_SELECTED_IDF_PATH}"
    )

    # ========================================================================
    # CONFIGURATION STATUS
    # ========================================================================

    message(STATUS
        "[RideCast] ESP-IDF SDK: ${RIDECAST_SELECTED_IDF_PATH}"
    )

endif()

# ============================================================================
# 11. ESP-IDF EXTERNAL COMPONENT REGISTRATION
# ============================================================================
# Register application-specific and reusable firmware components.
#
# EXTRA_COMPONENT_DIRS must be configured before the ESP-IDF
# project.cmake and project() commands are executed.
#
# Components:
#   system:
#     Shared application system utilities.
#
#   cmsis_esp:
#     CMSIS-RTOS2 wrapper for ESP-IDF / FreeRTOS.
#
#   backoffAlgorithmWrapper:
#     External backoff algorithm component.
#
#   bsp:
#     Reusable Board Support Package.
#
#   app/main:
#     Application entry and startup logic.
#
# BSP internal CLI:
#   The CLI implementation is located inside bsp/bsp_core/cli.
#   The old external/cli component is not required by the BSP CLI.
#
# IMPORTANT:
#   Every ESP-IDF component must provide its own CMakeLists.txt.
#   For BSP, bsp/CMakeLists.txt includes bsp.cmake.
# ============================================================================

if(ESPx_PLATFORM)

    set(EXTRA_COMPONENT_DIRS
        "${RC_ROOT_DIR}/system"
        "${RC_ROOT_DIR}/external/os/cmsis_esp"
        "${RC_ROOT_DIR}/external/component/backoffAlgorithmWrapper"
        "${RC_ROOT_DIR}/bsp"
        "${RC_ROOT_DIR}/app/main"
    )

    # Optional components can be enabled here when required.
    #
    # list(APPEND EXTRA_COMPONENT_DIRS
    #     "${RC_ROOT_DIR}/external/cli"
    #     "${RC_ROOT_DIR}/nal"
    # )

    message(STATUS
        "[RideCast] ESP-IDF external components configured"
    )

endif()


# ============================================================================
# 12. PROJECT INITIALIZATION
# ============================================================================
# Initialize the project using the selected platform build system.
#
# ESP32:
#   ESP-IDF project.cmake must be included before project().
#   ESP-IDF discovers the BSP through EXTRA_COMPONENT_DIRS.
#
# STM32H7:
#   Standard CMake project with C and C++ language support.
#   Toolchain selection must be configured before CMake initialization.
#
# Host:
#   Standard native CMake project for development and testing.
# ============================================================================

if(ESPx_PLATFORM)

    # Load the ESP-IDF build system.
    include("${IDF_PATH}/tools/cmake/project.cmake")

    # Initialize the ESP-IDF application.
    project(${RC_PROJECT_NAME}
        VERSION     ${RC_PROJECT_VERSION}
        DESCRIPTION "${RC_PROJECT_DESCRIPTION}"
    )

    message(STATUS
        "[RideCast] ESP-IDF project initialized"
    )

elseif(STM32H7x_PLATFORM)

    # Initialize the STM32 application.
    project(${RC_PROJECT_NAME}
        VERSION     ${RC_PROJECT_VERSION}
        DESCRIPTION "${RC_PROJECT_DESCRIPTION}"
        LANGUAGES C CXX ASM
    )

    message(STATUS
        "[RideCast] STM32H7 project initialized"
    )

else()

    # Initialize the native host project.
    project(${RC_PROJECT_NAME}
        VERSION     ${RC_PROJECT_VERSION}
        DESCRIPTION "${RC_PROJECT_DESCRIPTION}"
        LANGUAGES C CXX
    )

    message(STATUS
        "[RideCast] Native host project initialized"
    )

endif()

if(ESPx_PLATFORM)
    idf_build_set_property(
        COMPILE_DEFINITIONS
        CMAKE_ESPx_PLATFORM=1
        APPEND
    )
endif()

# ============================================================================
# 13. BUILD OUTPUT DIRECTORY CONFIGURATION
# ============================================================================
# Keep native build artifacts in dedicated output directories.
#
#   build/bin/  - Executables
#   build/lib/  - Shared libraries
#   build/lib/  - Static libraries
#
# ESP-IDF manages its own build outputs, so these settings are not
# applied to ESP-IDF builds.
# ============================================================================

if(NOT ESPx_PLATFORM)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
    )

    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/lib"
    )

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/lib"
    )
endif()


# ============================================================================
# 14. COMPILER DIAGNOSTIC CONFIGURATION
# ============================================================================
# Define reusable compiler warning settings for non-ESP builds.
#
# The INTERFACE target avoids modifying global compiler flags.
#
# Only compiler-supported options should be enabled.
# C-specific options are restricted to C compilation.
#
# IMPORTANT:
#   Compiler warnings are not equivalent to MISRA C:2012 compliance.
#   Formal compliance requires suitable static analysis and review.
#
# Targets opt into these warnings by linking:
#
#   target_link_libraries(<target> PRIVATE ridecast_project_warnings)
# ============================================================================

if(NOT ESPx_PLATFORM)

    add_library(ridecast_project_warnings INTERFACE)

    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")

        target_compile_options(ridecast_project_warnings
            INTERFACE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wundef
                -Wshadow
                -fno-common
                -ffunction-sections
                -fdata-sections

                # C-language-specific diagnostics.
                $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>
                $<$<COMPILE_LANGUAGE:C>:-Wwrite-strings>
        )

    endif()

endif()


# ============================================================================
# 15. STM32H7 BSP INTEGRATION
# ============================================================================
# Register the BSP with a standard CMake firmware project.
#
# bsp/CMakeLists.txt includes bsp.cmake, which creates:
#
#   bsp
#   bsp::core
#
# This integration does not create the STM32 startup executable or link
# the HAL/CMSIS libraries. Those remain application responsibilities.
#
# ESP-IDF BSP registration is already handled by component discovery.
# ============================================================================

if(STM32H7x_PLATFORM)

    add_subdirectory(
        "${RC_ROOT_DIR}/bsp"
    )

    message(STATUS
        "[RideCast] STM32H7 BSP registered"
    )

endif()


# ============================================================================
# 16. HOST TEST CONFIGURATION
# ============================================================================
# Build BSP unit and integration tests only for native host builds.
#
# Test directory:
#
#   bsp/tests/
#     unit/
#     integration/
#
# The test configuration is responsible for finding or linking CppUTest
# and registering the individual test executables.
#
# The host build directly adds bsp/tests rather than including bsp.cmake,
# because bsp.cmake currently requires an embedded platform selection.
# ============================================================================

if((NOT ESPx_PLATFORM) AND (NOT STM32H7x_PLATFORM))

    if(BSP_BUILD_TESTS)

        # Enable standard CMake test registration.
        include(CTest)

        if(NOT BUILD_TESTING)

            message(FATAL_ERROR
                "[RideCast] BSP_BUILD_TESTS is ON but "
                "BUILD_TESTING is OFF."
            )

        endif()

        # Register BSP host test targets.
        add_subdirectory(
            "${RC_ROOT_DIR}/bsp/tests"
        )

        message(STATUS
            "[RideCast] BSP host tests registered"
        )

    else()

        message(STATUS
            "[RideCast] BSP host tests disabled"
        )

    endif()

endif()


# ============================================================================
# 17. CONFIGURATION SUMMARY
# ============================================================================
# Print the resolved configuration after project initialization.
#
# This section provides a consistent summary for:
#   - Local development
#   - Continuous integration
#   - Firmware build diagnostics
#   - BSP module selection
# ============================================================================

message(STATUS "")
message(STATUS "[RideCast] ========================================")
message(STATUS "[RideCast] BUILD CONFIGURATION SUMMARY")
message(STATUS "[RideCast] ========================================")

# Application information.
message(STATUS "[RideCast] Application")
message(STATUS "[RideCast]   Name       : ${RC_PROJECT_NAME}")
message(STATUS "[RideCast]   Version    : ${APP_VERSION_STRING}")
message(STATUS "[RideCast]   Commit     : ${APP_SHORT_COMMIT_STRING}")
message(STATUS "[RideCast]   Date       : ${APP_RELEASE_DATE}")

# Platform information.
message(STATUS "[RideCast] Platform")
message(STATUS "[RideCast]   Target     : ${RC_PLATFORM_LABEL}")
message(STATUS "[RideCast]   ESP32      : ${ESPx_PLATFORM}")
message(STATUS "[RideCast]   STM32H7    : ${STM32H7x_PLATFORM}")

# BSP configuration.
message(STATUS "[RideCast] BSP Modules")
message(STATUS "[RideCast]   CLI        : ${BSP_BUILD_CLI}")
message(STATUS "[RideCast]   Logger     : ${BSP_BUILD_LOGGER}")
message(STATUS "[RideCast]   Port       : ${BSP_BUILD_PORT}")
message(STATUS "[RideCast]   Tests      : ${BSP_BUILD_TESTS}")

# Toolchain information.
message(STATUS "[RideCast] Build Environment")
message(STATUS "[RideCast]   Compiler   : ${CMAKE_C_COMPILER_ID}")
message(STATUS "[RideCast]   Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "[RideCast]   Build dir  : ${CMAKE_BINARY_DIR}")

message(STATUS "[RideCast] ========================================")
message(STATUS "[RideCast] Configuration complete")
message(STATUS "[RideCast] ========================================")


# ============================================================================
# File End
# ============================================================================
