
INCLUDES= -I${top_srcdir}/common \
	@PACKAGE_CFLAGS@ @VTE_CFLAGS@ @LP_CFLAGS@ $(LIBTAGCOLL_CFLAGS) $(LIBEPT_CFLAGS) -O0 -g3

noinst_PROGRAMS = test_rpackage

test_rpackage_LDADD = \
	${top_builddir}/common/libsynaptic.a\
	-lapt-pkg @RPM_LIBS@ @DEB_LIBS@ \
	@PACKAGE_LIBS@ @VTE_LIBS@ @LP_LIBS@\
	-lpthread $(LIBTAGCOLL_LIBS) $(LIBEPT_LIBS)

test_rpackage_SOURCES= test_rpackage.cc

CLEANFILES= $(wildcard *_wrap.*) $(wildcard *~) 

