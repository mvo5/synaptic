/* rdeb822file.h - edit a deb822 sources file in place
 *
 * Copyright (c) 2026 Synaptic development team
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

#include "config.h" // IWYU pragma: associated

#include <apt-pkg/tagfile.h>
#include <functional>
#include <string>
#include <vector>

// A deb822 file whose stanzas are changed one field at a time inside the
// original text, so comments, unknown fields and formatting survive a save
// byte for byte. Parsing is apt's, via pkgTagFile; each stanza's byte range
// comes from pkgTagFile::Offset(), which stays a real file offset even when
// comments are stripped.
class RDeb822File
{
 public:
   explicit RDeb822File(const std::string &Path);

   // Visit gets apt's view of every stanza, in file order
   using StanzaVisitor =
      std::function<void(const pkgTagSection &Sec, unsigned Index)>;
   bool Read(const StanzaVisitor &Visit = StanzaVisitor());

   unsigned StanzaCount() const;

   // Replaces the field's line(s) with "Key: Value", or inserts that in front
   // of the stanza's first field. Returns true if the text changed.
   bool SetField(unsigned Index, const std::string &Key,
                 const std::string &Value);
   // An absent Enabled field means enabled, as it does for apt
   bool SetEnabled(unsigned Index, bool Enabled);

   bool Changed() const;
   // Through WriteSourcesFile(): atomic, with a one-time .bak; a no-op when
   // nothing changed
   bool Write();

 private:
   struct Span
   {
      size_t Start;
      size_t End;
   };
   struct Field
   {
      bool Found = false;
      std::vector<Span> Lines; // first line plus continuation lines
      std::string Key;         // as spelled in the file
      std::string Value;       // continuation lines joined with a space
   };

   std::vector<Span> StanzaLines(unsigned Index) const;
   Field FindField(unsigned Index, const std::string &Key) const;

   std::string _path;
   std::string _text;
   std::string _original;
   std::string _eol;
   std::vector<Span> _stanzas;
};
