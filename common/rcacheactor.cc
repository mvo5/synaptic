#include "config.h" // IWYU pragma: associated

#include "rcacheactor.h"

#include "rpackage.h"
#include "rpackagelister.h"
#include <apt-pkg/fileutl.h>

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/tagfile.h>
#include <cstddef>
#include <fnmatch.h>
#include <regex.h>
#include <vector>

using namespace std;

void RCacheActor::notifyCachePostChange()
{
   // cout << "RCacheActor::notifyCachePostChange()" << endl;
   vector<RPackage *> toKeep;
   vector<RPackage *> toInstall;
   vector<RPackage *> toReInstall;
   vector<RPackage *> toUpgrade;
   vector<RPackage *> toRemove;
   vector<RPackage *> toDowngrade;
   vector<RPackage *> notAuthenticated;
   vector<RPackage *> exclude; // empty

   // cout << "_laststate: " << _laststate << endl;
   if (_laststate == NULL)
      return;

   if (_lister->getStateChanges(*_laststate,
                                toKeep,
                                toInstall,
                                toReInstall,
                                toUpgrade,
                                toRemove,
                                toDowngrade,
                                notAuthenticated,
                                exclude,
                                false)) {
      if (toKeep.empty() == false)
         run(toKeep, ACTION_KEEP);
      if (toInstall.empty() == false)
         run(toInstall, ACTION_INSTALL);
      if (toReInstall.empty() == false)
         run(toReInstall, ACTION_REINSTALL);
      if (toUpgrade.empty() == false)
         run(toUpgrade, ACTION_INSTALL);
      if (toDowngrade.empty() == false)
         run(toDowngrade, ACTION_INSTALL);
      if (toRemove.empty() == false)
         run(toRemove, ACTION_REMOVE);
   }
}
