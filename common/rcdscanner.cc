/* rcdscanner.cc
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
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


#include <sys/stat.h>
#include <sys/fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdio>

#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/cdromutl.h>
#include <apt-pkg/strutl.h>

#include "i18n.h"
#include "rcdscanner.h"

#ifdef HAVE_RPM
#include "rpmindexcopy.h"
#endif

using namespace std;

// ReduceSourceList - Takes the path list and reduces it		/*{{{*/
// ---------------------------------------------------------------------
/* This takes the list of source list expressed entires and collects
   similar ones to form a single entry for each dist */
bool ReduceSourcelist(string CD,vector<string> &List)
{
   sort(List.begin(),List.end());
   
   // Collect similar entries
   for (vector<string>::iterator I = List.begin(); I != List.end(); I++)
   {
      // Find a space..
      string::size_type Space = (*I).find(' ');
      if (Space == string::npos)
	 continue;
      string::size_type SSpace = (*I).find(' ',Space + 1);
      if (SSpace == string::npos)
	 continue;
      
      string Word1 = string(*I,Space,SSpace-Space);
      for (vector<string>::iterator J = List.begin(); J != I; J++)
      {
	 // Find a space..
	 string::size_type Space2 = (*J).find(' ');
	 if (Space2 == string::npos)
	    continue;
	 string::size_type SSpace2 = (*J).find(' ',Space2 + 1);
	 if (SSpace2 == string::npos)
	    continue;
	 
	 if (string(*J,Space2,SSpace2-Space2) != Word1)
	    continue;
	 
	 *J += string(*I,SSpace);
	 *I = string();
      }
   }   

   // Wipe erased entries
   for (unsigned int I = 0; I < List.size();)
   {
      if (List[I].empty() == false)
	 I++;
      else
	 List.erase(List.begin()+I);
   }
   return true;
}
									/*}}}*/


bool RCDScanner::writeDatabase(string id, string name)
{
   // Escape special characters
   string::iterator J = name.begin();
   for (; J != name.end(); J++)
      if (*J == '"' || *J == ']' || *J == '[')
         *J = '_';    
    
    _database->Set("CD::" + id, name);
    
    
    string DFile = _config->FindFile("Dir::State::cdroms");
    string NewFile = DFile + ".new";
   
    unlink(NewFile.c_str());
    ofstream Out(NewFile.c_str());
    if (!Out)
	return _error->Errno("ofstream::ofstream",
			     _("Failed to open %s.new"),DFile.c_str());
    
    /* Write out all of the configuration directives by walking the
     configuration tree */
    const Configuration::Item *Top = _database->Tree(0);
    for (; Top != 0;)
    {
	// Print the config entry
	if (Top->Value.empty() == false)
	    Out <<  Top->FullTag() + " \"" << Top->Value << "\";" << endl;
	
	if (Top->Child != 0)
	{
	    Top = Top->Child;
	    continue;
	}
	
	while (Top != 0 && Top->Next == 0)
	    Top = Top->Parent;
	if (Top != 0)
	    Top = Top->Next;
    }   
    
    Out.close();
    
    rename(DFile.c_str(),string(DFile + '~').c_str());
    if (rename(NewFile.c_str(),DFile.c_str()) != 0)
	return _error->Errno("rename",_("Failed to rename %s.new to %s"),
			     DFile.c_str(),DFile.c_str());

    return true;
}


bool RCDScanner::writeSourceList(string name, vector<string> &list, bool pkg)
{
    // copy&paste from apt-cdrom

    if (list.size() == 0)
	return true;

    string File = _config->FindFile("Dir::Etc::sourcelist");

    // Open the stream for reading 
    ifstream F(File.c_str(), ios::in);
    if (!F != 0)
	return _error->Errno("ifstream::ifstream","Opening %s",File.c_str());
    
    string NewFile = File + ".new";
    unlink(NewFile.c_str());
    ofstream Out(NewFile.c_str());
    if (!Out)
	return _error->Errno("ofstream::ofstream",
			     _("Failed to open %s.new"),File.c_str());
    
    // Create a short uri without the path
    string ShortURI = "cdrom:[" + name + "]/";   
    string ShortURI2 = "cdrom:" + name + "/";     // For Compatibility
    
    const char *Type;
    
    if (pkg) 
	Type = pkgSourceType().c_str();
    else
	Type = srcSourceType().c_str();

    char Buffer[300];
    int CurLine = 0;
    bool First = true;
    while (F.eof() == false)
    {      
	F.getline(Buffer,sizeof(Buffer));
	CurLine++;
	_strtabexpand(Buffer,sizeof(Buffer));
	_strstrip(Buffer);
	
	// Comment or blank
	if (Buffer[0] == '#' || Buffer[0] == 0)
	{
	    Out << Buffer << endl;
	    continue;
	}
	
	if (First == true)
	{
	    for (vector<string>::iterator I = list.begin(); I != list.end(); I++)
	    {
		string::size_type Space = (*I).find(' ');
		if (Space == string::npos)
		    return _error->Error(_("Internal error"));
		Out << Type << " cdrom:[" << name << "]/" << string(*I,0,Space) <<
		    " " << string(*I,Space+1) << endl;
	    }
	}
	First = false;
	
	// Grok it
	string cType;
	string URI;
	const char *C = Buffer;
	if (ParseQuoteWord(C,cType) == false ||
	    ParseQuoteWord(C,URI) == false)
	{
	    Out << Buffer << endl;
	    continue;
	}
	
	// Emit lines like this one
	if (cType != Type || (string(URI,0,ShortURI.length()) != ShortURI &&
			      string(URI,0,ShortURI.length()) != ShortURI2))
	{
	    Out << Buffer << endl;
	    continue;
	}      
    }
    
    // Just in case the file was empty
    if (First == true)
    {
	for (vector<string>::iterator I = list.begin(); I != list.end(); I++)
	{
	    string::size_type Space = (*I).find(' ');
	    if (Space == string::npos)
		return _error->Error(_("Internal error"));
	    
	    Out << Type << " cdrom:[" << name << "]/" << string(*I,0,Space) <<
		    " " << string(*I,Space+1) << endl;
	}
    }
    
   Out.close();
    
    rename(File.c_str(),string(File + '~').c_str());
    if (rename(NewFile.c_str(),File.c_str()) != 0)
	return _error->Errno("rename",_("Failed to rename %s.new to %s"),
			     File.c_str(),File.c_str());
    
    return true;
}


void RCDScanner::unmountCD()
{
    string mpoint = _config->FindDir("Acquire::cdrom::mount", "/cdrom/");
    
    UnmountCdrom(mpoint);
}



bool RCDScanner::scanCD(RCDScanProgress *progress)
{
    string mpoint = _config->FindDir("Acquire::cdrom::mount", "/cdrom/");

    if (!_database)
	_database = new Configuration;
    
    string DFile = _config->FindFile("Dir::State::cdroms");
    if (FileExists(DFile) == true)
    {
	if (ReadConfigFile(*_database,DFile) == false)
	    return _error->Error(_("Unable to read the cdrom database %s"),
				 DFile.c_str());
    }
   
   
    progress->update(_("Mounting CD-ROM..."));

    if (!MountCdrom(mpoint)) {
	progress->update(_("Could not mount CD-ROM"));
	return false;
    }
    
    progress->update(_("Identifying Disc..."));
    if (!IdentCdrom(mpoint, _cdId)) {
	progress->update(_("Could not Identify Disc"));
	return false;
    }
    
    progress->update(_("Scanning Disc..."), 0);
    
    string cwd = SafeGetCWD();

    _pkgList.erase(_pkgList.begin(), _pkgList.end());
    _srcList.erase(_srcList.begin(), _srcList.end());
    _infoDir = "";
    
    if (!scanDirectory(mpoint, progress)) {
	progress->update(_("Error Scanning Disc"));
	return false;
    }
    
    cleanPkgList(_pkgList);
    cleanSrcList(_srcList);
    
    return true;
}


void RCDScanner::countLists(int &pkgLists, int &srcLists)
{
    pkgLists = _pkgList.size();
    srcLists = _srcList.size();
}


string RCDScanner::getDiscName()
{
    string name = "";
    
    if (!_infoDir.empty() && FileExists(_infoDir + "/info") == true)
    {
	ifstream F(string(_infoDir + "/info").c_str());
	if (!F == 0)
            getline(F,name);
	
	if (name.empty() == false)
            _database->Set("CD::" + _cdId + "::Label", name);
    }

    return name;
}


bool RCDScanner::needsRegistration(string &defaultName)
{
    defaultName = getDiscName();
    
    return !_database->Exists("CD::"+_cdId);
}


bool RCDScanner::registerDisc(string name)
{
    if (!writeDatabase(_cdId, name))
	return false;
    
    _cdName = name;

    return true;
}


bool RCDScanner::finish()
{
    string mpoint = _config->FindDir("Acquire::cdrom::mount", "/cdrom/");

    UnmountCdrom(mpoint);

    ReduceSourcelist(mpoint, _pkgList);
    ReduceSourcelist(mpoint, _srcList);
    
    if (!writeSourceList(_cdName, _pkgList, true)
	|| !writeSourceList(_cdName, _srcList, false)) {
	    return false;
    }
    
    return true;
}








bool RCDScanner::copyLists(RCDScanProgress *progress)
{
    string mpoint = _config->FindDir("Acquire::cdrom::mount", "/cdrom/");
    
    // Copy the package files to the state directory
#ifdef HAVE_RPM
    RPMPackageCopy Copy;
    RPMSourceCopy SrcCopy;
   
    if (Copy.CopyPackages(mpoint,_cdName,_pkgList) == false ||
	SrcCopy.CopyPackages(mpoint,_cdName,_srcList) == false)
	return false;
#endif
    return true;
}



int Score(string Path)
{
    if (Path.find("base/") != string::npos) {
	return 1;
    } else {
	return 0;
    }
}


// DropRepeats - Drop repeated files resulting from symlinks		/*{{{*/
// ---------------------------------------------------------------------
/* Here we go and stat every file that we found and strip dup inodes. */
bool DropRepeats(vector<string> &List,const char *Name)
{
   // Get a list of all the inodes
   ino_t *Inodes = new ino_t[List.size()];
   for (unsigned int I = 0; I != List.size(); I++)
   {
      struct stat Buf;
      string path = List[I] + string(Name);
       
      if (stat(path.c_str(),&Buf) != 0 &&
	  stat((path + ".gz").c_str(),&Buf) != 0 &&
	  stat((path + ".bz2").c_str(),&Buf) != 0)
	  _error->Errno("stat","Failed to stat %s",path.c_str());
      Inodes[I] = Buf.st_ino;
   }
   
   if (_error->PendingError() == true)
      return false;
   
   // Look for dups
   for (unsigned int I = 0; I != List.size(); I++)
   {
      for (unsigned int J = I+1; J < List.size(); J++)
      {
	 // No match
	 if (Inodes[J] != Inodes[I])
	    continue;
	 
	 // We score the two paths.. and erase one
	 int ScoreA = Score(List[I]);
	 int ScoreB = Score(List[J]);
	 if (ScoreA < ScoreB)
	 {
	    List[I] = string();
	    break;
	 }
	 
	 List[J] = string();
      }
   }  
 
   // Wipe erased entries
   for (unsigned int I = 0; I < List.size();)
   {
      if (List[I].empty() == false)
	 I++;
      else
	 List.erase(List.begin()+I);
   }
   
   return true;
}


void RCDScanner::cleanPkgList(vector<string> &list)
{
    DropRepeats(list, "pkglist");
}



void RCDScanner::cleanSrcList(vector<string> &list)
{
    DropRepeats(list, "srclist");
}



string RCDScanner::pkgSourceType() const
{
    return "rpm";
}


string RCDScanner::srcSourceType() const
{
    return "rpm-src";
}


bool RCDScanner::isOurArch(string arch) const
{
    if (arch == "noarch" || arch == _config->Find("APT::Architecture")) {
	return true;
    } else {
	return false;
    }
}


static int strrcmp_(const char *a, const char *b)
{
   int la = strlen(a);
   int lb = strlen(b);

   if (la == 0 || lb == 0)
       return 0;
   
   if (la > lb)
       return strcmp(&a[la-lb], b);
   else
       return strcmp(&b[lb-la], a);
}


bool RCDScanner::scanDirectory(string path, RCDScanProgress *progress,
			       int depth)
{
    static ino_t inodes[9];

    if (depth > 8)
	return true;
    
    if (path[path.size()-1] != '/')
	path = path + "/";
    
    if (chdir(path.c_str()) != 0) {
	return _error->Errno("chdir", _("Could not change directory to %s"),
			     path.c_str());
    }
    
    // Look for a .disk subdirectory
    struct stat Buf;
    if (_infoDir.empty() && stat(".disk",&Buf) == 0)
    {
	_infoDir = path + ".disk/";
    }

    DIR *D = opendir(".");
    if (!D) {
	return _error->Errno("opendir", _("Unable to read directory %s"),
			     path.c_str());
    }
    
    // Run over the directory and look for "base" subdirectories
    for (struct dirent *Dir = readdir(D); Dir != 0; Dir = readdir(D))
    {
	// Skip some files..
	if (strcmp(Dir->d_name,".") == 0 ||
	    strcmp(Dir->d_name,"..") == 0 ||
	    //strcmp(Dir->d_name,"source") == 0 ||
	    strcmp(Dir->d_name,".disk") == 0 ||
	    strncmp(Dir->d_name,"RPMS", 4) == 0 ||
	    strstr(Dir->d_name,"image") != NULL)
	    continue;

	if (stat(Dir->d_name, &Buf) != 0)
	    continue;

	
	if (strncmp(Dir->d_name, "srclist", sizeof("srclist")-1) == 0
	    && (strrcmp_(Dir->d_name, ".gz") == 0 
		|| strrcmp_(Dir->d_name, ".bz2") == 0))
	    _srcList.push_back(path);
	
	if (strncmp(Dir->d_name, "pkglist", sizeof("pkglist")-1) == 0
	    && (strrcmp_(Dir->d_name, ".gz") == 0
		|| strrcmp_(Dir->d_name, ".bz2") == 0))
	    _pkgList.push_back(path);

	if (!S_ISDIR(Buf.st_mode)) {
	    continue;
	}
	
	int I;
	for (I = 0; I != depth; I++)
	    if (inodes[I] == Buf.st_ino)
		break;
	if (I != depth) // we've been in this dir before
	    continue;
      
	// Store the inodes weve seen
	inodes[depth] = Buf.st_ino;

	if (scanDirectory(path + string(Dir->d_name), progress,
			  depth + 1))
	    return false;
	
	if (chdir(path.c_str()) != 0)
	    return _error->Errno("chdir", _("Could not change directory to %s"),
				 path.c_str());
    }
    
    closedir(D);
    
    return !_error->PendingError();
}

