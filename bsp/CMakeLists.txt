# ============================================================================
# File        : CMakeLists.txt
# Description : BSP CMake entry point
# ============================================================================
#
# The BSP build implementation is maintained in bsp.cmake.
# This entry point is required for ESP-IDF component discovery
# and standard CMake add_subdirectory() integration.
# ============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/bsp.cmake")