cmake_minimum_required(VERSION 3.20)
project(TakumCpp LANGUAGES CXX)

# =============================================================================
# Compiler and C++ Standard Detection
# =============================================================================

# Pick a preferred C++ standard dynamically. Some toolchains / CMake
# installations do not know the cxx_std_26 feature or do not support /std:c++26.
# Probe for support and fall back to older standards when necessary.
include(CheckCXXCompilerFlag)

# Check for C++26 support
check_cxx_compiler_flag("-std=c++26" HAVE_CXX26_GNU)
check_cxx_compiler_flag("/std:c++26" HAVE_CXX26_MSVC)

# Check for C++23 support  
check_cxx_compiler_flag("-std=c++23" HAVE_CXX23_GNU)
check_cxx_compiler_flag("/std:c++23" HAVE_CXX23_MSVC)

# Check for C++20 support
check_cxx_compiler_flag("-std=c++20" HAVE_CXX20_GNU)
check_cxx_compiler_flag("/std:c++20" HAVE_CXX20_MSVC)

# Set C++ standard based on what's available
if(HAVE_CXX26_GNU OR HAVE_CXX26_MSVC)
  set(CMAKE_CXX_STANDARD 26)
  message(STATUS "Using C++26 standard")
elseif(HAVE_CXX23_GNU OR HAVE_CXX23_MSVC)
  set(CMAKE_CXX_STANDARD 23)
  message(STATUS "Using C++23 standard")
elseif(HAVE_CXX20_GNU OR HAVE_CXX20_MSVC)
  set(CMAKE_CXX_STANDARD 20)
  message(STATUS "Using C++20 standard")
else()
  message(FATAL_ERROR "At least C++20 is required. Please use a newer compiler.")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Compiler-Specific Configurations
# =============================================================================

# Detect compiler type for specific configurations
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  message(STATUS "Detected MSVC compiler")
  # MSVC-specific flags
  add_compile_options(/W4)  # Warning level 4
  add_compile_options(/permissive-)  # Strict standards conformance
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(STATUS "Detected GCC compiler version ${CMAKE_CXX_COMPILER_VERSION}")
  # GCC-specific flags
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "9.0")
    message(STATUS "GCC 9+ detected: enabling enhanced constexpr support")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(STATUS "Detected Clang compiler version ${CMAKE_CXX_COMPILER_VERSION}")
  # Clang-specific flags
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12.0")
    message(STATUS "Clang 12+ detected: full C++20/23 support available")
  endif()
else()
  message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Platform-specific configurations
if(WIN32)
  message(STATUS "Configuring for Windows platform")
  # Windows-specific settings
elseif(APPLE)
  message(STATUS "Configuring for macOS platform")
  # macOS-specific settings
elseif(UNIX)
  message(STATUS "Configuring for Unix/Linux platform")
  # Unix/Linux-specific settings
endif()

include(FetchContent)

# ---------------------------------------------------------------------------
# Generated Φ polynomial coefficients
# ---------------------------------------------------------------------------
set(PHI_COEFF_HEADER ${CMAKE_SOURCE_DIR}/include/takum/internal/generated/phi_coeffs.h)
set(PHI_GEN_SCRIPT   ${CMAKE_SOURCE_DIR}/scripts/gen_poly_coeffs.py)
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND AND EXISTS ${PHI_GEN_SCRIPT})
  add_custom_command(
    OUTPUT ${PHI_COEFF_HEADER}
    COMMAND ${Python3_EXECUTABLE} ${PHI_GEN_SCRIPT} ${PHI_COEFF_HEADER}
    DEPENDS ${PHI_GEN_SCRIPT}
    COMMENT "Generating Φ polynomial coefficient header"
    VERBATIM
  )
  add_custom_target(phi_coeffs_gen DEPENDS ${PHI_COEFF_HEADER})
else()
  add_custom_target(phi_coeffs_gen
    COMMAND ${CMAKE_COMMAND} -E echo "Python3 or script missing; using existing phi_coeffs.h if present"
  )
endif()

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

FetchContent_MakeAvailable(googletest)

# Provide an INTERFACE target for header-only library usage
add_library(TakumCpp INTERFACE)
target_include_directories(TakumCpp INTERFACE ${CMAKE_SOURCE_DIR}/include)

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

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(TAKUM_ENABLE_AUTOTEST_LOGS "Run tests automatically after build of 'tests' target and write log + JUnit files" ON)

# Tests (depend on generated header)
add_subdirectory(test)
add_dependencies(tests phi_coeffs_gen)

# Examples
file(GLOB EXAMPLE_SOURCES "${CMAKE_SOURCE_DIR}/examples/*.cpp")
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE TakumCpp)
  add_dependencies(${EXAMPLE_NAME} phi_coeffs_gen)
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