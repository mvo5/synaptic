@INTLTOOL_DESKTOP_RULE@

desktopdir       = $(datadir)/control-center-2.0/capplets/
desktop_in_files = synaptic.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

systemdir       = $(sysconfdir)/X11/sysconfig
system_in_files = synaptic.desktop.in
system_DATA     = $(system_in_files:.desktop.in=.desktop)

EXTRA_DIST = $(desktop_in_files) \
		$(desktop_DATA) \
		$(system_in_files)
