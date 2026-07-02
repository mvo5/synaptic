common_sources = files(
  'pkg_acqfile.cc',
  'rconfiguration.cc',
  'rinstallprogress.cc',
  'rpackage.cc',
  'rpackagestatus.cc',
  'rpackagecache.cc',
  'rpackagefilter.cc',
  'rpackagelister.cc',
  'rpackageview.cc',
  'rcdscanner.cc',
  'rpmindexcopy.cc',
  'raptoptions.cc',
  'rsources.cc',
  'rcacheactor.cc',
  'rpackagelistactor.cc',
  'rtagcollbuilder.cc',
  'rtagcollfilter.cc',
  'ruserdialog.cc',
  'sections_trans.cc',
)

libsynaptic = static_library(
  'synaptic',
  common_sources,
  cpp_args: base_cpp_args + rpm_compile_args,
  dependencies: common_deps + [task_dep],
  include_directories: [root_inc, common_inc],
)

libsynaptic_dep = declare_dependency(
  link_with: libsynaptic,
  include_directories: [root_inc, common_inc],
)
