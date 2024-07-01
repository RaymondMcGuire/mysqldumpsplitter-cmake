@echo off
cd ..
if not exist build mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo CMake configuration failed
    pause
    exit /b 1
)
echo CMake configuration completed successfully
pause