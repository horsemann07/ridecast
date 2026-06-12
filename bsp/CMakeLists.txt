#
# BSP (Board Support Package) CMake Configuration
#
# MISRA C:2012 compliant component structure
# - Platform-agnostic common code
# - Platform-specific port implementations
# - Clear dependency management
#
message(STATUS "[RideCast:BSP] Configuring BSP modules...")

# =========================================================================
# COMMON BSP SOURCES (shared across all board ports)
# =========================================================================
set(BSP_CFG_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/bsp_config/bsp_config.c"
)
set(BSP_COMMON_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/srcs/internal/bsp_log.c"
    "${CMAKE_CURRENT_LIST_DIR}/srcs/internal/bsp_err_sts.c"
)

set(BSP_INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/bsp_config"
    "${CMAKE_CURRENT_LIST_DIR}/include/internal"
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

# =========================================================================
# BOARD PORT SOURCES
# =========================================================================
if(ESP_PLATFORM)
    set(BSP_PORT_SRCS
        "${CMAKE_CURRENT_LIST_DIR}/srcs/port/esp32/bsp_uart.c"
        "${CMAKE_CURRENT_LIST_DIR}/srcs/port/esp32/bsp_wifi.c"
    )

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

elseif(STM32H7_PLATFORM)
    set(BSP_PORT_SRCS
        "${CMAKE_CURRENT_LIST_DIR}/srcs/port/stm32h7/bsp_uart.c"
        "${CMAKE_CURRENT_LIST_DIR}/srcs/port/stm32h7/bsp_wifi.c"
    )

    add_library(bsp STATIC
        ${BSP_COMMON_SRCS}
        ${BSP_PORT_SRCS}
        ${BSP_CFG_SRCS}
    )

    target_include_directories(bsp PUBLIC
        ${BSP_INCLUDE_DIRS}
    )

else()
    message(FATAL_ERROR
        "[RideCast:BSP] No valid platform selected. "
        "Set ESP_PLATFORM=ON or STM32H7_PLATFORM=ON"
    )
endif()

# =========================================================================
# SUMMARY
# =========================================================================
message(STATUS "[RideCast:BSP] ========================================")
message(STATUS "[RideCast:BSP] ✓ BSP configuration complete")
message(STATUS "[RideCast:BSP] ========================================")