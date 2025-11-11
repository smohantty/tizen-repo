# Opus OGG Compression Test

This test demonstrates compressing a WAV file using the Opus codec and saving it to an OGG container format.

## Features

- Reads WAV files using libsndfile
- Encodes audio using Opus codec at 128 kbps
- Writes encoded data to OGG container
- Generates test WAV files with musical content (A major chord)
- Reports compression statistics

## Dependencies

The following dependencies are required:

1. **libopus** - Opus audio codec
2. **libsndfile** - Audio file I/O library
3. **libogg** - OGG container format library

### Installing Dependencies

#### macOS (using Homebrew)
```bash
brew install opus libsndfile libogg
```

#### Linux (Debian/Ubuntu)
```bash
sudo apt-get install libopus-dev libsndfile1-dev libogg-dev
```

#### Linux (Fedora/RHEL)
```bash
sudo dnf install opus-devel libsndfile-devel libogg-devel
```

## Building

```bash
# Setup build directory (first time only)
meson setup builddir

# Build
cd builddir
meson compile
cd ..
```

## Usage

### Option 1: Run the automated test script

```bash
chmod +x run_opus_ogg_test.sh
./run_opus_ogg_test.sh
```

This script will:
1. Generate a 3-second test WAV file (`input.wav`)
2. Compress it to Opus OGG format (`output.opus`)
3. Display compression statistics

### Option 2: Manual execution

```bash
# Generate a test WAV file (3 seconds by default)
./builddir/generate_test_wav input.wav 3

# Compress the WAV file to Opus OGG
./builddir/opus_ogg_test input.wav output.opus

# Or use your own WAV file
./builddir/opus_ogg_test /path/to/your/file.wav output.opus
```

## Testing the Output

You can verify and play the generated OGG Opus file using:

```bash
# Play the files
ffplay input.wav      # Original WAV
ffplay output.opus    # Compressed OGG Opus

# Verify OGG structure
ogginfo output.opus

# Get detailed Opus information
opusinfo output.opus
```

## Expected Results

For a 3-second test file at 16 kHz mono:
- Input size: ~96 KB (WAV)
- Output size: ~6-8 KB (OGG Opus)
- Compression ratio: 12:1 to 16:1

## Technical Details

### Opus Encoder Settings
- Bitrate: 128 kbps
- Frame size: 20ms
- Variable bitrate (VBR) enabled
- Complexity: 10 (maximum quality)
- Application: OPUS_APPLICATION_AUDIO

### Supported Input Formats
The test reads WAV files with the following requirements:
- Any sample rate (automatically handled by Opus)
- Mono or stereo channels
- 16-bit PCM format

### OGG Container Structure
The output OGG file contains:
1. **OpusHead** packet - Opus stream header
2. **OpusTags** packet - Metadata comments
3. **Audio packets** - Encoded Opus frames

## Troubleshooting

### Build fails with "dependency not found"
Make sure you have installed all required dependencies. On macOS, you may need to set `PKG_CONFIG_PATH`:
```bash
export PKG_CONFIG_PATH=/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH
```

### File not found error
Ensure you're running the commands from the `opus` directory.

### Playback issues
Make sure you have ffmpeg/ffplay installed:
```bash
# macOS
brew install ffmpeg

# Linux
sudo apt-get install ffmpeg
```

## Code Structure

- `opusOggTest.cpp` - Main test program with WAV reader and OGG encoder
- `generateTestWav.cpp` - Utility to create test WAV files
- `run_opus_ogg_test.sh` - Automated test script
- `meson.build` - Build configuration

## Future Enhancements

- Support for more input formats (FLAC, MP3, etc.)
- Configurable bitrate and quality settings
- Batch processing of multiple files
- Decoder test to verify round-trip quality
- VBR quality modes (instead of fixed bitrate)

