# Only include lwip/mbedtls if not using ESP-IDF's versions

if(NOT USE_SYSTEM_LWIP)
    add_subdirectory(lwip)
endif()

if(NOT USE_SYSTEM_MBEDTLS)
    add_subdirectory(mbedtls)
endif()
