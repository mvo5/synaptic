# File:           $RCSfile: synaptic.spec,v $
# Revision:       $Revision: 1.10 $
# Last change:    $Date: 2004/05/16 19:47:31 $

%define _datadir /opt/gnome/share

Summary:	Graphical package management program using apt
Name:		synaptic
Version:	0.53
Release:	0.1.suse91
License:	GPL
Group:		System/Packages
Packager:	Sebastian Heinlein <sebastian.heinlein@web.de>
Source:		synaptic-0.53.tar.gz
#Source:	http://savannah.nongnu.org/download/synaptic/synaptic.pkg/%{version}/%{name}-%{version}.tar.gz
Source1:	synaptic.desktop
URL:		http://www.nongnu.org/synaptic/
Requires:	apt >= 0.1.15, gtk2, libglade2
Requires:	libstdc++ libzvt2 libart_lgpl
Requires:	scrollkeeper
BuildRequires:	apt-devel >= 0.5.5 rpm-devel >= 4.1 gtk2-devel libglade2-devel
BuildRequires:	libstdc++-devel libxml2-devel libglade2-devel libart_lgpl-devel
BuildRequires:	atk-devel pango-devel glib2-devel pkgconfig libzvt-devel
BuildRequires:	XFree86-devel scrollkeeper
BuildRoot:	%{_tmppath}/making_of_%{name}-%{version}

%description
Synaptic is a graphical package management tool based on GTK+ and APT.
Synaptic enables you to install, upgrade and remove software packages in
a user friendly way.

Besides these basic functions the following features are provided:
 * Search and filter the list of available packages
 * Perform smart system upgrades
 * Fix broken package dependencies
 * Edit the list of used repositories (sources.list)
 * Download the latest changelog of a package
 * Configure packages through the debconf system
 * Browse all available documentation related to a package (dwww is required)

%prep
%setup -n %{name}-0.53

%build
install -m 644 %{SOURCE1} data/%{name}.desktop
PKG_CONFIG_PATH="/opt/gnome/lib/pkgconfig" \
CPPFLAGS="-I/usr/include/g++/i586-suse-linux" CFLAGS=-O2 ./configure	\
  --with-zvt 		\
  --prefix=%{_prefix}	\
  --mandir=%{_mandir}	\
  --datadir=%{_datadir}	\
  --libdir=%{_libdir}	\
  --includedir=%{_includedir} \
  --sysconfdir=%{_sysconfdir} \
  --enable-docdir=%{_defaultdocdir} 
make

%install

# Replace gksu by kdesu
sed -ie "s/^Exec=gksu -u root \/usr\/sbin\/synaptic/Exec=kdesu -u root -c synaptic/" data/%{name}.desktop

make DESTDIR=%{buildroot} install

# locales must be located in /usr/share otherwise synaptic does not pick them up
mv %{buildroot}%{_datadir}/locale %{buildroot}%{_prefix}/share/

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT

%post
scrollkeeper-update -q

%postun
scrollkeeper-update -q

%files
%defattr(-, root, root)
%doc AUTHORS ChangeLog COPYING NEWS README TODO
%{_sbindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/omf/%{name}
%{_datadir}/gnome/help/%{name}
%{_mandir}/man8/%{name}.8*
/usr/share/locale/*/LC_MESSAGES/%{name}.mo
/etc/X11/sysconfig/synaptic.desktop
/opt/gnome/share/applications/synaptic.desktop
/opt/gnome/share/pixmaps/synaptic.png


%changelog
* Thu Aug 26 2004 Sebastian Heinlein <sebastian.heinlein@web.de>
- Use sed to replace gksu by kdesu
- Do not install any kde menu items, since the SuSE menu already integrates
  the GNOME stuff
* Tue Nov 18 2003 Richard Bos <rbos@users.sourceforge.net>
- Use /opt/gnome as for datadir
  However move the locate directory to /usr/share/
* Sat Jun 21 2003 Richard Bos <rbos@users.sourceforge.net>
- Updated to release 0.38
- Added CFLAGS=-O2 to prevent debug code to be added to the end product
  (per default -g is added to gcc) 

* Wed Jun 18 2003 Richard Bos <rbos@users.sourceforge.net>
- Updated to release 0.37

* Tue Jun 15 2003 Richard Bos <rbos@users.sourceforge.net>
- Initial release, version 0.36.1

