#!/usr/bin/env python

import synaptic_common
import sys

class TextProgress(synaptic_common.SwigOpProgress):
    def UpdateStatus(self, p):
        print "\r%.2f          " %(p),
    def Done(self):
        print "\nDone"
    def Update(self):
        print "?",

# FIXME: wrap this somewhere
_error = synaptic_common._GetErrorObj()
synaptic_common.RInitSystem()

lister = synaptic_common.RPackageLister()
t = TextProgress()
lister.setProgressMeter(t)

if not lister.openCache(False, False):
    print "error opening cache file"
    _error.DumpErrors()
    sys.exit(1)

#pkg = lister.getPackage("synaptic")
#print pkg.name()
#print pkg.installedVersion()
#print pkg.section()

vector_pkgs = lister.getPackages()
print "Available packages: %s", vector_pkgs.size()

i=0
while i<vector_pkgs.size():
    pkg = vector_pkgs[i]
    print pkg.name()
    print pkg.installedVersion()
    print
    i += 1


