%module synaptic
%{
/* Includes the header in the wrapper code */
#include "synapticinterface.h"
%}
 
/* Parse the header file to generate wrappers */
%include "std_string.i"
%include "synapticinterface.h"
