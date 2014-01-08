/* raptoptions.cc - configuration handling
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



#include "config.h"
#include <fstream>
#include <sstream>
#include <dirent.h>

#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/tagfile.h>
#include <apt-pkg/policy.h>
#include <apt-pkg/sptr.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/fileutl.h>

#include "rconfiguration.h"
#include "raptoptions.h"

#include "i18n.h"


RAPTOptions *_roptions = new RAPTOptions;

using namespace std;

ostream &operator<<(ostream &os, const RAPTOptions::packageOptions &o)
{
   os << o.isNew;
   return os;
}

istream &operator>>(istream &is, RAPTOptions::packageOptions &o)
{
   is >> o.isNew;
   return is;
}


bool RAPTOptions::store()
{
   ofstream out;
   if (!RPackageOptionsFile(out))
      return false;

   for (packageOptionsIter it = _roptions->_packageOptions.begin();
        it != _roptions->_packageOptions.end(); it++) {
      // we only write out if it's new and the pkgname is not empty
      if ((*it).second.isNew && !(*it).first.empty())
         out << (*it).first << " " << (*it).second << endl;
   }
   return true;
}


bool RAPTOptions::restore()
{
   string pkg, line;
   packageOptions o;

   //cout << "bool RAPTOptions::restore()" << endl;

   ifstream in;
   if (!RPackageOptionsFile(in))
      return false;

   // new stuff
   while (!in.eof()) {
      getline(in, line);
      istringstream strstr(line.c_str());
      strstr >> pkg >> o >> ws;
      _packageOptions[pkg] = o;
   }

   // upgrade code for older synaptic versions, can go away in the future
   if(FileExists(RConfDir()+"/preferences"))
      rename(string(RConfDir()+"/preferences").c_str(),
	     string(RStateDir()+"/preferences").c_str());


   // pining stuff
   string File = RStateDir() + "/preferences";

   if (!FileExists(File))
      return true;

   FileFd Fd(File, FileFd::ReadOnly);
   pkgTagFile TF(&Fd);
   if (_error->PendingError() == true) {
      return false;
   }
   pkgTagSection Tags;
   while (TF.Step(Tags) == true) {
      string Name = Tags.FindS("Package");
      if (Name.empty() == true)
         return _error->
            Error(_
                  ("Invalid record in the preferences file, no Package header"));
      if (Name == "*")
         Name = string();

      const char *Start;
      const char *End;
      if (Tags.Find("Pin", Start, End) == false)
         continue;

      const char *Word = Start;
      for (; Word != End && isspace(*Word) == 0; Word++);

      // Parse the type, we are only interesseted in "version" for now
      pkgVersionMatch::MatchType Type;
      if (stringcasecmp(Start, Word, "version") == 0 && Name.empty() == false)
         Type = pkgVersionMatch::Version;
      else
         continue;
      for (; Word != End && isspace(*Word) != 0; Word++);

      setPackageLock(Name.c_str(), true);
   }

   // deborphan stuff
   rereadOrphaned();

   // debconf stuff
   rereadDebconf();

   return true;
}

bool RAPTOptions::getPackageDebconf(const char *package)
{
   string tmp = string(package);

   if (_packageOptions.find(tmp) == _packageOptions.end())
      return false;

   //cout << "getPackageOrphaned("<<package<<") called"<<endl;
   return _packageOptions[tmp].isDebconf;
}


void RAPTOptions::setPackageDebconf(const char *package, bool flag)
{
   //cout << "debconf called pkg: " << package << endl;
   _packageOptions[string(package)].isDebconf = flag;
}

void RAPTOptions::rereadDebconf()
{
   //cout << "void RAPTOptions::rereadDebconf()" << endl;

   // forget about any previously debconf packages
   for (packageOptionsIter it = _roptions->_packageOptions.begin();
        it != _roptions->_packageOptions.end(); it++) {
      (*it).second.isDebconf = false;
   }

   // read dir
   const char infodir[] = "/var/lib/dpkg/info";
   const char configext[] = ".config";

   DIR *dir;
   struct dirent *dent;
   char *point;

   if ((dir = opendir(infodir)) == NULL) {
      //cerr << "Error opening " << infodir << endl;
      return;
   }
   for (int i = 3; (dent = readdir(dir)); i++) {
      if ((point = strrchr(dent->d_name, '.')) == NULL)
         continue;
      if (strcmp(point, configext) == 0) {
         memset(point, 0, strlen(point));
         //cout << (dent->d_name) << endl;
         setPackageDebconf(dent->d_name, true);
      }
   }
   closedir(dir);
}

void RAPTOptions::rereadOrphaned()
{
   // forget about any previously orphaned packages
   for (packageOptionsIter it = _roptions->_packageOptions.begin();
        it != _roptions->_packageOptions.end(); it++) {
      (*it).second.isOrphaned = false;
   }

   //mvo: call deborphan and read package list from it
   //     TODO: make deborphan a library to make this cleaner
   FILE *fp;
   char buf[255];
   char cmd[] = "/usr/bin/deborphan";
   //FIXME: fail silently if deborphan is not installed - change this?
   if (!FileExists(cmd))
      return;
   fp = popen(cmd, "r");
   if (fp == NULL) {
      //cerr << "deborphan failed" << endl;
      return;
   }
   while (fgets(buf, sizeof(buf), fp) != NULL) {
      // remove newline at end and strip architecture suffix
      buf[strcspn(buf, "\n:")] = '\0';
      //cout << "buf: " << buf << endl;
      setPackageOrphaned(buf, true);
   }
   pclose(fp);
}


bool RAPTOptions::getPackageOrphaned(const char *package)
{
   string tmp = string(package);

   if (_packageOptions.find(tmp) == _packageOptions.end())
      return false;

   //cout << "getPackageOrphaned("<<package<<") called"<<endl;
   return _packageOptions[tmp].isOrphaned;
}


void RAPTOptions::setPackageOrphaned(const char *package, bool flag)
{
   //cout << "orphaned called pkg: " << package << endl;
   _packageOptions[string(package)].isOrphaned = flag;
}


bool RAPTOptions::getPackageLock(const char *package)
{
   string tmp = string(package);

   if (_packageOptions.find(tmp) == _packageOptions.end())
      return false;

   return _packageOptions[tmp].isLocked;
}


void RAPTOptions::setPackageLock(const char *package, bool lock)
{
   _packageOptions[string(package)].isLocked = lock;
}

bool RAPTOptions::getPackageNew(const char *package)
{
   string tmp = string(package);

   if (_packageOptions.find(tmp) == _packageOptions.end())
      return false;

   return _packageOptions[tmp].isNew;
}

void RAPTOptions::setPackageNew(const char *package, bool lock)
{
   _packageOptions[string(package)].isNew = lock;
}

void RAPTOptions::forgetNewPackages()
{
   for (packageOptionsIter it = _roptions->_packageOptions.begin();
        it != _roptions->_packageOptions.end(); it++) {
      (*it).second.isNew = false;
   }
}

bool RAPTOptions::getFlag(const char *key)
{
   if (_options.find(key) != _options.end())
      return _options[string(key)] == "true";
   else
      return false;
}


string RAPTOptions::getString(const char *key)
{
   if (_options.find(key) != _options.end())
      return _options[string(key)];
   else
      return "";
}


void RAPTOptions::setFlag(const char *key, bool value)
{
   _options[string(key)] = string(value ? "true" : "false");
}


void RAPTOptions::setString(const char *key, string value)
{
   _options[string(key)] = value;
}
