/* rpackagefilter.h - filters for package listing
 * 
 * Copyright (c) 2000, 2001 Conectiva S/A 
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


#ifndef _RPACKAGEFILTER_H_
# define _RPACKAGEFILTER_H_

# include <vector>
# include <string>
# include <fstream>
# include <iostream>

# include <regex.h>

using namespace std;

class RPackage;

class Configuration;


class RPackageFilter {
public:
   virtual const char* type() = 0;

   virtual bool filter(RPackage *pkg) = 0;
   virtual void reset() = 0;

   virtual bool read(Configuration &conf, string key) = 0;
   virtual bool write(ofstream &out, string pad) = 0;
   //   ~RPackageFilter() = 0;
};


extern const char *RPFSection;

class RSectionPackageFilter : public RPackageFilter {
   vector<string> _groups;
   bool _inclusive; // include or exclude the packages
   
public:
   RSectionPackageFilter() : _inclusive(false) {};
   virtual ~RSectionPackageFilter() {};

   inline virtual void reset() { clear(); _inclusive = false; };

   inline virtual const char* type() { return RPFSection; };
   
   void setInclusive(bool flag) { _inclusive = flag; };
   bool inclusive();
   
   inline void addSection(string group) { _groups.push_back(group); };
   int count();
   string section(int index);
   void clear();
   
   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, string key);
   virtual bool write(ofstream &out, string pad);
};


extern const char *RPFPattern;

class RPatternPackageFilter : public RPackageFilter {
public:
  typedef enum  {
    Name,
    Version,
    Description,
    Depends,
    Provides,
    Conflicts,
    Replaces, // (or obsoletes)
    WeakDepends, // suggests et al
    RDepends // reverse depends
  } DepType;
   static char *TypeName[];

private:
   struct Pattern {
       DepType where;
       string pattern;
       bool exclusive;
       regex_t reg;
   };
   vector<Pattern> _patterns;
   
public:
   virtual ~RPatternPackageFilter();

   inline virtual void reset() { clear(); };
   
   inline virtual const char *type() { return RPFPattern; };
   
   void addPattern(DepType type, string pattern, bool exclusive);
   inline int count() { return _patterns.size(); };
   inline void pattern(int index, DepType &type, string &pattern, bool &exclusive) {
       type = _patterns[index].where;
       pattern = _patterns[index].pattern;
       exclusive = _patterns[index].exclusive;
   };
   void clear();
   
   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, string key);
   virtual bool write(ofstream &out, string pad);
};


extern const char *RPFStatus;

class RStatusPackageFilter : public RPackageFilter {   
public:
   enum Types {
       Installed = 1<<0,
       Upgradable = 1<<1, // installed but upgradable
       Broken = 1<<2, // installed but dependencies are broken
       NotInstalled = 1<<3,
       MarkInstall = 1<<4,
       MarkRemove = 1<<5,
       MarkKeep = 1<<6,
       NewPackage = 1<<7, // new Package (after update)
       PinnedPackage = 1<<8, // pinned Package (never upgrade)
       OrphanedPackage = 1<<9, // orphaned (identfied with deborphan)
       ResidualConfig = 1<<10,  // not installed but has config left
       DebconfPackage = 1<<11  // Package has debconf information
   };
private:
   int _status;
public:
   RStatusPackageFilter() : _status(0xffffffff) {};
   inline virtual void reset() { _status = 0xffffffff; };

   inline virtual const char *type() { return RPFStatus; };
   
   inline void setStatus(int status) { _status = status; };
   inline int status() { return _status; };
   
   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, string key);
   virtual bool write(ofstream &out, string pad);
};


extern const char *RPFPriority;

class RPriorityPackageFilter : public RPackageFilter {   
public:
   
private:
   
public:
   inline virtual void reset() {  };
   
   inline virtual const char *type() { return RPFPriority; };
   
   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, string key);
   virtual bool write(ofstream &out, string pad);
};



struct RFilter {
  public:
    RFilter() : preset(false) {};
    void setName(string name);
    string getName();
    bool read(Configuration &conf, string key);
    bool write(ofstream &out);
    bool apply(RPackage *package);
    void reset();
    RSectionPackageFilter section;
    RPatternPackageFilter pattern;
    RStatusPackageFilter status;
    RPriorityPackageFilter priority;
    bool preset;

  private:
    string name;
};


#endif

