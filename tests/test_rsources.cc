#include "config.h" // IWYU pragma: associated

#include "fake_sources_dir.h"
#include "rsources.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/init.h>
#include <apt-pkg/pkgsystem.h>
#include <gtest/gtest.h>
#include <list>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
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

// GetListOfFilesInDir() queues a notice for every oddly named file; a
// successful read must leave the queue exactly as it found it
TEST_F(RSourcesTest, SuccessfulReadLeavesErrorQueueUntouched)
{
   const string line = "deb http://deb.debian.org/debian bookworm main\n";
   box.put("sources.list.d/a.list", line);
   box.put("sources.list.d/old.list.old", line);
   _error->Warning("unrelated");

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   EXPECT_EQ(read_files(lst), (vector<string>{"a.list"}));
   string msg;
   EXPECT_FALSE(_error->PopMessage(msg));
   EXPECT_EQ(msg, "unrelated");
   EXPECT_TRUE(_error->empty());
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
// like apt: $(ARCH) is expanded, anything else is left alone
TEST_F(RSourcesTest, Deb822ArchIsExpanded)
{
   _config->Set("APT::Architecture", "riscv64");
   box.put("sources.list.d/v.sources",
           "Types: deb\n"
           "URIs: http://a.example/$(ARCH)/debian http://b.example/$(VERSION)\n"
           "Suites: $(ARCH)-stable\n"
           "Components: main\n");

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 1u);
   EXPECT_EQ(recs[0]->URI, "http://a.example/riscv64/debian http://b.example/$(VERSION)");
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

static const char *UBUNTU_SOURCES =
   "Enabled: yes\n"
   "Types: deb deb-src\n"
   "URIs: http://de.archive.ubuntu.com/ubuntu/\n"
   "Suites: noble\n"
   "Components: main restricted universe multiverse\n"
   "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n"
   "\n"
   "Enabled: yes\n"
   "Types: deb deb-src\n"
   "URIs: http://security.ubuntu.com/ubuntu/\n"
   "Suites: noble-security\n"
   "Components: main restricted universe multiverse\n"
   "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n";

static void set_sections(SourcesList::SourceRecord *rec, const vector<string> &secs)
{
   delete[] rec->Sections;
   rec->NumSections = secs.size();
   rec->Sections = new string[rec->NumSections];
   for (unsigned i = 0; i < rec->NumSections; i++)
      rec->Sections[i] = secs[i];
}

// Edits to a deb822 record are written back field by field, inside the
// stanza; everything not edited, including the other stanza, stays as it was.
TEST_F(RSourcesTest, Deb822EditsAreWrittenBackInPlace)
{
   box.put("sources.list.d/ubuntu.sources", UBUNTU_SOURCES);
   box.put("sources.list.d/ppa.sources", string("Enabled: yes\n") + STANZA_A);

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   ASSERT_EQ(records(lst).size(), 3u);
   SourcesList::SourceRecord *rec = nullptr;
   for (SourcesList::SourceRecord *r : lst.SourceRecords)
      if (r->SourceFile == box.path("sources.list.d/ubuntu.sources") && r->Dist == "noble")
         rec = r;
   ASSERT_NE(rec, nullptr);

   rec->Type = SourcesList::Deb | SourcesList::Disabled; // drop deb-src, disable
   rec->URI = "http://changed.example/ubuntu/";
   rec->Dist = "noble noble-updates";
   set_sections(rec, {"main", "universe"});
   EXPECT_TRUE(lst.UpdateSources());

   string expected = UBUNTU_SOURCES;
   expected.replace(expected.find("Enabled: yes\n"), 13, "Enabled: no\n");
   expected.replace(expected.find("Types: deb deb-src\n"), 19, "Types: deb\n");
   expected.replace(expected.find("URIs: http://de.archive.ubuntu.com/ubuntu/\n"), 43,
                    "URIs: http://changed.example/ubuntu/\n");
   expected.replace(expected.find("Suites: noble\n"), 14, "Suites: noble noble-updates\n");
   expected.replace(expected.find("Components: main restricted universe multiverse\n"), 48,
                    "Components: main universe\n");
   EXPECT_EQ(box.get("sources.list.d/ubuntu.sources"), expected);
   EXPECT_EQ(box.get("sources.list.d/ubuntu.sources.bak"), UBUNTU_SOURCES);
   EXPECT_EQ(box.get("sources.list.d/ppa.sources"), string("Enabled: yes\n") + STANZA_A);

   // the dialog's cancel path writes the untouched copy back: a no-op
   SourcesList saved;
   EXPECT_TRUE(saved.ReadSources());
   EXPECT_TRUE(saved.UpdateSources());
   EXPECT_EQ(box.get("sources.list.d/ubuntu.sources"), expected);
}

// The record shows $(ARCH) expanded, but an unchanged field is not written,
// so the variable survives in the file.
TEST_F(RSourcesTest, Deb822UnchangedFieldsKeepArchVariable)
{
   _config->Set("APT::Architecture", "riscv64");
   const string body =
      "Types: deb\nURIs: http://a.example/$(ARCH)/debian\nSuites: stable\nComponents: main\n";
   box.put("sources.list.d/a.sources", body);

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 1u);
   SourcesList::SourceRecord *rec = const_cast<SourcesList::SourceRecord *>(recs[0]);
   EXPECT_EQ(rec->URI, "http://a.example/riscv64/debian");

   rec->Dist = "testing";
   EXPECT_TRUE(lst.UpdateSources());
   EXPECT_EQ(box.get("sources.list.d/a.sources"),
             "Types: deb\nURIs: http://a.example/$(ARCH)/debian\nSuites: testing\nComponents: main\n");
}

TEST_F(RSourcesTest, Deb822ClearedComponentsRemoveTheField)
{
   box.put("sources.list.d/a.sources", STANZA_A);
   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 1u);
   set_sections(const_cast<SourcesList::SourceRecord *>(recs[0]), {});
   EXPECT_TRUE(lst.UpdateSources());
   EXPECT_EQ(box.get("sources.list.d/a.sources"),
             "Types: deb\nURIs: http://a.example/debian\nSuites: stable\n");
}

// A stanza apt would reject still counts for the stanza index, so the writer
// and the records agree on which stanza is which.
TEST_F(RSourcesTest, Deb822StanzaIndexSurvivesSkippedStanzas)
{
   box.put("sources.list.d/mix.sources",
           string(STANZA_A) + "\n" + "URIs: http://no-types.example/\nSuites: s\n\n" + STANZA_B);

   SourcesList lst;
   EXPECT_FALSE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 2u);
   EXPECT_EQ(recs[0]->StanzaIndex, 0u);
   EXPECT_EQ(recs[1]->StanzaIndex, 2u);

   const_cast<SourcesList::SourceRecord *>(recs[1])->Type |= SourcesList::Disabled;
   EXPECT_TRUE(lst.UpdateSources());
   EXPECT_EQ(box.get("sources.list.d/mix.sources"),
             string(STANZA_A) + "\n" + "URIs: http://no-types.example/\nSuites: s\n\n" +
                "Enabled: no\n" + STANZA_B);
}

// The first save keeps the original next to the file; later saves leave that
// backup alone, and files whose content would not change are not touched.
TEST_F(RSourcesTest, SavingBacksUpOnceAndSkipsUnchangedFiles)
{
   const string main_orig = "deb http://deb.debian.org/debian bookworm main\n";
   const string part_orig = "deb http://a.example/debian stable main\n";
   box.put("sources.list", main_orig);
   box.put("sources.list.d/a.list", part_orig);

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   EXPECT_TRUE(lst.UpdateSources());

   // both files are normalised on save (trailing slash, trailing space)...
   const string main_saved = box.get("sources.list");
   const string part_saved = box.get("sources.list.d/a.list");
   EXPECT_NE(main_saved, main_orig);
   EXPECT_NE(part_saved, part_orig);
   // ...and the originals are kept
   EXPECT_EQ(box.get("sources.list.bak"), main_orig);
   EXPECT_EQ(box.get("sources.list.d/a.list.bak"), part_orig);

   // saving again changes nothing, so the backups still hold the originals
   EXPECT_TRUE(lst.UpdateSources());
   EXPECT_EQ(box.get("sources.list"), main_saved);
   EXPECT_EQ(box.get("sources.list.bak"), main_orig);
   EXPECT_EQ(box.get("sources.list.d/a.list.bak"), part_orig);

   // a real change later on does not overwrite the first backup either
   for (SourcesList::SourceRecord *rec : lst.SourceRecords)
      if (!(rec->Type & SourcesList::Comment))
         rec->Type |= SourcesList::Disabled;
   EXPECT_TRUE(lst.UpdateSources());
   EXPECT_NE(box.get("sources.list"), main_saved);
   EXPECT_EQ(box.get("sources.list.bak"), main_orig);
   EXPECT_EQ(box.get("sources.list.d/a.list.bak"), part_orig);
}

TEST_F(RSourcesTest, WriteSourcesFileKeepsModeAndCreatesMissingFiles)
{
   const string path = box.path("sources.list.d/new.list");
   EXPECT_TRUE(WriteSourcesFile(path, "deb http://a/ s main\n"));
   EXPECT_EQ(box.get("sources.list.d/new.list"), "deb http://a/ s main\n");
   EXPECT_FALSE(FileExists(path + ".bak"));

   chmod(path.c_str(), 0600);
   EXPECT_TRUE(WriteSourcesFile(path, "deb http://b/ s main\n"));
   struct stat st;
   ASSERT_EQ(stat(path.c_str(), &st), 0);
   EXPECT_EQ(st.st_mode & 0777, 0600u);
   ASSERT_EQ(stat((path + ".bak").c_str(), &st), 0);
   EXPECT_EQ(st.st_mode & 0777, 0600u);
   EXPECT_EQ(box.get("sources.list.d/new.list.bak"), "deb http://a/ s main\n");

   EXPECT_TRUE(WriteSourcesFile(path, "deb http://c/ s main\n"));
   EXPECT_EQ(box.get("sources.list.d/new.list.bak"), "deb http://a/ s main\n");
}

// Reader corner cases taken from apt's own test suite.
TEST_F(RSourcesTest, Deb822AptTestsuiteFixtures)
{
   _config->Set("APT::Architecture", "riscv64");
   box.put("sources.list.d/a.sources",
           "# that contains a : as well\n"
           "#Types: meep\n"
           "\n"
           "# a free-standing comment appears\n"
           "\n"
           "Types: deb\n"
           "#Types: deb-src\n"
           "URIs: http://ftp.debian.org/debian\n"
           "Suites: stable\n"
           "Components: main\n"
           "Description: summary\n"
           "# comments are ignored\n");
   box.put("sources.list.d/b.sources",
           "#NOTE: Most preferred source listed first!\n"
           "Types:          deb deb-src\n"
           "URIs:http://ftp.uk.debian.org/debian/\n"
           "Suites:         stretch\n"
           "Components:     main contrib non-free\n");
   box.put("sources.list.d/c.sources",
           "Types: deb\n"
           "URIs: http://ftp.tlh.debian.org/universe\n"
           "Suites: stable/binary-$(ARCH)/\n"
           "Enabled: false\n");

   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());
   auto recs = records(lst);
   ASSERT_EQ(recs.size(), 3u);
   EXPECT_EQ(recs[0]->Type, SourcesList::Deb);
   EXPECT_EQ(recs[0]->URI, "http://ftp.debian.org/debian");
   EXPECT_EQ(sections(recs[0]), (vector<string>{"main"}));
   EXPECT_EQ(recs[1]->Type, SourcesList::Deb | SourcesList::DebSrc);
   EXPECT_EQ(recs[1]->URI, "http://ftp.uk.debian.org/debian/");
   EXPECT_EQ(recs[1]->Dist, "stretch");
   EXPECT_EQ(sections(recs[1]), (vector<string>{"main", "contrib", "non-free"}));
   EXPECT_EQ(recs[2]->Type, SourcesList::Deb | SourcesList::Disabled);
   EXPECT_EQ(recs[2]->Dist, "stable/binary-riscv64/");
   EXPECT_EQ(recs[2]->NumSections, 0);
}

// Debian's package ships sources files as symlinks in some setups; the edit
// must land in the target, not replace the link with a regular file.
TEST_F(RSourcesTest, WriteSourcesFileFollowsSymlinks)
{
   const string target = box.path("real.sources");
   const string link = box.path("sources.list.d/link.sources");
   box.put("real.sources", "Types: deb\nURIs: http://a/\nSuites: s\n");
   ASSERT_EQ(symlink(target.c_str(), link.c_str()), 0);

   EXPECT_TRUE(WriteSourcesFile(link, "Types: deb\nURIs: http://b/\nSuites: s\n"));
   struct stat st;
   ASSERT_EQ(lstat(link.c_str(), &st), 0);
   EXPECT_TRUE(S_ISLNK(st.st_mode));
   EXPECT_EQ(box.get("real.sources"), "Types: deb\nURIs: http://b/\nSuites: s\n");
   EXPECT_EQ(box.get("real.sources.bak"), "Types: deb\nURIs: http://a/\nSuites: s\n");
   EXPECT_FALSE(FileExists(link + ".bak"));
}

TEST_F(RSourcesTest, Deb822RemovedRecordIsDeletedOnSave)
{
   box.put("sources.list.d/ubuntu.sources", UBUNTU_SOURCES);
   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());

   SourcesList::SourceRecord *security = nullptr, *archive = nullptr;
   for (SourcesList::SourceRecord *r : lst.SourceRecords) {
      if (r->Dist == "noble-security")
         security = r;
      else if (r->Dist == "noble")
         archive = r;
   }
   ASSERT_NE(security, nullptr);
   ASSERT_NE(archive, nullptr);

   lst.RemoveSource(security);
   archive->Dist = "noble noble-updates";
   EXPECT_TRUE(lst.UpdateSources());

   string expected = UBUNTU_SOURCES;
   expected.erase(expected.find("\nEnabled: yes\nTypes: deb deb-src\nURIs: http://security"));
   expected.replace(expected.find("Suites: noble\n"), 14, "Suites: noble noble-updates\n");
   EXPECT_EQ(box.get("sources.list.d/ubuntu.sources"), expected);

   // a second save has nothing left to delete
   EXPECT_TRUE(lst.UpdateSources());
   EXPECT_EQ(box.get("sources.list.d/ubuntu.sources"), expected);
}

// The dialog's undo path after a save apt rejects: the deb822 files come
// back verbatim, even when a stanza was deleted and the indices shifted.
TEST_F(RSourcesTest, Deb822RevertRestoresFilesVerbatim)
{
   const string third = string("\n") + STANZA_B;
   box.put("sources.list.d/ubuntu.sources", UBUNTU_SOURCES + third);
   box.put("sources.list.d/ppa.sources", STANZA_A);
   SourcesList lst;
   EXPECT_TRUE(lst.ReadSources());

   SourcesList::SourceRecord *archive = nullptr, *testing = nullptr;
   for (SourcesList::SourceRecord *r : lst.SourceRecords) {
      if (r->Dist == "noble")
         archive = r;
      else if (r->Dist == "testing")
         testing = r;
   }
   ASSERT_NE(archive, nullptr);
   ASSERT_NE(testing, nullptr);
   lst.RemoveSource(archive);
   testing->URI = "not a valid uri";
   EXPECT_TRUE(lst.UpdateSources());
   EXPECT_NE(box.get("sources.list.d/ubuntu.sources"), UBUNTU_SOURCES + third);

   EXPECT_TRUE(lst.RevertDeb822Sources());
   EXPECT_EQ(box.get("sources.list.d/ubuntu.sources"), UBUNTU_SOURCES + third);
   EXPECT_EQ(box.get("sources.list.d/ppa.sources"), STANZA_A);
   // the one-time backup is from the first write and is not disturbed
   EXPECT_EQ(box.get("sources.list.d/ubuntu.sources.bak"), UBUNTU_SOURCES + third);
   // nothing left to revert
   EXPECT_TRUE(lst.RevertDeb822Sources());
}

int main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);
   return RUN_ALL_TESTS();
}
