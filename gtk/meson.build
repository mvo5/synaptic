gtk_sources = files(
  'gsynaptic.cc',
  'gtkpkglist.cc',
  'rgcacheprogress.cc',
  'rgpkgcdrom.cc',
  'rgpreferenceswindow.cc',
  'rgfetchprogress.cc',
  'rgfiltermanager.cc',
  'rgfindwindow.cc',
  'rgiconlegend.cc',
  'rglogview.cc',
  'rgutils.cc',
  'rgpackagestatus.cc',
  'rgsetoptwindow.cc',
  'rgdummyinstallprogress.cc',
  'rgdebinstallprogress.cc',
  'rgterminstallprogress.cc',
  'rginstallprogress.cc',
  'rgslideshow.cc',
  'rgtaskswin.cc',
  'rgcdscanner.cc',
  'rgmainwindow.cc',
  'rgpkgdetails.cc',
  'rgrepositorywin.cc',
  'rgsummarywindow.cc',
  'rgchangelogdialog.cc',
  'rgchangeswindow.cc',
  'rguserdialog.cc',
  'rgwindow.cc',
  'rggtkbuilderwindow.cc',
  'rgpkgtreeview.cc',
)

resources = gnome.compile_resources(
  'resources',
  'gtkbuilder/synaptic.gresource.xml',
  source_dir: '.'
)

executable(
  'synaptic',
  gtk_sources + resources,
  install: true,
  install_dir: get_option('bindir'),
  cpp_args: base_cpp_args + gtk_cpp_args + rpm_compile_args,
  dependencies: gui_deps,
  include_directories: [root_inc, common_inc, gtk_inc],
  link_with: libsynaptic,
)
