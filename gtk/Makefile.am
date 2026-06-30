SUBDIRS = gtkbuilder


AM_CPPFLAGS = -I@top_srcdir@ -I@top_srcdir@/common -I@top_srcdir@/gtk -I@top_builddir@ \
	@SYNAPTIC_APP_PATHS@ \
	@GTK_DISABLE_DEPRECATED@ \
	@GTK_CFLAGS@ \
	@VTE_CFLAGS@ \
	@LP_CFLAGS@ \
	@APT_PKG_CFLAGS@ \
	@SQLITE_CFLAGS@

bin_PROGRAMS = synaptic 

#synaptic_LDFLAGS= --export-dynamic

synaptic_LDADD = \
	${top_builddir}/common/libsynaptic.a \
	@RPM_LIBS@ \
	@DEB_LIBS@ \
	@GTK_LIBS@ \
	@VTE_LIBS@ \
	@LP_LIBS@ \
	@APT_PKG_LIBS@ \
	@SQLITE_LIBS@ \
	-lpthread \
	-lutil \
	-lX11

synaptic_SOURCES= \
	gsynaptic.cc\
	gtkpkglist.cc\
	gtkpkglist.h\
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

