project('utils', 'cpp',
  version : '1.0.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
    'werror=false'
  ])

# Include directories
inc_dirs = include_directories('inc')

# RingBuffer library
ringBuffer_lib = static_library('ringbuffer',
  'src/RingBuffer.cpp',
  include_directories : inc_dirs,
  install : false
)

# Executables
executable(
  'variantMessage',
  [files(
    'variantMessageTest.cpp'
  )],
  install : false
)

executable(
  'ringBufferTest',
  [files(
    'src/ringBufferTest.cpp'
  )],
  include_directories : inc_dirs,
  link_with : ringBuffer_lib,
  install : false
)