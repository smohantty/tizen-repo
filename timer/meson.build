# Timer library meson build configuration
project('timer', 'cpp',
  version : '1.0.0',
  default_options : ['cpp_std=c++17'])

# Create timer library
timer_sources = files(
    'timer.cpp'
)

timer_headers = files(
    'timer.h'
)

# Timer library
timer_lib = static_library(
    'timer',
    timer_sources,
    include_directories: include_directories('.'),
    cpp_args: ['-std=c++17'],
    install: true,
    install_dir: 'lib'
)

# Install headers
install_headers(
    timer_headers,
    subdir: 'timer'
)

# Timer library dependency for other projects
timer_dep = declare_dependency(
    link_with: timer_lib,
    include_directories: include_directories('.')
)

# Test executable
timer_test_sources = files(
    'timer_test.cpp'
)

timer_test = executable(
    'timer_test',
    timer_test_sources,
    dependencies: timer_dep,
    cpp_args: ['-std=c++17'],
    install: false
)

# Add test
test('timer_test', timer_test)
