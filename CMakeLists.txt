cmake_minimum_required(VERSION 3.20)
project(TakumCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable strict compliance
target_compile_features(TakumCpp PUBLIC cxx_std_23)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

FetchContent_MakeAvailable(googletest)

# Add subdirectories
add_subdirectory(include)
add_subdirectory(test)
add_subdirectory(examples)