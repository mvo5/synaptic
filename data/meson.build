po_dir = meson.project_source_root() / 'po'

i18n.merge_file(
  input: 'synaptic.desktop.in',
  output: 'synaptic.desktop',
  po_dir: po_dir,
  type: 'desktop',
  install: true,
  install_dir: datadir / 'applications',
)

i18n.merge_file(
  input: 'com.ubuntu.pkexec.synaptic.policy.in',
  output: 'com.ubuntu.pkexec.synaptic.policy',
  po_dir: po_dir,
  type: 'xml',
  install: true,
  install_dir: datadir / 'polkit-1' / 'actions',
)

i18n.merge_file(
  input: 'io.github.mvo5.synaptic.metainfo.xml.in',
  output: 'io.github.mvo5.synaptic.metainfo.xml',
  po_dir: po_dir,
  type: 'xml',
  install: true,
  install_dir: datadir / 'metainfo',
)
