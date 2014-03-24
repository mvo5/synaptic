/* rpackagefilter.cc - filters for package listing
 * 
 * Copyright (c) 2000-2003 Conectiva S/A 
 *               2002,2003 Michael Vogt <mvo@debian.org>
 * 
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "config.h"

#include <iostream>
#include <algorithm>
#include <cstdio>
#include <fnmatch.h>
#include <string.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/error.h>

#include "rpackagefilter.h"
#include "rpackagelister.h"
#include "rpackage.h"

#include "i18n.h"

using namespace std;

const char *RPatternPackageFilter::TypeName[] = {
   N_("Name"),
   N_("Description"),
   N_("Maintainer"),
   N_("Version"),
   N_("Depends"),
   N_("Provides"),
   N_("Conflicts"),
   N_("Replaces"),
   N_("Recommends"),
   N_("Suggests"),
   N_("ReverseDepends"),
   N_("Origin"),
   N_("Component"),
   NULL
};


const char *RPFStatus = _("Status");
const char *RPFPattern = _("Pattern");
const char *RPFSection = _("Section");
const char *RPFPriority = _("Priority");
const char *RPFReducedView = _("ReducedView");
const char *RPFFile = _("File");

int RSectionPackageFilter::count()
{
   return _groups.size();
}


bool RSectionPackageFilter::inclusive()
{
   return _inclusive;
}


string RSectionPackageFilter::section(int index)
{
   return _groups[index];
}


void RSectionPackageFilter::clear()
{
   //_groups.erase(_groups.begin(), _groups.end());
   _groups.clear();
}


bool RSectionPackageFilter::filter(RPackage *pkg)
{
   string sec;

   if (pkg->section() == NULL)
      return _inclusive ? false : true;

   sec = pkg->section();

   for (vector<string>::const_iterator iter = _groups.begin();
        iter != _groups.end(); iter++) {
      if (sec == (*iter)) {
         return _inclusive ? true : false;
      }
   }

   return _inclusive ? false : true;
}


bool RSectionPackageFilter::write(ofstream &out, string pad)
{
   out << pad + "inclusive " << (_inclusive ? "true;" : "false;") << endl;

   out << pad + "sections {" << endl;

   for (int i = 0; i < count(); i++) {
      out << pad + "  \"" << section(i) << "\"; " << endl;
   }

   out << pad + "};" << endl;
   return true;
}


bool RSectionPackageFilter::read(Configuration &conf, string key)
{
   const Configuration::Item *top;

   reset();

   _inclusive = conf.FindB(key + "::inclusive", _inclusive);

   top = conf.Tree(string(key + "::sections").c_str());
   if (top != NULL) {
      for (top = top->Child; top != NULL; top = top->Next)
	 addSection(top->Value);
   }

   return true;
}


bool RPatternPackageFilter::filterName(Pattern pat, RPackage *pkg)
{
   bool found=true;

   const char *name = pkg->name();
// if we want "real" nouseregexp support, we need to split the string
// here like we do with regexp
//              if(!useregexp) {
//                  if(strcasestr(name, iter->pattern.c_str()) == NULL) {
//                      found = false;
//                  } 
   for (unsigned int i = 0; i < pat.regexps.size(); i++) {
      if (regexec(pat.regexps[i], name, 0, NULL, 0) == 0)
	 found &= true;
      else
	 found = false;
   }
   return found;
}


bool RPatternPackageFilter::filterVersion(Pattern pat, RPackage *pkg)
{
   bool found = true;

   const char *version = pkg->availableVersion();
   if (version == NULL) {
      version = pkg->installedVersion();
   } 
   
   if(version != NULL) {
      for (unsigned int i = 0; i < pat.regexps.size(); i++) {
	 if (regexec(pat.regexps[i], version, 0, NULL, 0) == 0)
	    found &= true;
	 else
	    found = false;
      }
   } else {
      found = false;
   }
   return found;
}

bool RPatternPackageFilter::filterDescription(Pattern pat, RPackage *pkg)
{
   bool found=true;
   const char *s1 = pkg->summary();
   const char *s2 = pkg->description();
   for (unsigned int i = 0; i < pat.regexps.size(); i++) {
      if (regexec(pat.regexps[i], s1, 0, NULL, 0) == 0) {
	 found &= true;
      } else {
	 if (regexec(pat.regexps[i], s2, 0, NULL, 0) == 0)
	    found &= true;
	 else
	    found = false;
      }
   }
   return found;
}

bool RPatternPackageFilter::filterMaintainer(Pattern pat, RPackage *pkg)
{
   bool found=true;
   const char *maint = pkg->maintainer();
   for (unsigned int i = 0; i < pat.regexps.size(); i++) {
      if (regexec(pat.regexps[i], maint, 0, NULL, 0) == 0) {
	 found &= true;
      } else {
	 found = false;
      }
   }
   return found;
}

bool RPatternPackageFilter::filterDepends(Pattern pat, RPackage *pkg,
					  pkgCache::Dep::DepType filterType)
{
   vector<DepInformation> deps = pkg->enumDeps();

   if (pat.regexps.size() == 0) {
      return true;
   }
   
   for(unsigned int i=0;i<deps.size();i++) {
      if(deps[i].type == filterType) {
	    if (regexec(pat.regexps[0], deps[i].name, 0, NULL, 0) == 0) {
	       return true;
	    }
      }
   }
   return false;
}

bool RPatternPackageFilter::filterProvides(Pattern pat, RPackage *pkg)
{
   bool found = false;
   vector<string> provides = pkg->provides();

   if (pat.regexps.size() == 0) {
      return true;
   }
   
   for (unsigned int i = 0; i < provides.size(); i++) {
      if (regexec(pat.regexps[0], provides[i].c_str(), 0, NULL, 0) == 0) {
	 found = true;
	 break;
      }
   }
   return found;
}

#if 0
bool RPatternPackageFilter::filterWeakDepends(Pattern pat, RPackage *pkg)
{

   bool found = false;
   const char *depType, *depPkg, *depName;
   bool ok;
   if (pkg->enumWDeps(depType, depPkg, ok)) {
      do {
	 if (regexec(pat.regexps[0], depPkg, 0, NULL, 0) == 0) {
	    found = true;
	    break;
	 }
      } while (pkg->nextWDeps(depName, depPkg, ok));
   }
   return found;
}
#endif

bool RPatternPackageFilter::filterRDepends(Pattern pat, RPackage *pkg)
{
   vector<DepInformation> deps = pkg->enumRDeps();

   if (pat.regexps.size() == 0) {
      return true;
   }
   
   for(unsigned int i=0;i<deps.size();i++) {
      if (regexec(pat.regexps[0], deps[i].name, 0, NULL, 0) == 0) {
	 return true;
      }
   }
   return false;
}
bool RPatternPackageFilter::filterOrigin(Pattern pat, RPackage *pkg)
{
   bool found = false;
   vector<string>origins = pkg->getCandidateOriginSiteUrls();

   if (pat.regexps.size() == 0) {
      return true;
   }
   
   for (vector<string>::iterator it = origins.begin();
        it != origins.end();
        ++it)
   {
      if(regexec(pat.regexps[0],(*it).c_str(), 0, NULL, 0) == 0) {
         found = true;
      }
   }

   return found;
}

bool RPatternPackageFilter::filterComponent(Pattern pat, RPackage *pkg)
{
   bool found = false;
   string origin;
   origin = pkg->component();

   if (pat.regexps.size() == 0) {
      return true;
   }
   
   if(regexec(pat.regexps[0],origin.c_str(), 0, NULL, 0) == 0) {
      found = true;
   } 

   return found;
}

bool RPatternPackageFilter::filter(RPackage *pkg)
{
   bool found;
   //   bool and_mode = _config->FindB("Synaptic::Filters::andMode", true);
   bool globalfound = and_mode;
   bool useregexp = _config->FindB("Synaptic::UseRegexp", false);

   bool debug = _config->FindB("Debug::Synaptic::Filters", false);

   if (_patterns.size() == 0)
      return true;

   for (vector<Pattern>::const_iterator iter = _patterns.begin();
        iter != _patterns.end(); iter++) {
      
      Pattern pat = (*iter);
      switch(iter->where) {
      case Name:
	 found = filterName(pat, pkg);
	 break;
      case Description:
	 found = filterDescription(pat, pkg);
	 break;
      case Maintainer:
	 found = filterMaintainer(pat, pkg);
	 break;
      case Version:
	 found = filterVersion(pat,pkg);
	 break;
      case Depends:
	 found = filterDepends(pat, pkg, pkgCache::Dep::Depends);
	 break;
      case Conflicts:
	 found = filterDepends(pat, pkg, pkgCache::Dep::Conflicts);
	 break;
      case Replaces:
	 found = filterDepends(pat, pkg, pkgCache::Dep::Replaces);
	 break;
      case Recommends:
	 found =  filterDepends(pat, pkg, pkgCache::Dep::Recommends);
	 break;
      case Suggests:
	 found = filterDepends(pat, pkg, pkgCache::Dep::Suggests);
	 break;
      case Provides:
	 found = filterProvides(pat, pkg);
	 break;
      case RDepends:
	 found = filterRDepends(pat, pkg);
	 break;
      case Origin:
	 found = filterOrigin(pat, pkg);
	 break;
      case Component:
	 found = filterComponent(pat, pkg);
	 break;
      default:
	 cerr << "unknown pattern package filter (shouldn't happen) " << endl;
      }

      if (found && debug)
         clog << "RPatternPackageFilter::filter match for "
              << pkg->name() << endl;

      // each filter is applied in AND fasion
      // that means a include depends "mono" and include name "sharp"
      // results in all packages that depends on "mono" AND have sharp in name
      if (iter->exclusive) {
         found = !found;
      }

      if(and_mode)
	 globalfound &= found;
      else
	 globalfound |= found;
   }

   return globalfound;
}


void RPatternPackageFilter::addPattern(DepType type, string pattern,
                                       bool exclusive)
{
   //cout << "adding pattern: " << pattern << endl;
   Pattern pat;
   pat.where = type;
   pat.pattern = pattern;
   pat.exclusive = exclusive;

   // compile the regexps
   string S;
   const char *C = pattern.c_str();

   vector<regex_t *> regexps;
   while (*C != 0) {
      if (ParseQuoteWord(C, S) == true) {
         regex_t *reg = new regex_t;
         if (regcomp(reg, S.c_str(), REG_EXTENDED | REG_ICASE | REG_NOSUB) !=
             0) {
            cerr << "regexp compilation error" << endl;
            for (unsigned int i = 0; i < regexps.size(); i++) {
               regfree(regexps[i]);
            }
            return;
         }
         regexps.push_back(reg);
      }
   }
   pat.regexps = regexps;

   _patterns.push_back(pat);
}


bool RPatternPackageFilter::write(ofstream &out, string pad)
{
   DepType type;
   string pat;
   bool excl;

   out << pad + "andMode " << and_mode << ";" << endl;

   out << pad + "patterns {" << endl;

   for (int i = 0; i < count(); i++) {
      getPattern(i, type, pat, excl);
      out << pad + "  " + TypeName[(int)type] + ";"
         << " \"" << pat << "\"; " << (excl ? "true;" : "false;") << endl;
   }

   out << pad + "};" << endl;

   return true;
}


bool RPatternPackageFilter::read(Configuration &conf, string key)
{
   const Configuration::Item *top;
   DepType type;
   string pat;
   bool excl;

   and_mode = conf.FindB(key + "::andMode", true);

   top = conf.Tree(string(key + "::patterns").c_str());
   if (top == NULL)
      return true;

   top = top->Child;
   while (top) {
      int i;
      for (i = 0; TypeName[i] && top->Value != TypeName[i]; i++) 
	 /* nothing */
	 ;

      type = (DepType) i;
      top = top->Next;
      pat = top->Value;
      top = top->Next;
      excl = top->Value == "true";
      top = top->Next;

      if(TypeName[i] != NULL)
	 addPattern(type, pat, excl);
      
   }

   return true;
}

// copy constructor
RPatternPackageFilter::RPatternPackageFilter(RPatternPackageFilter &f)
{
   //cout << "RPatternPackageFilter(&RPatternPackageFilter f)" << endl;
   for (unsigned int i = 0; i < f._patterns.size(); i++) {
      addPattern(f._patterns[i].where,
                 f._patterns[i].pattern, f._patterns[i].exclusive);
   }
   and_mode = f.and_mode;
}

void RPatternPackageFilter::clear()
{
   // give back all the memory
   for (unsigned int i = 0; i < _patterns.size(); i++) {
      for (unsigned int j = 0; j < _patterns[i].regexps.size(); j++) {
         regfree(_patterns[i].regexps[j]);
         delete(regex_t *) _patterns[i].regexps[j];
      }
   }

   _patterns.erase(_patterns.begin(), _patterns.end());
}


RPatternPackageFilter::~RPatternPackageFilter()
{
   //cout << "RPatternPackageFilter::~RPatternPackageFilter()" << endl;

   this->clear();
}




bool RStatusPackageFilter::filter(RPackage *pkg)
{
   int flags = pkg->getFlags();

   if (_status & MarkKeep) {
      if (flags & RPackage::FKeep)
         return true;
   }

   if (_status & MarkInstall) {
      // this is a bit of a hack (to include reinstall here)
      // it would be better to seperate this 
      if ((flags & RPackage::FInstall) || (flags & RPackage::FReInstall))
         return true;
   }

   if (_status & MarkRemove) {
      if (flags & RPackage::FRemove)
         return true;
   }

   if (_status & Installed) {
      if (flags & RPackage::FInstalled)
         return true;
   }

   if (_status & NotInstalled) {
      if (!(flags & RPackage::FInstalled))
         return true;
   }

   if (_status & Broken) {
      if (flags & RPackage::FNowBroken || pkg->wouldBreak())
         return true;
   }

   if (_status & Upgradable) {
      if (flags & RPackage::FOutdated)
         return true;
   }

   if (_status & UpstreamUpgradable) {
      if (flags & RPackage::FOutdated) {
	 char *s;
	 char instVer[301];
	 char availVer[301];
	 strncpy(instVer, pkg->installedVersion(), 300);
	 strncpy(availVer, pkg->availableVersion(), 300);
	 
	 // strip from last "-" on
	 s = strrchr(instVer,'-');
	 if(s != NULL)
	    *s = '\0';
	 s = strrchr(availVer,'-');
	 if(s != NULL)
	    *s = '\0';

	 if(strcmp(instVer,availVer) != 0)
	    return true;
      }
   }

   if (_status & NewPackage) {
      if (flags & RPackage::FNew)
         return true;
   }

   if (_status & OrphanedPackage) {
      if (flags & RPackage::FOrphaned)
         return true;
   }

   if (_status & PinnedPackage) {
      if (flags & RPackage::FPinned)
         return true;
   }

   if (_status & ResidualConfig) {
      if (flags & RPackage::FResidualConfig) {
         return true;
      }
   }

   if (_status & NotInstallable) {
      if (flags & RPackage::FNotInstallable)
         return true;
   }

   if (_status & AutoInstalled) {
      if (flags & RPackage::FIsAuto)
         return true;
   }

   if (_status & Garbage) {
      if (flags & RPackage::FIsGarbage)
         return true;
   }

   if (_status & NowPolicyBroken) {
      if (!(flags & RPackage::FInstalled))
      {
	 pkgCache::DepIterator D;
	 bool inOr = false;
	 // FIXME: or-dependencies are not considered properly
	 for (D = pkg->package()->RevDependsList(); D.end() == false; D++)
	 {	    
	    if ((D->CompareOp & pkgCache::Dep::Or) == pkgCache::Dep::Or)
	       inOr = true;
	    else
	       inOr = false;
	    pkgCache::PkgIterator parent = D.ParentPkg();
	    if(parent->CurrentVer != 0)
	    {
	       RPackage *p = pkg->_lister->getPackage(parent);
	       if(p != NULL)
		  if(p->getFlags() & RPackage::FNowPolicyBroken)
		     return true;
	    }
	 }
      }
   }

   if (_status & ManualInstalled) {
      if ( !(flags & RPackage::FIsAuto) && 
            (flags & RPackage::FInstalled))
         return true;
   }

   return false;
}


bool RStatusPackageFilter::write(ofstream &out, string pad)
{
   char buf[16];

   snprintf(buf, sizeof(buf), "0x%x", _status);

   out << pad + "flags" + " " << buf << ";" << endl;
   return true;
}

bool RStatusPackageFilter::read(Configuration &conf, string key)
{
   _status = conf.FindI(key + "::flags", 0xffffff);
   return true;
}


bool RPriorityPackageFilter::filter(RPackage *pkg)
{
   return true;
}

// mvo: FIXME!
bool RPriorityPackageFilter::write(ofstream &out, string pad)
{
   return true;
}

// mvo: FIXME!
bool RPriorityPackageFilter::read(Configuration &conf, string key)
{
   return true;
}

RReducedViewPackageFilter::~RReducedViewPackageFilter()
{
   if (_hide_regex.empty() == false) {
      for (vector<regex_t *>::const_iterator I = _hide_regex.begin();
           I != _hide_regex.end(); I++) {
         delete *I;
      }
   }
}

// Filter out all packages for which there are ShowDependencies that
// are not installed.
bool RReducedViewPackageFilter::filter(RPackage *pkg)
{
   const char *name = pkg->name();
   if (_hide.empty() == false && _hide.find(name) != _hide.end())
      return false;
   if (_hide_wildcard.empty() == false) {
      for (vector<string>::const_iterator I = _hide_wildcard.begin();
           I != _hide_wildcard.end(); I++) {
         if (fnmatch(I->c_str(), name, 0) == 0)
            return false;
      }
   }
   if (_hide_regex.empty() == false) {
      for (vector<regex_t *>::const_iterator I = _hide_regex.begin();
           I != _hide_regex.end(); I++) {
         if (regexec(*I, name, 0, 0, 0) == 0)
            return false;
      }
   }
   return true;
}

void RReducedViewPackageFilter::addFile(string FileName)
{
   FileFd F(FileName, FileFd::ReadOnly);
   if (_error->PendingError()) {
      _error->Error("Internal Error: Could not open ReducedView file %s",
                    FileName.c_str());
      return;
   }
   pkgTagFile Tags(&F);
   pkgTagSection Section;

   string S;
   const char *C;
   while (Tags.Step(Section)) {
      string Name = Section.FindS("Name");
      string Match = Section.FindS("Match");
      string ReducedView = Section.FindS("ReducedView");
      if (Name.empty() == true || ReducedView.empty() == true)
         continue;

      C = ReducedView.c_str();
      if (ParseQuoteWord(C, S) && S == "hide") {
         if (Match.empty() == true || Match == "exact") {
            _hide.insert(Name);
         } else if (Match == "wildcard") {
            _hide_wildcard.push_back(Name);
         } else if (Match == "regex") {
            regex_t *ptrn = new regex_t;
            if (regcomp(ptrn, Name.c_str(),
                        REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
               _error->
                  Warning(_
                          ("Bad regular expression '%s' in ReducedView file."),
                          Name.c_str());
               delete ptrn;
            } else
               _hide_regex.push_back(ptrn);
         }
      }
   }
}

bool RReducedViewPackageFilter::write(ofstream &out, string pad)
{
   out << pad + "enabled " << (_enabled ? "true" : "false") << ";" << endl;
   return true;
}

bool RReducedViewPackageFilter::read(Configuration &conf, string key)
{
   _enabled = conf.FindB(key + "::enabled");
   if (_enabled == true) {
      string FileName = _config->Find("Synaptic::ReducedViewFile",
                                      "/etc/apt/metadata");
      if (FileExists(FileName))
         addFile(FileName);
   }
   return true;
}

bool RFilePackageFilter::addFile(string file)
{
  char str[255];
  filename = file;
  ifstream in(file.c_str());
  if(!in) 
     return false;
  while(in) {
     in.getline(str, 255);  
     pkgs.insert(pkgs.begin(), string(str));
  }
  in.close();
}

bool RFilePackageFilter::filter(RPackage *pkg)
{
   if (pkgs.size() == 0)
      return true;
   return pkgs.find(pkg->name()) != pkgs.end();
}

bool RFilePackageFilter::write(ofstream &out, string pad)
{
   out << pad + "file {" << endl;
   out << pad + "  \"" << filename << "\"; " << endl;
   out << pad + "};" << endl;
   return true;
}

bool RFilePackageFilter::read(Configuration &conf, string key)
{
   const Configuration::Item *top;

   reset();

   top = conf.Tree(string(key + "::file").c_str());
   if (top != NULL) {
      for (top = top->Child; top != NULL; top = top->Next)
	 filename = top->Value;
   }

   return true;
}

bool RFilter::apply(RPackage *package)
{
   if (!status.filter(package))
      return false;

   if (!pattern.filter(package))
      return false;

   if (!section.filter(package))
      return false;

   if (!priority.filter(package))
      return false;

   if (!reducedview.filter(package))
      return false;

   if (!file.filter(package))
      return false;

   return true;
}


void RFilter::reset()
{
   section.reset();
   status.reset();
   pattern.reset();
   priority.reset();
   reducedview.reset();
}

void RFilter::setName(string s)
{
   if (s.empty()) {
      cerr <<
         "Internal Error: empty filter name!? should _never_ happen, please report"
         << endl;
      name = "unknown";
   } else {
      if (s.length() > 55) {
         cerr << "Internal Error: filter name is longer than 55 chars!? "
            "Will be truncated.Please report" << endl;
         s.resize(55);
         name = s;
      } else {
         name = s;
      }
   }
}

string RFilter::getName()
{
   // Return name with i18n conversion. Filters names are saved without i18n.
   return _(name.c_str());
}

bool RFilter::read(Configuration &conf, string key)
{
   bool res = true;

   //cout << "reading filter "<< name << endl;

   res &= section.read(conf, key + "::section");
   res &= status.read(conf, key + "::status");
   res &= pattern.read(conf, key + "::pattern");
   res &= priority.read(conf, key + "::priority");
   res &= reducedview.read(conf, key + "::reducedview");

   return res;
}


bool RFilter::write(ofstream &out)
{
   bool res = true;

   //cout <<"writing filter: \""<<name<<"\""<<endl;

   if (name.empty()) {
      if (getenv("DEBUG_SYNAPTIC"))
         abort();

      name = "?";
   }

   out << "filter \"" << name << "\" {" << endl;
   string pad = "  ";

   out << pad + "section {" << endl;
   res &= section.write(out, pad + "  ");
   out << pad + "};" << endl;

   out << pad + "status {" << endl;
   res &= status.write(out, pad + "  ");
   out << pad + "};" << endl;

   out << pad + "pattern {" << endl;
   res &= pattern.write(out, pad + "  ");
   out << pad + "};" << endl;

   out << pad + "priority {" << endl;
   res &= priority.write(out, pad + "  ");
   out << pad + "};" << endl;

   out << pad + "reducedview {" << endl;
   res &= reducedview.write(out, pad + "  ");
   out << pad + "};" << endl;

   out << "};" << endl;

   return res;
}



// vim:sts=3:sw=3
