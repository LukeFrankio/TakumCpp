cmake_minimum_required(VERSION 3.10)
project(TestNarCheck)
# Test subdirectory

# Add the test executable
file(GLOB TEST_SOURCES "*.test.cpp")
add_executable(tests ${TEST_SOURCES})
add_executable(test_nar_check test_nar_check.cpp)
target_include_directories(tests PRIVATE ../include)
target_include_directories(test_nar_check PRIVATE ../include)

# Link GoogleTest libraries
target_link_libraries(tests PRIVATE TakumCpp gtest gtest_main)

# Discover tests
include(GoogleTest)
gtest_discover_tests(tests)