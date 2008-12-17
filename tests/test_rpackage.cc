#include <apt-pkg/init.h>
#include <iostream>

#include "config.h"
#include "rpackagelister.h"
#include "rpackage.h"

using namespace std;

int main(int argc, char **argv)
{
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);
   
   RPackageLister *lister = new RPackageLister();
   lister->openCache();
   RPackage *pkg = lister->getPackage("eog");
   cerr << "pkg: " << pkg->name() << endl;
   vector<DepInformation> deps = pkg->enumDeps();
   for(unsigned int i=0;i<deps.size();i++) {
      cerr << "deps: " << deps[i].name << endl;
   }
   cerr << "size: " << deps.size() << endl;

}
