#!/bin/bash

# Remove all translations in the desktop file
cp ../data/synaptic.desktop ../data/synaptic.desktop.old
perl -pi -e 's/(^Name\[.*\].*\n)|(^Comment\[.*\].*\n)//' ../data/synaptic.desktop

# Search for translations and append them to the desktop file
for i in `ls *.po`
	do SPRACHE=`echo $i | perl -pi -e 's/.po$//'`; \
	COMMENT=`egrep -A 1 '^msgid "Install, remove and upgrade software packages"' $i|egrep '^msgstr'| perl -pi -e 's/^msgstr "(.*)"/\1/'`; echo "Comment[$SPRACHE]=$COMMENT" >> ../data/synaptic.desktop; \
	NAME=`egrep -A 1 '^msgid "Software"' $i|egrep '^msgstr'| perl -pi -e 's/^msgstr "(.*)"/\1/'`; echo "Name[$SPRACHE]=$NAME" >> ../data/synaptic.desktop
done
