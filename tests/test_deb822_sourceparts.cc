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

static const char *STANZA =
   "Types: deb\n"
   "URIs: http://deb.debian.org/debian\n"
   "Suites: bookworm\n"
   "Components: main\n";

static void put(const string &path, const string &body)
{
   ofstream out(path.c_str(), ios::binary);
   out << body;
}

// Enumerating the sources directory is the maintainer's stated acceptance
// criterion: every *.sources under apt's Dir::Etc::sourceparts must show up,
// not just one file.
int main(int argc, char *argv[])
{
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);

   // Honour TMPDIR like the other tests here: a build host may have /tmp
   // read-only or redirected.
   string tmplstr = string(getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp") +
                    "/synaptic-dir-XXXXXX";
   vector<char> tmpl(tmplstr.begin(), tmplstr.end());
   tmpl.push_back('\0');
   const char *root = mkdtemp(tmpl.data());
   if (root == nullptr) {
      cerr << "FAIL: could not create a temp dir" << endl;
      return 1;
   }
   string parts = string(root) + "/sources.list.d";
   mkdir(parts.c_str(), 0755);

   // Three well-formed files. "a.sources" is 9 characters; the shortest name
   // that can exist here is ".sources" itself, so cover a short name too.
   put(parts + "/debian.sources", STANZA);
   put(parts + "/ubuntu.sources", STANZA);
   put(parts + "/a.sources", STANZA);
   // Files that must be ignored, including short names that are NOT .sources.
   put(parts + "/legacy.list", "deb http://x.example/ y z\n");
   put(parts + "/ab", "not a sources file\n");
   put(parts + "/x", "not a sources file\n");

   // Point apt at the sandbox exactly as the dialog would see it.
   _config->Set("Dir::Etc::sourceparts", parts);
   _config->Set("Dir::Etc::sourcelist", string(root) + "/sources.list");

   SourcesList lst;
   lst.ReadSources();

   unsigned records = 0;
   for (list<SourcesList::SourceRecord *>::const_iterator I =
           lst.SourceRecords.begin();
        I != lst.SourceRecords.end(); ++I) {
      if ((*I)->Type & SourcesList::Comment)
         continue;
      records++;
   }

   // Three .sources stanzas plus the one-line legacy.list entry, which apt
   // reads from the same directory and which ReadSourceDir is right to pick
   // up. Each must appear exactly once: before the sourcelist.d/sourceparts
   // de-duplication this was 6, because every .sources file was read twice.
   if (records != 4) {
      cerr << "FAIL enumerate-sourceparts: expected 4 record(s) (3 .sources + "
              "1 .list), got " << records << endl;
      failures++;
   } else {
      cerr << "ok   enumerate-sourceparts: " << records << " record(s), no duplicates"
           << endl;
   }

   if (failures > 0) {
      cerr << failures << " check(s) failed" << endl;
      return 1;
   }
   cerr << "all checks passed" << endl;
   return 0;
}
