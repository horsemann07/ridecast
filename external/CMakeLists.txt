message(STATUS "[RideCast:EXT] Configuring external modules...")

# Platform switch (ESP_PLATFORM comes from top-level / ESP-IDF)
option(STM32H7_PLATFORM "Build for STM32H7 platform" OFF)

if(ESP_PLATFORM)
    message(STATUS "[RideCast:EXT] Platform: ESP32 (ESP-IDF)")

    # On ESP32, use ESP-IDF provided stacks
    set(USE_SYSTEM_LWIP ON  CACHE BOOL "Use platform lwIP" FORCE)
    set(USE_SYSTEM_MBEDTLS ON CACHE BOOL "Use platform mbedTLS" FORCE)

elseif(STM32H7_PLATFORM)
    message(STATUS "[RideCast:EXT] Platform: STM32H7")

    # On STM32H7, use project external stacks
    set(USE_SYSTEM_LWIP OFF  CACHE BOOL "Use platform lwIP" FORCE)
    set(USE_SYSTEM_MBEDTLS OFF CACHE BOOL "Use platform mbedTLS" FORCE)

else()
    message(FATAL_ERROR
        "[RideCast:EXT] No valid platform selected. "
        "Set ESP_PLATFORM=ON or -DSTM32H7_PLATFORM=ON"
    )
endif()

# Add subdirectories (each has its own CMakeLists.txt)
add_subdirectory(cli)
add_subdirectory(component)
add_subdirectory(os)