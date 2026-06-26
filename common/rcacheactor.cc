#include "config.h"  // IWYU pragma: associated

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
#include <string>
#include <utility>
#include <vector>

using namespace std;

void RCacheActor::notifyCachePostChange()
{
   //cout << "RCacheActor::notifyCachePostChange()" << endl;
   vector<RPackage *> toKeep;
   vector<RPackage *> toInstall;
   vector<RPackage *> toReInstall; 
   vector<RPackage *> toUpgrade;
   vector<RPackage *> toRemove;
   vector<RPackage *> toDowngrade;
   vector<RPackage *> notAuthenticated;
   vector<RPackage *> exclude;        // empty

   //cout << "_laststate: " << _laststate << endl;
   if (_laststate == NULL)
      return;

   if (_lister->getStateChanges(*_laststate, toKeep, toInstall, toReInstall,
                                toUpgrade, toRemove, toDowngrade,
				notAuthenticated,
                                exclude, false)) {
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

RCacheActorRecommends::RCacheActorRecommends(RPackageLister *lister,
                                             string fileName)
: RCacheActor(lister)
{
   FileFd f(fileName, FileFd::ReadOnly);
   if (_error->PendingError()) {
      _error->Error("Could not open recommends file %s", fileName.c_str());
      return;
   }
   
   pkgTagFile tags(&f);
   pkgTagSection section;

   while (tags.Step(section)) {
      const string name = section.FindS("Name");
      const string match = section.FindS("Match");
      const string recommends = section.FindS("Recommends");

      if (name.empty() || recommends.empty()) continue;

      ListType *list = NULL;
      if (match.empty() || match == "exact") {
         list = &_map[name];
      } else if (match == "wildcard") {
         list = &_map_wildcard[name];
      } else if (match == "regex") {
         regex_t *ptrn = new regex_t;
         if (regcomp(ptrn, name.c_str(),
                     REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
            _error->
               Warning("Bad regular expression '%s' in Recommends file.",
                       name.c_str());
            delete ptrn;
         } else
            list = &_map_regex[ptrn];
      }

      if (list != NULL) {
         const char *c = recommends.c_str();
         while (*c != 0) {
            string s;
            if (ParseQuoteWord(c, s))
               list->push_back(s);
         }
      }
   }
}

RCacheActorRecommends::~RCacheActorRecommends()
{
   for (RegexMapType::const_iterator RMapI = _map_regex.begin();
        RMapI != _map_regex.end(); RMapI++) {
      delete RMapI->first;
   }
}

void RCacheActorRecommends::setLanguageCache()
{
   string LangList = _config->Find("Volatile::Languages", "");
   if (LangList == _langLast)
      return;

   _langCache.clear();

   if (LangList.empty())
      return;

   _langLast = LangList;

   const char *C = LangList.c_str();
   string S;
   while (*C != 0) {
      if (ParseQuoteWord(C, S)) {
         string::size_type end;
         end = S.find('@');
         if (end != string::npos)
            S.erase(end, string::npos);
         _langCache.push_back(S);
         end = S.rfind('_');
         if (end != string::npos) {
            S.erase(end, string::npos);
            _langCache.push_back(S);
         }
      }
   }
}

void RCacheActorRecommends::notifyCachePostChange()
{
   if (_config->FindB("Synaptic::UseRecommends", true) == true)
      RCacheActor::notifyCachePostChange();
}

void RCacheActorRecommends::run(vector<RPackage *> &PkgList, int Action)
{
   setLanguageCache();
   const char *name;
   const ListType *List;
   MapType::const_iterator MapI;
   for (vector<RPackage *>::const_iterator PkgI = PkgList.begin();
        PkgI != PkgList.end(); PkgI++) {
      name = (*PkgI)->name();
      List = NULL;
      MapI = _map.find(name);
      if (MapI != _map.end()) {
         List = &MapI->second;
      } else {
         if (_map_wildcard.empty() == false) {
            for (MapI = _map_wildcard.begin();
                 MapI != _map_wildcard.end(); MapI++) {
               if (fnmatch(MapI->first.c_str(), name, 0) == 0) {
                  List = &MapI->second;
                  break;
               }
            }
         }
         if (List == NULL && _map_regex.empty() == false) {
            for (RegexMapType::const_iterator RMapI = _map_regex.begin();
                 RMapI != _map_regex.end(); RMapI++) {
               if (regexec(RMapI->first, name, 0, 0, 0) == 0) {
                  List = &RMapI->second;
                  break;
               }
            }
         }
      }
      if (List != NULL) {
         for (ListType::const_iterator ListI = List->begin();
              ListI != List->end(); ListI++) {
            const string &Recommends = *ListI;
            if (actOnPkg(Recommends, Action) == false
                && Recommends.find("$(LANG)") != string::npos) {
               for (vector < string >::const_iterator LI = _langCache.begin();
                    LI != _langCache.end(); LI++) {
                  string Parsed = SubstVar(Recommends, "$(LANG)", *LI);
                  actOnPkg(Parsed, Action);
               }
            }
         }
      }
   }
}

// vim:sts=3:sw=3
