
#include "rpackagelistactor.h"
#include "rpackagelister.h"
#include "i18n.h"

#include <apt-pkg/error.h>
#include <apt-pkg/tagfile.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/configuration.h>
#include <algorithm>
#include <fnmatch.h>

void RPackageListActor::notifyPostFilteredChange()
{
   vector<RPackage *> removedList;
   vector<RPackage *> insertedList;

   removedList = _lastDisplayList;

   for (unsigned int i = 0; i < _lister->count(); i++) {
      RPackage *pkg = _lister->getElement(i);
      vector<RPackage *>::iterator I = removedList.find(pkg);
      if (I != removedList.end())
	 removedList.erase(it);
      else
         insertedList.push_back(pkg);
   }

   if (removedList.empty() == false)
      run(removedList, PKG_REMOVED);
   if (insertedList.empty() == false)
      run(insertedList, PKG_ADDED);
}

// vim:sts=3:sw=3
