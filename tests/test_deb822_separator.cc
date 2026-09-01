#include "config.h" // IWYU pragma: associated

#include "rsources.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/init.h>
#include <apt-pkg/pkgsystem.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace std;

static int failures = 0;

// Writes `body` to a temporary .sources file, reads it back through the real
// SourcesList code path, and checks how many records came out.
static void check(const string &name, const string &body, unsigned expected)
{
   string path = string(getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp") +
                 "/synaptic-test-" + name + ".sources";

   {
      // binary, so the \r in the CRLF case survives being written out
      ofstream out(path.c_str(), ios::binary);
      out << body;
   }

   SourcesList sl;
   bool ok = sl.ReadDeb822SourcePart(path);
   unsigned got = 0;
   for (list<SourcesList::SourceRecord *>::const_iterator I =
           sl.SourceRecords.begin();
        I != sl.SourceRecords.end(); ++I)
      got++;

   remove(path.c_str());

   if (!ok || got != expected) {
      cerr << "FAIL " << name << ": expected " << expected << " record(s), got "
           << got << (ok ? "" : " (read returned false)") << endl;
      failures++;
   } else {
      cerr << "ok   " << name << ": " << got << " record(s)" << endl;
   }
}

// Same two stanzas in every case below; only the separator differs.
static const char *STANZA_A =
   "Types: deb\n"
   "URIs: http://a.example/debian\n"
   "Suites: stable\n"
   "Components: main\n";

static const char *STANZA_B =
   "Types: deb\n"
   "URIs: http://b.example/debian\n"
   "Suites: testing\n"
   "Components: main\n";

int main(int argc, char **argv)
{
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);

   // Control: an ordinary blank separator. If this one ever fails, the harness
   // itself is broken and the cases below say nothing.
   check("blank-separator", string(STANZA_A) + "\n" + STANZA_B, 2);

   // A separator line of spaces/tabs also ends a stanza (deb822 / RFC 822).
   // Testing line.empty() alone let the next stanza's fields overwrite this
   // one's, so the FIRST source disappeared with no error at all.
   check("whitespace-separator", string(STANZA_A) + "   \n" + STANZA_B, 2);
   check("tab-separator", string(STANZA_A) + "\t\n" + STANZA_B, 2);

   // CRLF files hit the same path: std::getline leaves "\r" on the separator.
   check("crlf-separator", string(STANZA_A) + "\r\n" + STANZA_B, 2);

   // Guard the surrounding behaviour, so a future fix here cannot quietly
   // start inventing or dropping records.
   check("single-stanza", string(STANZA_A), 1);
   check("trailing-blank-lines", string(STANZA_A) + "\n\n\n", 1);
   check("comments-only", "# just a comment\n\n# another\n", 0);
   check("empty-file", "", 0);
   check("stanza-missing-required-field", "Types: deb\nComponents: main\n", 0);

   if (failures > 0) {
      cerr << failures << " check(s) failed" << endl;
      return 1;
   }
   cerr << "all checks passed" << endl;
   return 0;
}
