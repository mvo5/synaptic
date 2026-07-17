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

#pragma once

#include "config.h"  // IWYU pragma: associated

#include <apt-pkg/pkgcache.h>
#include <fstream>
#include <regex.h>
#include <set>
#include <string>
#include <vector>

class Configuration;
class RPackage;

class RPackageFilter {

   public:

   virtual const char *type() = 0;

   virtual bool filter(RPackage *pkg) = 0;
   virtual void reset() = 0;

   virtual bool read(Configuration &conf, std::string key) = 0;
   virtual bool write(std::ofstream &out, std::string pad) = 0;

   RPackageFilter() {}
   virtual ~RPackageFilter() {}
};


extern const char *RPFSection;

class RSectionPackageFilter : public RPackageFilter {

   protected:

   std::vector<std::string> _groups;
   bool _inclusive;             // include or exclude the packages

   public:

   RSectionPackageFilter() : _inclusive(false) {}
   virtual ~RSectionPackageFilter() {}

   inline virtual void reset() {
      clear();
      _inclusive = false;
   }

   inline virtual const char *type() { return RPFSection; }

   void setInclusive(bool flag) { _inclusive = flag; }
   bool inclusive();

   inline void addSection(std::string group) { _groups.push_back(group); }
   int count();
   std::string section(int index);
   void clear();

   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, std::string key);
   virtual bool write(std::ofstream &out, std::string pad);
};

extern const char *RPFPattern;

class RPatternPackageFilter : public RPackageFilter {
 public:
   typedef enum {
      Name,
      Description,
      Maintainer,
      Version,
      Depends,
      Provides,
      Conflicts,
      Replaces,                 // (or obsoletes)
      Recommends,
      Suggests,
      RDepends,                  // reverse depends
      Origin,                   // package origin (like security.debian.org)
      Component                   // package component (e.g. main)
   } DepType;


 protected:
   struct Pattern {
      DepType where;
      std::string pattern;
      bool exclusive;
        std::vector<regex_t *> regexps;
   };
   std::vector<Pattern> _patterns;

   bool and_mode; // patterns are applied in "AND" mode if true, "OR" if false

   inline bool filterName(Pattern pat, RPackage *pkg);
   inline bool filterVersion(Pattern pat, RPackage *pkg);
   inline bool filterDescription(Pattern pat, RPackage *pkg);
   inline bool filterMaintainer(Pattern pat, RPackage *pkg);
   inline bool filterDepends(Pattern pat, RPackage *pkg,
			     pkgCache::Dep::DepType filterType);
   inline bool filterProvides(Pattern pat, RPackage *pkg);
   inline bool filterRDepends(Pattern pat, RPackage *pkg);
   inline bool filterOrigin(Pattern pat, RPackage *pkg);
   inline bool filterComponent(Pattern pat, RPackage *pkg);

 public:

   static const char *TypeName[];

   RPatternPackageFilter() : and_mode(true) {}
   RPatternPackageFilter(RPatternPackageFilter &f);
   virtual ~RPatternPackageFilter();

   inline virtual void reset() { clear(); }

   inline virtual const char *type() { return RPFPattern; }

   void addPattern(DepType type, const std::string &pattern, bool exclusive);

   inline size_t count() {
      return _patterns.size();
   }

   inline void getPattern(size_t index, DepType &type, std::string &pattern, bool &exclusive) {
      type = _patterns[index].where;
      pattern = _patterns[index].pattern;
      exclusive = _patterns[index].exclusive;
   }

   void clear();
   bool getAndMode() { return and_mode; }
   void setAndMode(bool b) { and_mode=b; }

   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, std::string key);
   virtual bool write(std::ofstream &out, std::string pad);
};


extern const char *RPFStatus;

class RStatusPackageFilter : public RPackageFilter {

   protected:

   int _status;

   public:

   enum Types {
      Installed = 1 << 0,
      Upgradable = 1 << 1,      // installed but upgradable
      Broken = 1 << 2,          // installed but dependencies are broken
      NotInstalled = 1 << 3,
      MarkInstall = 1 << 4,
      MarkRemove = 1 << 5,
      MarkKeep = 1 << 6,
      NewPackage = 1 << 7,      // new Package (after update)
      PinnedPackage = 1 << 8,   // pinned Package (never upgrade)
      OrphanedPackage = 1 << 9, // orphaned (identfied with deborphan)
      ResidualConfig = 1 << 10, // not installed but has config left
      NotInstallable = 1 << 11,  // the package is not aviailable in repository
      UpstreamUpgradable = 1 << 12, // new upstream version
      AutoInstalled = 1 << 13, // automatically installed
      Garbage = 1 << 14, // automatically installed and no longer required
      NowPolicyBroken = 1 << 15,
      ManualInstalled = 1 << 16,  // !AutoInstalled
   };

   RStatusPackageFilter() : _status(~0)
   {}
   inline virtual void reset() { _status = ~0; }

   inline virtual const char *type() { return RPFStatus; }

   inline void setStatus(int status) { _status = status; }
   inline int status() { return _status; }

   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, std::string key);
   virtual bool write(std::ofstream &out, std::string pad);
};


extern const char *RPFPriority;

class RPriorityPackageFilter:public RPackageFilter {

   public:

   RPriorityPackageFilter()  {}

   inline virtual void reset() {}

   inline virtual const char *type() { return RPFPriority; }

   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, std::string key);
   virtual bool write(std::ofstream &out, std::string pad);
};


extern const char *RPFReducedView;

class RReducedViewPackageFilter : public RPackageFilter {

   protected:

   bool _enabled;

   std::set<std::string> _hide;
   std::vector<std::string> _hide_wildcard;
   std::vector<regex_t *> _hide_regex;

   void addFile(std::string FileName);

   public:

   RReducedViewPackageFilter() : _enabled(false) {}
   ~RReducedViewPackageFilter();

   inline virtual void reset() { _hide.clear(); }

   inline virtual const char *type() { return RPFReducedView; }

   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, std::string key);
   virtual bool write(std::ofstream &out, std::string pad);

   void enable() { _enabled = true; }
   void disable() { _enabled = false; }
};

extern const char *RPFFile;

class RFilePackageFilter : public RPackageFilter {

   protected:

   std::string filename;
   std::set<std::string> pkgs;

   public:

   RFilePackageFilter() {}
   virtual ~RFilePackageFilter() {}

   inline virtual void reset() {}
   inline virtual const char *type() { return RPFFile; }

   bool addFile(std::string file);

   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, std::string key);
   virtual bool write(std::ofstream &out, std::string pad);
};


struct RFilter {

   public:

   RFilter()
      :   section(), pattern(), status(),
          priority(), reducedview(), preset()
   {}

   void setName(std::string name);
   std::string getName();

   bool read(Configuration &conf, std::string key);
   bool write(std::ofstream &out);
   bool apply(RPackage *package);
   void reset();

   RSectionPackageFilter section;
   RPatternPackageFilter pattern;
   RStatusPackageFilter status;
   RPriorityPackageFilter priority;
   RReducedViewPackageFilter reducedview;
   RFilePackageFilter file;

   bool preset;

   protected:

   std::string name;
};
