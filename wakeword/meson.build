project('wakeword', 'cpp',
  version: '1.0.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
    'werror=false'
  ]
)

# Import filesystem module for directory checking
fs = import('fs')

incdir = include_directories('inc')

sources = [
  'src/WakeupWordDetector.cpp',
]

# Private include directories for srwakeup (if they exist)
# Add lib/include to private includes if srwakeup headers are placed there
srwakeup_private_inc = []
if fs.is_dir(join_paths(meson.current_source_dir(), 'lib', 'include'))
  srwakeup_private_inc += include_directories('lib/include')
endif

# Declare the external static library as a private dependency
libsrwakeup_dep = declare_dependency(
  link_args: [join_paths(meson.current_source_dir(), 'lib', 'libsrwakeup.lib')],
  include_directories: srwakeup_private_inc
)

wakeword_lib = static_library(
  'wakeword',
  sources,
  include_directories: [incdir] + srwakeup_private_inc,
  dependencies: libsrwakeup_dep,
  cpp_args: ['-std=c++17'],
  install: not meson.is_subproject()
)

# Public dependency - only exposes the public include directory (inc/)
# Does NOT include srwakeup headers or link dependencies
wakeword_dep = declare_dependency(
  include_directories: incdir,
  link_with: wakeword_lib
)

# Only build examples/tests when built as standalone
if not meson.is_subproject()
  # Add example or test executable here if needed
  # executable('wakeword_example', 'example.cpp', dependencies: wakeword_dep)
endif
