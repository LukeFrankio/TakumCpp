cmake_minimum_required(VERSION 3.20)
project(TakumCppDebug LANGUAGES CXX)

# Use same C++ standard as parent project
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include parent project headers
include_directories(../include)

# Find all debug source files
file(GLOB DEBUG_SOURCES "*.cpp")

# Create executables for each debug program
foreach(DEBUG_SOURCE ${DEBUG_SOURCES})
    get_filename_component(DEBUG_NAME ${DEBUG_SOURCE} NAME_WE)
    add_executable(${DEBUG_NAME} ${DEBUG_SOURCE})
    # Set include directories for the headers
    target_include_directories(${DEBUG_NAME} PRIVATE ../include)
endforeach()