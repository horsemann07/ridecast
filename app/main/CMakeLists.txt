
# ============================================================================
# File        : CMakeLists.txt
# Description : RideCast BSP and SDK validation build
# ============================================================================
#
# Purpose:
#   Build the reusable BSP and its SDK dependencies without compiling
#   the application implementation, interface, or configuration sources.
#
#   This is a temporary validation configuration. The full application
#   component can be restored after the BSP/SDK build succeeds.
# ============================================================================

cmake_minimum_required(VERSION 3.16)

# ============================================================================
# 1. BSP AND SDK VALIDATION
# ============================================================================

message(STATUS "[RideCast:APP] BSP and SDK validation build enabled")

# ============================================================================
# 2. COMPONENT DIRECTORY
# ============================================================================

set(RIDECAST_APP_MAIN_DIR "${CMAKE_CURRENT_LIST_DIR}")

# ============================================================================
# 3. VALIDATION ENTRY POINT
# ============================================================================

set(RIDECAST_VALIDATION_SRC
    "${RIDECAST_APP_MAIN_DIR}/bsp_sdk_check_main.c"
)

if(NOT EXISTS "${RIDECAST_VALIDATION_SRC}")
    message(FATAL_ERROR
        "[RideCast:APP] Missing BSP validation entry point: "
        "${RIDECAST_VALIDATION_SRC}"
    )
endif()

# ============================================================================
# 4. ESP-IDF COMPONENT REGISTRATION
# ============================================================================

idf_component_register(
    SRCS
        "${RIDECAST_VALIDATION_SRC}"

    PRIV_REQUIRES
        bsp
        cmsis_esp
)
# ============================================================================
# File End
# ============================================================================
