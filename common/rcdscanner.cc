/* rcdscanner.cc
 *
 * Copyright (c) 2000-2003 Conectiva S/A
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

#include<config.h>
#ifndef HAVE_APTPKG_CDROM

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
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
#else
#include "indexcopy.h"
#endif

using namespace std;

// ReduceSourceList - Takes the path list and reduces it                /*{{{*/
// ---------------------------------------------------------------------
/* This takes the list of source list expressed entires and collects
   similar ones to form a single entry for each dist */
void ReduceSourcelist(string CD, vector<string> &List)
{
   sort(List.begin(), List.end());

   // Collect similar entries
   for (vector<string>::iterator I = List.begin(); I != List.end(); I++) {
      // Find a space..
      string::size_type Space = (*I).find(' ');
      if (Space == string::npos)
         continue;
      string::size_type SSpace = (*I).find(' ', Space + 1);
      if (SSpace == string::npos)
         continue;

      string Word1 = string(*I, Space, SSpace - Space);
      string Prefix = string(*I, 0, Space);
      for (vector<string>::iterator J = List.begin(); J != I; J++) {
         // Find a space..
         string::size_type Space2 = (*J).find(' ');
         if (Space2 == string::npos)
            continue;
         string::size_type SSpace2 = (*J).find(' ', Space2 + 1);
         if (SSpace2 == string::npos)
            continue;

         if (string(*J, 0, Space2) != Prefix)
            continue;
         if (string(*J, Space2, SSpace2 - Space2) != Word1)
            continue;

         *J += string(*I, SSpace);
         *I = string();
      }
   }

   // Wipe erased entries
   for (unsigned int I = 0; I < List.size();) {
      if (List[I].empty() == false)
         I++;
      else
         List.erase(List.begin() + I);
   }
}

                                                                        /*}}} */
bool RCDScanner::writeDatabase()
{
   _database->Set("CD::" + _cdId, _cdName);

   string DFile = _config->FindFile("Dir::State::cdroms");
   string NewFile = DFile + ".new";

   unlink(NewFile.c_str());
   ofstream Out(NewFile.c_str());
   if (!Out)
      return _error->Errno("ofstream::ofstream",
                           _("Failed to open %s.new"), DFile.c_str());

   /* Write out all of the configuration directives by walking the
      configuration tree */
   const Configuration::Item *Top = _database->Tree(0);
   for (; Top != 0;) {
      // Print the config entry
      if (Top->Value.empty() == false)
         Out << Top->FullTag() + " \"" << Top->Value << "\";" << endl;

      if (Top->Child != 0) {
         Top = Top->Child;
         continue;
      }

      while (Top != 0 && Top->Next == 0)
         Top = Top->Parent;
      if (Top != 0)
         Top = Top->Next;
   }

   Out.close();

   rename(DFile.c_str(), string(DFile + '~').c_str());
   if (rename(NewFile.c_str(), DFile.c_str()) != 0)
      return _error->Errno("rename", _("Failed to rename %s.new to %s"),
                           DFile.c_str(), DFile.c_str());

   return true;
}

bool RCDScanner::writeSourceList(vector<string> &list, bool pkg)
{
   // copy&paste from apt-cdrom

   if (list.size() == 0)
      return true;

   string File = _config->FindFile("Dir::Etc::sourcelist");

   // Open the stream for reading 
   ifstream F((FileExists(File) ? File.c_str() : "/dev/null"), ios::in);
   if (!F != 0)
      return _error->Errno("ifstream::ifstream", "Opening %s", File.c_str());

   string NewFile = File + ".new";
   unlink(NewFile.c_str());
   ofstream Out(NewFile.c_str());
   if (!Out)
      return _error->Errno("ofstream::ofstream",
                           _("Failed to open %s.new"), File.c_str());

   // Create a short uri without the path
   string ShortURI = "cdrom:[" + _cdName + "]/";
   string ShortURI2 = "cdrom:" + _cdName + "/"; // For Compatibility

   string ShortOldURI;
   string ShortOldURI2;
   if (_cdOldName.empty() == false) {
      ShortOldURI = "cdrom:[" + _cdOldName + "]/";
      ShortOldURI2 = "cdrom:" + _cdOldName + "/";
   }

   string Type;

   if (pkg)
      Type = pkgSourceType().c_str();
   else
      Type = srcSourceType().c_str();

   char Buffer[300];
   int CurLine = 0;
   bool First = true;
   while (F.eof() == false) {
      F.getline(Buffer, sizeof(Buffer));
      CurLine++;
      _strtabexpand(Buffer, sizeof(Buffer));
      _strstrip(Buffer);

      // Comment or blank
      if (Buffer[0] == '#' || Buffer[0] == 0) {
         Out << Buffer << endl;
         continue;
      }

      if (First == true) {
         for (vector<string>::iterator I = list.begin(); I != list.end();
              I++) {
            string::size_type Space = (*I).find(' ');
            if (Space == string::npos)
               return _error->Error(_("Internal error"));
            Out << Type << " cdrom:[" << _cdName << "]/" << string(*I, 0,
                                                                   Space) <<
               " " << string(*I, Space + 1) << endl;
         }
      }
      First = false;

      // Grok it
      string cType;
      string URI;
      const char *C = Buffer;
      if (ParseQuoteWord(C, cType) == false || ParseQuoteWord(C, URI) == false) {
         Out << Buffer << endl;
         continue;
      }
      // Omit lines like this one
      if (cType != Type
          || (string(URI, 0, ShortURI.length()) != ShortURI &&
              string(URI, 0, ShortURI.length()) != ShortURI2 &&
              (_cdOldName.empty()
               || (string(URI, 0, ShortOldURI.length()) != ShortOldURI &&
                   string(URI, 0, ShortOldURI.length()) != ShortOldURI2)))) {
         Out << Buffer << endl;
         continue;
      }
   }

   // Just in case the file was empty
   if (First == true) {
      for (vector<string>::iterator I = list.begin(); I != list.end(); I++) {
         string::size_type Space = (*I).find(' ');
         if (Space == string::npos)
            return _error->Error(_("Internal error"));

         Out << Type << " cdrom:[" << _cdName << "]/" << string(*I, 0,
                                                                Space) << " "
            << string(*I, Space + 1) << endl;
      }
   }

   Out.close();

   rename(File.c_str(), string(File + '~').c_str());
   if (rename(NewFile.c_str(), File.c_str()) != 0)
      return _error->Errno("rename", _("Failed to rename %s.new to %s"),
                           File.c_str(), File.c_str());

   return true;
}


bool RCDScanner::start(RCDScanProgress *progress)
{
   _cdName = "";
   _scannedOk = false;

   progress->setTotal(STEP_LAST);
   progress->update(_("Preparing..."), STEP_PREPARE);

   // Startup
   string CDROM = _config->FindDir("Acquire::cdrom::mount", "/cdrom/");
   if (CDROM[0] == '.')
      CDROM = SafeGetCWD() + '/' + CDROM;

   if (!_database)
      _database = new Configuration();

   string DFile = _config->FindFile("Dir::State::cdroms");
   if (FileExists(DFile) == true) {
      if (ReadConfigFile(*_database, DFile) == false) {
         return _error->Error(_("Unable to read the cdrom database %s"),
                              DFile.c_str());
      }
   }
   // Unmount the CD and get the user to put in the one they want
   _cdromMounted = false;
   if (_config->FindB("APT::CDROM::NoMount", false) == false) {
      progress->update(_("Unmounting CD-ROM..."), STEP_UNMOUNT);
      UnmountCdrom(CDROM);

      progress->update(_("Waiting for disc..."), STEP_WAIT);
      if (_userDialog->proceed(_("Insert a disc in the drive.")) == false)
         return false;

      // Mount the new CDROM
      progress->update(_("Mounting CD-ROM..."), STEP_MOUNT);

      if (MountCdrom(CDROM) == false)
         return _error->Error(_("Failed to mount the cdrom."));
      _cdromMounted = true;
   }

   progress->update(_("Identifying disc..."), STEP_IDENT);

   if (!IdentCdrom(CDROM, _cdId)) {
      return _error->Error(_("Couldn't identify disc."));
   }

   progress->update(_("Scanning disc..."), STEP_SCAN);

   string cwd = SafeGetCWD();

   _pkgList.clear();
   _srcList.clear();
   _infoDir = "";

   if (!scanDirectory(CDROM, progress)) {
      chdir(cwd.c_str());
      return false;
   }

   chdir(cwd.c_str());

   progress->update(_("Cleaning package lists..."), STEP_CLEAN);

   cleanPkgList(_pkgList);
   cleanSrcList(_srcList);

   if (_pkgList.size() == 0 && _srcList.size() == 0) {
      progress->update(_("Unmounting CD-ROM..."), STEP_UNMOUNT2);

      if (_cdromMounted
          && _config->FindB("APT::CDROM::NoMount", false) == false) {
         UnmountCdrom(CDROM);
         _cdromMounted = false;
      }
      return _error->Error(_("Unable to locate any package files. "
                             "Perhaps this is not an APT enabled disc."));
   }

   _scannedOk = true;
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

   if (_database->Exists("CD::" + _cdId)) {
      name = _database->Find("CD::" + _cdId, "");
      _cdOldName = name;
   } else if (!_infoDir.empty() && FileExists(_infoDir + "/info")) {
      ifstream F(string(_infoDir + "/info").c_str());
      if (!F == 0)
         getline(F, name);

      if (name.empty() == false) {
         // Escape special characters
         string::iterator J = name.begin();
         for (; J != name.end(); J++)
            if (*J == '"' || *J == ']' || *J == '[')
               *J = '_';
      }
   }

   return name;
}

bool RCDScanner::setDiscName(string name)
{
   if (name.empty() == true ||
       name.find('"') != string::npos ||
       name.find('[') != string::npos || name.find(']') != string::npos)
      return false;
   _cdName = name;
   return true;
}

bool RCDScanner::finish(RCDScanProgress *progress)
{
   if (_scannedOk == false) {
      return _error->Error(_("Disc not successfully scanned."));
   }

   if (_cdName.empty() == true) {
      return _error->Error(_("Empty disc name."));
   }

   progress->update(_("Registering disc..."), STEP_REGISTER);

   if (writeDatabase() == false) {
      return false;
   }
   // Copy the package files to the state directory
#ifdef HAVE_RPM
   RPMPackageCopy Copy;
   RPMSourceCopy SrcCopy;
#else
   PackageCopy Copy;
   SourceCopy SrcCopy;
#endif

   progress->update(_("Copying package lists..."), STEP_COPY);

   string CDROM = _config->FindDir("Acquire::cdrom::mount", "/cdrom/");

   if (Copy.CopyPackages(CDROM, _cdName, _pkgList) == false ||
       SrcCopy.CopyPackages(CDROM, _cdName, _srcList) == false) {
      return false;
   }

   progress->update(_("Writing sources list..."), STEP_WRITE);

   ReduceSourcelist(CDROM, _pkgList);
   ReduceSourcelist(CDROM, _srcList);

   if (!writeSourceList(_pkgList, true)
       || !writeSourceList(_srcList, false)) {
      return false;
   }

   if (_cdromMounted) {
      progress->update(_("Unmounting CD-ROM..."), STEP_UNMOUNT3);
      UnmountCdrom(CDROM);
   }

   progress->update(_("Done!"), STEP_LAST);

   return true;
}

void RCDScanner::unmount()
{
   string CDROM = _config->FindDir("Acquire::cdrom::mount", "/cdrom/");
   if (_cdromMounted)
      UnmountCdrom(CDROM);
}

// DropBinaryArch - Dump dirs with a string like /binary-<foo>/         /*{{{*/
// ---------------------------------------------------------------------
/* Here we drop everything that is not this machines arch */
bool DropBinaryArch(vector<string> &List)
{
   char S[300];
   snprintf(S, sizeof(S), "/binary-%s/",
            _config->Find("Apt::Architecture").c_str());

   for (unsigned int I = 0; I < List.size(); I++) {
      const char *Str = List[I].c_str();

      const char *Res;
      if ((Res = strstr(Str, "/binary-")) == 0)
         continue;

      // Weird, remove it.
      if (strlen(Res) < strlen(S)) {
         List.erase(List.begin() + I);
         I--;
         continue;
      }
      // See if it is our arch
      if (stringcmp(Res, Res + strlen(S), S) == 0)
         continue;

      // Erase it
      List.erase(List.begin() + I);
      I--;
   }

   return true;
}

                                                                        /*}}} */
// Score - We compute a 'score' for a path                              /*{{{*/
// ---------------------------------------------------------------------
/* Paths are scored based on how close they come to what I consider
   normal. That is ones that have 'dist' 'stable' 'frozen' will score
   higher than ones without. */
int Score(string Path)
{
   int Res = 0;
#ifdef HAVE_RPM
   if (Path.find("base/") != string::npos)
      Res = 1;
#else
   if (Path.find("stable/") != string::npos)
      Res += 29;
   if (Path.find("/binary-") != string::npos)
      Res += 20;
   if (Path.find("frozen/") != string::npos)
      Res += 28;
   if (Path.find("unstable/") != string::npos)
      Res += 27;
   if (Path.find("/dists/") != string::npos)
      Res += 40;
   if (Path.find("/main/") != string::npos)
      Res += 20;
   if (Path.find("/contrib/") != string::npos)
      Res += 20;
   if (Path.find("/non-free/") != string::npos)
      Res += 20;
   if (Path.find("/non-US/") != string::npos)
      Res += 20;
   if (Path.find("/source/") != string::npos)
      Res += 10;
   if (Path.find("/debian/") != string::npos)
      Res -= 10;
#endif
   return Res;
}

                                                                        /*}}} */
// DropRepeats - Drop repeated files resulting from symlinks            /*{{{*/
// ---------------------------------------------------------------------
/* Here we go and stat every file that we found and strip dup inodes. */
bool DropRepeats(vector<string> &List, const char *Name)
{
   // Get a list of all the inodes
   ino_t *Inodes = new ino_t[List.size()];
   for (unsigned int I = 0; I != List.size(); I++) {
      struct stat Buf;
      if (stat((List[I]).c_str(), &Buf) != 0 &&
          stat((List[I] + Name).c_str(), &Buf) != 0 &&
          stat((List[I] + Name + ".gz").c_str(), &Buf) != 0)
         _error->Errno("stat", _("Failed to stat %s%s"), List[I].c_str(),
                       Name);
      Inodes[I] = Buf.st_ino;
   }

   if (_error->PendingError() == true)
      return false;

   // Look for dups
   for (unsigned int I = 0; I != List.size(); I++) {
      for (unsigned int J = I + 1; J < List.size(); J++) {
         // No match
         if (Inodes[J] != Inodes[I])
            continue;

         // We score the two paths.. and erase one
         int ScoreA = Score(List[I]);
         int ScoreB = Score(List[J]);
         if (ScoreA < ScoreB) {
            List[I] = string();
            break;
         }

         List[J] = string();
      }
   }

   // Wipe erased entries
   for (unsigned int I = 0; I < List.size();) {
      if (List[I].empty() == false)
         I++;
      else
         List.erase(List.begin() + I);
   }

   return true;
}

void RCDScanner::cleanPkgList(vector<string> &list)
{
#ifdef HAVE_RPM
   DropRepeats(list, "pkglist");
#else
   DropBinaryArch(list);
   DropRepeats(list, "Packages");
#endif
}

void RCDScanner::cleanSrcList(vector<string> &list)
{
#ifdef HAVE_RPM
   DropRepeats(list, "srclist");
#else
   DropRepeats(list, "Sources");
#endif
}

string RCDScanner::pkgSourceType() const
{
#ifdef HAVE_RPM
   return "rpm";
#else
   return "deb";
#endif
}

string RCDScanner::srcSourceType() const
{
#ifdef HAVE_RPM
   return "rpm-src";
#else
   return "deb-src";
#endif
}

#if 0
static int strrcmp_(const char *a, const char *b)
{
   int la = strlen(a);
   int lb = strlen(b);

   if (la == 0 || lb == 0)
      return 0;

   if (la > lb)
      return strcmp(&a[la - lb], b);
   else
      return strcmp(&b[lb - la], a);
}
#endif

bool RCDScanner::scanDirectory(string CD, RCDScanProgress *progress,
                               int Depth)
{
   static ino_t Inodes[9];
   if (Depth >= 7)
      return true;

   if (CD[CD.length() - 1] != '/')
      CD += '/';

   if (chdir(CD.c_str()) != 0)
      return _error->Errno("chdir", _("Unable to change to %s"), CD.c_str());

   // Look for a .disk subdirectory
   struct stat Buf;
   if (stat(".disk", &Buf) == 0) {
      if (_infoDir.empty() == true)
         _infoDir = CD + ".disk/";
   }
   // Don't look into directories that have been marked to ingore.
   if (stat(".aptignr", &Buf) == 0)
      return true;

#ifdef HAVE_RPM
   bool Found = false;
   if (stat("release", &Buf) == 0)
      Found = true;
#else
   /* Aha! We found some package files. We assume that everything under 
      this dir is controlled by those package files so we don't look down
      anymore */
   if (stat("Packages", &Buf) == 0 || stat("Packages.gz", &Buf) == 0) {
      _pkgList.push_back(CD);

      // Continue down if thorough is given
      if (_config->FindB("APT::CDROM::Thorough", false) == false)
         return true;
   }
   if (stat("Sources.gz", &Buf) == 0 || stat("Sources", &Buf) == 0) {
      _srcList.push_back(CD);

      // Continue down if thorough is given
      if (_config->FindB("APT::CDROM::Thorough", false) == false)
         return true;
   }
#endif

   DIR *D = opendir(".");
   if (D == 0)
      return _error->Errno("opendir", _("Unable to read %s"), CD.c_str());

   // Run over the directory
   for (struct dirent * Dir = readdir(D); Dir != 0; Dir = readdir(D)) {
      // Skip some files..
      if (strcmp(Dir->d_name, ".") == 0 || strcmp(Dir->d_name, "..") == 0 ||
          //strcmp(Dir->d_name,"source") == 0 ||
          strcmp(Dir->d_name, ".disk") == 0 ||
#ifdef HAVE_RPM
          strncmp(Dir->d_name, "RPMS", 4) == 0 ||
          strncmp(Dir->d_name, "doc", 3) == 0)
#else
          strcmp(Dir->d_name, "experimental") == 0 ||
          strcmp(Dir->d_name, "binary-all") == 0)
#endif
         continue;

#ifdef HAVE_RPM
      if (strncmp(Dir->d_name, "pkglist.", 8) == 0 &&
          strcmp(Dir->d_name + strlen(Dir->d_name) - 4, ".bz2") == 0) {
         _pkgList.push_back(CD + string(Dir->d_name));
         Found = true;
         continue;
      }
      if (strncmp(Dir->d_name, "srclist.", 8) == 0 &&
          strcmp(Dir->d_name + strlen(Dir->d_name) - 4, ".bz2") == 0) {
         _srcList.push_back(CD + string(Dir->d_name));
         Found = true;
         continue;
      }
      if (_config->FindB("APT::CDROM::Thorough", false) == false &&
          Found == true)
         continue;
#endif

      // See if the name is a sub directory
      struct stat Buf;
      if (stat(Dir->d_name, &Buf) != 0)
         continue;

      if (S_ISDIR(Buf.st_mode) == 0)
         continue;

      int I;
      for (I = 0; I != Depth; I++)
         if (Inodes[I] == Buf.st_ino)
            break;
      if (I != Depth)
         continue;

      // Store the inodes weve seen
      Inodes[Depth] = Buf.st_ino;

      // Descend
      if (scanDirectory(CD + Dir->d_name, progress, Depth + 1) == false)
         break;

      if (chdir(CD.c_str()) != 0)
         return _error->Errno("chdir", _("Unable to change to %s"),
                              CD.c_str());
   }

   closedir(D);

   return !_error->PendingError();
}

#endif
// vim:sts=4:sw=4
