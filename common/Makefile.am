

noinst_LIBRARIES = libsynaptic.a

INCLUDES = -I/usr/include/apt-pkg @RPM_HDRS@ @DEB_HDRS@ @GTK_CFLAGS@ \
	$(LIBEPT_CFLAGS) \
	-DSYNAPTICLOCALEDIR=\""$(synapticlocaledir)"\" \
	-DSYNAPTICSTATEDIR=\""$(localstatedir)"\"

libsynaptic_a_SOURCES =\
	pkg_acqfile.cc\
	pkg_acqfile.h\
	rconfiguration.cc\
	rconfiguration.h\
	rinstallprogress.cc\
	rinstallprogress.h\
	rpackage.cc\
	rpackage.h\
	rpackagestatus.cc\
	rpackagestatus.h\
	rpackagecache.cc\
	rpackagecache.h\
	rpackagefilter.cc\
	rpackagefilter.h\
	rpackagelister.cc\
	rpackagelister.h\
	rpackageview.cc\
	rpackageview.h\
	rcdscanner.cc\
	rcdscanner.h\
	indexcopy.cc\
	indexcopy.h\
	rpmindexcopy.cc \
	rpmindexcopy.h \
	raptoptions.cc \
	raptoptions.h\
	rsources.cc \
	rsources.h \
	rcacheactor.cc \
	rcacheactor.h \
	rpackagelistactor.cc \
	rpackagelistactor.h \
	rtagcollbuilder.cc \
	rtagcollbuilder.h \
	i18n.h \
	sections_trans.h \
	rtagcollfilter.cc \
	rtagcollfilter.h \
	ruserdialog.h \
	ruserdialog.cc \
	sections_trans.h \
	sections_trans.cc 


