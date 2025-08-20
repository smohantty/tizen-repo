project('perf_monitor', 'cpp',
    version : '1.0.0',
    default_options : [
        'cpp_std=c++17',
        'warning_level=3',
        'werror=false'
    ]
)

# Compiler setup
cpp = meson.get_compiler('cpp')

# Thread dependency (only dependency needed for simple time measurement)
thread_dep = dependency('threads')

# All dependencies
perf_deps = [thread_dep]

# Performance Monitor Library
perf_lib_sources = files(
    'PerfMonitor.cpp'
)

perf_lib_inc = include_directories('.')

perf_lib = static_library('perf_monitor',
    perf_lib_sources,
    include_directories : perf_lib_inc,
    dependencies : perf_deps,
    install : true,
    install_dir : get_option('libdir')
)

# Install header
install_headers('PerfMonitor.h', subdir : 'perf')

# Declare dependency for use by other projects
perf_dep = declare_dependency(
    link_with : perf_lib,
    include_directories : perf_lib_inc,
    dependencies : perf_deps
)



# Example/Demo executable
demo_exe = executable('demo_perf_monitor',
    'demo_perf_monitor.cpp',
    dependencies : [perf_dep],
    install : true,
    install_dir : get_option('bindir')
)

# Pkg-config file
pkg = import('pkgconfig')
pkg.generate(
    libraries : perf_lib,
    subdirs : 'perf',
    version : meson.project_version(),
    name : 'libperf_monitor',
    filebase : 'perf_monitor',
    description : 'Simple time-based function monitoring library'
)

# Summary
summary({
    'prefix': get_option('prefix'),
    'libdir': get_option('libdir'),
    'includedir': get_option('includedir'),
    'System': host_machine.system(),
    'CPU Family': host_machine.cpu_family(),
}, section: 'Build Configuration')

summary({
    'Thread Support': thread_dep.found(),
    'Real-time Monitoring': true,
    'Function Grouping': true,
    'Simple Time Measurement': true,
}, section: 'Features')
