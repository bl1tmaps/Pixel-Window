@echo off
setlocal

:: Set the path to the Amiga C++ compiler
set CXX="C:\amiga-gcc\bin\m68k-amigaos-g++.exe"

echo =========================================
echo Building PixelWindow for AmigaOS
echo =========================================

:: Define the highly optimized PiStorm RTG flags
set CXXFLAGS=-O3 -m68040 -fomit-frame-pointer -mcrt=clib2

:: Compile the single-file project directly into an executable
echo [INFO] Compiling main.cpp...
%CXX% main.cpp -o PixelWindowApp.exe %CXXFLAGS%

if ERRORLEVEL 1 goto error

echo =========================================
echo [SUCCESS] Build completed: PixelWindowApp.exe
echo =========================================
goto end

:error
echo =========================================
echo [ERROR] Build failed! Check the output above.
echo =========================================

:end
endlocal
pause