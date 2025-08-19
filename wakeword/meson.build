project('wakeword', 'cpp',
  version: '1.0.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
    'werror=false'
  ]
)

incdir = include_directories('inc')

sources = [
  'src/WakeupWordDetector.cpp',
]

wakeword_lib = static_library(
  'wakeword',
  sources,
  include_directories: incdir,
  cpp_args: ['-std=c++17'],
  install: not meson.is_subproject()
)

wakeword_dep = declare_dependency(
  include_directories: incdir,
  link_with: wakeword_lib
)

# Only build examples/tests when built as standalone
if not meson.is_subproject()
  # Add example or test executable here if needed
  # executable('wakeword_example', 'example.cpp', dependencies: wakeword_dep)
endif
