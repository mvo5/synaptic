/* rsource.h - access the sources.list file
 *
 * Copyright (c) 1999 Patrick Cole <z@amused.net>
 *           (c) 2002 Synaptic development team
 *
 * Author: Patrick Cole <z@amused.net>
 *         Michael Vogt <mvo@debian.org>
 *         Gustavo Niemeyer <niemeyer@conectiva.com>
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

#ifndef _RSOURCES_H
#define _RSOURCES_H

#include <string>
#include <list>
#include <vector>

using namespace std;

class SourcesList {
 public:
   enum RecType {
      Deb = 1 << 0,
      DebSrc = 1 << 1,
      Rpm = 1 << 2,
      RpmSrc = 1 << 3,
      Disabled = 1 << 4,
      Comment = 1 << 5,
      RpmDir = 1 << 6,
      RpmSrcDir = 1 << 7,
      Repomd = 1 << 8,
      RepomdSrc = 1 << 9
   };

   struct SourceRecord {
      unsigned int Type;
      string VendorID;
      string URI;
      string Dist;
      std::vector<string> Sections;
      string Comment;
      string SourceFile;

      bool SetType(string);
      string GetType();
      bool SetURI(string);

      SourceRecord():Type(0) {
      }
      ~SourceRecord() {
      }
      SourceRecord &operator=(const SourceRecord &);
   };

   struct VendorRecord {
      string VendorID;
      string FingerPrint;
      string Description;
   };

   list<SourceRecord *> SourceRecords;
   list<VendorRecord *> VendorRecords;

 private:
   SourceRecord *AddSourceNode(SourceRecord &);
   VendorRecord *AddVendorNode(VendorRecord &);
   bool ReadSourcePartOldStyle(string listpath);
   bool ReadSourcePartDeb822(string listpath);

 public:
   SourceRecord *AddSource(RecType Type,
                           string VendorID,
                           string URI,
                           string Dist,
                           std::vector<string> Sections,
                           string SourceFile);
   SourceRecord *AddEmptySource();
   void RemoveSource(SourceRecord *&);
   void SwapSources( SourceRecord *&, SourceRecord *& );
   bool ReadSourcePart(string listpath);
   bool ReadSourceDir(string Dir);
   bool ReadSources();
   bool UpdateSources();

   VendorRecord *AddVendor(string VendorID,
                           string FingerPrint, string Description);
   void RemoveVendor(VendorRecord *&);
   bool ReadVendors();
   bool UpdateVendors();

   SourcesList() {
   }
   ~SourcesList();
};

ostream &operator <<(ostream &, const SourcesList::SourceRecord &);

#endif
