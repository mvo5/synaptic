/* the big interface file */

%module(directors="1") synaptic_common
%feature("director"); 

%{
/* Put header files here (optional) */
#include<apt-pkg/error.h>
#include<apt-pkg/acquire.h>
#include<rswig.h>
#include<rpackage.h>
#include<rpackagelister.h>
#include<rpackagecache.h>
%}

%include "std_vector.i"
%include "std_string.i"

%include "/usr/include/apt-pkg/error.h"

%include "../common/rpackagelister.h"
%include "../common/rpackage.h"
%include "../common/rswig.h"



%template(RPackageVector) vector<RPackage *>;