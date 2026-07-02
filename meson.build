project(
  'synaptic',
  ['c', 'cpp'],
  version: '1',
  default_options: [
    'warning_level=1',
    'cpp_std=c++23',
  ],
)

add_project_arguments('-fcoroutines', language: ['cpp'])

i18n = import('i18n')
fs = import('fs')
cmake = import('cmake')

prefix = get_option('prefix')
datadir = get_option('datadir')
localedir = get_option('localedir')
localstatedir = get_option('localstatedir')

abs_datadir = datadir.startswith('/') ? datadir : join_paths(prefix, datadir)
abs_localedir = localedir.startswith('/') ? localedir : join_paths(prefix, localedir)
abs_localstatedir = localstatedir.startswith('/') ? localstatedir : join_paths(prefix, localstatedir)

synaptic_gtkbuilderdir = join_paths(abs_datadir, 'synaptic', 'gtkbuilder') + '/'
synaptic_pixmapdir = join_paths(abs_datadir, 'synaptic', 'pixmaps') + '/'

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
conf.set_quoted('GETTEXT_PACKAGE', 'synaptic')
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE_DATA_DIR', abs_datadir)
conf.set_quoted('PACKAGE_LOCALE_DIR', abs_localedir)
conf.set_quoted('SYNAPTIC_GTKBUILDERDIR', synaptic_gtkbuilderdir)
conf.set_quoted('SYNAPTIC_PIXMAPDIR', synaptic_pixmapdir)
conf.set_quoted('SYNAPTICLOCALEDIR', abs_localedir)
conf.set_quoted('SYNAPTICSTATEDIR', abs_localstatedir)
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

base_cpp_args = [
  '-DHAVE_CONFIG_H',
  '-DGETTEXT_PACKAGE="synaptic"',
  '-DPACKAGE_DATA_DIR="@0@"'.format(abs_datadir),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(abs_localedir),
  '-DSYNAPTIC_GTKBUILDERDIR="@0@"'.format(synaptic_gtkbuilderdir),
  '-DSYNAPTIC_PIXMAPDIR="@0@"'.format(synaptic_pixmapdir),
  '-DSYNAPTICLOCALEDIR="@0@"'.format(abs_localedir),
  '-DSYNAPTICSTATEDIR="@0@"'.format(abs_localstatedir),
  '-DVERSION="@0@"'.format(meson.project_version()),
]
gtk_cpp_args = [
  '-DGDK_DISABLE_DEPRECATED',
  '-DGTK_DISABLE_DEPRECATED',
]
if get_option('gtk_disable_deprecated')
  gtk_cpp_args += ['-DGTK_DISABLE_DEPRECATED']
endif

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
subdir('tests')

help_install_root = join_paths(get_option('datadir'), 'gnome', 'help', 'synaptic')
foreach lang : ['C', 'es', 'hr', 'sv']
  install_data(
    [
      'help/@0@/synaptic.xml'.format(lang),
      'help/@0@/gpl.xml'.format(lang),
    ],
    install_dir: join_paths(help_install_root, lang),
  )
  install_subdir(
    'help/@0@/figures'.format(lang),
    install_dir: join_paths(help_install_root, lang),
  )
endforeach

if fs.exists('doc/html/C/index.html')
  doc_html_files = files(
    'doc/html/C/apa.html',
    'doc/html/C/apas02.html',
    'doc/html/C/apas03.html',
    'doc/html/C/ar01s02.html',
    'doc/html/C/ar01s03.html',
    'doc/html/C/ar01s04.html',
    'doc/html/C/ar01s05.html',
    'doc/html/C/ar01s06.html',
    'doc/html/C/ar01s07.html',
    'doc/html/C/ar01s08.html',
    'doc/html/C/index.html',
  )
  install_data(doc_html_files, install_dir: join_paths(get_option('datadir'), 'synaptic', 'html'))
  install_data(
    files(
      'help/C/figures/synaptic-categories.png',
      'help/C/figures/synaptic-filter.png',
      'help/C/figures/synaptic-packagedetails.png',
      'help/C/figures/synaptic-packagelist.png',
      'help/C/figures/synaptic-repositories.png',
      'help/C/figures/synaptic-start.png',
      'help/C/figures/synaptic-toolbar.png',
    ),
    install_dir: join_paths(get_option('datadir'), 'synaptic', 'html', 'figures'),
  )
endif
