gladedir = $(datadir)/synaptic/glade/

INCLUDES= -I${top_srcdir}/common \
	-I${top_srcdir}/pixmaps  \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DSYNAPTIC_GLADEDIR=\""$(datadir)/synaptic/glade/"\" \
	@PACKAGE_CFLAGS@ @ZVT_CFLAGS@

sbin_PROGRAMS= synaptic 

synaptic_LDFLAGS=-export-dynamic

synaptic_LDADD = \
	${top_builddir}/common/libsynaptic.a\
	-lapt-pkg @RPM_LIBS@ @DEB_LIBS@ \
	@PACKAGE_LIBS@ @ZVT_LIBS@ \
	-lpthread

glade_DATA =                            \
        window_main.glade window_main.glade.strings \
	window_find.glade window_find.glade.strings \
	window_filters.glade window_filters.glade.strings \
	window_fetch.glade window_fetch.glade.strings \
	window_about.glade window_about.glade.strings \
	window_changes.glade window_changes.glade.strings \
	window_options.glade window_options.glade.strings \
	window_disc_name.glade window_disc_name.glade.strings \
	window_rginstall_progress.glade window_rginstall_progress.glade.strings \
	window_rginstall_progress_msgs.glade window_rginstall_progress_msgs.glade.strings \
	window_setopt.glade window_setopt.glade.strings


EXTRA_DIST = $(glade_DATA)


synaptic_SOURCES=\
	synapticinterface.i\
	gsynaptic.cc\
	gsynaptic.h\
	gtkpkgtree.cc\
	gtkpkgtree.h\
	rgaboutpanel.cc\
	rgaboutpanel.h\
	rgcacheprogress.cc\
	rgcacheprogress.h\
	rgconfigwindow.h\
	rgconfigwindow.cc\
	rgfetchprogress.cc\
	rgfetchprogress.h\
	rgfiltermanager.cc\
	rgfiltermanager.h\
	rgfindwindow.cc\
	rgfindwindow.h\
	rgsetoptwindow.cc\
	rgsetoptwindow.h\
	rgdummyinstallprogress.h\
	rgdummyinstallprogress.cc\
	rgzvtinstallprogress.h\
	rgzvtinstallprogress.cc\
	rginstallprogress.h\
	rginstallprogress.cc\
	rgcdscanner.h\
	rgcdscanner.cc\
	rgmainwindow.cc\
	rgmainwindow.h\
	rgsrcwindow.cc\
	rgsrcwindow.h\
	rgvendorswindow.cc\
	rgvendorswindow.h\
	rgsummarywindow.cc\
	rgsummarywindow.h\
	rgchangeswindow.cc\
	rgchangeswindow.h\
	rguserdialog.h\
	rguserdialog.cc\
	rgwindow.cc\
	rgwindow.h \
	synapticinterface.cc\
	synapticinterface.h

CLEANFILES= $(wildcard *_wrap.*) $(wildcard *~) synaptic.py synaptic.pyc _synaptic.so

# SWIG stuff  -- broken ATM
if COMPILE_SWIG

#noinst_PROGRAMS=synapticModule
#synapticModule_SOURCES=$(synaptic_SOURCES) synapticinterface.i

#synapticinterface_wrap.cxx: synapticinterface.i 
#	$(SWIG) -c++ -python synapticinterface.i

#synapticinterface_wrap.o: synapticinterface_wrap.cxx
#	$(CXX) -c synapticinterface_wrap.cxx $(PYTHON_INCLUDES) `pkg-config libglade-2.0 --cflags` -I../common

#synapticModule$(EXEEXT): synaptic synapticinterface_wrap.o
#	$(CXX) -shared -O2  -o _synaptic.so  synapticinterface_wrap.o $(synaptic_OBJECTS) $(synaptic_LDFLAGS)  $(synaptic_LDADD)
#	-ln -s _synaptic.so synapticModule

endif #SWIG
