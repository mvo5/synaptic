#!/bin/sh

DEBVER=$(dpkg-parsechangelog |sed -n -e '/^Version:/s/^Version: //p')
CONFVER=$(grep -oP "version:\s*'\K[^']+" meson.build | head -1)
if [ "$DEBVER" != "$CONFVER" ]; then
	sed -i -e "s/$CONFVER/$DEBVER/" meson.build
	echo "Changed version in meson.build, please commit."
	exit 1
fi


