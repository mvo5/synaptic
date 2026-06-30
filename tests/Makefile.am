
AM_CPPFLAGS = -I@top_srcdir@ -I@top_srcdir@/common -I@top_srcdir@/gtk -I@top_builddir@ \
	@SYNAPTIC_APP_PATHS@ \
	@GTK_DISABLE_DEPRECATED@ \
	@GTK_CFLAGS@ \
	@VTE_CFLAGS@ \
	@LP_CFLAGS@ \
	@APT_PKG_CFLAGS@ \
	@APT_PKG_CFLAGS@ \
	$(LIBTAGCOLL_CFLAGS) \
	-O0 -g3

noinst_PROGRAMS = test_rpackage test_rpackageview test_gtkpkglist test_rpackagefilter

LDADD = \
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

test_rpackage_SOURCES= test_rpackage.cc

test_rpackagefilter_SOURCES= test_rpackagefilter.cc

test_rpackageview_SOURCES= test_rpackageview.cc

test_gtkpkglist_SOURCES= test_gtkpkglist.cc \
	${top_srcdir}/gtk/rgpackagestatus.cc\
	${top_srcdir}/gtk/rgutils.cc\
	${top_srcdir}/gtk/rgpkgtreeview.cc\
	${top_srcdir}/gtk/gtkpkglist.cc

CLEANFILES= $(wildcard *_wrap.*) $(wildcard *~)
