i18n.merge_file(
  input: 'synaptic.desktop.in',
  output: 'synaptic.desktop',
  po_dir: join_paths(meson.project_source_root(), 'po'),
  type: 'desktop',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

i18n.merge_file(
  input: 'com.ubuntu.pkexec.synaptic.policy.in',
  output: 'com.ubuntu.pkexec.synaptic.policy',
  po_dir: join_paths(meson.project_source_root(), 'po'),
  type: 'xml',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'polkit-1', 'actions'),
)

i18n.merge_file(
  input: 'io.github.mvo5.synaptic.metainfo.xml.in',
  output: 'io.github.mvo5.synaptic.metainfo.xml',
  po_dir: join_paths(meson.project_source_root(), 'po'),
  type: 'xml',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo'),
)
