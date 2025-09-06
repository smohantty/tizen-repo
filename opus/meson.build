project('opus-audio-codec', 'cpp',
  version : '1.0.0',
  default_options : ['warning_level=3',
                     'cpp_std=c++17'])

# Dependencies
opus_dep = dependency('opus', required : true)

# Source files
opus_sources = [
  'OpusAudioCodec.cpp',
  'Base64Helper.cpp',
  'Base64.cpp'
]

# Header files
opus_headers = [
  'OpusAudioCodec.h',
  'Base64Helper.h',
  'Base64.h',
  'AudioStreamer.h'
]

# Create library
opus_lib = library('opuscodec',
  opus_sources,
  dependencies : opus_dep,
  include_directories : include_directories('.'),
  install : true)

# Create executable
opus_test = executable('opus_test',
  'opusTest.cpp',
  link_with : opus_lib,
  dependencies : opus_dep,
  include_directories : include_directories('.'),
  install : true)

# Audio streamer library (no main function, so not an executable)
opus_streamer_lib = library('opusstreamer',
  'AudioStreamer.cpp',
  dependencies : opus_dep,
  include_directories : include_directories('.'),
  install : true)

# Install headers
install_headers(opus_headers, subdir : 'opus')

# Declare dependency for use as subproject
opus_dep = declare_dependency(
  link_with : opus_lib,
  include_directories : include_directories('.'),
  dependencies : opus_dep)
