@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_POLICY_RULE@
@INTLTOOL_XML_RULE@

desktopdir       = $(datadir)/applications
desktop_in_files = synaptic.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

polkit_policydir = $(datadir)/polkit-1/actions
dist_polkit_policy_in_files = com.ubuntu.pkexec.synaptic.policy.in
dist_polkit_policy_DATA = $(dist_polkit_policy_in_files:.policy.in=.policy)

metainfodir       = $(datadir)/metainfo
metainfo_in_files = io.github.mvo5.synaptic.metainfo.xml.in
metainfo_DATA     = $(metainfo_in_files:.xml.in=.xml)

EXTRA_DIST = $(desktop_in_files) \
		$(desktop_DATA) \
		$(dist_polkit_policy_in_files) \
		$(metainfo_in_files) \
		$(metainfo_DATA)
