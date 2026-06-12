#
# RideCast Application CMake Configuration
#
# MISRA C:2012 compliant embedded project structure
# - No dynamic memory allocation
# - All sources explicitly listed
# - Clear component dependencies
#

cmake_minimum_required(VERSION 3.16)

message(STATUS "[RideCast] Configuring app/main")

# =========================================================================
# SOURCE FILES
# =========================================================================
set(RIDECAST_APP_CFG_SRCS
    "${CMAKE_SOURCE_DIR}/app/app_config/app_config.c"
)

# UART application layer
set(RIDECAST_APP_SRCS
    "${CMAKE_SOURCE_DIR}/app/srcs/uart_app.c"
    "${CMAKE_SOURCE_DIR}/app/srcs/wifi_app.c"
    "${CMAKE_SOURCE_DIR}/app/srcs/ridecast_main.c"
)

# Startup / main entry
set(APP_ENTRY_SRCS
    "${CMAKE_SOURCE_DIR}/app/main/app_main.c"
)

# Test app
set(TEST_SRCS
    "${CMAKE_SOURCE_DIR}/app/test/test_host_uart.c"
)

# =========================================================================
# CMSIS-RTOS2 Wrapper
# =========================================================================
set(CMSIS_RTOS2_SRCS
    "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp/Source/cmsis_os2.c"
)

# =========================================================================
# INCLUDE DIRECTORIES
# =========================================================================
set(APP_INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/app/app_config"
    "${CMAKE_SOURCE_DIR}/app/include"
    "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp/Include"
    "${CMAKE_BINARY_DIR}/version"
)

# =========================================================================
# COMPONENT REGISTRATION (ESP-IDF)
# =========================================================================
idf_component_register(
    SRCS
        ${APP_ENTRY_SRCS}
        ${RIDECAST_APP_CFG_SRCS}
        ${RIDECAST_APP_SRCS}
        ${CMSIS_RTOS2_SRCS}
        ${TEST_SRCS}

    INCLUDE_DIRS
        ${APP_INCLUDE_DIRS}

    PRIV_REQUIRES
        freertos
        cmsis_esp
        system
        # mbedtls
        lwip
        driver
        esp_netif
        esp_wifi
        esp_event
        log
        esp-tls
        esp_common
        bsp
        # nal 
        cli
        nvs_flash
        backoffAlgorithmWrapper
)
message(STATUS "[RideCast:BSP] ========================================")
message(STATUS "[RideCast:APP] ✓ app/main configured")
message(STATUS "[RideCast:BSP] ========================================")
