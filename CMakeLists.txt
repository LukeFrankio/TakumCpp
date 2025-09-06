cmake_minimum_required(VERSION 3.20)
project(TakumCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

FetchContent_MakeAvailable(googletest)

# Provide an INTERFACE target for header-only library usage
add_library(TakumCpp INTERFACE)
target_include_directories(TakumCpp INTERFACE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(TakumCpp INTERFACE cxx_std_23)

# Tests
add_subdirectory(test)