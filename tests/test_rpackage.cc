#include "config.h"  // IWYU pragma: associated

#include "rpackage.h"
#include "rpackagelister.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/init.h>
#include <apt-pkg/pkgsystem.h>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char **argv)
{
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);
   
   RPackageLister *lister = new RPackageLister();
   lister->openCache();

   RPackage *pkg = lister->getPackage("build-essential");
   if (pkg) {
      cerr << "pkg: " << pkg->name() << endl;
      cerr << "Bugs: " << pkg->findTagFromPkgRecord("Bugs") << endl;

      vector<DepInformation> deps = pkg->enumDeps();
      for(unsigned int i=0;i<deps.size();i++) {
         cerr << "deps: " << deps[i].name << endl;
      }
      cerr << "size: " << deps.size() << endl;
   } else {
      cerr << "There is no package named build-essential" << endl;
   }

   // go over the cache with findTagFromPkgRecord()
   vector<RPackage *> all = lister->getPackages();
   cerr << "All: " << all.size() << endl;
   for(int i=0;i<all.size();i++) {
      all[i]->findTagFromPkgRecord("Bugs");
   }
   
   // go over the cache with findTagFromPkgRecord()
   string s;
   unsigned long now = clock();
   for(int i=0;i<all.size();i++) {
      s = all[i]->getRawRecord();
   }
   cerr << "iterating each record: " << float(clock()-now)/CLOCKS_PER_SEC << endl;
}
