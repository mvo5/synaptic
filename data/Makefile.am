@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_POLICY_RULE@

desktopdir       = $(datadir)/applications
desktop_in_files = synaptic.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

kdeappdir       = $(datadir)/applications
kdeapp_in_files = synaptic-kde.desktop.in
kdeapp_DATA     = $(kdeapp_in_files:.desktop.in=.desktop)

polkit_policydir = $(datadir)/polkit-1/actions
dist_polkit_policy_in_files = com.ubuntu.pkexec.synaptic.policy.in
dist_polkit_policy_DATA = $(dist_polkit_policy_in_files:.policy.in=.policy)

EXTRA_DIST = $(desktop_in_files) \
		$(desktop_DATA) \
		$(kdeapp_in_files) \
		$(kdeapp_DATA) \
		$(dist_polkit_policy_in_files)
