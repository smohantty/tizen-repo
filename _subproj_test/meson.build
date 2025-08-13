project('parent', 'cpp', default_options: ['cpp_std=c++17', 'warning_level=3'])

ssl_sp = subproject('ssl')
ssl_dep = ssl_sp.get_variable('ssl_dep')

executable('app', 'src/main.cpp', dependencies: [ssl_dep])


