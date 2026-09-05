/* rdeb822file.cc - edit a deb822 sources file in place
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

#include "config.h" // IWYU pragma: associated

#include "rdeb822file.h"

#include "i18n.h"
#include "rsources.h"

#include <algorithm>
#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/tagfile.h>
#include <fstream>
#include <sstream>
#include <strings.h>

using namespace std;

RDeb822File::RDeb822File(const string &Path) : _path(Path), _eol("\n")
{
}

bool RDeb822File::Read(const StanzaVisitor &Visit)
{
   {
      ifstream In(_path, ios::binary);
      if (!In)
         return _error->Errno("open", _("Can't read %s"), _path.c_str());
      ostringstream Buf;
      Buf << In.rdbuf();
      _text = Buf.str();
   }
   _original = _text;
   _stanzas.clear();

   size_t FirstNewline = _text.find('\n');
   if (FirstNewline != string::npos && FirstNewline > 0 &&
       _text[FirstNewline - 1] == '\r')
      _eol = "\r\n";

   FileFd Fd;
   if (Fd.Open(_path, FileFd::ReadOnly) == false)
      return _error->Error(_("Can't read %s"), _path.c_str());
   pkgTagFile Tags(&Fd, pkgTagFile::SUPPORT_COMMENTS);
   pkgTagSection Sec;
   size_t Before = Tags.Offset();
   for (unsigned Index = 0; Tags.Step(Sec); Index++) {
      // comments after a stanza are attributed to it, up to the next field
      size_t After = min<size_t>(Tags.Offset(), _text.size());
      _stanzas.push_back({Before, After});
      if (Visit)
         Visit(Sec, Index);
      Before = After;
   }
   return true;
}

unsigned RDeb822File::StanzaCount() const
{
   return _stanzas.size();
}

enum LineKind { Blank, Comment, Continuation, FieldStart };

// The line classes pkgTagSection::Scan distinguishes. A line of only spaces
// is a continuation to apt, not a separator, so it is one here too.
static LineKind ClassifyLine(const string &Text, size_t Start, size_t End)
{
   if (Start == End)
      return Blank;
   const char c = Text[Start];
   if (c == '#')
      return Comment;
   if (c == '\n' || c == '\r')
      return Blank;
   if (c == ' ' || c == '\t')
      return Continuation;
   return FieldStart;
}

static string Trimmed(const string &Text, size_t Start, size_t End)
{
   while (Start < End && isspace(Text[Start]))
      Start++;
   while (End > Start && isspace(Text[End - 1]))
      End--;
   return Text.substr(Start, End - Start);
}

vector<RDeb822File::Span> RDeb822File::StanzaLines(unsigned Index) const
{
   vector<Span> Lines;
   size_t Pos = _stanzas[Index].Start;
   const size_t End = _stanzas[Index].End;
   while (Pos < End) {
      size_t Newline = _text.find('\n', Pos);
      size_t LineEnd = (Newline == string::npos || Newline >= End) ? End : Newline + 1;
      Lines.push_back({Pos, LineEnd});
      Pos = LineEnd;
   }
   return Lines;
}

// The last occurrence wins, as it does for pkgTagSection::Find(). Comment
// lines inside a field's continuation block are skipped, not terminating.
RDeb822File::Field RDeb822File::FindField(unsigned Index, const string &Key) const
{
   Field Result;
   bool Collecting = false;
   for (const Span &L : StanzaLines(Index)) {
      switch (ClassifyLine(_text, L.Start, L.End)) {
      case Comment:
         break;
      case Blank:
         Collecting = false;
         break;
      case Continuation:
         if (Collecting) {
            Result.Lines.push_back(L);
            Result.Value += " " + Trimmed(_text, L.Start, L.End);
         }
         break;
      case FieldStart: {
         size_t Colon = _text.find(':', L.Start);
         if (Colon == string::npos || Colon >= L.End) {
            Collecting = false;
            break;
         }
         string LineKey = Trimmed(_text, L.Start, Colon);
         Collecting = strcasecmp(LineKey.c_str(), Key.c_str()) == 0;
         if (Collecting) {
            Result = Field();
            Result.Found = true;
            Result.Key = LineKey;
            Result.Lines.push_back(L);
            Result.Value = Trimmed(_text, Colon + 1, L.End);
         }
         break;
      }
      }
   }
   if (Result.Found)
      Result.Value = Trimmed(Result.Value, 0, Result.Value.size());
   return Result;
}

bool RDeb822File::SetField(unsigned Index, const string &Key, const string &Value)
{
   if (Index >= _stanzas.size())
      return false;

   const Field Old = FindField(Index, Key);
   if (Old.Found && Old.Value == Value)
      return false;

   size_t InsertAt;
   size_t Removed = 0;
   string Line;
   if (Old.Found) {
      InsertAt = Old.Lines.front().Start;
      Line = Old.Key + ": " + Value + _eol;
      // back to front, so the earlier offsets stay valid
      for (auto L = Old.Lines.rbegin(); L != Old.Lines.rend(); ++L) {
         _text.erase(L->Start, L->End - L->Start);
         Removed += L->End - L->Start;
      }
   } else {
      // in front of the first field, after any comments introducing the stanza
      InsertAt = _stanzas[Index].End;
      for (const Span &L : StanzaLines(Index)) {
         if (ClassifyLine(_text, L.Start, L.End) == FieldStart) {
            InsertAt = L.Start;
            break;
         }
      }
      Line = Key + ": " + Value + _eol;
   }
   _text.insert(InsertAt, Line);

   const long Delta = static_cast<long>(Line.size()) - static_cast<long>(Removed);
   _stanzas[Index].End += Delta;
   for (unsigned I = Index + 1; I < _stanzas.size(); I++) {
      _stanzas[I].Start += Delta;
      _stanzas[I].End += Delta;
   }
   return true;
}

bool RDeb822File::SetEnabled(unsigned Index, bool Enabled)
{
   if (Index >= _stanzas.size())
      return false;
   const Field Old = FindField(Index, "Enabled");
   // apt: only an explicit false-ish value disables
   const bool Current =
      !(Old.Found && Old.Value.empty() == false && StringToBool(Old.Value) == false);
   if (Current == Enabled)
      return false;
   return SetField(Index, "Enabled", Enabled ? "yes" : "no");
}

bool RDeb822File::Changed() const
{
   return _text != _original;
}

bool RDeb822File::Write()
{
   if (Changed() == false)
      return true;
   if (WriteSourcesFile(_path, _text) == false)
      return false;
   _original = _text;
   return true;
}
