test_env = environment()
test_env.set('LC_ALL', 'C')

gtest_dep = dependency('gtest', required: get_option('tests'))

test_rpackage = executable(
  'test_rpackage',
  ['test_rpackage.cc'] + common_sources,
  cpp_args: gtk_cpp_args + rpm_compile_args + ['-O0', '-g3'],
  dependencies: [glib_dep, common_deps],
  include_directories: [root_inc, common_inc, gui_inc],
)

test_rpackagefilter = executable(
  'test_rpackagefilter',
  ['test_rpackagefilter.cc'] + common_sources,
  cpp_args: gtk_cpp_args + rpm_compile_args + ['-O0', '-g3'],
  dependencies: [glib_dep, common_deps],
  include_directories: [root_inc, common_inc, gui_inc],
)

test_rpackageview = executable(
  'test_rpackageview',
  ['test_rpackageview.cc'] + common_sources,
  cpp_args: gtk_cpp_args + rpm_compile_args + ['-O0', '-g3'],
  dependencies: [glib_dep, common_deps],
  include_directories: [root_inc, common_inc, gui_inc],
)

if gtest_dep.found()
  test_rsources = executable(
    'test_rsources',
    ['test_rsources.cc'] + common_sources,
    cpp_args: gtk_cpp_args + rpm_compile_args + ['-O0', '-g3'],
    dependencies: [glib_dep, common_deps, gtest_dep],
    include_directories: [root_inc, common_inc, gui_inc],
  )
  test('test_rsources', test_rsources, env: test_env)
endif

test_gtkpkglist = executable(
  'test_gtkpkglist',
  [
    'test_gtkpkglist.cc',
    '../gtk/rgpackagestatus.cc',
    '../gtk/rgutils.cc',
    '../gtk/rgpkgtreeview.cc',
    '../gtk/gtkpkglist.cc',
  ] + common_sources,
  cpp_args: gtk_cpp_args + rpm_compile_args + ['-O0', '-g3'],
  dependencies: [glib_dep, common_deps, gui_deps],
  include_directories: [root_inc, common_inc, gui_inc],
)

test('test_rpackage', test_rpackage, env: test_env)
test('test_rpackagefilter', test_rpackagefilter, env: test_env)
test('test_rpackageview', test_rpackageview, env: test_env)
