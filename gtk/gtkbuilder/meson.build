# Copy the .ui files into the build dir so that running synaptic from
# build/gtk/ picks them up via the "gtkbuilder/" development fallback
# in rggtkbuilderwindow.cc (and install them from here as well).
fs = import('fs')

gtkbuilder_ui_files = [
  'dialog_authentication.ui',
  'dialog_changelog.ui',
  'dialog_change_version.ui',
  'dialog_conffile.ui',
  'dialog_quit.ui',
  'dialog_task_descr.ui',
  'dialog_unmet.ui',
  'dialog_update_failed.ui',
  'dialog_upgrade.ui',
  'dialog_welcome.ui',
  'window_changes.ui',
  'window_details.ui',
  'window_disc_name.ui',
  'window_fetch.ui',
  'window_filters.ui',
  'window_find.ui',
  'window_iconlegend.ui',
  'window_logview.ui',
  'window_main.ui',
  'window_preferences.ui',
  'window_repositories.ui',
  'window_rgdebinstall_progress.ui',
  'window_rginstall_progress_msgs.ui',
  'window_rginstall_progress.ui',
  'window_setopt.ui',
  'window_summary.ui',
  'window_tasks.ui',
  'window_zvtinstallprogress.ui',
]

foreach ui : gtkbuilder_ui_files
  fs.copyfile(
    ui,
    install: true,
    install_dir: join_paths(get_option('datadir'), 'synaptic', 'gtkbuilder'),
  )
endforeach
