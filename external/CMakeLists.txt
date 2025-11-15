message(STATUS "Configuring external modules...")

# Detect ESP-IDF context
if(ESP_PLATFORM)
    message(STATUS "Building for ESP-IDF platform")

    # Do NOT use your own LWIP / mbedTLS on ESP32
    set(USE_SYSTEM_LWIP ON)
    set(USE_SYSTEM_MBEDTLS ON)
else()
    message(STATUS "Building for NON-ESP platform")

    # For STM32 / Linux, enable your own external libraries
    set(USE_SYSTEM_LWIP OFF)
    set(USE_SYSTEM_MBEDTLS OFF)
endif()

# Add subdirectories (each has its own CMakeLists.txt)
add_subdirectory(cli)
add_subdirectory(component)
add_subdirectory(os)
