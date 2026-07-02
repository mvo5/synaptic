/* raptoptions.h - configuration handling
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

#ifndef _RAPTOPTIONS_H_
#define _RAPTOPTIONS_H_

#include "config.h"

#include <map>
#include <string>
#include <apt-pkg/configuration.h>

class RAPTOptions {
 public:

   class packageOptions {
    public:
      packageOptions()
    :
      isLocked(false), isOrphaned(false), isNew(false),
      isDebconf(false) {
      }
      bool isLocked;
      bool isOrphaned;
      bool isNew;
      bool isDebconf;
   };

   bool store();
   bool restore();

   bool getPackageLock(const char *package);
   void setPackageLock(const char *package, bool lock);

   bool getPackageDebconf(const char *package);
   void setPackageDebconf(const char *package, bool flag = true);
   void rereadDebconf();

   bool getPackageOrphaned(const char *package);
   void setPackageOrphaned(const char *package, bool flag = true);
   void rereadOrphaned();

   bool getPackageNew(const char *package);
   void setPackageNew(const char *package, bool flag = true);
   void forgetNewPackages();

   bool getFlag(const char *key);
   std::string getString(const char *key);

   void setFlag(const char *key, bool value);
   void setString(const char *key, std::string value);

 private:
   std::map<std::string, packageOptions> _packageOptions;
   std::map<std::string, std::string> _options;
};

extern RAPTOptions *_roptions;

typedef std::map<std::string, RAPTOptions::packageOptions>::iterator packageOptionsIter;

std::ostream &operator<<(std::ostream &os, const RAPTOptions::packageOptions &);
std::istream &operator>>(std::istream &is, RAPTOptions::packageOptions &o);

#endif
