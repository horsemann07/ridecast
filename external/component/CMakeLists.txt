
# Current File Path
# ${CAMAKE_SOURCE_DIR}/external/component/CMakeLists.txt


# Only include lwip/mbedtls if not using ESP-IDF's versions

# if(NOT USE_SYSTEM_LWIP)
#     add_subdirectory(lwip)
# endif()

# if(NOT USE_SYSTEM_MBEDTLS)
#     add_subdirectory(mbedtls)
# endif()

# ESP-IDF wrapper for FreeRTOS Backoff Algorithm
# No tests, no modification to upstream sources
include("${CMAKE_CURRENT_LIST_DIR}/backoffAlgorithm/backoffAlgorithmFilePaths.cmake")

idf_component_register(
    SRCS
    ${BACKOFF_ALGORITHM_SOURCES}
    INCLUDE_DIRS
    ${BACKOFF_ALGORITHM_INCLUDE_PUBLIC_DIRS}
)
