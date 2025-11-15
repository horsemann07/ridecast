# Top-level CMake entry for Ridecast project
cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME "ridecast")

# Load custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Version generator (optional, your script)
include("${CMAKE_SOURCE_DIR}/cmake/generate_version.cmake")

# ----------------------------------------------------------------------
# 1) ESP-IDF / Generic board selection
# ----------------------------------------------------------------------
set(ESP_PLATFORM ON CACHE BOOL "Build for ESP board using ESP-IDF")
if(ESP_PLATFORM)
    message(STATUS "ESP board detected. Configuring ESP-IDF environment...")

    # ------------------------------------------------------------------
    # ESP-IDF setup
    # ------------------------------------------------------------------
    
    if(DEFINED ENV{IDF_PATH})
        message(WARNING 
            "IDF_PATH environment variable is set externally.\n"
            "If build fails, try unsetting IDF_PATH and rebuilding.\n"
        )
    endif()

    # Path to your local ESP-IDF SDK inside project
    set(IDF_PATH "${CMAKE_SOURCE_DIR}/sdk/esp" CACHE PATH "Path to ESP-IDF")
    list(APPEND CMAKE_PREFIX_PATH "${IDF_PATH}")

    # ESP-IDF expects a 'main' component → you set it explicitly
    set(EXTRA_COMPONENT_DIRS
        "${CMAKE_SOURCE_DIR}/external/cli"
        # "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp"
        "${CMAKE_SOURCE_DIR}/app/main"
    )

    # ------------------------------------------------------------------
    # Load ESP-IDF build system BEFORE project()
    # ------------------------------------------------------------------
    include(${IDF_PATH}/tools/cmake/project.cmake)

else()
    message(STATUS "Generic board detected. Using generic CMake flow...")

    # ------------------------------------------------------------------
    # Standard CMake project
    # ------------------------------------------------------------------
    project(${PROJECT_NAME})

endif()

# ----------------------------------------------------------------------
# 2) External module configuration (clean design)
# We call *external/CMakeLists.txt* once and let it handle everything
# ----------------------------------------------------------------------
# add_subdirectory(external)

# ----------------------------------------------------------------------
# 3) For ESP-IDF: project() MUST be after add_subdirectory(external)
#    (this is ESP-IDF’s requirement)
# ----------------------------------------------------------------------
if(ESP_PLATFORM)
    project(${PROJECT_NAME})
endif()
