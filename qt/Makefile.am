
AM_CXXFLAGS = $(QT_CXXFLAGS)

INCLUDES= -I${top_srcdir}/common \
	-I${top_srcdir}/pixmaps  \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DSYNAPTIC_PIXMAPDIR=\""$(datadir)/synaptic/pixmaps/"\"

sbin_PROGRAMS = synaptic 

synaptic_LDADD = $(LDADD) \
	${top_builddir}/common/libsynaptic.a\
	-lapt-pkg @RPM_LIBS@ @DEB_LIBS@ \
	-lpthread $(LIBTAGCOLL_LIBS) \
	$(QT_LDADD)

synaptic_UI = \
	window_main.h \
	window_main.cc \
	window_main.moc.cc \
	window_changes.h \
	window_changes.cc \
	window_changes.moc.cc \
	window_fetch.h \
	window_fetch.cc \
	window_fetch.moc.cc

synaptic_SOURCES = \
	$(synaptic_UI) \
	qsynaptic.h \
	qsynaptic.cc \
	rqmainwindow.h \
	rqmainwindow.cc \
	rqmainwindow.moc.cc \
	rqcacheprogress.h \
	rqcacheprogress.cc \
	rquserdialog.h \
	rquserdialog.cc \
	rqobservers.h \
	rqobservers.cc \
	rqobservers.moc.cc \
	rqpackageitem.h \
	rqpackageitem.cc \
	rqpackagepopup.h \
	rqpackagepopup.cc \
	rqpackagepopup.moc.cc \
	rqchangeswindow.h \
	rqchangeswindow.cc \
	rqchangeswindow.moc.cc \
	rqfetchwindow.h \
	rqfetchwindow.cc \
	rqfetchwindow.moc.cc \
	rqfetchitem.h \
	rqfetchitem.cc

EXTRA_DIST = 

%.moc.cc: %.h
	$(MOC) -o $@ $<

%.h: %.ui
	$(UIC) -o $@ $<

%.cc: %.ui %.h %.moc.cc
	$(UIC) -o $@ -impl $*.h $<

