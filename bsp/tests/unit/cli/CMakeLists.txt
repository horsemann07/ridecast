add_executable(test_cli
    test_cli.cpp
    ${CMAKE_SOURCE_DIR}/bsp/bsp_core/cli/cli.c
)

target_include_directories(test_cli PRIVATE
    ${CMAKE_SOURCE_DIR}/bsp/bsp_core/cli
    ${CMAKE_SOURCE_DIR}/bsp/bsp_core/status
)

target_link_libraries(test_cli PRIVATE
    CppUTest
    CppUTestExt
)

add_test(NAME cli_unit_tests COMMAND test_cli)