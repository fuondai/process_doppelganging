@echo off
echo Building RansomePicuter...

REM Check g++ installed
where g++ >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo g++ not found. Please install MinGW or MSYS2 and add to PATH.
    exit /b 1
)

REM Build with g++
g++ -std=c++17 -Wall -static -o RansomePicuter.exe main.cpp -lgdi32 -luser32

if %ERRORLEVEL% neq 0 (
    echo Build failed.
    exit /b 1
) else (
    echo Build successful! Executable file: RansomePicuter.exe
)

exit /b 0 