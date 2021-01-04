dnl Process this file with autoconf to produce a configure script.
AC_INIT(synaptic, 0.90.1)

AM_INIT_AUTOMAKE([subdir-objects -Wno-portability])
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_RANLIB

dnl generated with: cd po;for i in $(ls *.po); do echo -n "${i%%.po} "; done
dnl i18n
ALL_LINGUAS="af am ar ast az be@latin be bg bn bo br bs ca ca@valencia ckb crh csb cs cy da de_DE de dv el en_AU en_CA en_GB eo es et eu fa fil fi fo fr fur fy ga gl gu gv he hi hr ht hu hy ia id is it ja ka km kn ko ku ky la lo lt lv mk ml mr ms my nb nds ne nl_NL nl nn no oc pa pl ps pt_BR pt pt_PT qu ro ru sc si sk sl sq sr@Latn sr sv ta_LK ta te tet th tl tr tt ug uk ur uz vi xh zh_CN zh_HK zh_TW"
AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=synaptic
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[description])
AC_PROG_INTLTOOL([0.23])
synapticlocaledir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(synapticlocaledir)
     

dnl ask for wings
dnl AC_ARG_WITH(wings,
dnl 	[--with-wings   build with WINGs gui (obsolete)],
dnl	[AC_CHECK_PROG(WINGS, get-wings-flags, yes, no)
dnl	if test x"$WINGS" == xno; then
dnl	   AC_MSG_ERROR([WINGs is not installed])
dnl	fi
dnl	]
dnl )
dnl if test x"$WINGS" = xyes; then
dnl	BUILD_wings="wings"
dnl 	WINGS_LIBS=`get-wings-flags --ldflags`
dnl	WINGS_LIBS="$GUI_LIBS `get-wings-flags --libs`"
dnl	WINGS_HDRS=`get-wings-flags --cflags`
dnl	AC_SUBST(WINGS_LIBS)
dnl	AC_SUBST(WINGS_HDRS)
dnl fi
dnl AC_SUBST(BUILD_wings)

dnl we build for gtk3 by default now
pkg_modules="gtk+-3.0 >= 3.4.0, pango >= 1.0.0, glib-2.0"
vte_modules="vte-2.91"

PKG_CHECK_MODULES(GTK, [$pkg_modules])
BUILD_gtk="gtk"	

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(BUILD_gtk)

dnl if test x"$WINGS" = xno; then
if test x"$GTK" = xno; then
	AC_MSG_ERROR([ Gtk is not installed, you need to install it to get a GUI])
fi
dnl fi




dnl Check for rpm version 
dnl =====================
rpm_version="none"
AC_MSG_CHECKING(RPM version)
SAVE_LIBS="$LIBS"
SAVE_CPPFLAGS="$CPPFLAGS"

RPM_HDRS=""
LIBS="$SAVE_LIBS -lrpmio -lrpmdb -lpopt"
CPPFLAGS="$SAVE_CPPFLAGS -I/usr/include/rpm"
AC_CHECK_HEADER(rpm/rpmlib.h,
      [AC_CHECK_LIB(rpm,rpmdbInitIterator,
        [AC_DEFINE(HAVE_RPM, 1, [whether RPM is present]) 
         RPM_LIBS="-lrpm -lrpmio -lrpmdb -lpopt"
         SAVE_CPPFLAGS="$SAVE_CPPFLAGS -I/usr/include/rpm"
	 RPM_HDRS="-I/usr/include/rpm"
         rpm_version="4"])])

if test $rpm_version = "none"; then
LIBS="$SAVE_LIBS -lpopt"
CPPFLAGS="$SAVE_CPPFLAGS -I/usr/include/rpm"
AC_CHECK_HEADER(rpm/rpmlib.h,
      [AC_CHECK_LIB(rpm,rpmdbOpen,
        [AC_DEFINE(HAVE_RPM, 1, [wheter RPM is present]) 
         RPM_LIBS="-lrpm -lpopt"
         SAVE_CPPFLAGS="$SAVE_CPPFLAGS -I/usr/include/rpm"
	 RPM_HDRS="-I/usr/include/rpm"
         rpm_version="3"])])
fi

AC_SUBST(RPM_LIBS)
AC_SUBST(RPM_HDRS)
CPPFLAGS="$SAVE_CPPFLAGS"
LIBS="$SAVE_LIBS"
AC_MSG_RESULT("RPM version is $rpm_version")

AC_MSG_CHECKING(for --enable-scripts)
AC_ARG_ENABLE([scripts],
	      AC_HELP_STRING(--enable-scripts, enable the extension system),
	      [enable_scripts="$enableval"],[enable_scripts="no"])
if test "$enable_scripts" != "no"; then
  AC_MSG_RESULT(no)
  AC_DEFINE(WITH_LUA, 1, 
	    [Define if you want to enable the extension system.]) 
else
  AC_MSG_RESULT(yes)
fi
AM_CONDITIONAL(WITH_LUA, test "$enable_scripts" != "no")

DEB_HDRS=""
AC_SUBST(DEB_HDRS)
AC_CHECK_LIB(apt-inst, main, [DEB_LIBS=-lapt-inst], [DEB_LIBS=])
AC_SUBST(DEB_LIBS)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h libintl.h iconv.h)

# check apt configuration 
AC_LANG([C++])
# bail out if we don't have apts headers
AC_CHECK_HEADER(apt-pkg/configuration.h, [],
 AC_ERROR([You need the apt-pkg headers installed to compile synaptic.]))

# if metaindex is there we use a apt with authentication support
AC_CHECK_HEADER(apt-pkg/metaindex.h,
        AC_DEFINE(WITH_APT_AUTH, 1, [build with apt auth support] )
)
# check if we have apts new-style cdrom.h
AC_CHECK_HEADER(apt-pkg/cdrom.h, 
                AC_DEFINE(HAVE_APTPKG_CDROM, 1, 
		[whether apt-pkg/cdrom.h is present]) )

# check and use libept if available
PKG_CHECK_MODULES([LIBEPT], [libept >= 1.0],
                  [AC_DEFINE(WITH_EPT, 1, [build with libept])
                   AC_SUBST(LIBEPT_CFLAGS)
	           AC_SUBST(LIBEPT_LIBS)
                  ],
                  [AC_MSG_NOTICE([no libept found, building without])
                  ])

AC_LANG([C])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

dnl Checks for library functions.
AC_FUNC_STRCOLL
AC_CHECK_FUNCS(regcomp strdup iconv)

# use vte if available
# vte_module is set above in the gtk3 test
PKG_CHECK_MODULES(VTE, [$vte_modules],
	      [AC_DEFINE(HAVE_VTE, 1, [build with VTE as output terminal])
	       AC_DEFINE(HAVE_TERMINAL, 1, [build with a terminal widget]) 
	       AC_SUBST(VTE_CFLAGS)
	       AC_SUBST(VTE_LIBS)
              ], 
              [AC_MSG_NOTICE([no vte found, building without])
              ])

# use dpkg progress by default unless the user disables it
AC_ARG_WITH(dpkg-progress,
	[  --without-dpkg-progress build without support for dpkg progress bar],
	[],
        [with_dpkg_progress=yes])
# if dpkg-progress is used, we need vte, so check that here
# (FIXME: is this actually true? we could build without terminal and progress
#  bar only)
AS_IF([test "x$with_dpkg_progress" != xno],
        [PKG_CHECK_MODULES(VTE, [$vte_modules], 
                           [], 
                           [AC_MSG_FAILURE([dpkg-progress requires vte, you can build without vte when using --without-dpkg-progress])
                           ])
         AC_DEFINE(WITH_DPKG_STATUSFD, 1, [build with dpkg progress bar])
        ],
        [])

# launchpad integration
AC_ARG_WITH(launchpad-integration,
 	[  --with-launchpad-integration   build with launchpad-integration],
	[PKG_CHECK_MODULES(LP, ["launchpad-integration"])
	 AC_DEFINE(WITH_LAUNCHPAD_INTEGRATION, 1, [build with launchpad-integration])
         AC_SUBST(LP_CFLAGS)
         AC_SUBST(LP_LIBS)
	]
)

AC_ARG_WITH(pkg-hold, 
	[  --with-pkg-hold   build with experimental package "hold" feature],
        AC_DEFINE(SYNAPTIC_PKG_HOLD, 1, [build with package pin feature] )
)

AC_ARG_WITH(gtk-disable-deprecated,
        [  --with-gtk-disable-deprecated  build with -DGTK_DISABLE_DEPRECATED],
        [GTK_DISABLE_DEPRECATED="-DGTK_DISABLE_DEPRECATED"
         AC_SUBST(GTK_DISABLE_DEPRECATED)]
)

#use multiarch unless the user disables it
AC_ARG_WITH(apt-multiarch-support,
	[  --without-apt-multiarch-support build without support for apt multiarch],
        [], [with_apt_multiarch_support=yes])
AS_IF([test "x$with_apt_multiarch_support" != "xno"],
	[AC_DEFINE(WITH_APT_MULTIARCH_SUPPORT, 1, [build with multiarch])]
      )

AS_IF([dpkg-vendor --derives-from ubuntu],
	[AC_DEFINE(VENDOR_DERIVES_FROM_UBUNTU, 1, [vendor derives from ubuntu])]
      )
AC_OUTPUT([
tests/Makefile 
common/Makefile 
gtk/Makefile
gtk/gtkbuilder/Makefile
data/Makefile
man/Makefile
pixmaps/Makefile 
pixmaps/hicolor/Makefile 
pixmaps/hicolor/16x16/Makefile 
pixmaps/hicolor/24x24/Makefile 
po/Makefile.in
help/Makefile
help/C/Makefile
help/es/Makefile
help/hr/Makefile
help/sv/Makefile
doc/Makefile
doc/html/Makefile
doc/html/C/Makefile
doc/html/es/Makefile
doc/html/sv/Makefile
Makefile
],[sed -e "/POTFILES =/r po/POTFILES" po/Makefile.in > po/Makefile])


