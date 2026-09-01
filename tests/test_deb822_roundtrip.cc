#include "config.h" // IWYU pragma: associated

#include "rsource_deb822.h"

#include <apt-pkg/error.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

static int failures = 0;

static string tmppath(const string &name)
{
   return string(getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp") +
          "/synaptic-rt-" + name + ".sources";
}

static void write_file(const string &path, const string &body)
{
   ofstream out(path.c_str(), ios::binary);
   out << body;
}

static string read_file(const string &path)
{
   ifstream in(path.c_str(), ios::binary);
   ostringstream ss;
   ss << in.rdbuf();
   return ss.str();
}

// Reads `body`, then writes the parsed entries straight back out, exercising
// the same ParseDeb822File/WriteDeb822File pair the repository dialog uses.
// Checks the resulting FILE TEXT, not the parsed structs: a struct comparison
// reads the field through the parser on both sides, so it cannot see a field
// the parser never populates.
static void check_preserved(const string &name, const string &body,
                            const string &needle)
{
   string in = tmppath(name + "-in");
   string out = tmppath(name + "-out");
   write_file(in, body);

   vector<RDeb822Source::Deb822Entry> entries;
   bool ok = RDeb822Source::ParseDeb822File(in, entries);
   if (ok)
      ok = RDeb822Source::WriteDeb822File(out, entries);

   string saved = ok ? read_file(out) : string();
   remove(in.c_str());
   remove(out.c_str());

   if (!ok) {
      cerr << "FAIL " << name << ": read/write returned false" << endl;
      failures++;
      return;
   }
   if (saved.find(needle) == string::npos) {
      cerr << "FAIL " << name << ": \"" << needle
           << "\" is missing from the saved file" << endl;
      failures++;
      return;
   }
   cerr << "ok   " << name << ": \"" << needle << "\" survived the save" << endl;
}

// A field the parser DOES read. This is the positive control: if it ever
// fails, the harness itself is broken rather than the field handling.
static void control()
{
   check_preserved("control-components",
                   "Types: deb\n"
                   "URIs: http://deb.debian.org/debian\n"
                   "Suites: bookworm\n"
                   "Components: main contrib\n",
                   "Components: main contrib");
}

static const char *MULTIARCH =
   "Types: deb\n"
   "URIs: http://deb.debian.org/debian\n"
   "Suites: bookworm\n"
   "Components: main\n"
   "Architectures: amd64 arm64\n"
   "Languages: en de\n"
   "Targets: deb-src\n";

int main(int argc, char *argv[])
{
   control();

   // Each of these is emitted by WriteDeb822File and declared in Deb822Entry,
   // but was never populated by ParseDeb822File -- so a save truncated the
   // user's file and dropped the field.
   check_preserved("architectures", MULTIARCH, "Architectures: amd64 arm64");
   check_preserved("languages", MULTIARCH, "Languages: en de");
   check_preserved("targets", MULTIARCH, "Targets: deb-src");

   if (failures > 0) {
      cerr << failures << " check(s) failed" << endl;
      return 1;
   }
   cerr << "all checks passed" << endl;
   return 0;
}
