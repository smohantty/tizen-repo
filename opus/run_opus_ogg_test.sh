#!/bin/bash

# Script to run the Opus OGG compression test

set -e

echo "=== Opus OGG Compression Test Suite ==="
echo ""

# Check if builddir exists
if [ ! -d "builddir" ]; then
    echo "Build directory not found. Setting up build..."
    meson setup builddir
fi

# Build the project
echo "Building project..."
cd builddir
meson compile
cd ..

echo ""
echo "Step 1: Generating test WAV file..."
./builddir/generate_test_wav input.wav 3

echo ""
echo "Step 2: Compressing WAV to Opus OGG format..."
./builddir/opus_ogg_test input.wav output.opus

echo ""
echo "=== Test Complete ==="
echo ""
echo "You can play the generated files with:"
echo "  Input (WAV):  ffplay input.wav"
echo "  Output (OGG): ffplay output.opus"
echo ""
echo "Or verify the OGG file structure with:"
echo "  ogginfo output.opus"
echo "  opusinfo output.opus"

