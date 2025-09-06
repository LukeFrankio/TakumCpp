cmake_minimum_required(VERSION 3.20)
project(TakumCpp LANGUAGES CXX)

# Pick a preferred C++ standard dynamically. Some toolchains / CMake
# installations do not know the cxx_std_26 feature or do not support /std:c++26.
# Probe for support and fall back to C++23 when necessary.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++26" HAVE_CXX26_GNU)
check_cxx_compiler_flag("/std:c++26" HAVE_CXX26_MSVC)
if(HAVE_CXX26_GNU OR HAVE_CXX26_MSVC)
  set(CMAKE_CXX_STANDARD 26)
else()
  set(CMAKE_CXX_STANDARD 23)
endif()
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
include(CheckCXXCompilerFlag)
# Prefer C++26 when the compiler supports it; fall back to C++23 where CMake
# does not recognize the cxx_std_26 feature (MSVC older CMake mappings).
check_cxx_compiler_flag("-std=c++26" HAVE_STDCXX26)
if(HAVE_STDCXX26)
  target_compile_features(TakumCpp INTERFACE cxx_std_26)
else()
  target_compile_features(TakumCpp INTERFACE cxx_std_23)
endif()

# Tests
add_subdirectory(test)