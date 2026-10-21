####################
# Install the XML (yelp) manual.

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

####################
# Compile and install the  HTML manual (fallback for the Help menu when yelp is missing (rgmainwindow.cc)).

xmlto = find_program('xmlto')

html_install_root = pkg_datadir / 'html'

custom_target(
  'synaptic-html',
  input: 'C/synaptic.xml',
  output: 'synaptic.html',
  command: [ xmlto, 'html-nochunks', '--skip-validation', '-o', '@OUTDIR@', '@INPUT@', ],
  install: true,
  install_dir: html_install_root,
)

install_subdir(
  'C/figures',
  install_dir: html_install_root,
)
