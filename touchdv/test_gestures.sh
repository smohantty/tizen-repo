#!/bin/bash

# Test script for TouchDV Gesture Tester
# This script demonstrates various gesture commands

echo "=== TouchDV Gesture Tester Demo ==="
echo "This script will run various gesture commands to demonstrate functionality"
echo "Note: Requires sudo access for /dev/uinput"
echo ""

echo "Starting TouchDV..."

# Create input commands file
cat > /tmp/touchdv_commands << 'EOF'
help
tap
tap x=100 y=200
doubletap x=500 y=300
flick dir=right distance=200
flick dir=up distance=150
swipe dir=left distance=300 duration=500
swipe dir=down distance=250
pan x=100 y=100 endx=400 endy=400 duration=1000
quit
EOF

# Run TouchDV with commands
sudo ./buildir/touchdv < /tmp/touchdv_commands

# Cleanup
rm -f /tmp/touchdv_commands

echo ""
echo "Demo completed!"
