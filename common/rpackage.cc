/* rpackage.cc - wrapper for accessing package information
 * 
 * Copyright (c) 2000-2003 Conectiva S/A 
 *               2002 Michael Vogt <mvo@debian.org>
 * 
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
 * 
 * Portions Taken from Gnome APT
 *   Copyright (C) 1998 Havoc Pennington <hp@pobox.com>
 * 
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

#include "rpackage.h"
#include "rpackagelister.h"
#include "pkg_acqfile.h"

#include "i18n.h"

#include <map>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <sstream>



#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/depcache.h>
#include <apt-pkg/srcrecords.h>
#include <apt-pkg/algorithms.h>
#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/tagfile.h>
#include <apt-pkg/policy.h>
#include <apt-pkg/sptr.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/cacheiterators.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/versionmatch.h>
#include <apt-pkg/version.h>
#include <apt-pkg/policy.h>

#ifdef WITH_LUA
#include <apt-pkg/luaiface.h>
#endif

#include "raptoptions.h"

static int descrBufferSize = 4096;
static char *descrBuffer = new char[descrBufferSize];

static char *parseDescription(string descr);


RPackage::RPackage(RPackageLister *lister, pkgDepCache *depcache,
                   pkgRecords *records, pkgCache::PkgIterator &pkg)
: _lister(lister), _records(records), _depcache(depcache),
  _notify(true), _boolFlags(0)
{
   _package = new pkgCache::PkgIterator(pkg);

   pkgDepCache::StateCache & State = (*_depcache)[*_package];
   if (State.CandVersion != NULL)
      _defaultCandVer = State.CandVersion;
}


RPackage::~RPackage()
{
   delete _package;
}

#if 0
void RPackage::addVirtualPackage(pkgCache::PkgIterator dep)
{
   _virtualPackages.push_back(dep);
   _provides.push_back(dep.Name());
}
#endif

const char *RPackage::section()
{
   const char *s = _package->Section();
   if (s != NULL)
      return s;
   else
      return _("Unknown");
}

const char *RPackage::srcPackage()
{
   static string _srcPkg;

   pkgCache::VerIterator ver = (*_depcache)[*_package].CandidateVerIter(*_depcache);
   pkgRecords::Parser &rec=_records->Lookup(ver.FileList());
   _srcPkg = rec.SourcePkg().empty()?name():rec.SourcePkg();
 
   return _srcPkg.c_str();
}


const char *RPackage::summary()
{
   static string _summary;

   pkgCache::VerIterator ver = (*_depcache)[*_package].CandidateVerIter(*_depcache);
   if (!ver.end()) {
      pkgCache::DescIterator Desc = ver.TranslatedDescription();
      pkgRecords::Parser & parser = _records->Lookup(Desc.FileList());
      _summary = parser.ShortDesc();
      return _summary.c_str();
   }
   return "";
}


const char *RPackage::maintainer()
{
   static string _maintainer;
   pkgCache::VerIterator ver = (*_depcache)[*_package].CandidateVerIter(*_depcache);
   if (!ver.end()) {
      pkgRecords::Parser & parser = _records->Lookup(ver.FileList());
      _maintainer = parser.Maintainer();
      return _maintainer.c_str();
   }
   return "";
}


const char *RPackage::vendor()
{
   return "dunno";
}

const char *RPackage::installedVersion()
{
   if ((*_package)->CurrentVer == 0)
      return NULL;
   return _package->CurrentVer().VerStr();
}

const char *RPackage::availableVersion()
{
   pkgDepCache::StateCache & State = (*_depcache)[*_package];
   if (State.CandidateVer == 0)
      return NULL;
   return State.CandidateVerIter(*_depcache).VerStr();
}

const char *RPackage::priority()
{
   pkgCache::VerIterator ver = (*_depcache)[*_package].CandidateVerIter(*_depcache);
   if (ver != 0)
      return ver.PriorityType();
   else
      return NULL;
}

#ifndef HAVE_RPM
const char *RPackage::installedFiles()
{
   static string filelist;
   string s;

   filelist.erase(filelist.begin(), filelist.end());

   string f = "/var/lib/dpkg/info/" + string(name()) + ".list";
   if (FileExists(f)) {
      ifstream in(f.c_str());
      if (!in != 0)
         return "";
      while (in.eof() == false) {
         getline(in, s);
         filelist += s + "\n";
      }

      in >> filelist;
      return filelist.c_str();
   }
   filelist = _("The list of installed files is only available for installed packages");

   return filelist.c_str();
}
#else
const char *RPackage::installedFiles()
{
   return "";
}
#endif


const char *RPackage::description()
{
   static string _description;
   pkgCache::VerIterator ver = (*_depcache)[*_package].CandidateVerIter(*_depcache);

   if (!ver.end()) {
      pkgRecords::Parser & parser = _records->Lookup(ver.FileList());
      _description = parseDescription(parser.LongDesc());
      return _description.c_str();
   } else {
      return "";
   }
}

long RPackage::installedSize()
{
   pkgCache::VerIterator ver = _package->CurrentVer();

   if (!ver.end())
      return ver->InstalledSize;
   else
      return -1;
}

long RPackage::availableInstalledSize()
{
   pkgDepCache::StateCache & State = (*_depcache)[*_package];
   if (State.CandidateVer == 0)
      return -1;
   return State.CandidateVerIter(*_depcache)->InstalledSize;
}

long RPackage::availablePackageSize()
{
   pkgDepCache::StateCache & State = (*_depcache)[*_package];
   if (State.CandidateVer == 0)
      return -1;
   return State.CandidateVerIter(*_depcache)->Size;
}

int RPackage::getFlags()
{
   int flags = 0;

   pkgDepCache::StateCache &state = (*_depcache)[*_package];
   pkgCache::VerIterator ver = _package->CurrentVer();

   if (state.Install())
      flags |= FInstall;

   if (state.iFlags & pkgDepCache::ReInstall) {
      flags |= FReInstall;
   } else if (state.NewInstall()) { // Order matters here.
      flags |= FNewInstall;
   } else if (state.Upgrade()) {
      flags |= FUpgrade;
   } else if (state.Downgrade()) {
      flags |= FDowngrade;
   } else if (state.Delete()) {
      flags |= FRemove;
      if (state.iFlags & pkgDepCache::Purge)
         flags |= FPurge;
   } else if (state.Keep()) {
      flags |= FKeep;
   }

   if (!ver.end()) {
      flags |= FInstalled;

      if (state.Upgradable() && state.CandidateVer != NULL) {
         flags |= FOutdated;
         if (state.Keep())
            flags |= FHeld;
      }

      if (state.Downgrade())
         flags |= FDowngrade;
   }

   if (state.NowBroken())
      flags |= FNowBroken;

   if (state.InstBroken())
      flags |= FInstBroken;

   if ((*_package)->Flags & (pkgCache::Flag::Important |
                             pkgCache::Flag::Essential))
      flags |= FImportant;

   if ((*_package)->CurrentState == pkgCache::State::ConfigFiles)
      flags |= FResidualConfig;

   if (state.CandidateVer == 0 ||
       !state.CandidateVerIter(*_depcache).Downloadable())
      flags |= FNotInstallable;

   return flags | _boolFlags;
}

const char* RPackage::name()
{ 
   const char *s = _package->Name(); 
   if (s == NULL)
      return "";
   return s;
};

#if 0
bool RPackage::isWeakDep(pkgCache::DepIterator &dep)
{
   if (dep->Type != pkgCache::Dep::Suggests
       && dep->Type != pkgCache::Dep::Recommends)
      return false;
   else
      return true;
}


bool RPackage::enumWDeps(const char *&type, const char *&what,
                         bool &satisfied)
{
   pkgCache::VerIterator ver;
   pkgDepCache::StateCache & state = (*_depcache)[*_package];


   if (state.Keep() || state.Held()) {
      ver = (*_depcache)[*_package].InstVerIter(*_depcache);

      if (ver.end())
         ver = _package->VersionList();
   } else {
      ver = _package->VersionList();
   }
   if (ver.end())
      return false;

   _wdepI = ver.DependsList();
   // uninitialized but doesn't matter, they just have to be equal
   _wdepStart = _wdepEnd;

   return nextWDeps(type, what, satisfied);
}


bool RPackage::nextWDeps(const char *&type, const char *&what,
                         bool &satisfied)
{
   static char buffer[32];

   while (1) {
      if (_wdepStart == _wdepEnd) {
         if (_wdepI.end())
            return false;

         _wdepI.GlobOr(_wdepStart, _wdepEnd);

         snprintf(buffer, sizeof(buffer), "%s", _wdepEnd.DepType());
      } else {
         _wdepStart++;

         snprintf(buffer, sizeof(buffer), "| %s", _wdepEnd.DepType());
      }

      satisfied = false;
      if (!isWeakDep(_wdepEnd))
         continue;

      if (((*_depcache)[_wdepStart] & pkgDepCache::DepGInstall) ==
          pkgDepCache::DepGInstall)
         satisfied = true;

      type = buffer;

      pkgCache::PkgIterator depPkg = _wdepStart.TargetPkg();
      what = depPkg.Name();

      break;
   }
   return true;
}
#endif

vector<DepInformation> RPackage::enumRDeps()
{
   vector<DepInformation> deps;
   DepInformation dep;
   pkgCache::VerIterator Cur;

   for(pkgCache::DepIterator D = _package->RevDependsList(); D.end() != true; D++) {
      // clear old values
      dep.isOr=dep.isVirtual=false;
      dep.name=dep.version=dep.versionComp=NULL;

      // check target and or-depends status
      pkgCache::PkgIterator Trg = D.TargetPkg();
      if ((D->CompareOp & pkgCache::Dep::Or) == pkgCache::Dep::Or) {
	 dep.version = _("or dependency");
	 dep.versionComp = "";
      }

      //FIXME: HACK ALARM, we need a "RDepends" type, so we use the last
      // one in pkg-dep
      dep.type = (pkgCache::Dep::DepType)(pkgCache::Dep::Obsoletes+1);
      dep.name = D.ParentPkg().Name();

      if(Trg->VersionList == 0)
	 dep.isVirtual=true;

      deps.push_back(dep);
   }
   return deps;
}

#if 0
bool RPackage::enumRDeps(const char *&dep, const char *&what)
{
   _rdepI = _package->RevDependsList();

   _vpackI = 0;

   return nextRDeps(dep, what);
}


bool RPackage::nextRDeps(const char *&dep, const char *&what)
{
   while (_rdepI.end()) {
      if ((unsigned)_vpackI == _virtualPackages.size())
         return false;

      _rdepI = _virtualPackages[_vpackI].RevDependsList();
      _vpackI++;
   }
   what = _rdepI.TargetPkg().Name();
   dep = _rdepI.ParentPkg().Name();

   _rdepI++;

   return true;
}
#endif

#if 0
bool RPackage::enumAvailDeps(const char *&type, const char *&what,
                             const char *&pkg, const char *&which,
                             char *&summary, bool &satisfied)
{
   pkgCache::VerIterator ver;
   //   pkgDepCache::StateCache & state = (*_depcache)[*_package];

   //ver = _package->VersionList();
   ver = (*_depcache)[*_package].CandidateVerIter(*_depcache);

   if (ver.end())
      return false;

   _depI = ver.DependsList();
   // uninitialized but doesn't matter, they just have to be equal    
   _depStart = _depEnd;

   return nextDeps(type, what, pkg, which, summary, satisfied);
}


vector<RPackage *> RPackage::getInstalledDeps()
{
   vector < RPackage *>deps;
   pkgCache::VerIterator ver;

   ver = (*_depcache)[*_package].InstVerIter(*_depcache);

   if (ver.end())
      ver = _package->VersionList();

   if (ver.end())
      return deps;

   _depI = ver.DependsList();
   // uninitialized but doesn't matter, they just have to be equal    

   pkgCache::DepIterator depIter = _depI;
   while (!depIter.end()) {
      pkgCache::PkgIterator depPkg = depIter.TargetPkg();
      string name = depPkg.Name();
      deps.push_back(_lister->getPackage(name));
      depIter++;
   }

   return deps;
}

#endif



/* Mostly taken from apt-get.cc:ShowBroken() */
string RPackage::showWhyInstBroken()
{
   pkgCache::DepIterator depI;
   pkgCache::VerIterator Ver;
   bool First = true;
   ostringstream out;

   pkgDepCache::StateCache & State = (*_depcache)[*_package];
   Ver = State.CandidateVerIter(*_depcache);

   // check if there is actually something to install
   if (Ver == 0) {
      ioprintf(out,
               _
               ("\nPackage %s has no available version, but exists in the database.\n"
                "This typically means that the package was mentioned in a dependency and "
                "never uploaded, has been obsoleted or is not available with the contents "
                "of sources.list\n"), _package->Name());
      return out.str();
   }

   for (pkgCache::DepIterator D = Ver.DependsList(); D.end() == false;) {
      // Compute a single dependency element (glob or)
      pkgCache::DepIterator Start;
      pkgCache::DepIterator End;
      D.GlobOr(Start, End);

      if (_depcache->IsImportantDep(End) == false)
         continue;

      if (((*_depcache)[End] & pkgDepCache::DepGInstall) ==
          pkgDepCache::DepGInstall)
         continue;

      bool FirstOr = true;
      while (1) {
         /* Show a summary of the target package if possible. In the case
            of virtual packages we show nothing */
         pkgCache::PkgIterator Targ = Start.TargetPkg();
         if (Targ->ProvidesList == 0) {
            ioprintf(out, " ");
            pkgCache::VerIterator Ver =
               (*_depcache)[Targ].InstVerIter(*_depcache);
	    // add minimal version information
	    string requiredVersion;
	    if(Start.TargetVer() != 0)
	       requiredVersion = "("+string(Start.CompType())+string(Start.TargetVer())+")";
            if (Ver.end() == false) {
               if (FirstOr == false)
		  // TRANSLATORS: dependency error message, example:
		  // "apt 0.5.4 but 0.5.3 is to be installed"
                  ioprintf(out, _("\t%s %s but %s is to be installed"),
                           Start.TargetPkg().Name(), requiredVersion.c_str(),
			   Ver.VerStr());
               else
		  // TRANSLATORS: dependency error message, example:
		  // "Depends: apt 0.5.4 but 0.5.3 is to be installed"
                  ioprintf(out, _(" %s: %s %s but %s is to be installed"),
                           End.DepType(), Start.TargetPkg().Name(),
			   requiredVersion.c_str(), Ver.VerStr());
            } else {
               if ((*_depcache)[Targ].CandidateVerIter(*_depcache).end() ==
                   true) {
                  if (Targ->ProvidesList == 0)
                     if (FirstOr == false)
			// TRANSLATORS: dependency error message, example:
			// "apt 0.5.4 but it is not installable"
                        ioprintf(out, _("\t%s %s but it is not installable"),
                                 Start.TargetPkg().Name(), 
				 requiredVersion.c_str());
                     else
			// TRANSLATORS: dependency error message, example:
			// "Depends: apt 0.5.4  but it is not installable",
                        ioprintf(out, "%s: %s %s but it is not installable",
                                 End.DepType(), Start.TargetPkg().Name(),
				 requiredVersion.c_str());
                  else if (FirstOr == false)
		     // TRANSLATORS: dependency error message, example:
		     // "apt but it is a virtual package"
                     ioprintf(out, _("\t%s but it is a virtual package"),
                              Start.TargetPkg().Name());
                  else
		     // TRANSLATORS: dependency error message, example:
		     // "Depends: apt but it is a virtual package"
                     ioprintf(out, _("%s: %s but it is a virtual package"),
                              End.DepType(), Start.TargetPkg().Name());
               } else if (FirstOr == false)
		  // TRANSLATORS: dependency error message, example:
		  // "apt but it is not going to be installed"
                  ioprintf(out, _("\t%s but it is not going to be installed"),
                           Start.TargetPkg().Name());
               else
		  // TRANSLATORS: dependency error message, example:
		  // "Depends: apt but it is not going to be installed"
                  ioprintf(out, _("%s: %s but it is not going to be installed"),
                           End.DepType(), Start.TargetPkg().Name());
            }
         } else {
            // virtual pkgs
            if (FirstOr == false)
               ioprintf(out, "\t%s", Start.TargetPkg().Name());
            else
               ioprintf(out, " %s: %s", End.DepType(),
                        Start.TargetPkg().Name());
            // Show a quick summary of the version requirements
            if (Start.TargetVer() != 0)
               ioprintf(out, " (%s %s)", Start.CompType(), Start.TargetVer());
         }

         First = false;
         FirstOr = false;

         if (Start != End)
            ioprintf(out, _(" or"));
         ioprintf(out, "\n");

         if (Start == End)
            break;
         Start++;
      }
   }
   return out.str();
}


vector<DepInformation> RPackage::enumDeps(bool useCanidateVersion)
{
   vector<DepInformation> deps;
   DepInformation dep;
   pkgCache::VerIterator Cur;

   if(!useCanidateVersion)
      Cur = (*_depcache)[*_package].InstVerIter(*_depcache);
   if(useCanidateVersion || Cur.end())
      Cur = (*_depcache)[*_package].CandidateVerIter(*_depcache);

   // no information found 
   if(Cur.end())
      return deps;

   for(pkgCache::DepIterator D = Cur.DependsList(); D.end() != true; D++) {

      // clear old values
      dep.isOr=dep.isVirtual=dep.isSatisfied=false;
      dep.name=dep.version=dep.versionComp=NULL;

      // check target and or-depends status
      pkgCache::PkgIterator Trg = D.TargetPkg();
      if ((D->CompareOp & pkgCache::Dep::Or) == pkgCache::Dep::Or)
	 dep.isOr=true;

      // common information
      dep.type = (pkgCache::Dep::DepType)D->Type;
      dep.name = Trg.Name();

      // satisfied
      if (((*_depcache)[D] & pkgDepCache::DepGInstall) ==
          pkgDepCache::DepGInstall)
         dep.isSatisfied = true;
      if (Trg->VersionList == 0) {
	 dep.isVirtual = true;
      } else {
	 dep.version=D.TargetVer();
	 dep.versionComp=D.CompType();
      }
      deps.push_back(dep);
   }

   return deps;
}

bool RPackage::dependsOn(const char *pkgname)
{
   vector<DepInformation> deps = enumDeps();
   for(unsigned int i=0;i<deps.size();i++)
      if(strcmp(pkgname, deps[i].name) == 0)
	 return true;
   return false;
}

#ifdef WITH_APT_AUTH
bool RPackage::isTrusted()
{
   pkgCache::VerIterator Ver;
   pkgDepCache::StateCache & State = (*_depcache)[*_package];
   Ver = State.CandidateVerIter(*_depcache);
   if (Ver == 0) {
      //cerr << "CanidateVer == 0" << endl;
      return false;
   }
   pkgSourceList *Sources=_lister->getCache()->list();
   for (pkgCache::VerFileIterator i = Ver.FileList(); i.end() == false; i++)
   {
      pkgIndexFile *Index;
      if (Sources->FindIndex(i.File(),Index) == false)
         continue;
      if (_config->FindB("Debug::pkgAcquire::Auth", false))
      {
         std::cerr << "Checking index: " << Index->Describe()
                   << "(Trusted=" << Index->IsTrusted() << ")\n";
      }
      if (Index->IsTrusted())
         return true;
   }
   
   return false;
}
#else
// with apt-authentication we always trust that the package come from
// a trusted source
bool RPackage::isTrusted() 
{ 
   return true; 
};
#endif

bool RPackage::wouldBreak()
{
   int flags = getFlags();
   if ((flags & FRemove) || (!(flags & FInstalled) && (flags & FKeep)))
      return false;
   return flags & FInstBroken;
}

void RPackage::setNotify(bool flag)
{
   _notify = flag;
}

void RPackage::setKeep()
{
   _depcache->MarkKeep(*_package, false);
   if (_notify)
      _lister->notifyChange(this);
   setReInstall(false);
}


void RPackage::setInstall()
{
   _depcache->MarkInstall(*_package, true);
   pkgDepCache::StateCache & State = (*_depcache)[*_package];

   // FIXME: can't we get rid of it here?
   // if there is something wrong, try to fix it
   if (!State.Install() || _depcache->BrokenCount() > 0) {
      pkgProblemResolver Fix(_depcache);
      Fix.Clear(*_package);
      Fix.Protect(*_package);
      Fix.Resolve(true);
   }


#ifdef WITH_LUA
   _lua->SetDepCache(_depcache);
   _lua->SetGlobal("package", ((pkgCache::Package *) * _package));
   _lua->RunScripts("Scripts::Synaptic::SetInstall", true);
   _lua->ResetGlobals();
   _lua->ResetCaches();
#endif

   if (_notify)
      _lister->notifyChange(this);
}

void RPackage::setReInstall(bool flag)
{
    _depcache->SetReInstall(*_package, flag);
    if (_notify)
	_lister->notifyChange(this);
}


void RPackage::setRemove(bool purge)
{
   pkgProblemResolver Fix(_depcache);

   Fix.Clear(*_package);
   Fix.Protect(*_package);
   Fix.Remove(*_package);

   Fix.InstallProtect();
   Fix.Resolve(true);

   _depcache->SetReInstall(*_package, false);
   _depcache->MarkDelete(*_package, purge);

   if (_notify)
      _lister->notifyChange(this);
}


string RPackage::getChangelogFile(pkgAcquire *fetcher)
{
   string prefix;
   string srcpkg = srcPackage();
   string descr("Changelog for ");
   descr+=name();

   string src_section=section();
   if(src_section.find('/')!=src_section.npos)
      src_section=string(src_section, 0, src_section.find('/'));
   else
      src_section="main";

   prefix+=srcpkg[0];
   if(srcpkg.size()>3 && srcpkg[0]=='l' && srcpkg[1]=='i' && srcpkg[2]=='b')
      prefix=std::string("lib")+srcpkg[3];

   string verstr;
   if(availableVersion() != NULL) 
      verstr = availableVersion();
   
   if(verstr.find(':')!=verstr.npos)
      verstr=string(verstr, verstr.find(':')+1);
   char uri[512];
   snprintf(uri,512,"http://packages.debian.org/changelogs/pool/%s/%s/%s/%s_%s/changelog",
                               src_section.c_str(),
                               prefix.c_str(),
                               srcpkg.c_str(),
                               srcpkg.c_str(),
                               verstr.c_str());

   //cout << "uri is: " << uri << endl;

   // no need to translate this, the changelog is in english anyway
   string filename = RTmpDir()+"/tmp_cl";
   ofstream out(filename.c_str());
   out << "Failed to fetch the changelog for " << name() << endl;
   out << "URI was: " << uri << endl;
   out.close();
   new pkgAcqFileSane(fetcher, uri, descr, name(), filename);

   fetcher->Run();

   return filename;
}

string RPackage::getCanidateOrigin()
{
   for(pkgCache::VerIterator Ver = _package->VersionList(); Ver.end() == false; Ver++) {
      // we always take the first available version 
      pkgCache::VerFileIterator VF = Ver.FileList();
      if(!VF.end()) {
	 return VF.File().Site();
      }
   }
   return "";
}


void RPackage::setPinned(bool flag)
{
   FILE *out;
   struct stat stat_buf;

   string File =RStateDir() + "/preferences";

   _boolFlags = flag ? (_boolFlags | FPinned) : (_boolFlags & FPinned);

   if (flag) {
      // pkg already in pin-file
      if (_roptions->getPackageLock(name()))
         return;

      // write to pin-file
      ofstream out(File.c_str(), ios::app);
      out << "Package: " << name() << endl;
      // if the package is not installed, we pin it to the available version
      // and prevent installation of this package this way
      if(installedVersion() != NULL) 
	 out << "Pin: version " << installedVersion() << endl;
      else
	 out << "Pin: version " << " 0.0 " << endl;
      out << "Pin-Priority: "
         << _config->FindI("Synaptic::DefaultPinPriority", 1001)
         << endl << endl;
   } else {
      // delete package from pinning file
      stat(File.c_str(), &stat_buf);
      // create a tmp_pin file in the internal dir
      string filename = RStateDir() + "/.tmp_preferences";
      FILE *out = fopen(filename.c_str(),"w");
      if (out == NULL)
         cerr << "error opening tmpfile: " << filename << endl;
      FileFd Fd(File, FileFd::ReadOnly);
      pkgTagFile TF(&Fd);
      if (_error->PendingError() == true)
         return;
      pkgTagSection Tags;
      while (TF.Step(Tags) == true) {
         string Name = Tags.FindS("Package");
         if (Name.empty() == true) {
            _error->
               Error(_
                     ("Invalid record in the preferences file, no Package header"));
            return;
         }
         if (Name != name()) {
            TFRewriteData tfrd;
            tfrd.Tag = 0;
            tfrd.Rewrite = 0;
            tfrd.NewTag = 0;
            TFRewrite(out, Tags, TFRewritePackageOrder, &tfrd);
            fprintf(out, "\n");
         }
      }
      fflush(out);
      rename(filename.c_str(), File.c_str());
      chmod(File.c_str(), stat_buf.st_mode);
      fclose(out);
   }
}


// FIXME: this function is broken right now (and it never really wasn't :/
bool RPackage::isShallowDependency(RPackage *pkg)
{
#if 0
   pkgCache::DepIterator rdepI;

   // check whether someone else depends on a virtual pkg of this
   for (int i = -1; i < (int)pkg->_virtualPackages.size(); i++) {
      if (i < 0) {
         rdepI = pkg->_package->RevDependsList();
      } else {
         pkgCache::PkgIterator it = pkg->_virtualPackages[i];
         rdepI = it.RevDependsList();
      }

      while (!rdepI.end()) {

         // check whether the dependant is installed
         if (rdepI.ParentPkg().CurrentVer().end()) {    // not installed
            // XXX check whether its marked for install
            rdepI++;
            continue;
         }
         // check whether the dependant isn't the own package
         if (rdepI.ParentPkg() == *_package) {
            rdepI++;
            continue;
         }
         // XXX should check for dependencies that depend on
         // dependencies of the same package

         return false;
      }
   }

   return true;
#endif
}

// format: first version, second archives
vector<pair<string, string> > RPackage::getAvailableVersions()
{
   string VerTag;
   vector<pair<string, string> > versions;

   // Get available Versions.
   for (pkgCache::VerIterator Ver = _package->VersionList();
        Ver.end() == false; Ver++) {

      // We always take the first available version.
      pkgCache::VerFileIterator VF = Ver.FileList();
      if (!VF.end()) {
         pkgCache::PkgFileIterator File = VF.File();

         if (File->Archive != 0)
            versions.push_back(pair < string,
                               string > (Ver.VerStr(), File.Archive()));
         else
            versions.push_back(pair < string,
                               string > (Ver.VerStr(), File.Site()));
      }
   }

   return versions;
}


bool RPackage::setVersion(string verTag)
{
   pkgVersionMatch Match(verTag, pkgVersionMatch::Version);
   pkgCache::VerIterator Ver = Match.Find(*_package);

   if (Ver.end() == true)
      return false;

   _depcache->SetCandidateVersion(Ver);

   _boolFlags |= FOverrideVersion;

   return true;
}
 
void RPackage::unsetVersion() 
{ 
   //cout << "set version to " << _defaultCandVer << endl;
   setVersion(_defaultCandVer); 
   _boolFlags &= ~FOverrideVersion;
}

vector<string> RPackage::provides()
{
   vector<string> provides;

   pkgDepCache::StateCache & State = (*_depcache)[*_package];
   if (State.CandidateVer == 0)
      return provides;

   for (pkgCache::PrvIterator Prv =
        State.CandidateVerIter(*_depcache).ProvidesList(); Prv.end() != true;
        Prv++) {
      provides.push_back(Prv.Name());
   }

   return provides;
}

/* mvo: actually shallow = false does not make a lot of sense as it
 *      will remove packages very brutally (like removing libc6)
 */
void RPackage::setRemoveWithDeps(bool shallow, bool purge)
{
   setRemove();

   // remove packages that this one depends on, including this
   pkgCache::DepIterator deps = _package->VersionList().DependsList();
   pkgCache::DepIterator start, end;

   deps.GlobOr(start, end);

   while (1) {
      if (start == end) {
         deps++;
         if (deps.end())
            break;
         deps.GlobOr(start, end);
      } else {
         start++;
      }

      if (!_depcache->IsImportantDep(start))
         continue;


      pkgCache::PkgIterator depPkg = start.TargetPkg();

      // get the real package, in case this is a virtual pkg
      // mvo: does this actually work :) ?
      if (depPkg->VersionList == 0) {
         if (depPkg->ProvidesList != 0) {
            depPkg = depPkg.ProvidesList().OwnerPkg();
         } else {
            continue;
         }
      }

      RPackage *depackage = _lister->getPackage(depPkg);
      //cout << "testing(RPackage): " << depackage->name() << endl;

      if (!depackage)
         continue;

      // skip important packages
      if (depackage->getFlags() & FImportant)
         continue;

      // skip dependencies that are dependants of other packages
      // if shallow=true
      if (shallow && !isShallowDependency(depackage)) {
         continue;
      }
      // set this package for removal
      depackage->setRemove(purge);
   }
}


// description parser stuff
static char *debParser(string descr)
{
   unsigned int i;
   string::size_type nlpos=0;

   nlpos = descr.find('\n');
   // delete first line
   if (nlpos != string::npos)
      descr.erase(0, nlpos + 2);        // del "\n " too

   while (nlpos < descr.length()) {
      nlpos = descr.find('\n', nlpos);
      if (nlpos == string::npos)
         break;

      i = nlpos;
      // del char after '\n' (always " ")
      i++;
      descr.erase(i, 1);

      // delete lines likes this: " .", makeing it a \n
      if (descr[i] == '.') {
         descr.erase(i, 1);
         nlpos++;
         continue;
      }
      // skip ws
      while (descr[++i] == ' ');

//      // not a list, erase nl
//       if(!(descr[i] == '*' || descr[i] == '-' || descr[i] == 'o'))
//      descr.erase(nlpos,1);

      nlpos++;
   }
   strcpy(descrBuffer, descr.c_str());
   return descrBuffer;
}
static char *rpmParser(string descr)
{
   string::size_type pos = descr.find('\n');
   // delete first line
   if (pos != string::npos)
      descr.erase(0, pos + 2);  // del "\n " too

   strcpy(descrBuffer, descr.c_str());
   return descrBuffer;
}

static char *stripWsParser(string descr)
{
   const char *end;
   const char *p;

   p = descr.c_str();
   end = p + descr.size();      // mvo: hackish, but works


   int state = 0;
   char *pp = (char *)descrBuffer;

   while (p != end) {
      switch (state) {
         case 0:
            if (*p == '\n')
               state = 1;
            else
               *pp++ = *p;
            break;

         case 1:
            if (*p == ' ')
               state = 2;
            else {
               *pp++ = *p;
               state = 0;
            }
            break;

         case 2:
            if (!(*p == '\n' || *p == '.')) {
               *pp++ = ' ';
               *pp++ = *p;
            }
            state = 0;
            break;
      }
      p++;
   }
   *pp = '\0';

   return descrBuffer;
}


static char *parseDescription(string descr)
{

   if (descr.size() + 1 > descrBufferSize) {
      delete[] descrBuffer;
      descrBufferSize = descr.size() + 1;
      descrBuffer = new char[descrBufferSize];
   }
#ifdef HAVE_RPM
   int parser = _config->FindI("Synaptic::descriptionParser", NO_PARSER);
#else
   int parser = _config->FindI("Synaptic::descriptionParser", DEB_PARSER);
#endif
   switch (parser) {
      case DEB_PARSER:
         return debParser(descr);
      case STRIP_WS_PARSER:
         return stripWsParser(descr);
      case RPM_PARSER:
         return rpmParser(descr);
      case NO_PARSER:
      default:
         strcpy(descrBuffer, descr.c_str());
         return descrBuffer;
   }
}

string RPackage::component()
{
   string res;
#ifdef WITH_APT_AUTH
   // the apt-secure patch breaks File.Component
   const char *s = _package->Section();
   if(s == NULL)
      return "";

   string src_section(s);
   if(src_section.find('/')!=src_section.npos)
      src_section=string(src_section, 0, src_section.find('/'));
   else
      src_section="main";
   res = src_section;
#else
   pkgCache::VerIterator Ver;
   pkgDepCache::StateCache & State = (*_depcache)[*_package];
   if (State.CandidateVer == 0) {
      //cout << "CanidateVer == 0" << endl;
      return "";
   }
   Ver = State.CandidateVerIter(*_depcache);
   pkgCache::VerFileIterator VF = Ver.FileList();
   pkgCache::PkgFileIterator File = VF.File();

   if(File.Component() == NULL) {
      //cout << "File.Component() == NULL" << endl;
      return "";
   }

   res = File.Component();
#endif
   return res;
}



string RPackage::label()
{
   string res;
   pkgCache::VerIterator Ver;
   pkgDepCache::StateCache & State = (*_depcache)[*_package];
   if (State.CandidateVer == 0) {
      //cout << "CanidateVer == 0" << endl;
      return "";
   }
   Ver = State.CandidateVerIter(*_depcache);
   pkgCache::VerFileIterator VF = Ver.FileList();
   pkgCache::PkgFileIterator File = VF.File();

   if(File.Label() == NULL) {
      //cout << "File.Component() == NULL" << endl;
      return "";
   }

   res = File.Label();

   return res;
}


// class that finds out what do display to get user
void RPackageStatus::init()
{
   char *status_short[N_STATUS_COUNT] = {
      "install", "reinstall", "upgrade", "downgrade", "remove",
      "purge", "available", "available-locked",
      "installed-updated", "installed-outdated", "installed-locked",
      "broken", "new"
   };
   memcpy(PackageStatusShortString, status_short, sizeof(status_short));

   char *status_long[N_STATUS_COUNT] = {
      _("Marked for installation"),
      _("Marked for re-installation"),
      _("Marked for upgrade"),
      _("Marked for downgrade"),
      _("Marked for removal"),
      _("Marked for complete removal"),
      _("Not installed"),
      _("Not installed (locked)"),
      _("Installed"),
      _("Installed (upgradable)"),
      _("Installed (locked to the current version)"),
      _("Broken"),
      _("Not installed (new in repository)")
   };
   memcpy(PackageStatusLongString, status_long, sizeof(status_long));


   // check for unsupported stuff
   if(_config->FindB("Synaptic::mark-unsupported",true)) {
      string s, labels, components;
      markUnsupported = true;

      // read supported labels
      labels = _config->Find("Synaptic::supported-label", "Debian Debian-Security");
      stringstream sst1(labels);
      while(!sst1.eof()) {
	 sst1 >> s;
	 supportedLabels.push_back(s);
      }
      
      // read supported components
      components = _config->Find("Synaptic::supported-components", "main updates/main");
      stringstream sst2(components);
      while(!sst2.eof()) {
	 sst2 >> s;
	 supportedComponents.push_back(s);
      }
   } 

}

bool RPackageStatus::isSupported(RPackage *pkg) 
{
   bool res = true;

   if(markUnsupported) {
      bool sc, sl;

      sc=sl=false;

      string component = pkg->component();
      string label = pkg->label();

      for(unsigned int i=0;i<supportedComponents.size();i++) {
	 if(supportedComponents[i] == component) {
	    sc = true;
	    break;
	 }
      }
      for(unsigned int i=0;i<supportedLabels.size();i++) {
	 if(supportedLabels[i] == label) {
	    sl = true;
	    break;
	 }
      }
      res = (sc & sl & pkg->isTrusted());
   }

   return res;
}

int RPackageStatus::getStatus(RPackage *pkg)
{
   int flags = pkg->getFlags();
   int ret = NotInstalled;

   if (pkg->wouldBreak()) {
      ret = IsBroken;
   } else if (flags & RPackage::FNewInstall) {
      ret = ToInstall;
   } else if (flags & RPackage::FUpgrade) {
      ret = ToUpgrade;
   } else if (flags & RPackage::FReInstall) {
      ret = ToReInstall;
   } else if (flags & RPackage::FDowngrade) {
      ret = ToDowngrade;
   } else if (flags & RPackage::FPurge) {
      ret = ToPurge;
   } else if (flags & RPackage::FRemove) {
      ret = ToRemove;
   } else if (flags & RPackage::FInstalled) {
      if (flags & RPackage::FPinned)
         ret = InstalledLocked;
      else if (flags & RPackage::FOutdated)
         ret = InstalledOutdated;
      else
         ret = InstalledUpdated;
   } else {
      if (flags & RPackage::FPinned)
         ret = NotInstalledLocked;
      else if (flags & RPackage::FNew)
         ret = IsNew;
      else
         ret = NotInstalled;
   }

   return ret;
}


// vim:ts=3:sw=3:et
