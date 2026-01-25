# ESP-IDF wrapper for FreeRTOS Backoff Algorithm
# Does NOT modify upstream library

# This file lives in:
# external/component/backoffAlgorithmWrapper/CMakeLists.txt

# Go one level up to external/component
get_filename_component(COMPONENTS_DIR
    ${CMAKE_CURRENT_LIST_DIR}
    DIRECTORY
)

# Path to upstream BackoffAlgorithm
set(BACKOFF_ROOT
    ${COMPONENTS_DIR}/backoffAlgorithm
)

include(${BACKOFF_ROOT}/backoffAlgorithmFilePaths.cmake)

idf_component_register(
    SRCS
        ${BACKOFF_ALGORITHM_SOURCES}
    INCLUDE_DIRS
        ${BACKOFF_ALGORITHM_INCLUDE_PUBLIC_DIRS}
)
