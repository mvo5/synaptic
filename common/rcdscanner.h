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


#include <vector>
#include <string>

using namespace std;

class Configuration;


class RCDScanProgress {
   
   int _total;
   
public:
   void setTotal(int total) { _total = total; };

   virtual void update(string phase, int current=-1) = 0;
};



class RCDScanner {

   Configuration *_database;
   
protected:

   vector<string> _pkgList;
   vector<string> _srcList;
   string _infoDir;
   
   string _cdId;
   string _cdName;

   string pkgSourceType() const;
   string srcSourceType() const;
   bool isOurArch(string arch) const;
   bool scanDirectory(string path, RCDScanProgress *progress, int depth=0);

   void cleanPkgList(vector<string> &list);
   void cleanSrcList(vector<string> &list);
   
   bool writeDatabase(string id, string name);
   bool writeSourceList(string id, vector<string> &list, bool pkg);

   string getDiscName();
   
public:
   void unmountCD();

   bool scanCD(RCDScanProgress *progress);
   void countLists(int &pkgLists, int &srcLists);

   bool copyLists(RCDScanProgress *progress);   
   
   bool needsRegistration(string &defaultName);
   bool registerDisc(string name);
   
   bool finish();
   
};


#endif


