#include <apt-pkg/init.h>
#include <apt-pkg/configuration.h>
#include <iostream>

#include "config.h"
#include "rpackagelister.h"
#include "rpackageview.h"
#include "rpackage.h"

using namespace std;

int main(int argc, char **argv)
{
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);
   
   RPackageLister *lister = new RPackageLister();
   lister->openCache();
   lister->setView(PACKAGE_VIEW_SEARCH);
   OpProgress progress;

   _config->Set("Debug::Synaptic::View","true");
   unsigned long now = clock();
   lister->searchView()->setSearch("synaptic",  RPatternPackageFilter::Description, "synaptic", progress);
   cerr << "searching: " << float(clock()-now)/CLOCKS_PER_SEC << endl;
}
