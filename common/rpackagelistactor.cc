
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
//     cout << "RPackageListActor::notifyPostFilteredChange()" << endl;

    vector<RPackage*> removedList;
    vector<RPackage*> insertedList;

    // there is never a case (I know of) where packages are removed 
    // _and_ inserted on the same change, if there is
    if(_lastDisplayList.size() == _lister->count())
	return;

//     cout << "there are changes" << endl;
    // init 
    for(unsigned int i=0;i<_lastDisplayList.size();i++) {
	removedList.push_back(_lastDisplayList[i]);
    }

    for(unsigned int i=0;i<_lister->count();i++) {
	RPackage *pkg = _lister->getElement(i);
	// FIXME: it would be nice if this find could be optimized
	if(find(_lastDisplayList.begin(),_lastDisplayList.end(),pkg) != _lastDisplayList.end()) 
	    {
		// found, so not deleted
		vector<RPackage*>::iterator it;
		it = find(removedList.begin(),removedList.end(),pkg);
		if(it != removedList.end())
		    removedList.erase(it);
	    } else {
// // 		cout << "not found in _lastDisplayList" << endl;
		// not found, must be inserted
		insertedList.push_back(pkg);
	    }
    }
//     cout << "removed: " << removedList.size() << endl;
//     cout << "inserted: " << insertedList.size() << endl;
    if(removedList.empty() == false) {
	run(removedList, PKG_REMOVED);
    }
    if(insertedList.empty() == false)
	run(insertedList, PKG_ADDED);
}







// vim:sts=3:sw=3
