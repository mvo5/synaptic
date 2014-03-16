SUBDIRS = gtkbuilder


INCLUDES= -I${top_srcdir}/common \
	-I${top_srcdir}/pixmaps  \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DSYNAPTIC_GTKBUILDERDIR=\""$(datadir)/synaptic/gtkbuilder/"\" \
	-DSYNAPTIC_PIXMAPDIR=\""$(datadir)/synaptic/pixmaps/"\" \
	@GTK_CFLAGS@ \
	@VTE_CFLAGS@ \
	@LP_CFLAGS@ \
	@GTK_DISABLE_DEPRECATED@ \
	$(LIBEPT_CFLAGS)

sbin_PROGRAMS = synaptic 

#synaptic_LDFLAGS= --export-dynamic

synaptic_LDADD = \
	${top_builddir}/common/libsynaptic.a\
	-lapt-pkg -lX11 @RPM_LIBS@ @DEB_LIBS@ \
	@GTK_LIBS@ \
	@VTE_LIBS@ @LP_LIBS@\
	-lpthread $(LIBEPT_LIBS)  

synaptic_SOURCES= \
	gsynaptic.cc\
	gsynaptic.h\
	gtkpkglist.cc\
	gtkpkglist.h\
	gtk3compat.h\
	rgaboutpanel.cc\
	rgaboutpanel.h\
	rgcacheprogress.cc\
	rgcacheprogress.h\
	rgpkgcdrom.cc \
	rgpkgcdrom.h \
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
	rglogview.cc\
	rglogview.h\
	rgutils.cc\
	rgutils.h\
	rgpackagestatus.cc\
	rgpackagestatus.h\
	rgsetoptwindow.cc\
	rgsetoptwindow.h\
	rgdummyinstallprogress.cc\
	rgdummyinstallprogress.h\
	rgdebinstallprogress.cc\
	rgdebinstallprogress.h\
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
	rgchangelogdialog.cc\
	rgchangelogdialog.h\
	rgchangeswindow.cc\
	rgchangeswindow.h\
	rguserdialog.cc\
	rguserdialog.h\
	rgwindow.cc\
	rgwindow.h\
	rggtkbuilderwindow.cc\
	rggtkbuilderwindow.h\
	rgpkgtreeview.cc\
	rgpkgtreeview.h

CLEANFILES= $(wildcard *_wrap.*) $(wildcard *~) 

