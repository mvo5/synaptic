gladedir = $(datadir)/synaptic/glade/

INCLUDES= -I${top_srcdir}/common \
	-I${top_srcdir}/pixmaps  \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DSYNAPTIC_GLADEDIR=\""$(datadir)/synaptic/glade/"\" \
	@PACKAGE_CFLAGS@ @ZVT_CFLAGS@

sbin_PROGRAMS= synaptic 

synaptic_LDFLAGS=-export-dynamic

synaptic_LDADD = \
	${top_builddir}/common/libsynaptic.a\
	-lapt-pkg @RPM_LIBS@ @DEB_LIBS@ \
	@PACKAGE_LIBS@ @ZVT_LIBS@ \
	-lpthread

glade_DATA =                            \
        window_main.glade window_main.glade.strings \
	window_find.glade window_find.glade.strings \
	window_filters.glade window_filters.glade.strings \
	window_fetch.glade window_fetch.glade.strings \
	window_about.glade window_about.glade.strings \
	window_changes.glade window_changes.glade.strings \
	window_options.glade window_options.glade.strings \
	window_disc_name.glade window_disc_name.glade.strings \
	window_rginstall_progress.glade window_rginstall_progress.glade.strings \
	window_rginstall_progress_msgs.glade window_rginstall_progress_msgs.glade.strings \
	window_setopt.glade window_setopt.glade.strings \
	window_repositories.glade window_repositories.glade.strings


EXTRA_DIST = $(glade_DATA)


synaptic_SOURCES=\
	gsynaptic.cc\
	gtkpkgtree.cc\
	gtkpkglist.cc\
	rgaboutpanel.cc\
	rgcacheprogress.cc\
	rgconfigwindow.cc\
	rgfetchprogress.cc\
	rgfiltermanager.cc\
	rgfindwindow.cc\
	rgsetoptwindow.cc\
	rgdummyinstallprogress.cc\
	rgzvtinstallprogress.cc\
	rginstallprogress.cc\
	rgcdscanner.cc\
	rgmainwindow.cc\
	rgrepositorywin.cc\
	rgvendorswindow.cc\
	rgsummarywindow.cc\
	rgchangeswindow.cc\
	rguserdialog.cc\
	rgwindow.cc\
	rggladewindow.cc

CLEANFILES= $(wildcard *_wrap.*) $(wildcard *~) 

