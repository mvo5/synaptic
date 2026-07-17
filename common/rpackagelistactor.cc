#include "config.h" // IWYU pragma: associated

#include "rpackagelistactor.h"

#include "rpackagelister.h"

#include <algorithm>
#include <vector>

using namespace std;

void RPackageListActor::notifyPostFilteredChange()
{
   vector<RPackage *> removedList;
   vector<RPackage *> insertedList;

   const vector<RPackage *> &currentList = _lister->getViewPackages();

   removedList = _lastDisplayList;

   vector<RPackage *>::iterator I;
   for (unsigned int i = 0; i < currentList.size(); i++) {
      I = find(removedList.begin(), removedList.end(), currentList[i]);
      if (I != removedList.end())
         removedList.erase(I);
      else
         insertedList.push_back(currentList[i]);
   }

   if (removedList.empty() == false)
      run(removedList, PKG_REMOVED);
   if (insertedList.empty() == false)
      run(insertedList, PKG_ADDED);
}

// vim:ts=3:sw=3:et
