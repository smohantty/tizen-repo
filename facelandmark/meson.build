project('facelandmark', 'cpp',
  version : '1.0.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
    'default_library=static'
  ]
)

# Get the C++ compiler
cpp = meson.get_compiler('cpp')

# Add warning suppressions for third-party libraries
cpp_args = [
  '-Wno-c11-extensions',           # Suppress OpenCV C11 extension warnings
  '-Wno-dollar-in-identifier-extension',  # Suppress dlib dollar identifier warnings
  '-Wno-pedantic'                  # Suppress pedantic warnings from third-party code
]

# Dependencies
opencv_dep = dependency('opencv4', required: false)
if not opencv_dep.found()
  opencv_dep = dependency('opencv', required: false)
endif

if not opencv_dep.found()
  error('OpenCV not found. Please install OpenCV development libraries.')
endif

# Try to find dlib using pkg-config first
dlib_dep = dependency('dlib-1', required: false)

# If pkg-config doesn't work, try to find dlib manually
if not dlib_dep.found()
  dlib_inc = include_directories('/usr/local/include', '/opt/homebrew/include')
  dlib_lib = meson.get_compiler('cpp').find_library('dlib',
    dirs: ['/usr/local/lib', '/opt/homebrew/lib', '/usr/lib/x86_64-linux-gnu'],
    required: false)

  if dlib_lib.found()
    dlib_dep = declare_dependency(
      include_directories: dlib_inc,
      dependencies: dlib_lib
    )
  else
    error('dlib not found. Please install dlib development libraries.')
  endif
endif

# Include directories
inc_dir = include_directories('inc')

# Source files
sources = [
  'src/FaceLandmarkTracker.cpp',
  'src/main.cpp'
]

# Create executable
executable('face_tracker',
  sources,
  include_directories: inc_dir,
  dependencies: [opencv_dep, dlib_dep],
  cpp_args: cpp_args,
  install: true
)


# Installation
install_data('serde.h', install_dir: 'include')

# Print build information
message('Building Face Landmark Tracker')
message('OpenCV version: ' + opencv_dep.version())
message('C++ standard: ' + get_option('cpp_std'))
