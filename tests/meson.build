test_env = environment()
test_env.set('LC_ALL', 'C')

test_common_deps = gui_deps

test_rpackage = executable(
  'test_rpackage',
  'test_rpackage.cc',
  cpp_args: base_cpp_args + gtk_cpp_args + rpm_compile_args + ['-O0', '-g3'],
  dependencies: test_common_deps,
  include_directories: [root_inc, common_inc, gtk_inc],
  link_with: libsynaptic,
)

test_rpackagefilter = executable(
  'test_rpackagefilter',
  'test_rpackagefilter.cc',
  cpp_args: base_cpp_args + gtk_cpp_args + rpm_compile_args + ['-O0', '-g3'],
  dependencies: test_common_deps,
  include_directories: [root_inc, common_inc, gtk_inc],
  link_with: libsynaptic,
)

test_rpackageview = executable(
  'test_rpackageview',
  'test_rpackageview.cc',
  cpp_args: base_cpp_args + gtk_cpp_args + rpm_compile_args + ['-O0', '-g3'],
  dependencies: test_common_deps,
  include_directories: [root_inc, common_inc, gtk_inc],
  link_with: libsynaptic,
)

test_gtkpkglist = executable(
  'test_gtkpkglist',
  [
    'test_gtkpkglist.cc',
    '../gtk/rgpackagestatus.cc',
    '../gtk/rgutils.cc',
    '../gtk/rgpkgtreeview.cc',
    '../gtk/gtkpkglist.cc',
  ],
  cpp_args: base_cpp_args + gtk_cpp_args + rpm_compile_args + ['-O0', '-g3'],
  dependencies: test_common_deps,
  include_directories: [root_inc, common_inc, gtk_inc],
  link_with: libsynaptic,
)

# test_rpackage and test_rpackageview read every package's record from the
# on-disk Packages files; on a cold container cache that exceeds meson's 30s
# default, so give them headroom
test('test_rpackage', test_rpackage, env: test_env, timeout: 300)
test('test_rpackagefilter', test_rpackagefilter, env: test_env)
test('test_rpackageview', test_rpackageview, env: test_env, timeout: 300)
