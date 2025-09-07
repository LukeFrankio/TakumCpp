@echo off
REM Build script for TakumCpp on Windows (CMake + MSVC or MinGW)

set BUILD_DIR=build
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM Configure with CMake (uses default generator: Visual Studio if available, else Ninja/MinGW/etc.)
cmake -S . -B %BUILD_DIR% -DCMAKE_BUILD_TYPE=Release

REM Build
cmake --build %BUILD_DIR% --config Release --parallel

REM Run tests (assumes CTest setup in CMakeLists.txt)
pushd %BUILD_DIR%
if exist test\Release\tests.exe (
    test\Release\tests.exe
) else (
    echo Tests executable not found in test\Release\
    exit /b 1
)
popd