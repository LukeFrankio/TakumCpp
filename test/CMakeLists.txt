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

# ---------------------------------------------------------------------------
# Unified test logging
# Produces:
#  - test_results.log : human readable ctest --output-on-failure
#  - junit_results.xml : (if ctest supports) JUnit XML for CI tools
# The top-level build directory will receive copies for convenience.
# ---------------------------------------------------------------------------
set(TEST_LOG_DIR ${CMAKE_BINARY_DIR})
set(TEST_LOG_FILE ${TEST_LOG_DIR}/test_results.log)
set(TEST_JUNIT_FILE ${TEST_LOG_DIR}/junit_results.xml)

set(CTEST_LOG_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/ctest_log.cmake)

add_custom_target(test_log
	COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${CMAKE_BINARY_DIR} -DLOG_FILE=${TEST_LOG_FILE} -DJUNIT_FILE=${TEST_JUNIT_FILE} -P ${CTEST_LOG_SCRIPT}
	DEPENDS tests
	COMMENT "Running tests (non-fatal) and capturing output to ${TEST_LOG_FILE}"
	VERBATIM
)

if(TAKUM_ENABLE_AUTOTEST_LOGS)
	add_custom_target(auto_tests ALL DEPENDS test_log)
endif()

# CI wrapper: run tests and export `test_failures.log` (if created) to the build root
# For multi-config generators (like Visual Studio), we need to specify the config
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-config generator
    add_custom_target(ci_test_wrapper
        COMMAND ${CMAKE_COMMAND} -E echo "Starting CI test wrapper..."
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --timeout 600 --config $<CONFIG>
        COMMAND ${CMAKE_COMMAND} -E echo "CI test wrapper completed successfully"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Run tests with extended timeout and provide detailed logging for CI"
    )
else()
    # Single-config generator
    add_custom_target(ci_test_wrapper
        COMMAND ${CMAKE_COMMAND} -E echo "Starting CI test wrapper..."
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --timeout 600
        COMMAND ${CMAKE_COMMAND} -E echo "CI test wrapper completed successfully"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Run tests with extended timeout and provide detailed logging for CI"
    )
endif()

# Aggregate target for convenience
add_custom_target(test_reports DEPENDS test_log)