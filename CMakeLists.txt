# Top-level CMake entry for Ridecast project
cmake_minimum_required(VERSION 3.10)


set(PROJECT_NAME "ridecast")

# Add the path to the version generator script
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/${CMAKE_MODULE_PATH}")

# Include the version generator script
include("${CMAKE_MODULE_PATH}/generate_version.cmake")

set(ESP_BOARD ON)

if(ESP_BOARD)
    message(STATUS "ESP board detected, including ESP-specific configurations.")
    # Tell CMake where ESP-IDF SDK is
    set(IDF_PATH "${CMAKE_SOURCE_DIR}/sdk/esp")
    list(APPEND CMAKE_PREFIX_PATH "${IDF_PATH}")

    # Override project dir so ESP-IDF knows where main/ lives
    # set(PROJECT_DIR ${CMAKE_SOURCE_DIR}/app/main)

    # Include ESP-IDF project CMake
    include(${IDF_PATH}/tools/cmake/project.cmake)
else()
    message(STATUS "Generic board detected, including general configurations.")
endif()


# Defie the project
project(${PROJECT_NAME})