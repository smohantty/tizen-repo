project(
  'ssl',
  'cpp',
  version: '0.1.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
  ],
)

inc = include_directories('include')

sources = files(
  'src/ssl.cpp',
)

ssl_lib = (
  meson.is_subproject()
    ? static_library(
        'ssl',
        sources,
        include_directories: inc,
        install: false,
      )
    : shared_library(
        'ssl',
        sources,
        include_directories: inc,
        install: true,
        version: meson.project_version(),
        soversion: '0',
      )
)

# Export dependency for subproject consumers
ssl_dep = declare_dependency(
  link_with: ssl_lib,
  include_directories: inc,
)

if not meson.is_subproject()
  # Install public headers to <includedir>/ssl
  install_subdir('include/ssl', install_dir: get_option('includedir'))

  # Provide pkg-config file for system-wide usage
  pkg = import('pkgconfig')
  pkg.generate(
    libraries: ssl_lib,
    version: meson.project_version(),
    name: 'ssl',
    filebase: 'ssl',
    description: 'SSL library (example skeleton)',
    subdirs: 'ssl',
  )
endif

# Allow parent projects to use dependency('ssl', fallback: ['ssl', 'ssl_dep'])
meson.override_dependency('ssl', ssl_dep)


