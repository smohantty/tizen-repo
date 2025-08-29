project('aichat', 'cpp',
  version : '1.0.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
    'werror=false'
  ])

# Include directories
inc_dir = include_directories('inc')

# Source files
aichat_sources = files(
  'src/AiChatClient.cpp'
)

# Create the library
aichat_lib = static_library(
  'aichat',
  aichat_sources,
  include_directories : inc_dir,
  install : true,
  install_dir : get_option('libdir')
)

# Install headers
install_headers(
  'inc/AiChatClient.h',
  subdir : 'aichat'
)

# Declare dependency for other projects
aichat_dep = declare_dependency(
  include_directories : inc_dir,
  link_with : aichat_lib
)

# pkg-config file for external projects
pkg = import('pkgconfig')
pkg.generate(
  aichat_lib,
  name : 'aichat',
  description : 'AI Chat Client - Latency optimization layer for non-streaming chat backends',
  version : meson.project_version(),
  subdirs : 'aichat',
  requires : []
)

# Example executable (optional - for testing)
if get_option('build_examples')
  example_sources = files(
    'example/basic_usage.cpp'
  )

  executable(
    'aichat_example',
    example_sources,
    dependencies : aichat_dep,
    install : false
  )
endif

# Summary
summary({
  'prefix' : get_option('prefix'),
  'libdir' : get_option('libdir'),
  'includedir' : get_option('includedir'),
  'C++ standard' : get_option('cpp_std'),
  'Build examples' : get_option('build_examples'),
}, section: 'Directories')

summary({
  'Library type' : 'static',
  'Sources' : 'AiChatClient.cpp',
  'Dependencies' : 'none (pure C++17)',
}, section: 'Build')