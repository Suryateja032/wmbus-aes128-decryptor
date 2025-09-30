@echo off
REM Build script for W-MBus Decryptor (Windows)
echo W-MBus Decryptor Build Script (Windows)
echo =========================================

REM Check for Visual Studio
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: Visual Studio compiler not found
    echo Please run this from a Visual Studio Command Prompt
    pause
    exit /b 1
)

REM Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: CMake not found
    echo Please install CMake and add it to PATH
    pause
    exit /b 1
)

echo Building project...

mkdir build 2>nul
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo CMake configuration failed
    pause
    exit /b 1
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed
    pause
    exit /b 1
)

echo.
echo Build successful!
echo Executable: Release\wmbus_decryptor.exe
pause
