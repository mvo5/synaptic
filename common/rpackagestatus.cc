/* rpackagestatus.cc - wrapper for accessing packagestatus information
 * 
 * Copyright (c) 2000-2003 Conectiva S/A 
 *               2002-2008 Michael Vogt <mvo@debian.org>
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

#include <apt-pkg/tagfile.h>
#include <apt-pkg/strutl.h>
#include "rpackagestatus.h"

// init the static release array so that we need to
// run lsb_release only once
char RPackageStatus::release[255] = {0,};

// class that finds out what do display to get user
void RPackageStatus::init()
{
   const char *status_short[N_STATUS_COUNT] = {
      "install", "reinstall", "upgrade", "downgrade", "remove",
      "purge", "available", "available-locked",
      "installed-updated", "installed-outdated", "installed-locked",
      "broken", "new"
   };
   memcpy(PackageStatusShortString, status_short, sizeof(status_short));

   const char *status_long[N_STATUS_COUNT] = {
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
   if(_config->FindB("Synaptic::mark-unsupported", true)) {
      string s, labels, origin, components;
      markUnsupported = true;

      // read supported labels
      labels = _config->Find("Synaptic::supported-label", "Debian Debian-Security");
      stringstream sst1(labels);
      while(!sst1.eof()) {
	 sst1 >> s;
	 supportedLabels.push_back(s);
      }

      // read supported origins
      origin = _config->Find("Synaptic::supported-origins", "Debian");
      stringstream sst2(origin);
      while(!sst2.eof()) {
	 sst2 >> s;
	 supportedOrigins.push_back(s);
      }
      
      // read supported components
      components = _config->Find("Synaptic::supported-components", "main updates/main");
      stringstream sst3(components);
      while(!sst3.eof()) {
	 sst3 >> s;
	 supportedComponents.push_back(s);
      }
   } 

   // init the static release once
   FILE *fp = popen("lsb_release -c -s","r");
   if(fp) {
      fgets((char *)RPackageStatus::release, 255, fp);
      pclose(fp);
      _strstrip(release);
   } 
}

bool RPackageStatus::isSupported(RPackage *pkg) 
{
   bool res = true;

   if(markUnsupported) {
      bool sc, sl, so;

      sc=sl=so=false;

      string component = pkg->component();
      string label = pkg->label();
      string origin = pkg->origin();

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
      for(unsigned int i=0;i<supportedOrigins.size();i++) {
	 if(supportedOrigins[i] == origin) {
	    so = true;
	    break;
	 }
      }

      res = (sc & sl & so & pkg->isTrusted());
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

bool RPackageStatus::maintenanceEndTime(RPackage *pkg, struct tm *res) 
{
   //cerr << "RPackageStatus::maintenanceEndTime()" << std::endl;

   pkgTagSection sec;
   time_t release_date = -1;

   string distro = _config->Find("Synaptic::supported-label");
   string releaseFile = pkg->getReleaseFileForOrigin(distro, release);
   if(!FileExists(releaseFile)) {
      // happens e.g. when there is no release file and is harmless
      //cerr << "mainenanceEndTime(): can not find file: " << releaseFile << endl;
      return false;
   }
   
   // read the relase file
   FileFd fd(releaseFile, FileFd::ReadOnly);
   pkgTagFile t(&fd);
   t.Step(sec);

   // get the time_t form the string
   if(!StrToTime(sec.FindS("Date"), release_date))
      return false;

   // if its not a supported package, return 0 
   if(!isSupported(pkg))
      return false;

   // now calculate the time until there is support
   gmtime_r(&release_date, res);
   
   const int support_time =_config->FindI("Synaptic::supported-month", 0);
   if (support_time <= 0)
      return false;

   res->tm_year += (support_time / 12) + ((res->tm_mon + support_time % 12) / 12);
   res->tm_mon = (res->tm_mon + support_time) % 12;

   return true;
}
