gladedir = $(datadir)/synaptic/glade/

INCLUDES= -I${top_srcdir}/common \
	-I${top_srcdir}/pixmaps  \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DSYNAPTIC_GLADEDIR=\""$(datadir)/synaptic/glade/"\" \
	-DSYNAPTIC_PIXMAPDIR=\""$(datadir)/synaptic/pixmaps/"\" \
	@PACKAGE_CFLAGS@ @ZVT_CFLAGS@ @VTE_CFLAGS@ $(LIBTAGCOLL_CFLAGS)

sbin_PROGRAMS = synaptic 

#synaptic_LDFLAGS= --export-dynamic

synaptic_LDADD = \
	${top_builddir}/common/libsynaptic.a\
	-lapt-pkg @RPM_LIBS@ @DEB_LIBS@ \
	@PACKAGE_LIBS@ @ZVT_LIBS@ @VTE_LIBS@ \
	-lpthread $(LIBTAGCOLL_LIBS) 

glade_DATA =                            \
	window_about.glade  \
	window_changes.glade  \
	window_details.glade \
	window_disc_name.glade  \
	window_fetch.glade  \
	window_find.glade  \
	window_filters.glade \
	window_iconlegend.glade \
	dialog_changelog.glade \
	dialog_change_version.glade \
	dialog_locales_warning.glade \
        window_main.glade \
	window_preferences.glade \
	dialog_quit.glade  \
	window_rginstall_progress.glade \
	window_rginstall_progress_msgs.glade \
	window_repositories.glade \
	window_setopt.glade  \
	window_summary.glade \
	window_tasks.glade \
	dialog_download_error.glade \
	dialog_task_descr.glade \
	dialog_unmet.glade \
	dialog_update_failed.glade \
	dialog_update_outdated.glade \
	dialog_upgrade.glade \
	dialog_welcome.glade \
	window_zvtinstallprogress.glade 


EXTRA_DIST = $(glade_DATA) 


synaptic_SOURCES= \
	gsynaptic.cc\
	gsynaptic.h\
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
	rgiconlegend.h\
	rgiconlegend.cc\
	rgmisc.cc\
	rgmisc.h\
	rgsetoptwindow.cc\
	rgsetoptwindow.h\
	rgdummyinstallprogress.cc\
	rgdummyinstallprogress.h\
	rgterminstallprogress.cc\
	rgterminstallprogress.h\
	rginstallprogress.cc\
	rginstallprogress.h\
	rgslideshow.cc\
	rgslideshow.h\
	rgtaskswin.cc\
	rgtaskswin.h\
	rgcdscanner.cc\
	rgcdscanner.h\
	rgmainwindow.cc\
	rgmainwindow.h\
	rgpkgdetails.cc \
	rgpkgdetails.h \
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

