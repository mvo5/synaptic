# Makefile.in generated automatically by automake 1.4-p4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/xawtv_applet
pkglibdir = $(libdir)/xawtv_applet
pkgincludedir = $(includedir)/xawtv_applet

top_builddir = ..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = 
host_triplet = i686-pc-linux-gnu
BUILD_INCLUDED_LIBINTL = no
CATALOGS =  de.gmo fr.gmo
CATOBJEXT = .gmo
CC = gcc
DATADIRNAME = share
GENCAT = gencat
GLIBC21 = yes
GMOFILES =  de.gmo fr.gmo
GMSGFMT = /usr/bin/msgfmt
GNOMEUI_CFLAGS =  -I/usr/include/libgnome-2.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/libbonobo-2.0 -I/usr/include/gnome-vfs-2.0 -I/usr/lib/gnome-vfs-2.0/include -I/usr/include/bonobo-activation-2.0 -I/usr/include/libxml2 -I/usr/include/orbit-2.0 -I/usr/include/gconf/2 -I/usr/include/linc-1.0  
GNOMEUI_LIBS =  -lgnome-2 -lbonobo-2 -lgnomevfs-2 -lbonobo-activation -lxml2 -lz -lgconf-2 -lORBit-2 -lgthread-2.0 -lpthread -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0 -lm -llinc  
GNOME_CFLAGS = 
GNOME_LIBS = 
INSTOBJEXT = .mo
INTLBISON = bison
INTLLIBS = 
INTLOBJS = 
INTL_LIBTOOL_SUFFIX_PREFIX = 
LIBICONV = 
MAKEINFO = makeinfo
MKINSTALLDIRS = ./mkinstalldirs
MSGFMT = /usr/bin/msgfmt
PACKAGE = xawtv_applet
PKG_CONFIG = /usr/bin/pkg-config
POFILES =  de.po fr.po
POSUB = po
RANLIB = ranlib
USE_INCLUDED_LIBINTL = no
USE_NLS = yes
VERSION = 0.9.14

MACROS =    aclocal-include.m4				  compiler-flags.m4				  curses.m4					  gnome-bonobo-check.m4				  gnome-fileutils.m4				  gnome-ghttp-check.m4				  gnome-gnorba-check.m4				  gnome-guile-checks.m4				  gnome-libgtop-check.m4			  gnome-objc-checks.m4				  gnome-orbit-check.m4				  gnome-print-check.m4				  gnome-pthread-check.m4			  gnome-support.m4				  gnome-undelfs.m4				  gnome-vfs.m4					  gnome-x-checks.m4				  gnome-xml-check.m4				  gnome.m4					  gperf-check.m4				  linger.m4					  need-declaration.m4


EXTRA_DIST = $(MACROS) gnome-common.m4 autogen.sh
MAINTAINERCLEANFILES = macros.dep

@INSIDE_GNOME_COMMON_TRUE@gnome_aclocaldir = $(datadir)/aclocal/gnome-macros

@INSIDE_GNOME_COMMON_TRUE@gnome_aclocal_DATA = $(MACROS) gnome-macros.dep gnome-common.m4 	autogen.sh
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_CLEAN_FILES = 
DATA =  $(gnome_aclocal_DATA)

DIST_COMMON =  README Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP_ENV = --best
all: all-redirect
.SUFFIXES:
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu macros/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


install-gnome_aclocalDATA: $(gnome_aclocal_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(gnome_aclocaldir)
	@list='$(gnome_aclocal_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(gnome_aclocaldir)/$$p"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(gnome_aclocaldir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(gnome_aclocaldir)/$$p"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(gnome_aclocaldir)/$$p; \
	  fi; fi; \
	done

uninstall-gnome_aclocalDATA:
	@$(NORMAL_UNINSTALL)
	list='$(gnome_aclocal_DATA)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(gnome_aclocaldir)/$$p; \
	done
tags: TAGS
TAGS:


distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = macros

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --gnu macros/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$d/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am:
install-exec: install-exec-am

install-data-am: install-gnome_aclocalDATA
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am: uninstall-gnome_aclocalDATA
uninstall: uninstall-am
all-am: Makefile $(DATA)
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(gnome_aclocaldir)


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
	-test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
mostlyclean-am:  mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-generic mostlyclean-am

clean: clean-am

distclean-am:  distclean-generic clean-am

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: uninstall-gnome_aclocalDATA install-gnome_aclocalDATA tags \
distdir info-am info dvi-am dvi check check-am installcheck-am \
installcheck install-exec-am install-exec install-data-am install-data \
install-am install uninstall-am uninstall all-redirect all-am all \
installdirs mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


macros.dep: Makefile.am
	@echo '$$(top_srcdir)/aclocal.m4: $(MACROS:%=macros/%)' > $@

@INSIDE_GNOME_COMMON_TRUE@gnome-macros.dep: Makefile.am
@INSIDE_GNOME_COMMON_TRUE@	@echo '$$(top_srcdir)/aclocal.m4: $(MACROS:%=$(gnome_aclocaldir)/%)' > $@

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
