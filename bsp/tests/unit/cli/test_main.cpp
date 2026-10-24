/**
 * @file test_main.cpp
 * @brief CppUTest host test runner for the Ridecast CLI unit tests.
 */
#include "CppUTest/CommandLineTestRunner.h"

int utest_cli_main(int argc, char** argv)
{
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
