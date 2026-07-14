project(
  'synaptic',
  ['c', 'cpp'],
  version: '0.91.7',
  default_options: [
    'warning_level=1',
    'cpp_std=c++23',
  ],
)

add_project_arguments('-fcoroutines', language: ['cpp'])

i18n = import('i18n')
gnome = import('gnome')
cmake = import('cmake')

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
localedir = prefix / get_option('localedir')
localstatedir = prefix / get_option('localstatedir')

pkg_datadir = datadir / meson.project_name()

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

root_inc = include_directories('.')
common_inc = include_directories('common')
gtk_inc = include_directories('gtk')

task_options = cmake.subproject_options()
task_options.add_cmake_defines({'BEMAN_USE_MODULES': 'OFF'})
task = cmake.subproject('task', options: task_options)
task_dep = task.get_variable('beman_task_dep')

threads_dep = dependency('threads')
gtk_dep = dependency('gtk4', version: '>= 4.20.0')
tbb_dep = dependency('tbb')
glib_dep = dependency('glib-2.0')
apt_pkg_dep = dependency('apt-pkg', required: true)
x11_dep = cpp.find_library('X11', required: true)
util_dep = cpp.find_library('util', required: true)
apt_inst_dep = cpp.find_library('apt-inst', required: false)
dependency('polkit-gobject-1', required: true)

if not cpp.has_header('apt-pkg/configuration.h', dependencies: apt_pkg_dep)
  error('You need the apt-pkg headers installed to compile synaptic.')
endif

rpm_deps = []
rpm_compile_args = []
have_rpm = false
if cpp.has_header('rpm/rpmlib.h', args: ['-I/usr/include/rpm'])
  rpm_compile_args = ['-I/usr/include/rpm']
  rpm_dep_v4 = [
    cpp.find_library('rpm', required: false),
    cpp.find_library('rpmio', required: false),
    cpp.find_library('rpmdb', required: false),
    cpp.find_library('popt', required: false),
  ]
  if rpm_dep_v4[0].found() and rpm_dep_v4[1].found() and rpm_dep_v4[2].found() and rpm_dep_v4[3].found()
    rpm_deps = rpm_dep_v4
    have_rpm = true
  else
    rpm_dep_v3 = [
      cpp.find_library('rpm', required: false),
      cpp.find_library('popt', required: false),
    ]
    if rpm_dep_v3[0].found() and rpm_dep_v3[1].found()
      rpm_deps = rpm_dep_v3
      have_rpm = true
    endif
  endif
endif

have_apt_auth = cpp.has_header('apt-pkg/metaindex.h', dependencies: apt_pkg_dep)
have_aptpkg_cdrom = cpp.has_header('apt-pkg/cdrom.h', dependencies: apt_pkg_dep)

vte_dep = dependency('vte-2.91-gtk4', version: '>= 0.5', required: false)
have_vte = vte_dep.found()

if get_option('dpkg_progress').enabled() and not have_vte
  error('dpkg-progress requires vte. Disable it with -Ddpkg_progress=disabled.')
endif

launchpad_dep = dependency('launchpad-integration', required: get_option('launchpad_integration'))

xapian_dep = dependency('', required: false)
have_xapian = false
xapian_config = find_program('xapian-config', required: false)
if xapian_config.found()
  xapian_dep = declare_dependency(
    compile_args: run_command(xapian_config, '--cxxflags', check: true).stdout().strip().split(),
    link_args: run_command(xapian_config, '--libs', check: true).stdout().strip().split(),
  )
  have_xapian = true
endif

dpkg_vendor = find_program('dpkg-vendor', required: false)
vendor_derives_from_ubuntu = false
if dpkg_vendor.found()
  vendor_derives_from_ubuntu = run_command(
    dpkg_vendor,
    '--derives-from',
    'ubuntu',
    check: false,
  ).returncode() == 0
endif

conf = configuration_data()
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE_DATA_DIR', pkg_datadir)
conf.set_quoted('PACKAGE_LOCALE_DIR', localedir)
conf.set_quoted('SYNAPTICSTATEDIR', localstatedir)
conf.set_quoted('VERSION', meson.project_version())
conf.set('ENABLE_NLS', 1)

if have_rpm
  conf.set('HAVE_RPM', 1)
endif
if get_option('scripts')
  conf.set('WITH_LUA', 1)
endif
if have_apt_auth
  conf.set('WITH_APT_AUTH', 1)
endif
if have_aptpkg_cdrom
  conf.set('HAVE_APTPKG_CDROM', 1)
endif
if have_xapian
  conf.set('HAVE_XAPIAN', 1)
endif
if have_vte
  conf.set('HAVE_VTE', 1)
  conf.set('HAVE_TERMINAL', 1)
endif
if get_option('dpkg_progress').enabled()
  conf.set('WITH_DPKG_STATUSFD', 1)
endif
if launchpad_dep.found()
  conf.set('WITH_LAUNCHPAD_INTEGRATION', 1)
endif
if get_option('pkg_hold')
  conf.set('SYNAPTIC_PKG_HOLD', 1)
endif
if get_option('apt_multiarch_support')
  conf.set('WITH_APT_MULTIARCH_SUPPORT', 1)
endif
if vendor_derives_from_ubuntu
  conf.set('VENDOR_DERIVES_FROM_UBUNTU', 1)
endif

configure_file(output: 'config.h', configuration: conf)

gtk_cpp_args = [
  '-DG_DISABLE_DEPRECATED',
  '-DGDK_DISABLE_DEPRECATED',
  '-DGTK_DISABLE_DEPRECATED',
  '-DGDK_PIXBUF_DISABLE_DEPRECATED',
]

common_deps = [apt_pkg_dep, task_dep]
common_deps += rpm_deps
if apt_inst_dep.found()
  common_deps += [apt_inst_dep]
endif
if have_xapian
  common_deps += [xapian_dep]
endif

gui_deps = [
  task_dep,
  tbb_dep,
  gtk_dep,
  glib_dep,
  apt_pkg_dep,
  x11_dep,
  util_dep,
  threads_dep,
]
gui_deps += rpm_deps
if apt_inst_dep.found()
  gui_deps += [apt_inst_dep]
endif
if have_vte
  gui_deps += [vte_dep]
endif
if have_xapian
  gui_deps += [xapian_dep]
endif
if launchpad_dep.found()
  gui_deps += [launchpad_dep]
endif

subdir('po')
subdir('common')
subdir('gtk')
subdir('data')
subdir('pixmaps')
subdir('man')
subdir('help')
subdir('tests')

gnome.post_install(
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)

run_target('lint', command: [files('tools/lint-whitespace.sh')])
