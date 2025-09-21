idf_component_register(
    SRCS "app_main.c" 
        "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp/Source/cmsis_os2.c"
    INCLUDE_DIRS "." 
        "${CMAKE_SOURCE_DIR}/external/os/cmsis_esp/Include"
    REQUIRES freertos
)