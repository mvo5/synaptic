INCLUDES=@WINGS_HDRS@ -I${top_srcdir}/common -I${top_srcdir}/pixmaps

sbin_PROGRAMS=wsynaptic


wsynaptic_LDADD =-lExtraWINGs @WINGS_LIBS@ \
	${top_builddir}/common/libsynaptic.a\
	-lapt-pkg @RPM_LIBS@ @DEB_LIBS@\
	-lpthread -L/usr/X11R6/lib



wsynaptic_SOURCES=\
	synaptic.cc\
	rwaboutpanel.cc\
	rwcacheprogress.cc\
	rwconfigwindow.cc\
	rwfetchprogress.cc\
	rwfiltereditor.cc\
	rwfiltermanager.cc\
	rwfilterwindow.cc\
	rwdummyinstallprogress.cc\
	rwinstallprogress.cc\
	rwmainwindow.cc\
	rwsourceswindow.cc\
	rwsummarywindow.cc\
	rwuserdialog.cc\
	rwwindow.cc

