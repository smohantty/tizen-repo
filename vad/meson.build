project('vad', 'cpp',
  version: '1.0.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
    'werror=false'
  ]
)

incdir = include_directories('inc')

sources = [
  'src/VoiceActivityDetector.cpp',
]

vad_lib = static_library(
  'vad',
  sources,
  include_directories: incdir,
  cpp_args: ['-std=c++17'],
  install: not meson.is_subproject()
)

# Public dependency - exposes the public include directory
vad_dep = declare_dependency(
  include_directories: incdir,
  link_with: vad_lib
)

# Only build examples/tests when built as standalone
if not meson.is_subproject()
  # Basic VAD example
  executable('vad_example',
    'example/vad_example.cpp',
    dependencies: vad_dep,
    install: false
  )

    # VAD-ASR integration example
  executable('vad_asr_integration',
    'example/vad_asr_integration.cpp',
    dependencies: vad_dep,
    install: false
  )

  # Callback-based ASR integration example
  executable('callback_asr_integration',
    'example/callback_asr_integration.cpp',
    dependencies: vad_dep,
    install: false
  )
endif
