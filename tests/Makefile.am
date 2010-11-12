
INCLUDES= -I${top_srcdir}/common \
	@GTK_CFLAGS@ @VTE_CFLAGS@ @LP_CFLAGS@ $(LIBTAGCOLL_CFLAGS) $(LIBEPT_CFLAGS) -O0 -g3

noinst_PROGRAMS = test_rpackage test_rpackageview

LDADD = \
	${top_builddir}/common/libsynaptic.a\
	-lapt-pkg @RPM_LIBS@ @DEB_LIBS@ \
	@GTK_LIBS@ @VTE_LIBS@ @LP_LIBS@\
	-lpthread $(LIBTAGCOLL_LIBS) $(LIBEPT_LIBS)

test_rpackage_SOURCES= test_rpackage.cc

test_rpackageview_SOURCES= test_rpackageview.cc

CLEANFILES= $(wildcard *_wrap.*) $(wildcard *~) 

