# how to build
swig -python -c++ synaptic_common.i 
g++ -c synaptic_common_wrap.cxx -I/usr/include/python2.4 -I/usr/include/apt-pkg/ -I../common -I../
g++ -shared synaptic_common_wrap.o -o _synaptic_common.so ../common/libsynaptic.a -lapt-pkg 
