/* rcdscanner.h
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


#ifndef _RCDSCANNER_H_
#define _RCDSCANNER_H_

#include<config.h>
#ifndef HAVE_APTPKG_CDROM

#include <vector>
#include <string>

#include "rpackagelister.h"

using namespace std;

class Configuration;


class RCDScanProgress {

 protected:
   int _total;

 public:
   void setTotal(int total) {
      _total = total;
   };

   virtual void update(string text, int current) = 0;
};

class RCDScanner {

   Configuration *_database;

   RUserDialog *_userDialog;

 protected:

   enum {
      STEP_PREPARE = 1,
      STEP_UNMOUNT,
      STEP_WAIT,
      STEP_MOUNT,
      STEP_IDENT,
      STEP_SCAN,
      STEP_CLEAN,
      STEP_UNMOUNT2,
      STEP_REGISTER,
      STEP_COPY,
      STEP_WRITE,
      STEP_UNMOUNT3,
      STEP_LAST
   };

   vector<string> _pkgList;
   vector<string> _srcList;
   string _infoDir;

   string _cdId;
   string _cdName;
   string _cdOldName;

   bool _cdromMounted;
   bool _scannedOk;

   string pkgSourceType() const;
   string srcSourceType() const;
   bool scanDirectory(string path, RCDScanProgress *progress, int depth = 0);

   void cleanPkgList(vector<string> &list);
   void cleanSrcList(vector<string> &list);

   bool writeDatabase();
   bool writeSourceList(vector<string> &list, bool pkg);

 public:
   bool start(RCDScanProgress *progress);
   bool finish(RCDScanProgress *progress);
   void unmount();

   string getDiscName();
   bool setDiscName(string name);

   void countLists(int &pkgLists, int &srcLists);

   RCDScanner(RUserDialog *userDialog)
 :   _database(0), _userDialog(userDialog), _cdromMounted(0), _scannedOk(0) {
   };
};

#endif
#endif
