@echo off
echo Compiling with g++...

g++ -Wall -O2 -static -static-libgcc -static-libstdc++ -DUNICODE -D_UNICODE persistence.cpp -o persistence.exe -mwindows -lwininet -lole32 -lshell32 -luuid -loleaut32 -lcomdlg32 -ltaskschd -Wl,--subsystem,windows -Wl,--allow-multiple-definition
if %ERRORLEVEL% NEQ 0 (
    echo Failed to build persistence.cpp
    exit /b 1
)

g++ -Wall -O2 -static -static-libgcc -static-libstdc++ -DUNICODE -D_UNICODE docx.cpp -o docx.exe -mwindows -lwininet -lshell32 -luuid -Wl,--subsystem,windows -Wl,--allow-multiple-definition
if %ERRORLEVEL% NEQ 0 (
    echo Failed to build docx.cpp
    exit /b 1
)

echo Build successful!
echo Executable files: persistence.exe and docx.exe 