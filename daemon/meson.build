daemon_sources = files(
  'interface.c',
  'synapticd.cc',
)

synapticd = executable(
  'synapticd',
  daemon_sources,
  install: true,
  install_dir: libexecdir / 'synaptic',
  # the SD_VARLINK_DEFINE_* macros are only available in C with _GNU_SOURCE
  c_args: ['-D_GNU_SOURCE'],
  dependencies: [libsystemd_dep, util_dep] + common_deps,
  include_directories: [root_inc, common_inc],
  link_with: libsynaptic,
)
