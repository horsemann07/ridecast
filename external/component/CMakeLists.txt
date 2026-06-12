# ============================================================================
# RideCast External Components
#
# Purpose:
# - Manage shared external components for all platforms
# - Use system stacks on ESP-IDF (lwIP/mbedTLS)
# - Use project stacks on STM32H7 (if enabled)
# - Provide Backoff Algorithm support
# ============================================================================

message(STATUS "[RideCast:COMP] Configuring external/component...")

# Platform switch (project convention)
option(STM32H7_PLATFORM "Build for STM32H7 platform" OFF)

# These are normally set by parent (external/CMakeLists.txt).
# Keep defaults here for safety.
if(NOT DEFINED USE_PLATFORM_LWIP)
    if(ESP_PLATFORM)
        set(USE_PLATFORM_LWIP ON CACHE BOOL "Use platform lwIP" FORCE)
    else()
        set(USE_PLATFORM_LWIP OFF CACHE BOOL "Use platform lwIP" FORCE)
    endif()
endif()

if(NOT DEFINED USE_PLATFORM_MBEDTLS)
    if(ESP_PLATFORM)
        set(USE_PLATFORM_MBEDTLS ON CACHE BOOL "Use platform mbedTLS" FORCE)
    else()
        set(USE_PLATFORM_MBEDTLS OFF CACHE BOOL "Use platform mbedTLS" FORCE)
    endif()
endif()

# ----------------------------------------------------------------------------
# Optional component stacks
# ----------------------------------------------------------------------------
if(NOT USE_PLATFORM_LWIP)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/lwip/CMakeLists.txt")
        add_subdirectory(lwip)
        message(STATUS "[RideCast:COMP] ✓ Added local lwIP")
    else()
        message(WARNING "[RideCast:COMP] lwip/CMakeLists.txt not found")
    endif()
else()
    message(STATUS "[RideCast:COMP] Using system lwIP")
endif()

if(NOT USE_PLATFORM_MBEDTLS)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/mbedtls/CMakeLists.txt")
        add_subdirectory(mbedtls)
        message(STATUS "[RideCast:COMP] ✓ Added local mbedTLS")
    else()
        message(WARNING "[RideCast:COMP] mbedtls/CMakeLists.txt not found")
    endif()
else()
    message(STATUS "[RideCast:COMP] Using system mbedTLS")
endif()

# ----------------------------------------------------------------------------
# Backoff Algorithm support
# Prefer wrapper component if present, else fallback to direct upstream sources
# ----------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/backoffAlgorithmWrapper/CMakeLists.txt")
    add_subdirectory(backoffAlgorithmWrapper)
    message(STATUS "[RideCast:COMP] ✓ Added backoffAlgorithmWrapper")

elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/backoffAlgorithm/backoffAlgorithmFilePaths.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/backoffAlgorithm/backoffAlgorithmFilePaths.cmake")

    if(ESP_PLATFORM)
        idf_component_register(
            SRCS
                ${BACKOFF_ALGORITHM_SOURCES}
            INCLUDE_DIRS
                ${BACKOFF_ALGORITHM_INCLUDE_PUBLIC_DIRS}
        )
        message(STATUS "[RideCast:COMP] ✓ Registered backoffAlgorithm (ESP-IDF)")
    else()
        add_library(backoff_algorithm STATIC
            ${BACKOFF_ALGORITHM_SOURCES}
        )
        target_include_directories(backoff_algorithm PUBLIC
            ${BACKOFF_ALGORITHM_INCLUDE_PUBLIC_DIRS}
        )
        message(STATUS "[RideCast:COMP] ✓ Created backoff_algorithm static library")
    endif()

else()
    message(FATAL_ERROR
        "[RideCast:COMP] No backoff component found. "
        "Expected backoffAlgorithmWrapper or backoffAlgorithm sources."
    )
endif()

message(STATUS "[RideCast:COMP] USE_PLATFORM_LWIP=${USE_PLATFORM_LWIP}")
message(STATUS "[RideCast:COMP] USE_PLATFORM_MBEDTLS=${USE_PLATFORM_MBEDTLS}")
message(STATUS "[RideCast:COMP] ✓ external/component configured")