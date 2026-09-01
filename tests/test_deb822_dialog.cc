// Drives the real repository-dialog population path headlessly.
//
// mvo5's blocking report on this feature was functional -- "I only get an
// empty window when I open the repository dialog" -- and that path was the
// one thing never exercised. This walks the same steps RGRepositoryEditor
// does when it fills the list: read the sources, then build the display
// string for each record exactly as the dialog does.
//
// It deliberately does NOT construct the GTK window (that needs the full
// builder resources and a real display); it exercises the data that decides
// whether the window comes up populated or empty, which is what the report
// is about.

#include "config.h" // IWYU pragma: associated

#include "rsources.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/init.h>
#include <apt-pkg/pkgsystem.h>
#include <cstdio>
#include <fstream>
#include <list>
#include <iostream>
#include <string>

using namespace std;

static int failures = 0;

// The exact type-display logic from rgrepositorywin.cc, so a Deb822 stanza
// carrying both types is shown as "deb, deb-src" rather than one of them.
static string type_display(SourcesList::SourceRecord *rec)
{
   bool is_deb = (rec->Type & SourcesList::Deb) != 0;
   bool is_debsrc = (rec->Type & SourcesList::DebSrc) != 0;
   if (is_deb && is_debsrc)
      return "deb, deb-src";
   if (is_deb)
      return "deb";
   if (is_debsrc)
      return "deb-src";
   return rec->GetType();
}

// Counts the rows the dialog would append, applying the same Comment skip.
static unsigned rows_for(const string &body, const string &name)
{
   string path = string(getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp") +
                 "/synaptic-dlg-" + name + ".sources";
   {
      ofstream out(path.c_str(), ios::binary);
      out << body;
   }

   SourcesList lst;
   lst.ReadDeb822SourcePart(path);

   unsigned rows = 0;
   for (list<SourcesList::SourceRecord *>::const_iterator it =
           lst.SourceRecords.begin();
        it != lst.SourceRecords.end(); it++) {
      if ((*it)->Type & SourcesList::Comment)
         continue;
      cerr << "     row: " << type_display(*it) << "  " << (*it)->URI << "  "
           << (*it)->Dist << endl;
      rows++;
   }

   remove(path.c_str());
   return rows;
}

static void check(const string &name, const string &body, unsigned expected)
{
   cerr << "  " << name << ":" << endl;
   unsigned got = rows_for(body, name);
   if (got != expected) {
      cerr << "FAIL " << name << ": dialog would show " << got << " row(s), expected "
           << expected << endl;
      failures++;
   } else {
      cerr << "ok   " << name << ": " << got << " row(s)" << endl;
   }
}

// Verbatim from mvo5's comment reporting the empty window.
static const char *MVO5_SOURCES =
   "Types: deb deb-src\n"
   "URIs: http://ftp.de.debian.org/debian/\n"
   "Suites: trixie\n"
   "Components: main non-free-firmware\n"
   "\n"
   "Types: deb\n"
   "URIs: http://security.debian.org/debian-security/\n"
   "Suites: trixie-security\n"
   "Components: main non-free-firmware\n";

int main(int argc, char *argv[])
{
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);

   // The report itself: this file must not produce an empty list.
   check("mvo5-reported-file", MVO5_SOURCES, 2);

   // An empty file legitimately yields an empty dialog -- the negative
   // control, so "2 rows" above cannot be an artifact of always counting.
   check("empty-file-is-empty", "", 0);

   if (failures > 0) {
      cerr << failures << " check(s) failed" << endl;
      return 1;
   }
   cerr << "all checks passed" << endl;
   return 0;
}
