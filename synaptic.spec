Name: synaptic
Version: 0.20
Release: 1cl
Summary: WINGs based graphical front-end for APT
Summary(pt_BR): Front-end gráfico para APT baseado em WINGs
Summary(es): Front-end grafico para APT
Group: Administration
Group(pt_BR): Administração
Group(es): Administración
License: GPL
Source0: %{name}-%{version}.tar.gz
Source100: %{name}.menu
Source101: %{name}-16.png
Source102: %{name}-32.png
Requires: apt >= 0.3.19cnc32
Requires: usermode
BuildRequires: rpm-devel >= 3.0.5, libbz2-devel, zlib-devel
BuildRequires: WindowMaker-devel >= 0.65.0-2cl, libwraster-devel
BuildRequires: apt-devel >= 0.3.19cnc32
BuildRequires: gtk+-devel >= 1.2.0
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
Synaptic is a graphical front-end for APT (Advanced Package Tool) written
with the Window Maker toolkit (Gtk version also available).

Instead of using trees to display packages, Synaptic is heavily based on a
powerful package filtering system. That greatly simplifies the interface
while giving a lot more flexibility to browse through very long package
lists.

%description -l pt_BR
Synaptic é um front-end gráfico para o APT (Advanced Package Tool) escrito
com o toolkit do Window Maker (versão em Gtk também disponível).

Em vez de utilizar estruturas em árvore para mostrar os pacotes, Synaptic 
utiliza um sistema de filtro de pacotes, simplificando a interface e 
oferecendo mais flexibilidade quando houver um grande numero de pacotes
listado.

%prep
%setup -q

%build
%configure 
make 

%install
rm -fr %{buildroot}
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}%{_bindir}
ln -s %{_bindir}/consolehelper %{buildroot}%{_bindir}/synaptic
ln -s %{_bindir}/consolehelper %{buildroot}%{_bindir}/gsynaptic
mkdir -p %{buildroot}%{_sysconfdir}/security/console.apps
echo USER=root > %{buildroot}%{_sysconfdir}/security/console.apps/synaptic
echo USER=root > %{buildroot}%{_sysconfdir}/security/console.apps/gsynaptic
mkdir -p %{buildroot}%{_sysconfdir}/pam.d

# menu
mkdir -p %{buildroot}%{_menudir} \
         %{buildroot}%{_datadir}/icons/mini
install -m 644 %{_sourcedir}/%{name}.menu %{buildroot}%{_menudir}/%{name}
cp -f %{_sourcedir}/%{name}-16.png %{buildroot}%{_datadir}/icons/mini/%{name}.png
cp -f %{_sourcedir}/%{name}-32.png %{buildroot}%{_datadir}/icons/%{name}.png



cat << EOF > %{buildroot}%{_sysconfdir}/pam.d/synaptic
#%PAM-1.0
auth	   sufficient	/lib/security/pam_rootok.so
auth       required     /lib/security/pam_pwdb.so shadow nullok
account    required     /lib/security/pam_pwdb.so
EOF

cp %{buildroot}%{_sysconfdir}/pam.d/synaptic %{buildroot}%{_sysconfdir}/pam.d/gsynaptic

%post
# menu
%update_menus

%postun
# menu
%clean_menus


%clean
rm -rf %{buildroot}

%files
%defattr(0644,root,root,755)
%doc COPYING* README* TODO
%defattr(755,root,root)
%{_sbindir}/synaptic
%{_bindir}/synaptic

# menu
%defattr(0644,root,root,0755)
%{_sysconfdir}/security/console.apps/synaptic
%{_sysconfdir}/pam.d/synaptic
%{_datadir}/locale/*/LC_MESSAGES/%{name}.mo

%{_menudir}/%{name}
%{_datadir}/icons/%{name}.png
%{_datadir}/icons/mini/%{name}.png


%package gsynaptic
Summary: Gtk based graphical front-end for APT
Summary(pt_BR): Front-end gráfico para APT baseado em Gtk
Summary(es): Front-end grafico para APT en Gtk
Group: Administration
Group(pt_BR): Administração
Group(es): Administración
Requires: apt >= 0.3.19cnc32
Requires: usermode

%description
Synaptic is a graphical front-end for APT (Advanced Package Tool). This
version is written with the Gtk toolkit. 

Instead of using trees to display packages, Synaptic is heavily based on a
powerful package filtering system. That greatly simplifies the interface
while giving a lot more flexibility to browse through very long package
lists.

%description -l pt_BR
Synaptic é um front-end gráfico para o APT (Advanced Package Tool). Esta
versão usa o toolkit Gtk.

Em vez de utilizar estruturas em árvore para mostrar os pacotes, Synaptic 
utiliza um sistema de filtro de pacotes, simplificando a interface e 
oferecendo mais flexibilidade quando houver um grande numero de pacotes
listado.

%files
%defattr(755,root,root)
%{_sbindir}/gsynaptic
%{_bindir}/gsynaptic

# menu
%defattr(0644,root,root,0755)
%{_sysconfdir}/security/console.apps/gsynaptic
%{_sysconfdir}/pam.d/gsynaptic
%{_datadir}/locale/*/LC_MESSAGES/%{name}.mo

%{_menudir}/%{name}
%{_datadir}/icons/%{name}.png
%{_datadir}/icons/mini/%{name}.png




%changelog
* Mon Dec 10 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
+ synaptic-0.20-1cl
- added the Gtk version
- done some minor improvements

* Fri Aug 10 2001 Alfredo K. Kojima <kojima@conectiva.com.br>
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

