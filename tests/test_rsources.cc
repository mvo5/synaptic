#include "config.h" // IWYU pragma: associated

#include "fake_sources_dir.h"
#include "rsources.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/init.h>
#include <apt-pkg/pkgsystem.h>
#include <gtest/gtest.h>
#include <list>
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace std;

class RSourcesTest : public ::testing::Test
{
 protected:
   FakeSourcesDir box;

   static vector<string> read_files(const SourcesList &lst)
   {
      vector<string> files;
      for (const SourcesList::SourceRecord *rec : lst.SourceRecords) {
         if (rec->Type & SourcesList::Comment)
            continue;
         files.push_back(rec->SourceFile.substr(rec->SourceFile.rfind('/') + 1));
      }
      return files;
   }

   static vector<const SourcesList::SourceRecord *> records(const SourcesList &lst)
   {
      vector<const SourcesList::SourceRecord *> recs;
      for (const SourcesList::SourceRecord *rec : lst.SourceRecords)
         if (!(rec->Type & SourcesList::Comment))
            recs.push_back(rec);
      return recs;
   }

   static vector<string> sections(const SourcesList::SourceRecord *rec)
   {
      return vector<string>(rec->Sections, rec->Sections + rec->NumSections);
   }
};

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

// Which files in Dir::Etc::sourceparts are picked up, and in what order.
// Every entry below is one line, so the record count equals the file count.
TEST_F(RSourcesTest, OnlyListFilesInSortedOrder)
{
   const string line = "deb http://deb.debian.org/debian bookworm main\n";
   // good
   box.put("sources.list.d/b.list", line);
   box.put("sources.list.d/a.list", line);
   box.put("sources.list.d/legacy.list", line);

   // bad (ignore)
   box.put("sources.list.d/ab", line);
   box.put("sources.list.d/x", line);
   box.put("sources.list.d/bad name.list", line);
   box.put("sources.list.d/foo.list.bak", line);
   box.put("sources.list.d/.hidden.list", line);
   mkdir(box.path("sources.list.d/dir.list").c_str(), 0755);

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   EXPECT_EQ(read_files(lst), (vector<string>{"a.list", "b.list", "legacy.list"}));
}

TEST_F(RSourcesTest, Deb822Stanzas)
{
   box.put("sources.list.d/debian.sources", MVO5_SOURCES);

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 2u);

   EXPECT_EQ(recs[0]->Type, SourcesList::Deb | SourcesList::DebSrc);
   EXPECT_EQ(recs[0]->TypeLabel(), "deb deb-src");
   EXPECT_EQ(recs[0]->URI, "http://ftp.de.debian.org/debian/");
   EXPECT_EQ(recs[0]->Dist, "trixie");
   EXPECT_EQ(sections(recs[0]), (vector<string>{"main", "non-free-firmware"}));
   EXPECT_EQ(recs[0]->Format, SourcesList::Deb822);
   EXPECT_EQ(recs[0]->SourceFile, box.path("sources.list.d/debian.sources"));

   EXPECT_EQ(recs[1]->Type, SourcesList::Deb);
   EXPECT_EQ(recs[1]->TypeLabel(), "deb");
   EXPECT_EQ(recs[1]->Dist, "trixie-security");
}

TEST_F(RSourcesTest, Deb822EnabledField)
{
   box.put("sources.list.d/e.sources",
           string(STANZA_A) + "Enabled: no\n\n" + STANZA_A + "Enabled: false\n\n" +
              STANZA_A + "Enabled: yes\n\n" + STANZA_A + "Enabled: bogus\n\n" +
              STANZA_A);

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 5u);
   EXPECT_TRUE(recs[0]->Type & SourcesList::Disabled);
   EXPECT_TRUE(recs[1]->Type & SourcesList::Disabled);
   EXPECT_FALSE(recs[2]->Type & SourcesList::Disabled);
   EXPECT_FALSE(recs[3]->Type & SourcesList::Disabled); // like apt: unknown = enabled
   EXPECT_FALSE(recs[4]->Type & SourcesList::Disabled); // absent = enabled
}

// A stanza may list several URIs and suites. The record keeps them all, in
// the URI and Dist strings, rather than silently showing only the first.
TEST_F(RSourcesTest, Deb822MultiValueFields)
{
   box.put("sources.list.d/ubuntu.sources",
           "Types: deb\n"
           "URIs: http://archive.ubuntu.com/ubuntu/ http://mirror.example/ubuntu/\n"
           "Suites: noble noble-updates noble-backports\n"
           "Components: main universe restricted multiverse\n"
           "\n"
           "Types: deb\n"
           "URIs:\n"
           " http://a.example/debian\n"
           " http://b.example/debian\n"
           "Suites:\tstable   testing\n"
           "Components: main\n");

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 2u);
   EXPECT_EQ(recs[0]->URI,
             "http://archive.ubuntu.com/ubuntu/ http://mirror.example/ubuntu/");
   EXPECT_EQ(recs[0]->Dist, "noble noble-updates noble-backports");
   EXPECT_EQ(sections(recs[0]),
             (vector<string>{"main", "universe", "restricted", "multiverse"}));
   // continuation lines and odd whitespace collapse to single spaces
   EXPECT_EQ(recs[1]->URI, "http://a.example/debian http://b.example/debian");
   EXPECT_EQ(recs[1]->Dist, "stable testing");
}

// $(ARCH) and $(VERSION) are expanded as they are for one-line sources, so
// the same repository shows the same URI whichever file format it came from.
TEST_F(RSourcesTest, Deb822VariablesAreExpanded)
{
   _config->Set("APT::Architecture", "riscv64");
   _config->Set("APT::DistroVersion", "13");
   box.put("sources.list.d/v.sources",
           "Types: deb\n"
           "URIs: http://a.example/$(ARCH)/debian http://b.example/$(VERSION)\n"
           "Suites: $(ARCH)-stable\n"
           "Components: main\n");

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 1u);
   EXPECT_EQ(recs[0]->URI, "http://a.example/riscv64/debian http://b.example/13");
   EXPECT_EQ(recs[0]->Dist, "riscv64-stable");
}

TEST_F(RSourcesTest, Deb822MultilineSignedBy)
{
   box.put("sources.list.d/key.sources",
           "Types: deb\n"
           "URIs: https://pkg.example.com/apt\n"
           "Suites: stable\n"
           "Components: main\n"
           "Signed-By:\n"
           " -----BEGIN PGP PUBLIC KEY BLOCK-----\n"
           " # an indented hash is content, not a comment\n"
           " mQINBF\n"
           " -----END PGP PUBLIC KEY BLOCK-----\n");

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 1u);
   EXPECT_EQ(recs[0]->URI, "https://pkg.example.com/apt");
   EXPECT_EQ(sections(recs[0]), (vector<string>{"main"}));
}

TEST_F(RSourcesTest, Deb822Comments)
{
   box.put("sources.list.d/c.sources",
           string("# leading comment\n") + STANZA_A +
              "# comment between two stanzas\n\n" + "# comment before B\n" +
              "Types: deb\n"
              "# comment between fields\n"
              "URIs: http://b.example/debian\n"
              "Suites: testing\n"
              "Components: main\n"
              "\n"
              "# trailing comment\n");

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 2u);
   EXPECT_EQ(recs[0]->URI, "http://a.example/debian");
   EXPECT_EQ(recs[1]->URI, "http://b.example/debian");
   EXPECT_EQ(recs[1]->Dist, "testing");
}

TEST_F(RSourcesTest, Deb822CrlfSeparator)
{
   box.put("sources.list.d/crlf.sources", string(STANZA_A) + "\r\n" + STANZA_B);

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   EXPECT_EQ(records(lst).size(), 2u);
}

// To apt a line of only spaces continues the previous field, it does not end
// the stanza (pkgTagSection::Scan). We must agree, or the dialog would show a
// repository apt never loads.
TEST_F(RSourcesTest, Deb822WhitespaceOnlyLineIsNotASeparator)
{
   box.put("sources.list.d/ws.sources", string(STANZA_A) + "   \n" + STANZA_B);

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 1u);
   EXPECT_EQ(recs[0]->URI, "http://b.example/debian");
}

TEST_F(RSourcesTest, Deb822EmptyAndCommentOnlyFiles)
{
   box.put("sources.list.d/empty.sources", "");
   box.put("sources.list.d/comments.sources", "# nothing here\n\n# still nothing\n");

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   EXPECT_EQ(records(lst).size(), 0u);
}

TEST_F(RSourcesTest, Deb822BadStanzasAreSkipped)
{
   box.put("sources.list.d/bad.sources",
           string(STANZA_A) + "\n" +
              "URIs: http://no-types.example/\nSuites: stable\n\n" +
              "Types: deb foo\nURIs: http://unknown-type.example/\nSuites: stable\n");

   SourcesList lst;
   EXPECT_FALSE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 1u);
   EXPECT_EQ(recs[0]->URI, "http://a.example/debian");
}

TEST_F(RSourcesTest, SourcepartsMixesFormatsInSortedOrder)
{
   const string line = "deb http://deb.debian.org/debian bookworm main\n";
   box.put("sources.list.d/b.list", line);
   box.put("sources.list.d/legacy.list", line);
   box.put("sources.list.d/a.sources", STANZA_A);
   box.put("sources.list.d/z.sources", STANZA_B);

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   EXPECT_EQ(read_files(lst),
             (vector<string>{"a.sources", "b.list", "legacy.list", "z.sources"}));
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 4u);
   EXPECT_EQ(recs[0]->Format, SourcesList::Deb822);
   EXPECT_EQ(recs[1]->Format, SourcesList::OneLine);
}

// Until there is a writer that edits stanzas in place, saving must not touch
// .sources files at all, whatever happened to their records in memory.
TEST_F(RSourcesTest, Deb822FilesAreNeverWritten)
{
   const string ubuntu =
      "# See sources.list(5) for details\n"
      "Types: deb\n"
      "URIs: http://archive.ubuntu.com/ubuntu/\n"
      "Suites: noble noble-updates noble-backports\n"
      "Components: main universe restricted multiverse\n"
      "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n";
   box.put("sources.list.d/ubuntu.sources", ubuntu);
   box.put("sources.list", "deb http://deb.debian.org/debian bookworm main\n");

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   for (SourcesList::SourceRecord *rec : lst.SourceRecords) {
      if (rec->Format != SourcesList::Deb822)
         continue;
      rec->Type |= SourcesList::Disabled;
      rec->URI = "http://changed.example/";
   }
   EXPECT_TRUE(lst.UpdateSources());

   EXPECT_EQ(box.get("sources.list.d/ubuntu.sources"), ubuntu);
   // the one-line file is still rewritten as before
   EXPECT_NE(box.get("sources.list").find("deb http://deb.debian.org/debian/ bookworm main"),
             string::npos);
}

int main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);
   return RUN_ALL_TESTS();
}
