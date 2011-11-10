/* rconfiguration.cc
 *
 * Copyright (c) 2000-2003 Conectiva S/A
 *               2002 Michael Vogt <mvo@debian.org>
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

#include <pwd.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <apt-pkg/init.h>
#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/fileutl.h>

#include "rconfiguration.h"

#include "i18n.h"

static string ConfigFilePath;
static string ConfigFileDir;


// #ifndef HAVE_RPM
// bool _ReadConfigFile(Configuration &Conf,string FName,bool AsSectional = false,
//                  unsigned Depth = 0);
// #endif

static void dumpToFile(const Configuration::Item *Top, ostream &out,
                       string pad)
{
   while (Top) {
      out << pad << Top->Tag << " \"" << Top->Value << "\"";

      if (Top->Child) {
         out << " {" << endl;
         dumpToFile(Top->Child, out, pad + "  ");
         out << pad << "};" << endl;

         if (Top->Next)
            out << endl;
      } else {
         out << ";" << endl;
      }

      if (pad.empty())
         break;                 // dump only synaptic section

      Top = Top->Next;
   }
}



bool RWriteConfigFile(Configuration &Conf)
{
   const Configuration::Item *Synaptic;

   // when running non-interactivly don't save any config (there should be no 
   // need)
   if(_config->FindB("Volatile::Non-Interactive", false) == true) 
      return true;

   // store option 'consider recommended packages as dependencies'
   // to config of apt if we run as root
   if (getuid() == 0) {
      // FIXME: use findDir
      string aptConfPath = _config->Find("Dir", "/")
                         + _config->Find("Dir::Etc", "etc/apt/")
                         + _config->Find("Dir::Etc:parts", "apt.conf.d")
                         + "/99synaptic";

      int old_umask = umask(0022);
      ofstream aptfile(aptConfPath.c_str(), ios::out);
      if (!aptfile != 0) {
         cerr << "cannot open " << aptConfPath.c_str() <<
                 " to write APT::Install-Recommends" << endl;
      } else {
         if (_config->FindB("APT::Install-Recommends", false))
            aptfile << "APT::Install-Recommends \"true\";" << endl;
         else
            aptfile << "APT::Install-Recommends \"false\";" << endl;
         aptfile.close();
      }
      umask(old_umask);
   }
   // and backup Install-Recommends to config of synaptic
   _config->Set("Synaptic::Install-Recommends",
                _config->FindB("APT::Install-Recommends",
                _config->FindB("Synaptic::Install-Recommends",
                false)));

   ofstream cfile(ConfigFilePath.c_str(), ios::out);
   if (!cfile != 0)
      return _error->Errno("ofstream",
                           _("ERROR: couldn't open %s for writing"),
                           ConfigFilePath.c_str());

   Synaptic = Conf.Tree(0);
   while (Synaptic) {
      if (Synaptic->Tag == "Synaptic")
         break;
      Synaptic = Synaptic->Next;
   }
   dumpToFile(Synaptic, cfile, "");

   cfile.close();

   return true;
}


static bool checkConfigDir(string &path)
{
   struct stat stbuf;
   struct passwd *pwd;

   pwd = getpwuid(getuid());
   if (!pwd) {
      return _error->Errno("getpwuid",
                           _
                           ("ERROR: Could not get password entry for superuser"));
   }
   path = string(pwd->pw_dir) + "/.synaptic";
   //path = "/etc/synaptic";

   if (stat(path.c_str(), &stbuf) < 0) {
      if (mkdir(path.c_str(), 0700) < 0) {
         return _error->Errno("mkdir",
                              _
                              ("ERROR: could not create configuration directory %s"),
                              path.c_str());
      }
   }
   return true;
}


string RConfDir()
{
   static string confDir;
   if (!checkConfigDir(confDir))
      cerr << "checkConfigDir() failed! please report to mvo@debian.org" <<
         endl;
   return confDir;
}

string RStateDir()
{
   struct stat stbuf;
   static string stateDir = string(SYNAPTICSTATEDIR);
   if (stat(stateDir.c_str(), &stbuf) < 0) {
      if (mkdir(stateDir.c_str(), 0755) < 0) {
	 _error->Errno("mkdir",
		       _("ERROR: could not create state directory %s"),
		       stateDir.c_str());
	 return "";
      }
   }

   return stateDir;
}

// we use the ConfDir for now to store very small tmpfiles
string RTmpDir()
{
   struct stat stbuf;
   static string tmpDir = RConfDir() + string("/tmp/");
   if (stat(tmpDir.c_str(), &stbuf) < 0) {
      if (mkdir(tmpDir.c_str(), 0700) < 0) {
	 _error->Errno("mkdir",
		       _("ERROR: could not create tmp directory %s"),
		       tmpDir.c_str());
	 return "";
      }
   }

   return tmpDir;
}


string RLogDir()
{
   struct stat stbuf;
   static string logDir = RConfDir() + string("/log/");

   if (stat(logDir.c_str(), &stbuf) < 0) {
      if (mkdir(logDir.c_str(), 0700) < 0) {
	 _error->Errno("mkdir",
		       _("ERROR: could not create log directory %s"),
		       logDir.c_str());
	 return "";
      }
   }

   return logDir;
}


bool RInitConfiguration(string confFileName)
{
   string configDir;

   if (!pkgInitConfig(*_config))
      return false;

   _config->Set("Program", "synaptic");

   if (!pkgInitSystem(*_config, _system))
      return false;

   if (!checkConfigDir(configDir))
      return false;

   ConfigFilePath = configDir + "/" + confFileName;
   ConfigFileDir = configDir;

   if (!ReadConfigFile(*_config, ConfigFilePath)) {
      _error->Discard();
   }

   // read Install-Recommends, preferably from APT:: if we run as root
   // or from Synaptic:: otherwise
   if(getuid() == 0) {
      _config->Set("APT::Install-Recommends",
                   _config->FindB("APT::Install-Recommends",
                   _config->FindB("Synaptic::Install-Recommends",
                   false)));
   } else {
      _config->Set("APT::Install-Recommends",
                   _config->FindB("Synaptic::Install-Recommends",
                   _config->FindB("APT::Install-Recommends",
                   false)));
   }

   return true;
}


bool RReadFilterData(Configuration &config)
{
   string defaultPath = ConfigFileDir + "/filters";
   string path = _config->Find("Volatile::filterFile", defaultPath.c_str());

   if (!FileExists(path)) {
      return false;
   }

   if (!ReadConfigFile(config, path, true)) {
      _error->DumpErrors();
      _error->Discard();
      return false;
   }
   return true;
}

bool RPackageOptionsFile(ofstream &out)
{
   string path = ConfigFileDir + "/options";
   out.open(path.c_str());
   if (!out != 0)
      return _error->Errno("ofstream",
                           _("ERROR: couldn't open %s for writing"),
                           path.c_str());
   return true;
}

bool RPackageOptionsFile(ifstream &in)
{
   string path = ConfigFileDir + "/options";
   in.open(path.c_str());
   if (!in != 0)
      return false;
//      return _error->Errno("ifstream", _("ERROR: couldn't open %s for reading"),
//                           path.c_str());
   return true;
}


bool RFilterDataOutFile(ofstream &out)
{
   string defaultPath = ConfigFileDir + "/filters";
   string path = _config->Find("Volatile::filterFile", defaultPath.c_str());

   out.open(path.c_str(), ios::out);

   if (!out != 0)
      return _error->Errno("ofstream", _("couldn't open %s for writing"),
                           path.c_str());

   return true;
}
