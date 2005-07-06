
#include "rcacheactor.h"
#include "rpackagelister.h"
#include "i18n.h"

#include <apt-pkg/error.h>
#include <apt-pkg/tagfile.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/configuration.h>
#include <algorithm>
#include <fnmatch.h>

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
                                             string FileName)
: RCacheActor(lister)
{
   FileFd F(FileName, FileFd::ReadOnly);
   if (_error->PendingError()) {
      _error->Error("could not open recommends file %s", FileName.c_str());
      return;
   }
   pkgTagFile Tags(&F);
   pkgTagSection Section;

   string Name;
   string Match;
   string Recommends;
   while (Tags.Step(Section)) {
      string Name = Section.FindS("Name");
      string Match = Section.FindS("Match");
      string Recommends = Section.FindS("Recommends");
      if (Name.empty() == true || Recommends.empty() == true)
         continue;

      ListType *List = NULL;
      if (Match.empty() == true || Match == "exact") {
         List = &_map[Name];
      } else if (Match == "wildcard") {
         List = &_map_wildcard[Name];
      } else if (Match == "regex") {
         regex_t *ptrn = new regex_t;
         if (regcomp(ptrn, Name.c_str(),
                     REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
            _error->
               Warning("Bad regular expression '%s' in Recommends file.",
                       Name.c_str());
            delete ptrn;
         } else
            List = &_map_regex[ptrn];
      }

      if (List != NULL) {
         const char *C = Recommends.c_str();
         string S;
         while (*C != 0) {
            if (ParseQuoteWord(C, S))
               List->push_back(S);
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
