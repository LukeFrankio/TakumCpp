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
check_cxx_compiler_flag("-std=c++26" HAVE_CXX26_GNU)
check_cxx_compiler_flag("/std:c++26" HAVE_CXX26_MSVC)
check_cxx_compiler_flag("-std=c++23" HAVE_CXX23_GNU)
check_cxx_compiler_flag("/std:c++23" HAVE_CXX23_MSVC)
check_cxx_compiler_flag("-std=c++20" HAVE_CXX20_GNU)
check_cxx_compiler_flag("/std:c++20" HAVE_CXX20_MSVC)
if(HAVE_CXX26_GNU OR HAVE_CXX26_MSVC)
  set(CMAKE_CXX_STANDARD 26)
elseif(HAVE_CXX23_GNU OR HAVE_CXX23_MSVC)
  set(CMAKE_CXX_STANDARD 23)
elseif(HAVE_CXX20_GNU OR HAVE_CXX20_MSVC)
  set(CMAKE_CXX_STANDARD 20)
else()
  message(FATAL_ERROR "At least C++20 is required")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use the detected standard for target features
if(CMAKE_CXX_STANDARD EQUAL 26)
  target_compile_features(TakumCpp INTERFACE cxx_std_26)
elseif(CMAKE_CXX_STANDARD EQUAL 23)
  target_compile_features(TakumCpp INTERFACE cxx_std_23)
elseif(CMAKE_CXX_STANDARD EQUAL 20)
  target_compile_features(TakumCpp INTERFACE cxx_std_20)
endif()

# Enable CTest
include(CTest)
enable_testing()

# Tests
add_subdirectory(test)

# Examples
file(GLOB EXAMPLE_SOURCES "${CMAKE_SOURCE_DIR}/examples/*.cpp")
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE TakumCpp)
    if(BUILD_TESTING)
        add_test(NAME ${EXAMPLE_NAME} COMMAND ${EXAMPLE_NAME})
    endif()
endforeach()

# Doxygen documentation: try to find a doxygen executable either in PATH
# or at a common installation location on Windows. If not found we still
# expose a `docs` target that will show an informative error when built.
find_program(DOXYGEN_EXECUTABLE NAMES doxygen HINTS "C:/Program Files/doxygen/bin" "C:/Program Files (x86)/doxygen/bin")
if(DOXYGEN_EXECUTABLE)
  add_custom_target(docs
    COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/Doxyfile"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
else()
  add_custom_target(docs
    COMMAND ${CMAKE_COMMAND} -E echo "Doxygen executable not found. Install Doxygen or add it to PATH to build docs."
    COMMENT "Doxygen not found: docs target is a no-op"
  )
  message(WARNING "Doxygen not found; 'docs' target will be a no-op. Install doxygen to enable documentation generation.")
endif()