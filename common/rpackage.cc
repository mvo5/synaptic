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

#include "raptoptions.h"


static char descrBuffer[4096];

static char *parseDescription(string descr);


RPackage::RPackage(RPackageLister *lister, pkgDepCache *depcache,
		   pkgRecords *records, pkgCache::PkgIterator &pkg)
  : _lister(lister), _records(records), _depcache(depcache), 
    _newPackage(false), _pinnedPackage(false), _orphanedPackage(false),
     _notify(true)
{
    _package = new pkgCache::PkgIterator(pkg);
    
    pkgDepCache::StateCache &State = (*_depcache)[*_package];
    if (State.CandVersion != 0) {
	_candidateVer = strdup(State.CandVersion);
    }
}


RPackage::~RPackage()
{
    free((void*)_candidateVer);
    delete _package;
}


void RPackage::addVirtualPackage(pkgCache::PkgIterator dep)
{
  _virtualPackages.push_back(dep);
  _provides.push_back(dep.Name());
}

#ifdef HAVE_DEBTAGS
const char *RPackage::tags() 
{
    int handle = _lister->_hmaker->getHandle(this);
    OpSet<string> tags = _lister->_coll.getTagsetForItem(handle);
    static string s;
    for(OpSet<string>::const_iterator it = tags.begin(); it != tags.end();
	it++)
	if(it == tags.begin())
	    s = *it;
	else
	    s += ", " + *it;
    return s.c_str();
}
#endif


const char *RPackage::section()
{
    const char *s = _package->Section();
    if(s!=NULL)
	return s;
    else
	return _("unknown");
}


const char *RPackage::summary()
{
    pkgCache::VerIterator ver = _package->VersionList();
    
    if (!ver.end()) {
	pkgRecords::Parser &parser = _records->Lookup(ver.FileList());
	
	return parser.ShortDesc().c_str();
    }
    return "";
}


const char *RPackage::maintainer()
{
    pkgCache::VerIterator ver = _package->VersionList();
    
    if (!ver.end()) {
	pkgRecords::Parser &parser = _records->Lookup(ver.FileList());
	return parser.Maintainer().c_str();
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
    pkgDepCache::StateCache &State = (*_depcache)[*_package];
    if (State.CandidateVer == 0)
        return NULL;
    return State.CandidateVerIter(*_depcache).VerStr();
}

const char *RPackage::priority()
{
    pkgCache::VerIterator ver = _package->VersionList();
    
    if (ver != 0)
	return ver.PriorityType();
    else
	return NULL;
}



bool RPackage::isImportant()
{
    if ((*_package)->Flags & (pkgCache::Flag::Important|pkgCache::Flag::Essential))
	return true;

    return false;
}

#ifndef HAVE_RPM
const char *RPackage::installedFiles()
{
    static string filelist;
    string s;

    filelist.erase(filelist.begin(), filelist.end());

    string f = "/var/lib/dpkg/info/" + string(name()) + ".list";
    if(FileExists(f)) {
	ifstream in(f.c_str());
	if (!in != 0) 
	    return "";
	while (in.eof() == false)	    {
		getline(in, s);
		filelist += s+"\n";
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
    pkgCache::VerIterator ver = _package->VersionList();
    
    if (!ver.end()) {
	pkgRecords::Parser &parser = _records->Lookup(ver.FileList());

	return parseDescription(parser.LongDesc());
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
    pkgDepCache::StateCache &State = (*_depcache)[*_package];
    if (State.CandidateVer == 0)
        return -1;
    return State.CandidateVerIter(*_depcache)->InstalledSize;
}

long RPackage::availablePackageSize()
{
    pkgDepCache::StateCache &State = (*_depcache)[*_package];
    if (State.CandidateVer == 0)
        return -1;
    return State.CandidateVerIter(*_depcache)->Size;
}

int RPackage::getOtherStatus()
{
    //pkgCache::VerIterator ver = _package->CurrentVer();
    //pkgDepCache::StateCache &state = (*_depcache)[*_package];

    int status = 0;

    if(_newPackage) 
	status |= ONew;

    if(_pinnedPackage)
	status |= OPinned;
    
    if(_orphanedPackage)
	status |= OOrphaned;

    if((*_package)->CurrentState == pkgCache::State::ConfigFiles) {
      //cout << "ResidentalConfig found for: "<<name()<<endl;
      status |= OResidualConfig;
    }

    pkgDepCache::StateCache &State = (*_depcache)[*_package];
    if (State.CandidateVer == 0 ||
	!State.CandidateVerIter(*_depcache).Downloadable()) {
	//cout << "Not in archive found for: "<<name()<<endl;
	status |= ONotInstallable;
    }

    return status;
}


RPackage::PackageStatus RPackage::getStatus()
{
    pkgCache::VerIterator ver = _package->CurrentVer();
    pkgDepCache::StateCache &state = (*_depcache)[*_package];

    if (ver.end())
	return SNotInstalled;
    
    if (state.NowBroken())
	return SInstalledBroken;

    if (state.Upgradable()) {
	pkgCache::VerIterator cand = state.CandidateVerIter(*_depcache);
	
	if (!cand.end())
	    return SInstalledOutdated;
    }

    // special case for downgrades. we don't handle downgrade gracefully yet
    // instead pretend we return the status as if there is no downgrade
    if(state.Status < 0) {
	if(_depcache->VS().CmpVersion(_package->CurrentVer().VerStr(),
				      string(_candidateVer)) == 0)
	    return SInstalledUpdated;
	else 
	    return SInstalledOutdated;
    }
    if (!ver.end())
	return SInstalledUpdated;

    return SNotInstalled;
}



#if 0
RPackage::PackageStatus RPackage::getFutureStatus()
{
    PackageStatus status = getStatus();
    MarkedStatus mark = getMarkedStatus();
    
    switch (status) {
     case SNotInstalled:
	switch (mark) {
	 case MInstall:
	    if (_state.InstBroken()) 
		return SInstalledBroken;
	    else
		return SInstalledUpdated;
	 default:
	    return SNotInstalled;
	}
	break;
	
     case SInstalledBroken:
	if (_state.InstBroken()) 
	    return SInstalledBroken;

	switch (mark) {
	 case MInstall:
	 case MUpgrade:
	    return SInstalledUpdated;
	 case MDowngrade:
	    return SInstalledOutdated;
	 case MRemove:
	    return SNotInstalled;
	 default:
	    if (_state.Upgradable()) {
		pkgCache::VerIterator cand = _state.CandidateVerIter(*_depcache);
		
		if (!cand.end())
		    return SInstalledOutdated;
	    }
	    return SInstalledUpdated;
	}
	break;

     case SInstalledOutdated:
	switch (mark) {
	 case MInstall:
	    if (_state.InstBroken())
		return SInstalledBroken;
	    
	}
	break;
	
     case SInstalledUpdated:
	break;
    }
}
#endif

RPackage::MarkedStatus RPackage::getMarkedStatus()
{
    pkgDepCache::StateCache &state = (*_depcache)[*_package];

    if (state.NewInstall())
	return MInstall;
    
    if (state.Upgrade())
	return MUpgrade;
    
    if (state.Downgrade())
	return MDowngrade;
    
    if (state.Delete())
	return MRemove;
    
    if (state.Install())
	return MUpgrade;

    if (state.Keep())
	return MKeep;

    if (state.Held())
	return MHeld;
    
    cout << _("OH SHIT DUNNO WTF IS GOIN ON!") << endl;
    return MKeep;
}


bool RPackage::isWeakDep(pkgCache::DepIterator &dep)
{
    if (dep->Type != pkgCache::Dep::Suggests
	&& dep->Type != pkgCache::Dep::Recommends)
	return false;
    else
	return true;
}


bool RPackage::enumWDeps(const char *&type, const char *&what, bool &satisfied)
{
    pkgCache::VerIterator ver;
    pkgDepCache::StateCache &state = (*_depcache)[*_package];


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


bool RPackage::nextWDeps(const char *&type, const char *&what, bool &satisfied)
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

	if (((*_depcache)[_wdepStart] & pkgDepCache::DepGInstall) == pkgDepCache::DepGInstall)
	    satisfied = true;

	type = buffer;

	pkgCache::PkgIterator depPkg = _wdepStart.TargetPkg();
	what = depPkg.Name();

	break;
    }
    return true;    
}


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

bool RPackage::enumAvailDeps(const char *&type, const char *&what, 
			const char *&pkg, const char *&which, char *&summary,
			bool &satisfied)
{
    pkgCache::VerIterator ver;
    pkgDepCache::StateCache &state = (*_depcache)[*_package];

    //ver = _package->VersionList();
    ver = (*_depcache)[*_package].CandidateVerIter(*_depcache);

    if (ver.end())
	return false;
    
    _depI = ver.DependsList();
    // uninitialized but doesn't matter, they just have to be equal    
    _depStart = _depEnd;

    return nextDeps(type, what, pkg, which, summary, satisfied);
}


vector<RPackage*> RPackage::getInstalledDeps()
{
    vector<RPackage*> deps;
    pkgCache::VerIterator ver;

    ver = (*_depcache)[*_package].InstVerIter(*_depcache);
    
    if (ver.end())
	ver = _package->VersionList();

    if (ver.end())
	return deps;
    
    _depI = ver.DependsList();
    // uninitialized but doesn't matter, they just have to be equal    

    pkgCache::DepIterator  depIter = _depI;
    while(!depIter.end()) {
	pkgCache::PkgIterator depPkg = depIter.TargetPkg();
	string name = depPkg.Name();
	RPackage *pkg = _lister->getElement(name);
	deps.push_back(pkg);
	depIter++;
    }

    return deps;
}

bool RPackage::dependsOn(const char *pkgname)
{
    const char *depType, *depPkg, *depName, *depVer;
    char *summary; 
    bool ok;
    if(enumDeps(depType, depName, depPkg, depVer, summary, ok)) {
	do {
	    //cout << "got: " << depType << " " << depName << endl;
	    if(strcmp(pkgname, depName) == 0)
		return true;
	} while (nextDeps(depType, depName, depPkg, depVer, summary, ok));
    }
    return false;
}

/* mostly taken from apt-get.cc:ShowBroken() */
string RPackage::showWhyInstBroken()
{
    pkgCache::DepIterator depI;
    pkgCache::VerIterator Ver;
    bool First = true;
    ostringstream out;

    pkgDepCache::StateCache &State = (*_depcache)[*_package];
    Ver = State.CandidateVerIter(*_depcache);

    // check if there is actually something to install
    if(Ver == 0) {
	ioprintf(out,
		 _("\nPackage %s has no available version, but exists in the database.\n"
		   "This typically means that the package was mentioned in a dependency and "
		   "never uploaded, has been obsoleted or is not available with the contents "
		   "of sources.list\n"),_package->Name());
	return out.str();
    }

    for (pkgCache::DepIterator D = Ver.DependsList(); D.end() == false;) {
	// Compute a single dependency element (glob or)
	pkgCache::DepIterator Start;
	pkgCache::DepIterator End;
	D.GlobOr(Start,End);
	
	if (_depcache->IsImportantDep(End) == false)
            continue;
	
	if (((*_depcache)[End] & pkgDepCache::DepGInstall) == pkgDepCache::DepGInstall)
	    continue;
	
	bool FirstOr = true;
	while (1)
         {
            First = false;

            if (FirstOr == false)
		ioprintf(out,"\t");
            else
               ioprintf(out," %s: ",End.DepType());

            FirstOr = false;
            ioprintf(out, "%s", Start.TargetPkg().Name());

            // Show a quick summary of the version requirements
            if (Start.TargetVer() != 0)
		ioprintf(out," (%s %s)",Start.CompType(),Start.TargetVer());

            /* Show a summary of the target package if possible. In the case
               of virtual packages we show nothing */
            pkgCache::PkgIterator Targ = Start.TargetPkg();
            if (Targ->ProvidesList == 0) {
		ioprintf(out," ");
		pkgCache::VerIterator Ver = (*_depcache)[Targ].InstVerIter(*_depcache);
		if (Ver.end() == false)  {
		    ioprintf(out,_("but %s is to be installed"),Ver.VerStr());
		} else {
		    if ((*_depcache)[Targ].CandidateVerIter(*_depcache).end() == true) {
			if (Targ->ProvidesList == 0)
			    ioprintf(out, _("but it is not installable"));
			else
			    ioprintf(out,_("but it is a virtual package"));
		    }
		    else
			ioprintf(out,_("but it is not going to be installed"));
		}
            }
            if (Start != End)
		ioprintf(out, _(" or"));
            ioprintf(out,"\n");

            if (Start == End)
               break;
            Start++;
         }
    }
    return out.str();
}

bool RPackage::enumDeps(const char *&type, const char *&what, 
			const char *&pkg, const char *&which, char *&summary,
			bool &satisfied)
{
    pkgCache::VerIterator ver;
    pkgDepCache::StateCache &state = (*_depcache)[*_package];

    if (state.Keep() || state.Held()) {
	ver = (*_depcache)[*_package].InstVerIter(*_depcache);

	if (ver.end())
	    ver = (*_depcache)[*_package].CandidateVerIter(*_depcache);
    } else {
	//ver = _package->VersionList();
	ver = (*_depcache)[*_package].CandidateVerIter(*_depcache);
    }
    if (ver.end())
	return false;
    
    _depI = ver.DependsList();
    // uninitialized but doesn't matter, they just have to be equal    
    _depStart = _depEnd;

    return nextDeps(type, what, pkg, which, summary, satisfied);
}


bool RPackage::nextDeps(const char *&type, const char *&what,
			const char *&pkg, const char *&which, char *&summary,
			bool &satisfied)
{
    static char buffer[32]; // type
    static char buffer2[32]; // which
    static char buffer3[64]; // summary

    while (1) {
	if (_depStart == _depEnd) {
	    if (_depI.end())
		return false;

	    _depI.GlobOr(_depStart, _depEnd);

	    snprintf(buffer, sizeof(buffer), "%s", _depEnd.DepType());
	} else {
	    _depStart++;

	    snprintf(buffer, sizeof(buffer), "| %s", _depEnd.DepType());
	}

	if (isWeakDep(_depEnd))
	    continue;

	satisfied = false;
	if (((*_depcache)[_depStart] & pkgDepCache::DepGInstall) == pkgDepCache::DepGInstall)
	    satisfied = true;

	type = buffer;
	//cout << "type: " << buffer << endl;

	pkgCache::PkgIterator depPkg = _depStart.TargetPkg();
	what = depPkg.Name();

	//cout << "what (before): " << what << endl;

	if (depPkg->VersionList!=0)
	    pkg = what;
	else if (depPkg->ProvidesList!=0)	    
	    pkg = depPkg.ProvidesList().OwnerPkg().Name();
	else
	    pkg = 0;
	//cout << "what (after): " << what << endl;	

	if (_depStart.TargetVer()) {
	    snprintf(buffer2, sizeof(buffer2), "(%s %s)",
		     _depStart.CompType(), _depStart.TargetVer());
	    which = buffer2;	
	} else {
	    which = "";
	}
	//cout << "which: " << buffer2 << endl;

	buffer3[0] = 0;
	if (!satisfied && depPkg->ProvidesList == 0) {
	    pkgCache::VerIterator Ver = (*_depcache)[depPkg].InstVerIter(*_depcache);
	    
	    if (!Ver.end())
		snprintf(buffer3, sizeof(buffer3), _("%s is/will be installed"),
			 Ver.VerStr());
	    else {
		if ((*_depcache)[depPkg].CandidateVerIter(*_depcache).end()) {
		    if (depPkg->ProvidesList == 0)
			strcpy(buffer3, _("package is not installable"));
		    else
			strcpy(buffer3, _("package is a virtual package"));
		} else
		    strcpy(buffer3, _("package is/will not be installed"));
	    }
	} else if (satisfied) {
	    strcpy(buffer3, _("dependency is satisfied"));
	}
	summary = buffer3;	
	//cout << "summary: " << summary << endl;

	break;
    }
    return true;
}



RPackage::UpdateImportance RPackage::updateImportance()
{
    return IUnknown;//(*name()) == 'a' ? ISecurity : INormal;
}


const char *RPackage::updateSummary()
{
    return "Test\nAdvisory\nYour computer will blow if you don't update.";
    return NULL;
}


const char *RPackage::updateDate()
{
    return "2001-1-1 12:43 GMT";
    return NULL;
}


const char *RPackage::updateURL()
{
    return "http://sekure.org/~dumped/exploitz";
    return NULL;
}


bool RPackage::wouldBreak()
{    
    MarkedStatus mark = getMarkedStatus();
    pkgDepCache::StateCache &state = (*_depcache)[*_package];

    
    if (getStatus() == SNotInstalled) {
	if (mark == MKeep || mark == MHeld)
	    return false;
    } else {
	if (mark == MRemove)
	    return false;
    }
    return state.InstBroken();
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
}


void RPackage::setInstall()
{
    _depcache->MarkInstall(*_package, true);
    pkgDepCache::StateCache &State = (*_depcache)[*_package];

    // if there is something wrong, try to fix it
    if(!State.Install() || _depcache->BrokenCount() > 0) {
	pkgProblemResolver Fix(_depcache);
	Fix.Clear(*_package);
	Fix.Protect(*_package);
	Fix.Resolve(true);
    }

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
    
    _depcache->MarkDelete(*_package, purge);
    
    if (_notify)
	_lister->notifyChange(this);
}

void create_tmpfile(const char *pattern, char **filename, FILE **out)
{
  char *tmpdir = NULL;
  
  // remove from pin-file
  if (! ((tmpdir=getenv("TMPDIR")))) {        
    tmpdir=getenv("TMP");                     
  }                                         
  if (!tmpdir) {
    tmpdir = "/tmp";
  }
  *filename = (char*)malloc(strlen(tmpdir)+strlen(pattern)+2);  
  assert(filename);

  strcpy(*filename, tmpdir); 
  strcat(*filename, "/");   
  strcat(*filename, pattern);   
  int tmp_fd = mkstemp(*filename);
  *out = fdopen(tmp_fd, "w+b");
}


void RPackage::setPinned(bool flag)
{
    //cout << "void RPackage::setPinned(bool flag)" << endl;

    FILE *out;
    char pattern[] = "syn-XXXXXX";
    char *filename = NULL;
    struct stat stat_buf;

    string File = _config->FindFile("Dir::Etc::Preferences");
    _pinnedPackage = flag;
    
    if (flag) {
	// pkg already in pin-file
	if(_roptions->getPackageLock(name()))
	    return;

	// write to pin-file
	ofstream out(File.c_str(), ios::app);
	out << "Package: " << name() << endl;
	out << "Pin: version " << installedVersion() << endl;
	out << "Pin-Priority: " 
	    <<  _config->FindI("Synaptic::DefaultPinPriority", 1001) 
	    << endl << endl;
    } else {
	// delete package from pinning file
	stat(File.c_str(),&stat_buf);
	create_tmpfile(pattern, &filename, &out);
	if(out == NULL)
	    cerr << "error opening tmpfile: " << filename << endl;
	FileFd Fd(File,FileFd::ReadOnly);
	pkgTagFile TF(&Fd);
	if (_error->PendingError() == true)
	    return;
	pkgTagSection Tags;
	while (TF.Step(Tags) == true) {
	    string Name = Tags.FindS("Package");
	    if (Name.empty() == true) {
		_error->Error(_("Invalid record in the preferences file, no Package header"));
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
	rename(filename, File.c_str());
	chmod(File.c_str(), stat_buf.st_mode);
	free(filename);
    }
}


bool RPackage::isShallowDependency(RPackage *pkg)
{
  pkgCache::DepIterator rdepI;

  // check whether someone else depends on a virtual pkg of this
  for (int i = -1; i < (int)pkg->_virtualPackages.size(); i++) {
    if( i < 0 ) {
      rdepI = pkg->_package->RevDependsList();
    } else {
      pkgCache::PkgIterator it = pkg->_virtualPackages[i];
      rdepI = it.RevDependsList();
    }

    while (!rdepI.end()) {

      // check whether the dependant is installed
      if (rdepI.ParentPkg().CurrentVer().end()) { // not installed
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
}

// format: first version, second archives
vector<pair<string,string> > RPackage::getAvailableVersions()
{
    string VerTag;
    vector<pair<string,string> > versions;

    //cout << "vector<string> RPackage::getAvailableVersions()"<<endl;
    
    // get available Versions
    for(pkgCache::VerIterator Ver = _package->VersionList(); Ver.end() == false; Ver++) {
	//cout << "Ver: " << Ver.VerStr() << endl;
	// we always take the first available version 
	pkgCache::VerFileIterator VF = Ver.FileList();
	if(!VF.end()) {
	    pkgCache::PkgFileIterator File = VF.File();
	    //cout << "  Archive: " << File.Archive() << endl;
	    //cout << "  Site (origin): " << File.Site() << endl;
	    
	    if(File->Archive != 0)
		versions.push_back(pair<string,string>(Ver.VerStr(),File.Archive()));
	    else 
		versions.push_back(pair<string,string>(Ver.VerStr(),File.Site()));
	}
    }
    //cout << endl;
    return versions;
}


bool RPackage::setVersion(const char* VerTag)
{
    pkgVersionMatch Match(VerTag,pkgVersionMatch::Version);
    pkgCache::VerIterator Ver = Match.Find(*_package);
			
    //cout << "Ver is: " << VerTag << endl;
 
    if (Ver.end() == true) {
	return _error->Error(_("Version '%s' for '%s' was not found"),
				 VerTag,_package->Name());
    }

    //printf("Release: Selected version %s (%s) for %s\n",
    //	   Ver.VerStr(),Ver.RelStr().c_str(),_package->Name());
   
    _depcache->SetCandidateVersion(Ver);

    return true;
}


vector<const char*> RPackage::provides()
{
    pkgDepCache::StateCache &State = (*_depcache)[*_package];
    if (State.CandidateVer == 0)
        return _provides;

    for (pkgCache::PrvIterator Prv = State.CandidateVerIter(*_depcache).ProvidesList(); Prv.end() != true; Prv++) {
	    if(find(_provides.begin(),_provides.end(),Prv.Name()) == _provides.end()) 
		    _provides.push_back(Prv.Name());
	}

    return _provides;
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

	RPackage *depackage = _lister->getElement(depPkg);
	//cout << "testing(RPackage): " << depackage->name() << endl;
	
	if (!depackage)
	    continue;
	
	// skip important packages
	if (depackage->isImportant())
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
    int i;
    unsigned int nlpos = descr.find('\n');
    // delete first line
    if( nlpos != string::npos )
      descr.erase(0,nlpos+2); // del "\n " too
    
    while( nlpos < descr.length() ) {
      nlpos = descr.find('\n', nlpos);
      if( nlpos == string::npos )
	break;

      i = nlpos;
      // del char after '\n' (always " ")
      i++;
      descr.erase(i,1);

      // delete lines likes this: " .", makeing it a \n
      if(descr[i] == '.') {
	descr.erase(i,1);
	nlpos++;
	continue;
      }

      // skip ws
      while(descr[++i] == ' ');

//      // not a list, erase nl
//       if(!(descr[i] == '*' || descr[i] == '-' || descr[i] == 'o'))
// 	descr.erase(nlpos,1);

      nlpos++;
    }
    strcpy(descrBuffer, descr.c_str());
    return descrBuffer;
}
static char *rpmParser(string descr)
{
    unsigned int pos = descr.find('\n');
    // delete first line
    if( pos != string::npos )
      descr.erase(0,pos+2); // del "\n " too
    
    strcpy(descrBuffer, descr.c_str());
    return descrBuffer;
}

static char *stripWsParser(string descr)
{
    const char *end;
    const char *p;

    p = descr.c_str();
    end = p+descr.size(); // mvo: hackish, but works


    int state = 0;
    char *pp = (char*)descrBuffer;

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

    if (descr.size() > sizeof(descrBuffer))
	return "Description Too Long";

#ifdef HAVE_RPM
    int parser = _config->FindI("Synaptic::descriptionParser", NO_PARSER);
#else    
    int parser = _config->FindI("Synaptic::descriptionParser", DEB_PARSER);
#endif 
    switch(parser) {
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


// vim:sts=4:sw=4
