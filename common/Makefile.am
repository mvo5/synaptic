

noinst_LIBRARIES = libsynaptic.a

INCLUDES = -I/usr/include/apt-pkg @RPM_HDRS@ @DEB_HDRS@ @PACKAGE_CFLAGS@ \
	$(LIBTAGCOLL_CFLAGS)

EXTRA_DIST = *.h

libsynaptic_a_SOURCES =\
	rconfiguration.cc\
	rinstallprogress.cc\
	rpackage.cc\
	rpackagecache.cc\
	rpackagefilter.cc\
	rpackagelister.cc\
	rcdscanner.cc\
	indexcopy.cc\
	rpmindexcopy.cc \
	raptoptions.cc \
	rsources.cc \
	rcacheactor.cc \
	rpackagelistactor.cc \
	rtagcollbuilder.cc \
	i18n.h \
	tree.hh \
	rtagcollfilter.cc 


