# TouchDV - Virtual Touch Device Gesture Tester

TouchDV is an interactive terminal application for testing touch gestures on Linux systems. It creates a virtual touch device using the Linux uinput subsystem and allows you to generate various touch gestures through keyboard commands.

sudo apt install libinput-tools
sudo libinput debug-gui


sudo evtest



## Features

- **Interactive Terminal Interface**: Command-line interface for real-time gesture testing
- **Multiple Gesture Types**: Support for tap, double-tap, flick, swipe, and pan gestures
- **Configurable Parameters**: Customize coordinates, duration, distance, and direction for each gesture
- **Advanced Smoothing**: OneEuro filter for smooth gesture interpolation
- **Real-time Processing**: High-frequency output (120Hz) with configurable input rates

## Supported Gestures

### 1. Tap (`tap` or `t`)
Single touch at specified coordinates.
- **Parameters**: `x`, `y`, `duration`
- **Example**: `tap x=100 y=200 duration=150`

### 2. Double Tap (`doubletap` or `dt`)
Two quick taps at the same location.
- **Parameters**: `x`, `y`, `duration`
- **Example**: `doubletap x=500 y=300`

### 3. Flick (`flick` or `f`)
Quick, short gesture in a specified direction.
- **Parameters**: `x`, `y`, `dir`, `distance`
- **Directions**: `up`, `down`, `left`, `right`
- **Example**: `flick dir=right distance=200`

### 4. Swipe (`swipe` or `s`)
Smooth gesture movement in a specified direction.
- **Parameters**: `x`, `y`, `dir`, `distance`, `duration`
- **Example**: `swipe dir=left distance=300 duration=500`

### 5. Pan (`pan` or `p`)
Controlled movement from start to end coordinates.
- **Parameters**: `x`, `y`, `endx`, `endy`, `duration`
- **Example**: `pan x=100 y=100 endx=400 endy=400 duration=1000`

## Usage

### Prerequisites
- Linux system with uinput support
- Root privileges (required for `/dev/uinput` access)
- Meson build system for compilation

### Building
```bash
# Configure build
meson setup buildir

# Compile
meson compile -C buildir
```

### Running
```bash
# Run with sudo (required for uinput access)
sudo ./buildir/touchdv
```

### Interactive Commands
Once running, you can use the following commands:

```
touchdv> help                           # Show help
touchdv> tap                            # Simple tap at center
touchdv> tap x=100 y=200                # Tap at specific coordinates
touchdv> swipe dir=right distance=300   # Swipe right 300 pixels
touchdv> pan endx=800 endy=600          # Pan to coordinates
touchdv> quit                           # Exit application
```

### Test Script
Use the included test script to see all gestures in action:
```bash
./test_gestures.sh
```

## Configuration

The application uses the following default configuration:
- **Screen Resolution**: 2560x1440
- **Input Rate**: 30 Hz
- **Output Rate**: 120 Hz
- **Smoothing**: OneEuro filter with optimized parameters
- **Touch Timeout**: 200ms auto-release

## Technical Details

### Architecture
- **VirtualTouchDevice**: Core touch device abstraction
- **GestureGenerator**: High-level gesture creation
- **CommandParser**: Interactive command processing
- **Smoothing Algorithms**: EMA, Kalman, and OneEuro filters

### Gesture Timing
- **Tap**: 100ms default duration
- **Flick**: Very fast (50ms) with immediate release
- **Swipe**: Smooth movement at 60fps
- **Pan**: Controlled movement at 50fps with hold at end

## Troubleshooting

### Permission Denied
If you get "Permission denied" errors:
```bash
# Check uinput device permissions
ls -l /dev/uinput

# Run with sudo
sudo ./buildir/touchdv
```

### Device Not Found
If `/dev/uinput` doesn't exist:
```bash
# Load uinput module
sudo modprobe uinput

# Check if loaded
lsmod | grep uinput
```

## Development

The codebase follows C++17 standards with:
- **Header Files**: `.h` extension in `inc/` directory
- **Source Files**: `.cpp` extension in `src/` directory
- **Build System**: Meson with standalone/subproject support
- **Naming Convention**: `mPrefix` for member variables, camelCase style

## License

This project is part of the Tizen touch device virtualization system.
