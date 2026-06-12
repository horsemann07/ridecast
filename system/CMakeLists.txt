#
# RideCast System Component CMake Configuration
#
# MISRA C:2012 compliant embedded project structure
# - All dependencies explicitly declared
# - No duplicate entries in dependency lists
# - Guard ensures ESP-IDF only build
#

# =========================================================================
# GUARD: This component is only valid under ESP-IDF build system.
# Fail early and clearly if invoked outside ESP-IDF context.
# =========================================================================
if(NOT ESP_PLATFORM)
    message(FATAL_ERROR
        "[RideCast] system component requires ESP-IDF.\n"
        "           Set ESP_PLATFORM=ON or build via idf.py."
    )
endif()

message(STATUS "[RideCast] Configuring component: system")

# =========================================================================
# SOURCE FILES
# =========================================================================
# All source files must be explicitly listed.
# Globbing (file GLOB) is forbidden — it hides dependency changes from CMake.
# =========================================================================
set(SYSTEM_SRCS
    # Add system source files here explicitly, e.g.:
    # "${CMAKE_CURRENT_LIST_DIR}/src/system.c"
)

# =========================================================================
# INCLUDE DIRECTORIES
# =========================================================================
# Public headers exposed to other components via INCLUDE_DIRS.
# Private headers used only by this component go in PRIV_INCLUDE_DIRS.
# =========================================================================
set(SYSTEM_PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

# =========================================================================
# COMPONENT REGISTRATION (ESP-IDF)
# =========================================================================
# PRIV_REQUIRES: components needed to compile THIS component only.
# REQUIRES     : components whose headers are exposed in our public headers.
#
# MISRA note   : No duplicate entries — mbedtls and lwip appeared twice in
#                the original; duplicates removed here.
# =========================================================================
idf_component_register(
    SRCS
        ${SYSTEM_SRCS}

    INCLUDE_DIRS
        ${SYSTEM_PUBLIC_INCLUDES}

    PRIV_REQUIRES
        freertos           # RTOS task / queue APIs
        cmsis_esp          # CMSIS-RTOS2 wrapper over FreeRTOS
        mbedtls            # TLS / crypto support
        lwip               # TCP/IP network stack
        driver             # ESP peripheral driver APIs
        esp_netif          # Network interface abstraction
        esp_wifi           # Wi-Fi driver
        esp_event          # Event loop library
        log                # ESP logging macros
        esp-tls            # TLS connection helper
        esp_common         # Common ESP-IDF utilities
)

message(STATUS "[RideCast] ✓ system component configured")