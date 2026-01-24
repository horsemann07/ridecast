# ---------------------------------------------------------
# CMSIS for ESP — Component CMake
# ---------------------------------------------------------

# Only build this module when building for ESP boards.
# You will set -DESP_BOARD=ON from the top-level CMakeLists.
if(ESP_PLATFORM)
    message(STATUS "[cmsis_esp] Configuring CMSIS adaptation layer for ESP-IDF")

    # Collect source files
    set(CMSIS_ESP_SRCS
        "${CMAKE_CURRENT_LIST_DIR}/Source/cmsis_os2.c"
    )

    # Collect include directories
    set(CMSIS_ESP_INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/Include"
    )

    # ESP-IDF component registration
    idf_component_register(
        SRCS ${CMSIS_ESP_SRCS}
        INCLUDE_DIRS ${CMSIS_ESP_INCLUDE_DIRS}
        PRIV_REQUIRES freertos
    )
endif()

# Optional: Definitions / compile options (if needed)
# target_compile_definitions(${COMPONENT_LIB} PRIVATE
# CMSIS_ESP_ENABLED=1
# )
