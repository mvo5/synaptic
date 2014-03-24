#include <apt-pkg/init.h>
#include <apt-pkg/error.h>
#include <iostream>
#include <cstdlib>

#include "config.h"
#include "rpackagelister.h"
#include "rpackage.h"

using namespace std;

int main(int argc, char **argv)
{
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);
   if (!RInitConfiguration("synaptic.conf")) {
      _error->DumpErrors();
      std::exit(1);
   }

   RFilter *filter, *filter_copy;
   vector<RPackage *> pkgs;
   RPackageViewFilter filter_view(pkgs);
   for(int i=0; i < filter_view.nrOfFilters(); i++) {
      filter = filter_view.findFilter(i);
      std::cerr << "orig: " << filter->getName()
                << " and-mode: " << filter->pattern.getAndMode()
                << std::endl;
      filter_copy = new RFilter(*filter);
      std::cerr << "copy: " << filter_copy->getName()
                << " and-mode: " << filter_copy->pattern.getAndMode()
                << std::endl;
      std::cerr << std::endl;

      delete filter_copy;
      filter = filter_view.findFilter(i);
      std::cerr << "orig: " << filter->getName()
                << " and-mode: " << filter->pattern.getAndMode()
                << std::endl;
   }
      
}
