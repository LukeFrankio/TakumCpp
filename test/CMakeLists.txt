# Test subdirectory

# Add the test executable
add_executable(tests placeholder.test.cpp)

# Link GoogleTest libraries
target_link_libraries(tests gtest gtest_main)

# Discover tests
include(GoogleTest)
gtest_discover_tests(tests)