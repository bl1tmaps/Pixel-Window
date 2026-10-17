#!/bin/bash
set -e

# Set the path to the Amiga C++ compiler in Downloads
CXX="$HOME/Downloads/m68k-amigaos-gcc/bin/m68k-amigaos-g++"

echo "========================================="
echo "Building PixelWindow for AmigaOS"
echo "========================================="

# Define the highly optimized PiStorm RTG flags (matching your Windows script)
CXXFLAGS="-O3 -m68040 -fomit-frame-pointer -mcrt=clib2"

# Compile the single-file project directly into an executable[cite: 5]
echo "[INFO] Compiling main.cpp..."
"$CXX" main.cpp -o PixelWindowApp $CXXFLAGS

echo "========================================="
echo "[SUCCESS] Build completed: PixelWindowApp"
echo "========================================="
