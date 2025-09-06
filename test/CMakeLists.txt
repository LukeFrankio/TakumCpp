cmake_minimum_required(VERSION 3.10)
project(TestNarCheck)
# Test subdirectory

# Add the test executable
add_executable(tests placeholder.test.cpp spec_reference.test.cpp core.test.cpp)
add_executable(test_nar_check test_nar_check.cpp)
target_include_directories(test_nar_check PRIVATE ../include)

# Link GoogleTest libraries
target_link_libraries(tests PRIVATE TakumCpp gtest gtest_main)

# Discover tests
include(GoogleTest)
gtest_discover_tests(tests)