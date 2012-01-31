#!/bin/sh

DEBVER=$(dpkg-parsechangelog |sed -n -e '/^Version:/s/^Version: //p')
CONFVER=$(sed -n -e 's/AM_INIT_AUTOMAKE(synaptic, \(.*\))/\1/p' configure.in)
if [ "$DEBVER" != "$CONFVER" ]; then
	sed -i -e "s/$CONFVER/$DEBVER/" configure.in
fi


