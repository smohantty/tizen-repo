project('utilsTest', 'cpp',
  version : '1.0.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
    'werror=false'
  ])

# Include directories
inc_dirs = include_directories('inc')

# utils library
utils_lib = static_library('utils',
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
  link_with : utils_lib,
  install : false
)

executable(
  'spanTest',
  [files(
    'src/spanTest.cpp'
  )],
  include_directories : inc_dirs,
  link_with : utils_lib,
  install : false
)
executable(
  'slidingWindowTest',
  [files(
    'src/slidingWindowTest.cpp'
  )],
  include_directories : inc_dirs,
  link_with : utils_lib,
  install : false
)
