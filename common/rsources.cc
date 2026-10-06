/* rsource.cc - access the sources.list file
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

#include "config.h" // IWYU pragma: associated

#include "rsources.h"

#include "i18n.h"
#include "rdeb822file.h"

#include <algorithm>
#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/tagfile.h>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace std;

SourcesList::~SourcesList()
{
   for (list<SourceRecord *>::iterator it = SourceRecords.begin();
        it != SourceRecords.end();
        it++)
      delete *it;
   for (list<VendorRecord *>::iterator it = VendorRecords.begin();
        it != VendorRecords.end();
        it++)
      delete *it;
}

SourcesList::SourceRecord *SourcesList::AddSourceNode(SourceRecord &rec)
{
   SourceRecord *newrec = new SourceRecord;
   *newrec = rec;
   SourceRecords.push_back(newrec);

   return newrec;
}

static string ExpandArch(const string &S)
{
   return SubstVar(S, "$(ARCH)", _config->Find("APT::Architecture"));
}

bool SourcesList::ReadSourcePart(string listpath)
{
   // cout << "SourcesList::ReadSourcePart() "<< listpath  << endl;
   char buf[512];
   const char *p;
   ifstream ifs(listpath.c_str(), ios::in);
   bool record_ok = true;

   // cannot open file
   if (!ifs != 0)
      return _error->Error(_("Can't read %s"), listpath.c_str());

   while (ifs.eof() == false) {
      p = buf;
      SourceRecord rec;
      string Type;
      string Section;
      string VURI;

      ifs.getline(buf, sizeof(buf));

      rec.SourceFile = listpath;
      while (isspace(*p))
         p++;
      if (*p == '#') {
         rec.Type = Disabled;
         p++;
         while (isspace(*p))
            p++;
      }

      if (*p == '\r' || *p == '\n' || *p == 0) {
         rec.Type = Comment;
         rec.Comment = p;

         AddSourceNode(rec);
         continue;
      }

      bool Failed = true;
      if (ParseQuoteWord(p, Type) == true && rec.SetType(Type) == true &&
          ParseQuoteWord(p, VURI) == true) {
         if (VURI[0] == '[') {
            rec.VendorID = VURI.substr(1, VURI.length() - 2);
            if (ParseQuoteWord(p, VURI) == true && rec.SetURI(VURI) == true)
               Failed = false;
         } else if (rec.SetURI(VURI) == true) {
            Failed = false;
         }
         if (Failed == false && ParseQuoteWord(p, rec.Dist) == false)
            Failed = true;
      }

      if (Failed == true) {
         if (rec.Type == Disabled) {
            // treat as a comment field
            rec.Type = Comment;
            rec.Comment = buf;
         } else {
            // syntax error on line
            rec.Type = Comment;
            string s = "#" + string(buf);
            rec.Comment = s;
            record_ok = false;
            // return _error->Error(_("Syntax error in line %s"), buf);
         }
      }
#ifndef HAVE_RPM
      // check for absolute dist
      if (rec.Dist.empty() == false && rec.Dist[rec.Dist.size() - 1] == '/') {
         // make sure there's no section
         if (ParseQuoteWord(p, Section) == true)
            return _error->Error(_("Syntax error in line %s"), buf);

         rec.Dist = ExpandArch(rec.Dist);

         AddSourceNode(rec);
         continue;
      }
#endif

      const char *tmp = p;
      rec.NumSections = 0;
      while (ParseQuoteWord(p, Section) == true)
         rec.NumSections++;
      if (rec.NumSections > 0) {
         p = tmp;
         rec.Sections = new string[rec.NumSections];
         rec.NumSections = 0;
         while (ParseQuoteWord(p, Section) == true) {
            // comments inside the record are preserved
            if (Section[0] == '#') {
               SourceRecord rec;
               string s = Section + string(p);
               rec.Type = Comment;
               rec.Comment = s;
               rec.SourceFile = listpath;
               AddSourceNode(rec);
               break;
            } else {
               rec.Sections[rec.NumSections++] = Section;
            }
         }
      }
      AddSourceNode(rec);
   }

   ifs.close();
   return record_ok;
}

// copy libapt:sourcelist.cc FindMultiValue()
// TODO: expose in libapt as pkgTagSection::FindMultiValue() and drop this
static vector<string> FindMultiValue(const pkgTagSection &Sec,
                                     const char *Field)
{
   string value = Sec.FindS(Field);
   replace_if(value.begin(), value.end(), isspace_ascii, ' ');
   vector<string> parts = VectorizeString(value, ' ');
   parts.erase(remove_if(parts.begin(),
                         parts.end(),
                         [](const string &s) { return s.empty(); }),
               parts.end());
   return parts;
}

// What a deb822 stanza contributes to a SourceRecord, with apt's rules for
// Types, Enabled and multi-value fields. Ok is false for a stanza apt would
// reject.
struct StanzaFields
{
   bool Ok = false;
   unsigned int Type = 0;
   std::string URI;
   std::string Dist;
   std::vector<std::string> Components;
};

static StanzaFields ParseStanza(const pkgTagSection &Sec)
{
   StanzaFields F;
   for (const string &T : FindMultiValue(Sec, "Types")) {
      if (T == "deb")
         F.Type |= SourcesList::Deb;
      else if (T == "deb-src")
         F.Type |= SourcesList::DebSrc;
      else
         return F;
   }
   if (F.Type == 0)
      return F;

   // as in apt's ParseStanza(): absent means enabled, and so does an
   // unparseable value; StringToBool("") alone would say disabled
   string Enabled = Sec.FindS("Enabled");
   if (Enabled.empty() == false && StringToBool(Enabled) == false)
      F.Type |= SourcesList::Disabled;

   // expanded like SetURI() does it, but without its trailing slash, which
   // only makes sense for a single URI
   vector<string> uris = FindMultiValue(Sec, "URIs");
   for (string &uri : uris)
      uri = ExpandArch(uri);
   F.URI = APT::String::Join(uris, " ");
   // apt expands $(ARCH) in deb822 suites too
   vector<string> suites = FindMultiValue(Sec, "Suites");
   for (string &suite : suites)
      suite = ExpandArch(suite);
   F.Dist = APT::String::Join(suites, " ");

   F.Components = FindMultiValue(Sec, "Components");
   F.Ok = true;
   return F;
}

static vector<string> SectionsOf(const SourcesList::SourceRecord &rec)
{
   return vector<string>(rec.Sections, rec.Sections + rec.NumSections);
}

static string TypesField(unsigned int Type)
{
   vector<string> types;
   if (Type & SourcesList::Deb)
      types.push_back("deb");
   if (Type & SourcesList::DebSrc)
      types.push_back("deb-src");
   return APT::String::Join(types, " ");
}

bool SourcesList::ReadDeb822SourcePart(string path)
{
   bool record_ok = true;
   RDeb822File File(path);
   bool read_ok = File.Read([&](const pkgTagSection &Sec, unsigned Index) {
      const StanzaFields F = ParseStanza(Sec);
      if (F.Ok == false) {
         record_ok = false;
         return;
      }
      SourceRecord rec;
      rec.SourceFile = path;
      rec.Format = Deb822;
      rec.StanzaIndex = Index;
      rec.Type = F.Type;
      rec.URI = F.URI;
      rec.Dist = F.Dist;
      rec.NumSections = F.Components.size();
      rec.Sections = new string[rec.NumSections];
      for (unsigned short i = 0; i < rec.NumSections; i++)
         rec.Sections[i] = F.Components[i];
      AddSourceNode(rec);
   });
   return read_ok && record_ok;
}

bool SourcesList::ReadSourceDir(string Dir)
{
   bool ok = true;
   for (const string &File :
        GetListOfFilesInDir(Dir,
                            vector<string>{"list", "sources"},
                            /* SortList */ true)) {
      if (flExtension(File) == "sources")
         ok = ReadDeb822SourcePart(File) && ok;
      else
         ok = ReadSourcePart(File) && ok;
   }
   return ok;
}

bool SourcesList::ReadSources()
{
   // cout << "SourcesList::ReadSources() " << endl;

   // libapt queued notices about odd files in sources.list.d when it read them
   // at startup and they were shown then; do not queue them again on success
   _error->PushToStack();
   bool Res = true;

   string Parts = _config->FindDir("Dir::Etc::sourceparts");
   if (FileExists(Parts) == true)
      Res &= ReadSourceDir(Parts);
   string Main = _config->FindFile("Dir::Etc::sourcelist");
   if (FileExists(Main) == true)
      Res &= ReadSourcePart(Main);

   if (Res)
      _error->RevertToStack();
   else
      _error->MergeWithStack();
   return Res;
}

SourcesList::SourceRecord *SourcesList::AddEmptySource()
{
   SourceRecord rec;
#ifdef HAVE_RPM
   rec.Type = Rpm;
#else
   rec.Type = Deb;
#endif
   rec.VendorID = "";
   rec.SourceFile = _config->FindFile("Dir::Etc::sourcelist");
   rec.Dist = "";
   rec.NumSections = 0;
   return AddSourceNode(rec);
}

SourcesList::SourceRecord *SourcesList::AddSource(RecType Type,
                                                  string VendorID,
                                                  string URI,
                                                  string Dist,
                                                  string *Sections,
                                                  unsigned short count,
                                                  string SourceFile)
{
   SourceRecord rec;
   rec.Type = Type;
   rec.VendorID = VendorID;
   rec.SourceFile = SourceFile;

   if (rec.SetURI(URI) == false) {
      return NULL;
   }
   rec.Dist = Dist;
   rec.NumSections = count;
   rec.Sections = new string[count];
   for (unsigned int i = 0; i < count; i++)
      rec.Sections[i] = Sections[i];

   return AddSourceNode(rec);
}

void SourcesList::RemoveSource(SourceRecord *&rec)
{
   if (rec->Format == Deb822)
      _removedStanzas.push_back({rec->SourceFile, rec->StanzaIndex});
   SourceRecords.remove(rec);
   delete rec;
   rec = 0;
}

void SourcesList::SwapSources(SourceRecord *&rec_one, SourceRecord *&rec_two)
{
   list<SourceRecord *>::iterator rec_p;
   list<SourceRecord *>::iterator rec_n;

   rec_p = find(SourceRecords.begin(), SourceRecords.end(), rec_one);
   rec_n = find(SourceRecords.begin(), SourceRecords.end(), rec_two);

   SourceRecords.insert(rec_p, rec_two);
   SourceRecords.erase(rec_n);
}

// deb822 files are edited in place, field by field, because a SourceRecord
// holds only what the dialog shows and regenerating the file from it would
// drop Signed-By, unknown fields and comments. Only Enabled is written so far.
bool SourcesList::UpdateDeb822Sources()
{
   map<string, vector<SourceRecord *>> byFile;
   for (SourceRecord *rec : SourceRecords)
      if (rec->Format == Deb822)
         byFile[rec->SourceFile].push_back(rec);
   for (const auto &removed : _removedStanzas)
      byFile[removed.first];

   for (const auto &entry : byFile) {
      RDeb822File File(entry.first);
      map<unsigned, StanzaFields> onDisk;
      if (File.Read([&](const pkgTagSection &Sec, unsigned Index) {
             onDisk[Index] = ParseStanza(Sec);
          }) == false)
         return false;

      for (const SourceRecord *rec : entry.second) {
         const unsigned Index = rec->StanzaIndex;
         auto it = onDisk.find(Index);
         if (it == onDisk.end() || it->second.Ok == false)
            continue;
         const StanzaFields &was = it->second;

         // Only fields the user changed are written. The record holds $(ARCH)
         // expanded for display, so writing an unchanged field back would
         // replace the variable in the file with its expansion.
         File.SetEnabled(Index, (rec->Type & Disabled) == 0);
         const unsigned int TypeMask = Deb | DebSrc;
         if ((rec->Type & TypeMask) != (was.Type & TypeMask))
            File.SetField(Index, "Types", TypesField(rec->Type));
         if (rec->URI != was.URI)
            File.SetField(Index, "URIs", rec->URI);
         if (rec->Dist != was.Dist)
            File.SetField(Index, "Suites", rec->Dist);
         if (SectionsOf(*rec) != was.Components) {
            if (rec->NumSections == 0)
               File.RemoveField(Index, "Components");
            else
               File.SetField(Index, "Components",
                             APT::String::Join(SectionsOf(*rec), " "));
         }
      }
      for (const auto &removed : _removedStanzas)
         if (removed.first == entry.first)
            File.RemoveStanza(removed.second);
      if (File.Write() == false)
         return false;
   }
   _removedStanzas.clear();
   return true;
}

bool SourcesList::UpdateSources()
{
   if (UpdateDeb822Sources() == false)
      return false;

   list<string> filenames;
   for (list<SourceRecord *>::iterator it = SourceRecords.begin();
        it != SourceRecords.end();
        it++) {
      if ((*it)->SourceFile == "" || (*it)->Format == Deb822)
         continue;
      filenames.push_front((*it)->SourceFile);
   }
   filenames.sort();
   filenames.unique();

   for (list<string>::iterator fi = filenames.begin(); fi != filenames.end();
        fi++) {
      ostringstream ofs;

      for (list<SourceRecord *>::iterator it = SourceRecords.begin();
           it != SourceRecords.end();
           it++) {
         if ((*fi) != (*it)->SourceFile)
            continue;
         string S;
         if (((*it)->Type & Comment) != 0) {
            S = (*it)->Comment;
         } else if ((*it)->URI.empty() || (*it)->Dist.empty()) {
            continue;
         } else {
            if (((*it)->Type & Disabled) != 0)
               S = "# ";

            S += (*it)->GetType() + " ";

            if ((*it)->VendorID.empty() == false)
               S += "[" + (*it)->VendorID + "] ";

            S += (*it)->URI + " ";
            S += (*it)->Dist + " ";

            for (unsigned int J = 0; J < (*it)->NumSections; J++)
               S += (*it)->Sections[J] + " ";
         }
         ofs << S << endl;
      }
      if (WriteSourcesFile(*fi, ofs.str()) == false)
         return false;
   }
   return true;
}

bool SourcesList::SourceRecord::SetType(string S)
{
   if (S == "deb")
      Type |= Deb;
   else if (S == "deb-src")
      Type |= DebSrc;
   else if (S == "rpm")
      Type |= Rpm;
   else if (S == "rpm-src")
      Type |= RpmSrc;
   else if (S == "rpm-dir")
      Type |= RpmDir;
   else if (S == "rpm-src-dir")
      Type |= RpmSrcDir;
   else if (S == "repomd")
      Type |= Repomd;
   else if (S == "repomd-src")
      Type |= RepomdSrc;
   else
      return false;
   // cout << S << " settype " << (Type | Repomd) << endl;
   return true;
}

string SourcesList::SourceRecord::TypeLabel() const
{
   if ((Type & Deb) != 0 && (Type & DebSrc) != 0)
      return "deb deb-src";
   return GetType();
}

string SourcesList::SourceRecord::GetType() const
{
   if ((Type & Deb) != 0)
      return "deb";
   else if ((Type & DebSrc) != 0)
      return "deb-src";
   else if ((Type & Rpm) != 0)
      return "rpm";
   else if ((Type & RpmSrc) != 0)
      return "rpm-src";
   else if ((Type & RpmDir) != 0)
      return "rpm-dir";
   else if ((Type & RpmSrcDir) != 0)
      return "rpm-src-dir";
   else if ((Type & Repomd) != 0)
      return "repomd";
   else if ((Type & RepomdSrc) != 0)
      return "repomd-src";
   // cout << "type " << (Type & Repomd) << endl;
   return "unknown";
}

bool SourcesList::SourceRecord::SetURI(string S)
{
   if (S.empty() == true)
      return false;
   if (S.find(':') == string::npos)
      return false;

   URI = ExpandArch(S);

   // append a / to the end if one is not already there
   if (URI[URI.size() - 1] != '/')
      URI += '/';

   return true;
}

SourcesList::SourceRecord &SourcesList::SourceRecord::operator=(
   const SourceRecord &rhs)
{
   // Needed for a proper deep copy of the record; uses the string operator= to
   // properly copy the strings
   Type = rhs.Type;
   VendorID = rhs.VendorID;
   URI = rhs.URI;
   Dist = rhs.Dist;
   Sections = new string[rhs.NumSections];
   for (unsigned int I = 0; I < rhs.NumSections; I++)
      Sections[I] = rhs.Sections[I];
   NumSections = rhs.NumSections;
   Comment = rhs.Comment;
   SourceFile = rhs.SourceFile;
   Format = rhs.Format;
   StanzaIndex = rhs.StanzaIndex;

   return *this;
}

SourcesList::VendorRecord *SourcesList::AddVendorNode(VendorRecord &rec)
{
   VendorRecord *newrec = new VendorRecord;
   *newrec = rec;
   VendorRecords.push_back(newrec);

   return newrec;
}

bool SourcesList::ReadVendors()
{
   Configuration Cnf;

   string CnfFile = _config->FindFile("Dir::Etc::vendorlist");
   if (FileExists(CnfFile) == true)
      if (ReadConfigFile(Cnf, CnfFile, true) == false)
         return false;

   for (list<VendorRecord *>::const_iterator I = VendorRecords.begin();
        I != VendorRecords.end();
        I++)
      delete *I;
   VendorRecords.clear();

   // Process 'simple-key' type sections
   const Configuration::Item *Top = Cnf.Tree("simple-key");
   for (Top = (Top == 0 ? 0 : Top->Child); Top != 0; Top = Top->Next) {
      Configuration Block(Top);
      VendorRecord Vendor;

      Vendor.VendorID = Top->Tag;
      Vendor.FingerPrint = Block.Find("Fingerprint");
      Vendor.Description = Block.Find("Name");

      char *buffer = new char[Vendor.FingerPrint.length() + 1];
      char *p = buffer;
      ;
      for (string::const_iterator I = Vendor.FingerPrint.begin();
           I != Vendor.FingerPrint.end();
           I++) {
         if (*I != ' ' && *I != '\t')
            *p++ = *I;
      }
      *p = 0;
      Vendor.FingerPrint = buffer;
      delete[] buffer;

      if (Vendor.FingerPrint.empty() == true ||
          Vendor.Description.empty() == true) {
         _error->Error(_("Vendor block %s is invalid"),
                       Vendor.VendorID.c_str());
         continue;
      }

      AddVendorNode(Vendor);
   }

   return !_error->PendingError();
}

SourcesList::VendorRecord *SourcesList::AddVendor(string VendorID,
                                                  string FingerPrint,
                                                  string Description)
{
   VendorRecord rec;
   rec.VendorID = VendorID;
   rec.FingerPrint = FingerPrint;
   rec.Description = Description;
   return AddVendorNode(rec);
}

bool SourcesList::UpdateVendors()
{
   ofstream ofs(_config->FindFile("Dir::Etc::vendorlist").c_str(), ios::out);
   if (!ofs != 0)
      return false;

   for (list<VendorRecord *>::iterator it = VendorRecords.begin();
        it != VendorRecords.end();
        it++) {
      ofs << "simple-key \"" << (*it)->VendorID << "\" {" << endl;
      ofs << "\tFingerPrint \"" << (*it)->FingerPrint << "\";" << endl;
      ofs << "\tName \"" << (*it)->Description << "\";" << endl;
      ofs << "}" << endl;
   }

   ofs.close();
   return true;
}


void SourcesList::RemoveVendor(VendorRecord *&rec)
{
   VendorRecords.remove(rec);
   delete rec;
   rec = 0;
}

static bool WriteAtomically(const string &Path, const string &Content,
                            mode_t Mode)
{
   // WriteAtomic writes a temporary file next to Path and renames it into
   // place on Close(), so an interrupted save never leaves a truncated file
   FileFd Out;
   if (Out.Open(Path, FileFd::WriteAtomic, Mode) == false ||
       Out.Write(Content.data(), Content.size()) == false ||
       Out.Close() == false)
      return _error->Error(_("Can't write %s"), Path.c_str());
   return true;
}

bool WriteSourcesFile(const string &Target, const string &Content)
{
   // rename-into-place would replace a symlink with a regular file, so edit
   // the file the link points to instead
   string Path = Target;
   if (char *Real = realpath(Target.c_str(), nullptr)) {
      Path = Real;
      free(Real);
   }

   string Old;
   mode_t Mode = 0644;
   const bool Exists = FileExists(Path);
   if (Exists) {
      ifstream In(Path, ios::binary);
      ostringstream Buf;
      Buf << In.rdbuf();
      Old = Buf.str();
      struct stat St;
      if (stat(Path.c_str(), &St) == 0)
         Mode = St.st_mode & 07777;
      if (Old == Content)
         return true;
   }

   // Written once only, so it keeps the file as it was before synaptic first
   // changed it. apt ignores *.bak in sources.list.d silently
   // (Dir::Ignore-Files-Silently).
   const string Backup = Path + ".bak";
   if (Exists && FileExists(Backup) == false &&
       WriteAtomically(Backup, Old, Mode) == false)
      return false;
   return WriteAtomically(Path, Content, Mode);
}

ostream &operator<<(ostream &os, const SourcesList::SourceRecord &rec)
{
   os << "Type: ";
   if ((rec.Type & SourcesList::Comment) != 0)
      os << "Comment ";
   if ((rec.Type & SourcesList::Disabled) != 0)
      os << "Disabled ";
   if ((rec.Type & SourcesList::Deb) != 0)
      os << "Deb";
   if ((rec.Type & SourcesList::DebSrc) != 0)
      os << "DebSrc";
   if ((rec.Type & SourcesList::Rpm) != 0)
      os << "Rpm";
   if ((rec.Type & SourcesList::RpmSrc) != 0)
      os << "RpmSrc";
   if ((rec.Type & SourcesList::RpmDir) != 0)
      os << "RpmDir";
   if ((rec.Type & SourcesList::RpmSrcDir) != 0)
      os << "RpmSrcDir";
   if ((rec.Type & SourcesList::Repomd) != 0)
      os << "Repomd";
   if ((rec.Type & SourcesList::RepomdSrc) != 0)
      os << "RepomdSrc";
   os << endl;
   os << "SourceFile: " << rec.SourceFile << endl;
   os << "VendorID: " << rec.VendorID << endl;
   os << "URI: " << rec.URI << endl;
   os << "Dist: " << rec.Dist << endl;
   os << "Section(s):" << endl;
#if 0
   for (unsigned int J = 0; J < rec.NumSections; J++) {
      cout << "\t" << rec.Sections[J] << endl;
   }
#endif
   os << endl;
   return os;
}

ostream &operator<<(ostream &os, const SourcesList::VendorRecord &rec)
{
   os << "VendorID: " << rec.VendorID << endl;
   os << "FingerPrint: " << rec.FingerPrint << endl;
   os << "Description: " << rec.Description << endl;
   return os;
}

// vim:sts=4:sw=4
