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
#define _RPACKAGEFILTER_H_

#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <apt-pkg/tagfile.h>

#ifdef HAVE_DEBTAGS
#include <TagcollParser.h>
#include <StdioParserInput.h>
#include <SmartHierarchy.h>
#include <TagcollBuilder.h>
#include <HandleMaker.h>
#include <TagCollection.h>
#endif

#include <regex.h>

#include "rpackagelister.h"

using namespace std;

class RPackage;
class RPackageLister;

class Configuration;


class RPackageFilter {
protected:
   RPackageLister *_lister;

public:
   virtual const char* type() = 0;

   virtual bool filter(RPackage *pkg) = 0;
   virtual void reset() = 0;

   virtual bool read(Configuration &conf, string key) = 0;
   virtual bool write(ofstream &out, string pad) = 0;

   RPackageFilter() {};
   RPackageFilter(RPackageLister *lister) : _lister(lister) {};
   virtual ~RPackageFilter() {};
};


extern const char *RPFSection;

class RSectionPackageFilter : public RPackageFilter {
   vector<string> _groups;
   bool _inclusive; // include or exclude the packages
   
public:
   RSectionPackageFilter(RPackageLister *lister)
	   : RPackageFilter(lister), _inclusive(false) {};
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
    Maintainer,
    Depends,
    Provides,
    Conflicts,
    Replaces, // (or obsoletes)
    WeakDepends, // suggests et al
    RDepends // reverse depends
  } DepType;
   static char *TypeName[];

protected:
   struct Pattern {
       DepType where;
       string pattern;
       bool exclusive;
       vector<regex_t*> regexps;
   };
   vector<Pattern> _patterns;
   
public:
   RPatternPackageFilter(RPackageLister *lister)
	   : RPackageFilter(lister) {};
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
protected:
   int _status;

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
       NotInstallable = 1<<11  // the package is not aviailable in repository
   };

   RStatusPackageFilter(RPackageLister *lister)
	   : RPackageFilter(lister), _status(~0) {};
   inline virtual void reset() { _status = ~0; };

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
   RPriorityPackageFilter(RPackageLister *lister)
	   : RPackageFilter(lister) {};
   inline virtual void reset() {};
   
   inline virtual const char *type() { return RPFPriority; };
   
   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, string key);
   virtual bool write(ofstream &out, string pad);
};


extern const char *RPFReducedView;

class RReducedViewPackageFilter : public RPackageFilter {   
protected:
   bool _enabled;

   set<string> _hide;
   vector<string> _hide_wildcard;
   vector<regex_t*> _hide_regex;

   void addFile(string FileName);

public:
   RReducedViewPackageFilter(RPackageLister *lister)
	   : RPackageFilter(lister), _enabled(false) {};
   ~RReducedViewPackageFilter();

   inline virtual void reset() { _hide.clear(); };
   
   inline virtual const char *type() { return RPFReducedView; };

   virtual bool filter(RPackage *pkg);
   virtual bool read(Configuration &conf, string key);
   virtual bool write(ofstream &out, string pad);

   void enable() { _enabled = true; };
   void disable() { _enabled = false; };
};

#ifdef HAVE_DEBTAGS
extern const char *RPFTags;

class RTagPackageFilter : public RPackageFilter, public TagcollConsumer<int, string> {
    protected:
    OpSet<string> included;
    OpSet<string> excluded;
    set<RPackage*> selected;
    bool dirty;

    void rebuildSelected()
    {
	selected.clear();
	_lister->_coll.output(*this);
	dirty = false;
    }
    
    public:
    RTagPackageFilter(RPackageLister *lister) 
	: RPackageFilter(lister), dirty(true)  {};

    inline virtual void reset() 
    { 
	included.clear(); 
	excluded.clear();
	selected.clear();
	dirty = false;
    };
    inline virtual const char *type() { return RPFTags; };
   
    void include(string tag) { included.insert(tag); dirty = true; }
    void exclude(string tag) { excluded.insert(tag); dirty = true; }

    OpSet<string>& getIncluded() { return included; };
    OpSet<string>& getExcluded() { return excluded; };

    virtual bool filter(RPackage *pkg)
    {
	// if we do not use tags, ignore them
	if(included.empty() && excluded.empty())
	    return true;

	if (dirty)
	    rebuildSelected();

	return selected.find(pkg) != selected.end();
    }

    virtual bool read(Configuration &conf, string key);
    virtual bool write(ofstream &out, string pad);

    virtual void consume(const int& item) throw ()
    {
	if (included.empty())
	    selected.insert(_lister->_hmaker->getItem(item));
    }
    virtual void consume(const int& item, const OpSet<string>& tags) throw ()
    {
	OpSet<string> inters = tags ^ excluded;
	if (inters.empty() && tags.contains(included))
	    selected.insert(_lister->_hmaker->getItem(item));
    }
	
    virtual void consume(const OpSet<int>& items) throw ()
    {
	if (included.empty())
	    for (OpSet<int>::const_iterator i = items.begin();
		     i != items.end(); i++)
		selected.insert(_lister->_hmaker->getItem(*i));
    }

    virtual void consume(const OpSet<int>& items, const OpSet<string>& tags) throw ()
    {
	OpSet<string> inters = tags ^ excluded;
	if (inters.empty() && tags.contains(included))
	    for (OpSet<int>::const_iterator i = items.begin();
		     i != items.end(); i++)
		selected.insert(_lister->_hmaker->getItem(*i));
    }
};
#endif


struct RFilterView {
    RFilterView() : viewMode(0),expandMode(0) {};
    int viewMode;
    int expandMode;
};

struct RFilter {
  public:
    RFilter(RPackageLister *lister)
       : section(lister), pattern(lister), status(lister),
	 priority(lister), reducedview(lister),
#ifdef HAVE_DEBTAGS
	 tags(lister),
#endif
	 preset(false)
    {};

    void setName(string name);
    string getName();

    void setViewMode(RFilterView view) {_view=view;};
    RFilterView getViewMode() {return _view;};

    bool read(Configuration &conf, string key);
    bool write(ofstream &out);
    bool apply(RPackage *package);
    void reset();

    RSectionPackageFilter section;
    RPatternPackageFilter pattern;
    RStatusPackageFilter status;
    RPriorityPackageFilter priority;
    RReducedViewPackageFilter reducedview;
#ifdef HAVE_DEBTAGS
    RTagPackageFilter tags;
#endif
    bool preset;

  private:
    string name;
    RFilterView _view;
};


#endif

// vim:sts=3:sw=3
