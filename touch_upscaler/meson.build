project('touch_upscaler', 'cpp',
  version : '1.0.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17'  ]
)

# Compiler setup
cpp = meson.get_compiler('cpp')

# Dependencies
thread_dep = dependency('threads')

# Source files
touch_upscaler_sources = files([
  'TouchUpscaler.cpp',
  'TouchUpscaler.h'
])


# Test executable
test_sources = files(['test.cpp'])

test_exe = executable('touch_upscaler_test',
  [test_sources, touch_upscaler_sources],
  dependencies : thread_dep,
  install : false
)
