help_install_root = datadir / 'gnome' / 'help' / 'synaptic'
foreach lang : ['C', 'es', 'hr', 'sv']
  install_data(
    [
      lang / 'synaptic.xml',
      lang / 'gpl.xml',
    ],
    install_dir: help_install_root / lang,
  )
  install_subdir(
    lang / 'figures',
    install_dir: help_install_root / lang,
  )
endforeach

# HTML manual: the fallback the Help menu opens when yelp is missing (rgmainwindow.cc)
xmlto = find_program('xmlto')
custom_target(
  'synaptic-html',
  input: files('C/synaptic.xml'),
  output: 'html',
  # drop xmlto's .proc intermediate so only the manual itself is shipped
  command: [
    'sh', '-c',
    xmlto.full_path() + ' html --skip-validation -o "$1" "$2" && rm -f "$1"/*.proc',
    'sh', '@OUTPUT@', '@INPUT@',
  ],
  install: true,
  install_dir: pkg_datadir,
)

install_subdir(
  'C/figures',
  install_dir: pkg_datadir / 'html',
)