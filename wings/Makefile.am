INCLUDES=@WINGS_HDRS@ -I${top_srcdir}/common -I${top_srcdir}/pixmaps

sbin_PROGRAMS=wsynaptic


wsynaptic_LDADD =-lExtraWINGs @WINGS_LIBS@ \
	${top_builddir}/common/libraptor.a\
	-lapt-pkg @RPM_LIBS@ @DEB_LIBS@\
	-lpthread -L/usr/X11R6/lib



wsynaptic_SOURCES=\
	synaptic.cc\
	rwaboutpanel.cc\
	rwaboutpanel.h\
	rwcacheprogress.cc\
	rwcacheprogress.h\
	rwconfigwindow.h\
	rwconfigwindow.cc\
	rwfetchprogress.cc\
	rwfetchprogress.h\
	rwfiltereditor.cc\
	rwfiltereditor.h\
	rwfiltermanager.cc\
	rwfiltermanager.h\
	rwfilterwindow.cc\
	rwfilterwindow.h\
	rwdummyinstallprogress.h\
	rwdummyinstallprogress.cc\
	rwinstallprogress.h\
	rwinstallprogress.cc\
	rwmainwindow.cc\
	rwmainwindow.h\
	rwsourceswindow.cc\
	rwsourceswindow.h\
	rwsummarywindow.cc\
	rwsummarywindow.h\
	rwuserdialog.cc\
	rwuserdialog.h\
	rwwindow.cc\
	rwwindow.h


