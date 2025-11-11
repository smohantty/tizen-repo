#!/bin/bash

# Verification script for Opus OGG output

echo "=== Opus OGG Output Verification ==="
echo ""

if [ ! -f "output.opus" ]; then
    echo "Error: output.opus not found. Run the test first:"
    echo "  ./run_opus_ogg_test.sh"
    exit 1
fi

echo "1. File Information:"
ls -lh output.opus input.wav 2>/dev/null

echo ""
echo "2. Audio Stream Details:"
ffprobe -hide_banner output.opus 2>&1 | grep -E "(Duration|Stream|Audio)"

echo ""
echo "3. OGG Structure (first 100 bytes in hex):"
hexdump -C output.opus | head -10

echo ""
echo "4. Verify OGG signature (should start with 'OggS'):"
head -c 4 output.opus | cat -v

echo ""
echo "5. Verify OpusHead marker:"
strings output.opus | grep -E "(OpusHead|OpusTags)" | head -2

echo ""
echo "=== Verification Complete ==="
echo ""
echo "To play the audio files:"
echo "  Original: ffplay -nodisp -autoexit input.wav"
echo "  Encoded:  ffplay -nodisp -autoexit output.opus"
echo ""
echo "To compare quality, play both files back-to-back:"
echo "  ffplay -nodisp -autoexit input.wav && ffplay -nodisp -autoexit output.opus"

