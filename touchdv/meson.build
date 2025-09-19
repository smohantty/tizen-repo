project('touchdv', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++17'])

# Thread dependency for all executables
thread_dep = dependency('threads')

# Source files shared between executables
touchdev_sources = ['VirtualTouchDevice.cpp']

# Main demo executable
exe = executable('touchdv',
  ['touchdv.cpp'] + touchdev_sources,
  dependencies : [thread_dep],
  install : true)

# Real-world mixed gesture test executable (uses clean callback-only interface)
realworld_test_exe = executable('test_realworld_gestures',
  ['test_realworld_gestures.cpp'] + touchdev_sources,
  dependencies : [thread_dep],
  install : false)

# Legacy test executables (use deprecated interfaces - commented out)
# gesture_test_exe = executable('gesture_tests',
#   ['gesture_tests.cpp'] + touchdev_sources,
#   dependencies : [thread_dep],
#   install : false)
#
# discrete_test_exe = executable('test_discrete_only',
#   ['test_discrete_only.cpp'] + touchdev_sources,
#   dependencies : [thread_dep],
#   install : false)
#
# continuous_test_exe = executable('test_continuous_gestures',
#   ['test_continuous_gestures.cpp'] + touchdev_sources,
#   dependencies : [thread_dep],
#   install : false)

# Tests
test('basic_demo', exe)
test('realworld_gestures', realworld_test_exe)
