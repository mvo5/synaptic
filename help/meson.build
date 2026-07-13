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