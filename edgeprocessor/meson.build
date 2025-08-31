project('edgeprocessor', 'cpp',
  version : '1.0.0',
  default_options : [
    'cpp_std=c++17',
    'warning_level=2',
    'buildtype=debugoptimized'
  ]
)



# Compiler options
cpp = meson.get_compiler('cpp')

# Add warning flags (only if not using built-in warning_level)
if get_option('warning_level') == '0'
  if cpp.get_id() == 'gcc' or cpp.get_id() == 'clang'
    add_project_arguments(
      '-Wall',
      '-Wextra',
      '-Wpedantic',
      '-Wno-unused-parameter',
      language : 'cpp'
    )
  endif
endif

# Include directories
inc = include_directories('inc')

# Source files
sources = [
  'src/AudioStreaming.cpp',
  'src/JsonFormatter.cpp'
]

# Header files
headers = [
  'inc/AudioStreaming.h',
  'inc/AudioStreamingConfig.h',
  'inc/IAudioStreamingListener.h',
  'inc/Message.h',
  'inc/JsonFormatter.h'
]

# Dependencies
deps = [
  dependency('threads', required : true)
]

# Create the library
libedgeprocessor = static_library('edgeprocessor',
  sources,
  include_directories : inc,
  dependencies : deps,
  install : true
)

# Create the library dependency object
edgeprocessor_dep = declare_dependency(
  link_with : libedgeprocessor,
  include_directories : inc,
  dependencies : deps
)

# Install headers
install_headers(headers, subdir : 'edgeprocessor')

# Create pkg-config file
pkg = import('pkgconfig')
pkg.generate(
  libedgeprocessor,
  name : 'edgeprocessor',
  description : 'C++17 AudioStreaming module for Edge ASR',
  url : 'https://github.com/example/edgeprocessor',
  version : meson.project_version()
)

# Example application
if get_option('examples')
  example_sources = [
    'example/audio_streaming_example.cpp'
  ]

  example_exe = executable('audio_streaming_example',
    example_sources,
    dependencies : edgeprocessor_dep,
    install : false
  )
endif

# Tests
if get_option('tests')
  # Individual test executables
  test_audio_streaming = executable('test_audio_streaming',
    'test/test_audio_streaming.cpp',
    dependencies : edgeprocessor_dep,
    install : false
  )



  test_json_formatter = executable('test_json_formatter',
    'test/test_json_formatter.cpp',
    dependencies : edgeprocessor_dep,
    install : false
  )

  # Run all tests
  test('test_audio_streaming', test_audio_streaming)
  test('test_json_formatter', test_json_formatter)
endif

# Subproject support
if meson.is_subproject()
  # When used as a subproject, expose the dependency
  meson.override_dependency('edgeprocessor', edgeprocessor_dep)
endif
