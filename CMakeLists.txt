#
# RideCast Top-Level CMake Configuration
#
# MISRA C:2012 compliant embedded project
# - Modular component structure
# - ESP-IDF integration
# - Clear dependency management
#

cmake_minimum_required(VERSION 3.16)

# =========================================================================
# PROJECT METADATA
# =========================================================================
set(PROJECT_NAME "ridecast")
set(PROJECT_VERSION "0.1.0")
set(PROJECT_DESCRIPTION "RideCast - Embedded WiFi/BLE/BT Navigation System")

# =========================================================================
# CMAKE MODULES
# =========================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Safe defaults: used if Git or generate_version.cmake is unavailable.
# These are overridden by generate_version.cmake if it exists.
set(APP_VERSION_STRING      "0.0.0.0")
set(APP_SHORT_COMMIT_STRING "dev-unknown")
set(APP_RELEASE_DATE        "unknown")

# Load version generator (sets APP_VERSION_STRING, APP_SHORT_COMMIT_STRING,
# APP_RELEASE_DATE and generates app_version.h in the build directory).
if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/generate_version.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/generate_version.cmake")
    # Synchronize CMake project version with the Git-derived version string.
    set(PROJECT_VERSION "${APP_VERSION_STRING}")
    message(STATUS "[RideCast] ✓ Version generator loaded")
else()
    message(WARNING "[RideCast] Version generator not found; using fallback metadata")
endif()


# =========================================================================
# PLATFORM SWITCH
# =========================================================================
set(ESP_PLATFORM ON CACHE BOOL "Build for ESP board using ESP-IDF")


# Human-readable platform label (CMAKE_SYSTEM_NAME is not reliable here;
# it is set by the toolchain AFTER project() is called).
if(ESP_PLATFORM)
    set(RC_PLATFORM_LABEL "ESP32 (ESP-IDF)")
else()
    set(RC_PLATFORM_LABEL "Generic Host / Simulation")
endif()


# =========================================================================
# PROJECT INFORMATION BANNER
# Printed early so CI logs always show identity, even on failure.
# =========================================================================
message(STATUS "[RideCast] ========================================")
message(STATUS "[RideCast] Project : ${PROJECT_NAME}")
message(STATUS "[RideCast] Desc    : ${PROJECT_DESCRIPTION}")
message(STATUS "[RideCast] Version : ${APP_VERSION_STRING}")
message(STATUS "[RideCast] Commit  : ${APP_SHORT_COMMIT_STRING}")
message(STATUS "[RideCast] Date    : ${APP_RELEASE_DATE}")
message(STATUS "[RideCast] Platform: ${RC_PLATFORM_LABEL}")
message(STATUS "[RideCast] ========================================")


# =========================================================================
# ESP-IDF PLATFORM BUILD
# =========================================================================
if(ESP_PLATFORM)
    # -------------------------------------------------------------------
    # Warn if IDF_PATH is set externally; it may conflict with local SDK.
    # -------------------------------------------------------------------
    if(DEFINED ENV{IDF_PATH})
        message(WARNING
            "[RideCast] IDF_PATH is set externally in environment.\n"
            "           This may override the local SDK.\n"
            "           If build fails: unset IDF_PATH && idf.py build"
        )
    endif()

    # Local ESP-IDF SDK path (bundled with project under sdk/esp).
    set(IDF_PATH "${CMAKE_SOURCE_DIR}/sdk/esp" CACHE PATH "Path to ESP-IDF")

    # Guard: fail early and clearly if ESP-IDF is missing.
    if(NOT EXISTS "${IDF_PATH}/tools/cmake/project.cmake")
        message(FATAL_ERROR
            "[RideCast] ESP-IDF not found at: ${IDF_PATH}\n"
            "           Clone it: git submodule update --init --recursive\n"
            "           Or download: https://github.com/espressif/esp-idf"
        )
    endif()

    list(APPEND CMAKE_PREFIX_PATH "${IDF_PATH}")
    message(STATUS "[RideCast] IDF_PATH: ${IDF_PATH}")

    # -------------------------------------------------------------------
    # EXTRA_COMPONENT_DIRS: registers custom components with ESP-IDF.
    # Must be set BEFORE include(project.cmake).
    # -------------------------------------------------------------------
    set(EXTRA_COMPONENT_DIRS
        "${CMAKE_SOURCE_DIR}/system"                                    # System utilities
        "${CMAKE_SOURCE_DIR}/external/cli"                              # CLI component
        "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp"                     # CMSIS-RTOS2 wrapper
        "${CMAKE_SOURCE_DIR}/external/component/backoffAlgorithmWrapper"# Backoff algorithm
        "${CMAKE_SOURCE_DIR}/bsp"                                       # Board support package
        # "${CMAKE_SOURCE_DIR}/nal"                                     # NAL (disabled)
        "${CMAKE_SOURCE_DIR}/app/main"                                  # Application entry
    )

    message(STATUS "[RideCast] Component directories registered")

    # -------------------------------------------------------------------
    # Load ESP-IDF build system.
    # IMPORTANT: this MUST be included BEFORE calling project().
    # -------------------------------------------------------------------
    include(${IDF_PATH}/tools/cmake/project.cmake)

    # Declare the CMake project (ESP-IDF style, after its include).
    project(${PROJECT_NAME}
        VERSION     ${PROJECT_VERSION}
        DESCRIPTION ${PROJECT_DESCRIPTION}
    )

    message(STATUS "[RideCast] ESP-IDF project initialized")

else()
    # -------------------------------------------------------------------
    # Generic / host build (unit test or simulation).
    # -------------------------------------------------------------------
    message(STATUS "[RideCast] Platform: Generic (non-ESP)")

    project(${PROJECT_NAME}
        VERSION     ${PROJECT_VERSION}
        DESCRIPTION ${PROJECT_DESCRIPTION}
    )
endif()

# =========================================================================
# COMPILER FLAGS (MISRA C:2012 compliant warning set)
# Applied only for non-ESP builds; ESP-IDF manages its own flags.
# =========================================================================
if(NOT ESP_PLATFORM)
    # Rule: enable all standard diagnostics to catch MISRA violations early.
    target_compile_options(${PROJECT_NAME} INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wstrict-prototypes
        -Wwrite-strings
        -Wconversion
        -Wsign-conversion
        -Wundef
        -Wshadow
        -fno-common
        -ffunction-sections
        -fdata-sections
    )
endif()

# =========================================================================
# BUILD OUTPUT DIRECTORIES
# Centralise all binaries under build/bin and libraries under build/lib.
# =========================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# =========================================================================
# EXTERNAL COMPONENTS (Generic build only)
# =========================================================================
if(NOT ESP_PLATFORM)
    # Uncomment when generic external libs are needed:
    # add_subdirectory(external)
    message(STATUS "[RideCast] Generic build: external components optional")
endif()

# =========================================================================
# CONFIGURATION SUMMARY (printed last so all values are resolved)
# =========================================================================
message(STATUS "[RideCast] ========================================")
message(STATUS "[RideCast] ✓ Configuration complete")
message(STATUS "[RideCast] ✓ Platform   : ${RC_PLATFORM_LABEL}") 
message(STATUS "[RideCast] ✓ Compiler   : ${CMAKE_C_COMPILER_ID}")
message(STATUS "[RideCast] ✓ Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "[RideCast] ✓ Version    : ${APP_VERSION_STRING}")
message(STATUS "[RideCast] ✓ Commit     : ${APP_SHORT_COMMIT_STRING}")
message(STATUS "[RideCast] ✓ Date       : ${APP_RELEASE_DATE}")
message(STATUS "[RideCast] ✓ Output dir : ${CMAKE_BINARY_DIR}")
message(STATUS "[RideCast] ========================================")