gladedir = $(datadir)/synaptic/glade/

INCLUDES= -I${top_srcdir}/common \
	-I${top_srcdir}/pixmaps  \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DSYNAPTIC_GLADEDIR=\""$(datadir)/synaptic/glade/"\" \
	@PACKAGE_CFLAGS@ @ZVT_CFLAGS@ $(LIBTAGCOLL_CFLAGS)

sbin_PROGRAMS = synaptic 

#synaptic_LDFLAGS= --export-dynamic

synaptic_LDADD = \
	${top_builddir}/common/libsynaptic.a\
	-lapt-pkg @RPM_LIBS@ @DEB_LIBS@ \
	@PACKAGE_LIBS@ @ZVT_LIBS@ \
	-lpthread $(LIBTAGCOLL_LIBS) 

glade_DATA =                            \
        window_main_vpaned.glade window_main_vpaned.glade.strings \
        window_main_hpaned.glade window_main_hpaned.glade.strings \
	window_find.glade window_find.glade.strings \
	window_filters.glade window_filters.glade.strings \
	window_fetch.glade window_fetch.glade.strings \
	window_about.glade window_about.glade.strings \
	window_changes.glade window_changes.glade.strings \
	window_preferences.glade window_preferences.glade.strings \
	window_disc_name.glade window_disc_name.glade.strings \
	window_rginstall_progress.glade window_rginstall_progress.glade.strings \
	window_rginstall_progress_msgs.glade window_rginstall_progress_msgs.glade.strings \
	window_setopt.glade window_setopt.glade.strings \
	window_repositories.glade window_repositories.glade.strings \
	dialog_quit.glade dialog_quit.glade.strings


EXTRA_DIST = $(glade_DATA) 


synaptic_SOURCES= \
	gsynaptic.cc\
	gsynaptic.h\
	gtkpkgtree.cc\
	gtkpkgtree.h\
	gtktagtree.cc\
	gtktagtree.h\
	gtkpkglist.cc\
	gtkpkglist.h\
	rgaboutpanel.cc\
	rgaboutpanel.h\
	rgcacheprogress.cc\
	rgcacheprogress.h\
	rgpreferenceswindow.cc\
	rgpreferenceswindow.h\
	rgfetchprogress.cc\
	rgfetchprogress.h\
	rgfiltermanager.cc\
	rgfiltermanager.h\
	rgfindwindow.cc\
	rgfindwindow.h\
	rgsetoptwindow.cc\
	rgsetoptwindow.h\
	rgdummyinstallprogress.cc\
	rgdummyinstallprogress.h\
	rgzvtinstallprogress.cc\
	rgzvtinstallprogress.h\
	rginstallprogress.cc\
	rginstallprogress.h\
	rgcdscanner.cc\
	rgcdscanner.h\
	rgmainwindow.cc\
	rgmainwindow.h\
	rgrepositorywin.cc\
	rgrepositorywin.h\
	rgvendorswindow.cc\
	rgvendorswindow.h\
	rgsummarywindow.cc\
	rgsummarywindow.h\
	rgchangeswindow.cc\
	rgchangeswindow.h\
	rguserdialog.cc\
	rguserdialog.h\
	rgwindow.cc\
	rgwindow.h\
	rggladewindow.cc\
	rggladewindow.h

CLEANFILES= $(wildcard *_wrap.*) $(wildcard *~) 

