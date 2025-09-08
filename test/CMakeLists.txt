cmake_minimum_required(VERSION 3.10)
project(TestNarCheck)
# Test subdirectory

# Add the test executable
file(GLOB TEST_SOURCES "*.test.cpp")
add_executable(tests ${TEST_SOURCES})
# Allow exhaustive tests to run up to N=16 when requested; default kept small.
target_compile_definitions(tests PRIVATE EXHAUSTIVE_MAX_N=16)
add_executable(test_nar_check test_nar_check.cpp)
target_include_directories(tests PRIVATE ../include)
target_include_directories(test_nar_check PRIVATE ../include)

# Link GoogleTest libraries
target_link_libraries(tests PRIVATE TakumCpp gtest gtest_main)

# Discover tests
include(GoogleTest)
gtest_discover_tests(tests)

# CI wrapper: run tests and export `test_failures.log` (if created) to the build root
add_custom_target(ci_test_wrapper
	COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
	COMMAND ${CMAKE_COMMAND} -E if_exists ${CMAKE_CURRENT_BINARY_DIR}/test_failures.log ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/test_failures.log ${CMAKE_BINARY_DIR}/test_failures.log
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMENT "Run tests and copy test_failures.log to build root for CI"
)