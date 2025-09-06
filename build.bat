@echo off
REM Build script for TakumCpp on Windows (CMake + MSVC or MinGW)

set BUILD_DIR=build
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM Configure with CMake (assumes Visual Studio or MinGW; adjust generator if needed)
cmake -S . -B %BUILD_DIR% -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64

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