pkg_icon_dir = datadir / meson.project_name() / 'icons' / 'hicolor'

install_data(
  files(
    'update_small.png',
    'upgrade_small.png',
    'distupgrade_small.png',
    'proceed_small.png',
    'pref_vpaned.xpm',
    'pref_hpaned.xpm',
    'stock_filter-data-by-criteria.png',
    'stock_filter-navigator.png',
    'stock_menu_about.png',
    'stock_help-book.png',
    'deb.png',
    'cnc.png',
    'logo.png',
  ),
  install_dir: join_paths(get_option('datadir'), 'synaptic', 'gtkbuilder'),
)

install_data(
  files(
    'green.png',
    'yellow.png',
    'red.png',
  ),
  install_dir: join_paths(get_option('datadir'), 'synaptic', 'pixmaps'),
)

install_data(
  files(
    'hicolor/16x16/actions/package-available-locked.png',
    'hicolor/16x16/actions/package-available.png',
    'hicolor/16x16/actions/package-broken.png',
    'hicolor/16x16/actions/package-downgrade.png',
    'hicolor/16x16/actions/package-install.png',
    'hicolor/16x16/actions/package-installed-locked.png',
    'hicolor/16x16/actions/package-installed-outdated.png',
    'hicolor/16x16/actions/package-installed-updated.png',
    'hicolor/16x16/actions/package-new.png',
    'hicolor/16x16/actions/package-purge.png',
    'hicolor/16x16/actions/package-reinstall.png',
    'hicolor/16x16/actions/package-remove.png',
    'hicolor/16x16/actions/package-supported.png',
    'hicolor/16x16/actions/package-upgrade.png',
    'hicolor/16x16/actions/system-upgrade.png',
  ),
  install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', '16x16', 'actions'),
)

install_data(
  files('hicolor/24x24/actions/system-upgrade.png'),
  install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', '24x24', 'actions'),
)

install_data(
  ['warning.svg'],
  install_dir: pkg_icon_dir / 'scalable' / 'categories',
)

install_data(
  files('hicolor/256x256/apps/synaptic.png'),
  install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', '256x256', 'apps'),
)

install_data(
  files('hicolor/scalable/apps/synaptic.svg'),
  install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', 'scalable', 'apps'),
)
