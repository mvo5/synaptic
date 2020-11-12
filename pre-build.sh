#!/bin/sh

DEBVER=$(dpkg-parsechangelog |sed -n -e '/^Version:/s/^Version: //p')
CONFVER=$(sed -n -e 's/AC_INIT(synaptic, \(.*\))/\1/p' configure.ac)
if [ "$DEBVER" != "$CONFVER" ]; then
	sed -i -e "s/$CONFVER/$DEBVER/" configure.ac
	echo "Changed version in configure.ac, please commit."
	exit 1
fi


