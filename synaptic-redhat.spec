# $Id: synaptic.spec,v 1.19 2003/08/12 14:41:38 dude Exp $

%define desktop_vendor synaptic-team

Summary: Graphical package management program using apt.
Name: synaptic
Version: 0.55.4
Release: 1
License: GPL
Group: Applications/System
Source: http://savannah.nongnu.org/download/synaptic/synaptic.pkg/%{version}/%{name}-%{version}.tar.gz
URL: http://www.nongnu.org/synaptic/
BuildRoot: %{_tmppath}/%{name}-root
Requires: apt >= 0.5.4, usermode, gtk2, libglade2,scrollkeeper
Requires: libstdc++
BuildRequires: apt-devel, rpm-devel, gtk2-devel, libglade2-devel
BuildRequires: libstdc++-devel, desktop-file-utils, sed, xmlto
BuildRequires: scrollkeeper, intltool

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

%post
scrollkeeper-update
 
%postun
scrollkeeper-update

%prep
%setup -q

%build
%configure 
make %{?_smp_mflags}

%install
rm -fr %{buildroot}
make install DESTDIR=%{buildroot}
%find_lang %{name}

mkdir -p %{buildroot}%{_bindir}
ln -s %{_bindir}/consolehelper %{buildroot}%{_bindir}/synaptic

mkdir -p %{buildroot}%{_sysconfdir}/security/console.apps
cat << EOF > %{buildroot}%{_sysconfdir}/security/console.apps/synaptic
USER=root
PROGRAM=%{_sbindir}/synaptic
SESSION=true
FALLBACK=false
EOF

mkdir -p %{buildroot}%{_sysconfdir}/pam.d
cat << EOF > %{buildroot}%{_sysconfdir}/pam.d/synaptic
#%PAM-1.0
auth       sufficient   pam_rootok.so
auth       sufficient   pam_timestamp.so
auth       required     pam_stack.so service=system-auth
session    required     pam_permit.so
session    optional     pam_xauth.so
session    optional     pam_timestamp.so
account    required     pam_permit.so
EOF

# Remove the default menu entry and install our own
rm -f %{buildroot}%{_datadir}/gnome/apps/System/%{name}.desktop

mkdir -p %{buildroot}%{_datadir}/applications

sed -e "s/(^Exec=.*$\n)|(^Icon=.*$\n)|(^Categories=.*$\n)//" \
	data/%{name}.desktop \
	> %{buildroot}%{_datadir}/applications/%{name}.desktop

cat << EOF >> %{buildroot}%{_datadir}/applications/%{name}.desktop
Icon=%{_datadir}/%{name}/pixmaps/%{name}_48x48.png
Exec=%{_bindir}/%{name}
Categories=System;Application;SystemSetup;GTK;X-Red-Hat-Base;
EOF

%clean
rm -rf %{buildroot}

%files -f %{name}.lang
%defattr(-, root, root)
%doc AUTHORS ChangeLog COPYING NEWS README TODO
%{_sysconfdir}/pam.d/%{name}
%{_sysconfdir}/security/console.apps/%{name}
%exclude %{_sysconfdir}/X11/sysconfig/%{name}.desktop
%{_bindir}/%{name}
%{_sbindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/%{name}
%{_datadir}/gnome/help/synaptic
%{_datadir}/omf/synaptic/synaptic-C.omf
%{_mandir}/man8/%{name}.8*

%changelog
* Tue Dec 1 2003 Sebastian Heinlein <allerlei@renates-welt.de>
- Update to 0.47

* Tue Nov 17 2003 Sebastian Heinlein <allerlei@renates-welt.de>
- Update to 0.46

* Tue Aug 12 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.42.

* Sun Aug  3 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.40.
- Put back into "System tools" instead of "System settings".
- Added control center file and excluded X11/sysconfig one.

* Tue Apr 22 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.36.1.

* Wed Apr  9 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.35.1.

* Mon Mar 31 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.35.
- Rebuilt for Red Hat Linux 9.

* Tue Mar 11 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.32.
                                                                                
* Tue Jan 14 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.31.
                                                                                
* Thu Jan  2 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.30.
                                                                                
* Mon Dec  9 2002 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.28.1.
                                                                                
* Mon Oct 21 2002 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.25.
- New icon, thanks to Alan Cramer.

* Mon Sep 30 2002 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.24.1.

* Thu Sep 26 2002 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 0.24.
- Rebuilt for Red Hat Linux 8.0.
- Major spec file cleanup since the app now uses apt 0.5, gtk+ etc.
- Use the redhat-config-packages icon.
- Menu entry now uses the freedesktop approach.
- Use timestamp too in the pam file.

* Tue May  7 2002 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Removed the libPropList dependency.
- Changed pam entry and console.apps entry.

* Thu May  2 2002 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Rebuilt against Red Hat Linux 7.3.
- Added the %{?_smp_mflags} expansion.

* Fri Mar 22 2002 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Spec file cleanup for Red Hat Linux 7.2.

* Tue Nov 13 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.16-1cl
- nothing new, 0.16 is for apt 0.5 support

* Sun Jul  1 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.15-1cl
- auto-fix broken dependencies on Upgrade/Install package (closes: #3967)
- always create config dir in /root

* Sat Jun 30 2001 Osvaldo Santana Neto <osvaldo@conectiva.com>
+ synaptic-0.14-3cl
- added icon in desktop (Closes: #3955)

* Sat Jun 30 2001 Osvaldo Santana Neto <osvaldo@conectiva.com>
+ synaptic-0.14-2cl
- added icon tag in menu descriptor (Closes: #3955)

* Thu Jun 28 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.14-1cl
- fixed show summary dialog (closes: #4007)
- fixed broken texts (closes: #4006)
- updated pt_BR potfile

* Wed Jun 27 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.13-1cl
- fixed some stuff in filter editor
- added default task filter
- fixed crash when changing filter (closes: #3959)


* Tue Jun 26 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.12-1cl
- added consolehelper support
- added menu (closes: #1369)
- reassigned icons credits to KDE ppl
- added little note to config window (closes: #1282)

* Wed Jun 20 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.11-1cl
- changed pkg fetch error message (closes: #1306)
- compiled against new apt (closes: #3256)
- compiled against patched wmaker (closes: #3291, #3370, #3235)
- added new potfiles (closes: #1614, #3072)
- fixed locale setting


* Fri May 18 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.10-1cl
- fixed various glitches (closes: #3235)
- bug fixed by new apt (closes: #3068)

* Tue May 14 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.9-1cl
- no longer reset package selection state when download only option is set
  (closes: #1307)
- added tooltips
- replaced N/A -> "" in version field in package list (closes: #1277)
- fixed bug in error dialogs (closes: #1280)
- added about dlg close btn (closes: #1285)
- s/Scratch Filter/Search Filter/ (closes: #1283)
- recompiled (closes: #1559)
- recompiled against new wmaker (closes: #1309, #1428, #3031)
- fixed bug when listing too many packages 
- did some magick (closes: #2818)
- fixed filter button bug (closes: #1332)

* Sat Apr 28 2001 Arnaldo Carvalho de Melo <acme@conectiva.com>
+ synaptic-0.8-4cl
- minor spec changes for policy compliance
- BuildRequires libbz2-devel, not bzip2-devel

* Fri Mar 23 2001 Conectiva <dist@conectiva.com>
+ synaptic-0.8-2cl
- rebuilt with newer rpm

* Wed Feb 21 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.8-2cl
- recompiled (closes: #1559)

* Wed Feb 14 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.8-1cl
- first official release (closes: #1417)

* Wed Jan 24 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.7-1cl
- i18n
- pt_BR

* Wed Jan 24 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.6-1cl
- depends on apt cnc32

* Thu Jan 23 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.5-1cl
- renamed from raptor to Synaptic

* Mon Jan 22 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ raptor-0.4-1cl

* Tue Jan 18 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ raptor-0.3-1cl

* Mon Jan 15 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ raptor-0.2-1cl
- release version 0.2 (first)

