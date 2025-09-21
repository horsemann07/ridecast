# Top-level CMake entry for Ridecast project
cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME "ridecast")

# Add the path to the version generator script
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include the version generator script
include("${CMAKE_SOURCE_DIR}/cmake/generate_version.cmake")

set(ESP_BOARD ON)

if(ESP_BOARD)
    message(STATUS "ESP board detected, including ESP-specific configurations.")

    if(DEFINED ENV{IDF_PATH})
        message("WARNING: IDF_PATH environment variable is not cleared.
        If CMake is generating an error, consider clearing the IDF_PATH environment
        variable, and generating a clean build. This message can be ignored if
        CMake was successful.")
    endif()

    # Tell CMake where ESP-IDF SDK is
    set(IDF_PATH "${CMAKE_SOURCE_DIR}/sdk/esp" CACHE PATH "Path to ESP-IDF")
    list(APPEND CMAKE_PREFIX_PATH "${IDF_PATH}")

    # Override project dir so ESP-IDF knows where main/ lives
    # set(PROJECT_DIR ${CMAKE_SOURCE_DIR}/app/main)
    set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/app/main")

    # Include ESP-IDF project CMake
    include(${IDF_PATH}/tools/cmake/project.cmake)
else()
    message(STATUS "Generic board detected, including general configurations.")
endif()

# Defie the project
project(${PROJECT_NAME})