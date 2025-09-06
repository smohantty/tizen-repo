# Face Landmark Tracker

A C++17 application that performs real-time face detection, landmark extraction, and user identification using OpenCV and dlib.

## Features

- Real-time face detection from camera feed
- 68-point facial landmark extraction
- User database with normalized landmarks
- Cross-platform support (Mac and Linux)
- Serialization using custom serde library
- Real-time user identification
- **Performance optimized** with frame skipping and face tracking
- **FPS monitoring** with real-time performance display

## Dependencies

### macOS (using Homebrew)
```bash
brew install opencv dlib
```

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libopencv-dev libdlib-dev
```

### CentOS/RHEL/Fedora
```bash
sudo yum install opencv-devel dlib-devel
# or for newer versions:
sudo dnf install opencv-devel dlib-devel
```

## Required Model File

Download the dlib shape predictor model:
```bash
wget http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2
bunzip2 shape_predictor_68_face_landmarks.dat.bz2
```

Place `shape_predictor_68_face_landmarks.dat` in the project directory.

## Building

### Using Meson (Recommended)
```bash
# Configure build
meson setup build

# Build
meson compile -C build

# Run
./build/face_tracker
```

### Manual Build (Alternative)
```bash
g++ -std=c++17 -Iinc src/*.cpp -o face_tracker \
    `pkg-config --cflags --libs opencv4` \
    -ldlib
```

## Usage

1. Run the application: `./face_tracker`
2. Position your face in front of the camera
3. Press 'a' to add a new user (enter name when prompted)
4. Press 'q' to quit and save the database
5. Press 's' to manually save the database

## Controls

- **'a'** - Add new user (prompts for name)
- **'s'** - Save database
- **'q'** - Quit and save database
- **ESC** - Quit and save database

## Performance Optimizations

The application includes several performance optimizations to achieve real-time processing:

- **Reduced Resolution Detection**: Face detection runs on 1/2 resolution for faster processing
- **Frame Skipping**: Face detection only runs every 3rd frame, with tracking in between
- **Face Tracking**: Reuses detected face positions for up to 10 frames before re-detection
- **Optimized Drawing**: Landmark visualization only updates every other frame
- **FPS Monitoring**: Real-time FPS display shows current performance

**Expected Performance:**
- **Without optimizations**: ~1-2 FPS (very slow)
- **With optimizations**: 15-30+ FPS (real-time capable)

## File Structure

```
facelandmark/
├── inc/
│   └── FaceLandmarkTracker.h
├── src/
│   ├── FaceLandmarkTracker.cpp
│   └── main.cpp
├── serde.h
├── meson.build
├── README.md
└── requirement.md
```

## Generated Files

- `user_db.bin` - Serialized user database
- `shape_predictor_68_face_landmarks.dat` - dlib model (download separately)

## Technical Details

- **Landmark Normalization**: Translates to origin and scales by interocular distance
- **Distance Threshold**: 0.05 (configurable)
- **Camera Resolution**: Minimum 640x480 recommended
- **Frame Rate**: 15-30 FPS for real-time tracking

## Troubleshooting

### Camera Issues
- Ensure camera permissions are granted
- Try different camera indices (0, 1, 2, etc.)
- Check if camera is being used by another application

### Build Issues
- Ensure all dependencies are installed
- Check that OpenCV and dlib are properly linked
- Verify C++17 compiler support

### Model File Issues
- Ensure `shape_predictor_68_face_landmarks.dat` is in the project directory
- Verify the file is not corrupted (should be ~99MB)
