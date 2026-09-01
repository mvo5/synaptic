#include "config.h" // IWYU pragma: associated

#include "rsources.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/init.h>
#include <apt-pkg/pkgsystem.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <sys/stat.h>

using namespace std;

static int failures = 0;

// mvo5's own file: Types carries BOTH deb and deb-src.
static const char *BOTH_TYPES =
   "Types: deb deb-src\n"
   "URIs: http://ftp.de.debian.org/debian/\n"
   "Suites: trixie\n"
   "Components: main non-free-firmware\n";

// The dialog fills TYPE_COLUMN from SourceRecord::GetTypeLabel() and DoEdit()
// parses that same column back into the record, so the two must agree or a
// round trip through the dialog loses a type. This test links the real
// GetTypeLabel() out of libsynaptic rather than reimplementing it, so a
// regression in that function fails here.

// Edit side, rgrepositorywin.cc DoEdit(): substring test on that column.
static unsigned reparsed_type(const string &type_val)
{
   unsigned t = 0;
   if (type_val.find("deb") != string::npos)
      t |= SourcesList::Deb;
   if (type_val.find("deb-src") != string::npos)
      t |= SourcesList::DebSrc;
   return t;
}

int main(int argc, char *argv[])
{
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);

   // Honour TMPDIR like the other tests here, and check mkdtemp: dereferencing
   // a null root below would crash instead of reporting a failure.
   string tmplstr = string(getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp") +
                    "/synaptic-types-XXXXXX";
   vector<char> tmpl(tmplstr.begin(), tmplstr.end());
   tmpl.push_back('\0');
   const char *root = mkdtemp(tmpl.data());
   if (root == nullptr) {
      cerr << "FAIL: could not create a temp dir" << endl;
      return 1;
   }
   string parts = string(root) + "/sources.list.d";
   mkdir(parts.c_str(), 0755);
   {
      ofstream out((parts + "/debian.sources").c_str(), ios::binary);
      out << BOTH_TYPES;
   }
   _config->Set("Dir::Etc::sourceparts", parts);
   _config->Set("Dir::Etc::sourcelist", string(root) + "/sources.list");

   SourcesList lst;
   lst.ReadSources();

   for (list<SourcesList::SourceRecord *>::const_iterator I =
           lst.SourceRecords.begin();
        I != lst.SourceRecords.end(); ++I) {
      if ((*I)->Type & SourcesList::Comment)
         continue;

      const bool had_deb = ((*I)->Type & SourcesList::Deb) != 0;
      const bool had_src = ((*I)->Type & SourcesList::DebSrc) != 0;
      if (!(had_deb && had_src)) {
         cerr << "FAIL setup: the record should carry deb AND deb-src" << endl;
         failures++;
         break;
      }

      // What the dialog puts in the column, then reads back out when the user
      // edits the row.
      const string shown = (*I)->GetTypeLabel();
      const unsigned back = reparsed_type(shown);

      const bool keeps_deb = (back & SourcesList::Deb) != 0;
      const bool keeps_src = (back & SourcesList::DebSrc) != 0;

      cerr << "     stored in TYPE_COLUMN: \"" << shown << "\"" << endl;
      cerr << "     re-parsed on edit    : deb=" << keeps_deb
           << " deb-src=" << keeps_src << endl;

      if (!keeps_deb || !keeps_src) {
         cerr << "FAIL types-survive-edit: a source with \"Types: deb deb-src\" "
                 "loses deb-src when the row is edited" << endl;
         failures++;
      } else {
         cerr << "ok   types-survive-edit: both types survive the round trip"
              << endl;
      }
      break;
   }

   if (failures > 0) {
      cerr << failures << " check(s) failed" << endl;
      return 1;
   }
   cerr << "all checks passed" << endl;
   return 0;
}
