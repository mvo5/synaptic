#include "config.h" // IWYU pragma: associated

#include "fake_sources_dir.h"
#include "rdeb822file.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/init.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/tagfile.h>
#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace std;

class RDeb822FileTest : public ::testing::Test
{
 protected:
   FakeSourcesDir box;

   // Writes body, applies edit, writes back, returns the file text.
   string roundtrip(const string &body, void (*edit)(RDeb822File &))
   {
      const string rel = "sources.list.d/t.sources";
      box.put(rel, body);
      RDeb822File f(box.path(rel));
      EXPECT_TRUE(f.Read());
      if (edit)
         edit(f);
      EXPECT_TRUE(f.Write());
      return box.get(rel);
   }
};

static const char *TWO_STANZAS =
   "# leading comment\n"
   "Types: deb\n"
   "URIs: http://a.example/debian\n"
   "# comment between fields\n"
   "Suites: stable\n"
   "Components: main\n"
   "Signed-By:\n"
   " -----BEGIN PGP PUBLIC KEY BLOCK-----\n"
   " # indented, so content\n"
   " mQINBF\n"
   " -----END PGP PUBLIC KEY BLOCK-----\n"
   "\n"
   "# introduces B\n"
   "Enabled: no\n"
   "Types: deb deb-src\n"
   "URIs: http://b.example/debian\n"
   "Suites: testing\n"
   "Components: main contrib\n"
   "\n"
   "# trailing comment\n";

TEST_F(RDeb822FileTest, UnchangedFileIsWrittenIdentically)
{
   EXPECT_EQ(roundtrip(TWO_STANZAS, nullptr), TWO_STANZAS);
}

TEST_F(RDeb822FileTest, ReadVisitsEveryStanzaWithAptsView)
{
   box.put("sources.list.d/t.sources", TWO_STANZAS);
   RDeb822File f(box.path("sources.list.d/t.sources"));
   vector<string> uris;
   EXPECT_TRUE(f.Read([&](const pkgTagSection &sec, unsigned idx) {
      EXPECT_EQ(idx, uris.size());
      uris.push_back(sec.FindS("URIs"));
   }));
   EXPECT_EQ(f.StanzaCount(), 2u);
   EXPECT_EQ(uris, (vector<string>{"http://a.example/debian", "http://b.example/debian"}));
}

TEST_F(RDeb822FileTest, EnableRewritesTheExistingLine)
{
   string expected = TWO_STANZAS;
   expected.replace(expected.find("Enabled: no\n"), 12, "Enabled: yes\n");
   EXPECT_EQ(roundtrip(TWO_STANZAS, [](RDeb822File &f) {
                EXPECT_TRUE(f.SetEnabled(1, true));
             }),
             expected);
}

TEST_F(RDeb822FileTest, DisableInsertsBeforeTheFirstField)
{
   // after the leading comment, before Types
   string expected = TWO_STANZAS;
   expected.insert(expected.find("Types: deb\n"), "Enabled: no\n");
   EXPECT_EQ(roundtrip(TWO_STANZAS, [](RDeb822File &f) {
                EXPECT_TRUE(f.SetEnabled(0, false));
             }),
             expected);
}

TEST_F(RDeb822FileTest, SettingTheCurrentStateIsANoop)
{
   EXPECT_EQ(roundtrip(TWO_STANZAS, [](RDeb822File &f) {
                EXPECT_FALSE(f.SetEnabled(0, true));  // absent means enabled
                EXPECT_FALSE(f.SetEnabled(1, false)); // already "no"
                EXPECT_FALSE(f.Changed());
             }),
             TWO_STANZAS);
   EXPECT_EQ(roundtrip("Enabled: yes\nTypes: deb\nURIs: http://a/\nSuites: s\n",
                       [](RDeb822File &f) { EXPECT_FALSE(f.SetEnabled(0, true)); }),
             "Enabled: yes\nTypes: deb\nURIs: http://a/\nSuites: s\n");
}

TEST_F(RDeb822FileTest, EnabledValuesFollowApt)
{
   // false-ish spellings disable, anything else keeps the default
   for (const char *v : {"no", "false", "0", "off"}) {
      string body = string("Enabled: ") + v + "\nTypes: deb\nURIs: http://a/\nSuites: s\n";
      EXPECT_EQ(roundtrip(body, [](RDeb822File &f) { EXPECT_TRUE(f.SetEnabled(0, true)); }),
                "Enabled: yes\nTypes: deb\nURIs: http://a/\nSuites: s\n")
         << "value " << v;
   }
   EXPECT_EQ(roundtrip("Enabled: bogus\nTypes: deb\nURIs: http://a/\nSuites: s\n",
                       [](RDeb822File &f) { EXPECT_FALSE(f.SetEnabled(0, true)); }),
             "Enabled: bogus\nTypes: deb\nURIs: http://a/\nSuites: s\n");
}

TEST_F(RDeb822FileTest, KeySpellingIsKeptAndMatchedCaseInsensitively)
{
   EXPECT_EQ(roundtrip("enabled: no\nTypes: deb\nURIs: http://a/\nSuites: s\n",
                       [](RDeb822File &f) { EXPECT_TRUE(f.SetEnabled(0, true)); }),
             "enabled: yes\nTypes: deb\nURIs: http://a/\nSuites: s\n");
}

TEST_F(RDeb822FileTest, MultilineFieldCollapsesButInteriorCommentStays)
{
   const string body =
      "Types: deb\n"
      "URIs:\n"
      " http://a.example/\n"
      "# keep me\n"
      " http://b.example/\n"
      "Suites: s\n";
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) {
                EXPECT_TRUE(f.SetField(0, "URIs", "http://c.example/"));
             }),
             "Types: deb\n"
             "URIs: http://c.example/\n"
             "# keep me\n"
             "Suites: s\n");
}

TEST_F(RDeb822FileTest, EditingTheSecondStanzaLeavesTheFirstAlone)
{
   string expected = TWO_STANZAS;
   expected.replace(expected.find("Suites: testing\n"), 16, "Suites: unstable\n");
   EXPECT_EQ(roundtrip(TWO_STANZAS, [](RDeb822File &f) {
                EXPECT_TRUE(f.SetField(1, "Suites", "unstable"));
             }),
             expected);
}

TEST_F(RDeb822FileTest, SeveralEditsKeepOffsetsInSync)
{
   string expected = TWO_STANZAS;
   expected.insert(expected.find("Types: deb\n"), "Enabled: no\n");
   expected.replace(expected.find("Enabled: no\nTypes: deb deb-src"), 12, "Enabled: yes\n");
   expected.replace(expected.find("Suites: testing\n"), 16, "Suites: unstable\n");
   EXPECT_EQ(roundtrip(TWO_STANZAS, [](RDeb822File &f) {
                EXPECT_TRUE(f.SetEnabled(0, false)); // grows stanza 0
                EXPECT_TRUE(f.SetEnabled(1, true));
                EXPECT_TRUE(f.SetField(1, "Suites", "unstable"));
             }),
             expected);
}

TEST_F(RDeb822FileTest, CrlfFilesKeepTheirLineEndings)
{
   EXPECT_EQ(roundtrip("Types: deb\r\nURIs: http://a/\r\nSuites: s\r\n",
                       [](RDeb822File &f) { EXPECT_TRUE(f.SetEnabled(0, false)); }),
             "Enabled: no\r\nTypes: deb\r\nURIs: http://a/\r\nSuites: s\r\n");
}

TEST_F(RDeb822FileTest, MissingTrailingNewlineIsHandled)
{
   EXPECT_EQ(roundtrip("Types: deb\nURIs: http://a/\nSuites: s",
                       [](RDeb822File &f) { EXPECT_TRUE(f.SetField(0, "Suites", "t")); }),
             "Types: deb\nURIs: http://a/\nSuites: t\n");
}

TEST_F(RDeb822FileTest, WritePreservesFileMode)
{
   const string rel = "sources.list.d/t.sources";
   box.put(rel, TWO_STANZAS);
   chmod(box.path(rel).c_str(), 0600);
   RDeb822File f(box.path(rel));
   EXPECT_TRUE(f.Read());
   EXPECT_TRUE(f.SetEnabled(0, false));
   EXPECT_TRUE(f.Write());
   struct stat st;
   ASSERT_EQ(stat(box.path(rel).c_str(), &st), 0);
   EXPECT_EQ(st.st_mode & 0777, 0600u);
}

TEST_F(RDeb822FileTest, OutOfRangeStanzaIsRejected)
{
   EXPECT_EQ(roundtrip(TWO_STANZAS, [](RDeb822File &f) {
                EXPECT_FALSE(f.SetEnabled(2, false));
                EXPECT_FALSE(f.SetField(7, "Suites", "x"));
             }),
             TWO_STANZAS);
}

TEST_F(RDeb822FileTest, MissingFileFails)
{
   RDeb822File f(box.path("sources.list.d/nope.sources"));
   EXPECT_FALSE(f.Read());
   _error->Discard();
}

// ---- corner cases from apt's own test suite, its manpage and real files ----

// test/integration/test-apt-sources-deb822: comments that look like fields,
// a comment with a colon, an unknown field, a free-standing comment.
static const char *APT_TESTSUITE_FILE =
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
   "# comments are ignored\n";

TEST_F(RDeb822FileTest, CommentedOutFieldsAreNotFields)
{
   string expected = APT_TESTSUITE_FILE;
   expected.replace(expected.find("Types: deb\n"), 11, "Types: deb deb-src\n");
   EXPECT_EQ(roundtrip(APT_TESTSUITE_FILE, [](RDeb822File &f) {
                EXPECT_EQ(f.StanzaCount(), 1u);
                EXPECT_TRUE(f.SetField(0, "Types", "deb deb-src"));
             }),
             expected);

   expected = APT_TESTSUITE_FILE;
   expected.insert(expected.find("Types: deb\n"), "Enabled: no\n");
   EXPECT_EQ(roundtrip(APT_TESTSUITE_FILE, [](RDeb822File &f) {
                EXPECT_TRUE(f.SetEnabled(0, false));
             }),
             expected);
}

// test-apt-sources-deb822 "NEW MULTI-LINE FORMAT" fixture: aligned values,
// no space after the colon, banner comments with colons.
static const char *ALIGNED_FILE =
   "#NOTE: Most preferred source listed first!\n"
   "#=== NEW MULTI-LINE FORMAT ===============\n"
   "Types:          deb deb-src\n"
   "URIs:http://ftp.uk.debian.org/debian/\n"
   "Suites:         stretch\n"
   "Components:     main contrib non-free\n"
   "#=== NEW MULTI-LINE FORMAT ===============\n";

TEST_F(RDeb822FileTest, AlignedAndSpacelessFields)
{
   string expected = ALIGNED_FILE;
   expected.replace(expected.find("Suites:         stretch\n"), 24, "Suites: buster\n");
   EXPECT_EQ(roundtrip(ALIGNED_FILE, [](RDeb822File &f) {
                // same value, differently spaced: nothing to do
                EXPECT_FALSE(f.SetField(0, "URIs", "http://ftp.uk.debian.org/debian/"));
                EXPECT_FALSE(f.SetField(0, "Types", "deb deb-src"));
                EXPECT_TRUE(f.SetField(0, "Suites", "buster"));
             }),
             expected);
}

TEST_F(RDeb822FileTest, EmptyValueIsReplaced)
{
   // "Signed-By: " with a trailing space, as apt modernize-sources writes it
   EXPECT_EQ(roundtrip("Types: deb\nURIs: http://a/\nSuites: s\nSigned-By: \n",
                       [](RDeb822File &f) {
                          EXPECT_TRUE(f.SetField(0, "Signed-By", "/usr/share/keyrings/k.gpg"));
                       }),
             "Types: deb\nURIs: http://a/\nSuites: s\nSigned-By: /usr/share/keyrings/k.gpg\n");
}

TEST_F(RDeb822FileTest, HashInsideAValueIsContent)
{
   const string body = "Types: deb\nURIs: http://a/\nSuites: s\nComponents: main # not a comment\n";
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) { EXPECT_TRUE(f.SetEnabled(0, false)); }),
             "Enabled: no\n" + body);
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) {
                EXPECT_TRUE(f.SetField(0, "Components", "contrib"));
             }),
             "Types: deb\nURIs: http://a/\nSuites: s\nComponents: contrib\n");
}

// To apt a line of only whitespace continues the field above it, so it is
// part of what gets replaced.
TEST_F(RDeb822FileTest, WhitespaceOnlyLineBelongsToThePreviousField)
{
   EXPECT_EQ(roundtrip("Types: deb\nURIs: http://a/\n   \nSuites: s\n",
                       [](RDeb822File &f) { EXPECT_TRUE(f.SetField(0, "URIs", "http://b/")); }),
             "Types: deb\nURIs: http://b/\nSuites: s\n");
}

TEST_F(RDeb822FileTest, TabContinuationsAndUnknownFieldsSurvive)
{
   const string body =
      "Types: deb\n"
      "URIs: http://a/\n"
      "Suites: s\n"
      "Components:\n"
      "\tmain\n"
      "\tcontrib\n"
      "Architectures-Add: armel\n"
      "Check-Valid-Until: no\n"
      "X-Repolib-Name: Example\n";
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) { EXPECT_TRUE(f.SetEnabled(0, false)); }),
             "Enabled: no\n" + body);
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) {
                EXPECT_TRUE(f.SetField(0, "Components", "main"));
             }),
             "Types: deb\nURIs: http://a/\nSuites: s\nComponents: main\n"
             "Architectures-Add: armel\nCheck-Valid-Until: no\nX-Repolib-Name: Example\n");
}

TEST_F(RDeb822FileTest, DuplicateKeysTheLastOneWinsAndIsRewritten)
{
   EXPECT_EQ(roundtrip("Suites: a\nURIs: http://x/\nSuites: b\nTypes: deb\n",
                       [](RDeb822File &f) { EXPECT_TRUE(f.SetField(0, "Suites", "c")); }),
             "Suites: a\nURIs: http://x/\nSuites: c\nTypes: deb\n");
}

TEST_F(RDeb822FileTest, EnabledInTheMiddleIsRewrittenInPlace)
{
   EXPECT_EQ(roundtrip("Types: deb\nURIs: http://a/\nEnabled: no\nSuites: s\n",
                       [](RDeb822File &f) { EXPECT_TRUE(f.SetEnabled(0, true)); }),
             "Types: deb\nURIs: http://a/\nEnabled: yes\nSuites: s\n");
}

TEST_F(RDeb822FileTest, LeadingBlankLinesAndTrailingCommentWithoutNewline)
{
   const string body =
      "\n\n# lead\nTypes: deb\nURIs: http://a/\nSuites: s\n\n\n\n"
      "Types: deb\nURIs: http://b/\nSuites: t\n# tail";
   string expected = body;
   expected.insert(expected.find("Types: deb\nURIs: http://b/"), "Enabled: no\n");
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) {
                EXPECT_EQ(f.StanzaCount(), 2u);
                EXPECT_TRUE(f.SetEnabled(1, false));
             }),
             expected);
   expected = body;
   expected.insert(expected.find("Types: deb\nURIs: http://a/"), "Enabled: no\n");
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) { EXPECT_TRUE(f.SetEnabled(0, false)); }),
             expected);
}

// A comment line does not separate stanzas; only a blank line does. apt
// sees one stanza here, so the last Suites is the one that counts.
TEST_F(RDeb822FileTest, CommentWithoutBlankLineDoesNotSplitStanzas)
{
   const string body =
      "Types: deb\nURIs: http://a/\nSuites: s\n# comment\nTypes: deb\nURIs: http://b/\nSuites: t\n";
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) {
                EXPECT_EQ(f.StanzaCount(), 1u);
                EXPECT_TRUE(f.SetField(0, "Suites", "u"));
             }),
             "Types: deb\nURIs: http://a/\nSuites: s\n# comment\nTypes: deb\nURIs: http://b/\nSuites: u\n");
}

TEST_F(RDeb822FileTest, ExactPathSuitesAreKept)
{
   for (const char *suite : {"stable/", "stable/binary-$(ARCH)/"}) {
      const string body = string("Types: deb\nURIs: http://a/\nSuites: ") + suite + "\n";
      EXPECT_EQ(roundtrip(body, [](RDeb822File &f) { EXPECT_TRUE(f.SetEnabled(0, false)); }),
                "Enabled: no\n" + body);
   }
}

// sources.list(5): an embedded key, with the empty line encoded as " ."
static const char *INLINE_KEY_FILE =
   "Types: deb\n"
   "URIs: https://deb.debian.org\n"
   "Suites: stable\n"
   "Components: main contrib non-free non-free-firmware\n"
   "Signed-By:\n"
   " -----BEGIN PGP PUBLIC KEY BLOCK-----\n"
   " .\n"
   " mDMEYCQjIxYJKwYBBAHaRw8BAQdAD/P5Nvvnvk66SxBBHDbhRml9ORg1WV5CvzKY\n"
   " CuMfoIS0BmFiY2RlZoiQBBMWCgA4FiEErCIG1VhKWMWo2yfAREZd5NfO31cFAmAk\n"
   " =AbCd\n"
   " -----END PGP PUBLIC KEY BLOCK-----\n";

TEST_F(RDeb822FileTest, EmbeddedKeyBlockIsUntouched)
{
   EXPECT_EQ(roundtrip(INLINE_KEY_FILE, [](RDeb822File &f) { EXPECT_TRUE(f.SetEnabled(0, false)); }),
             string("Enabled: no\n") + INLINE_KEY_FILE);
   string expected = INLINE_KEY_FILE;
   expected.replace(expected.find("Suites: stable\n"), 15, "Suites: testing\n");
   EXPECT_EQ(roundtrip(INLINE_KEY_FILE, [](RDeb822File &f) {
                EXPECT_TRUE(f.SetField(0, "Suites", "testing"));
             }),
             expected);
}

// Stock Ubuntu: Enabled first, two stanzas, nothing else unusual.
static const char *UBUNTU_FILE =
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

TEST_F(RDeb822FileTest, StockUbuntuFileDisableSecondStanza)
{
   string expected = UBUNTU_FILE;
   expected.replace(expected.rfind("Enabled: yes\n"), 13, "Enabled: no\n");
   EXPECT_EQ(roundtrip(UBUNTU_FILE, [](RDeb822File &f) {
                EXPECT_FALSE(f.SetEnabled(0, true));
                EXPECT_TRUE(f.SetEnabled(1, false));
             }),
             expected);
}

// Files larger than pkgTagFile's read buffer: the offsets must still be
// right after the buffer has been refilled many times.
TEST_F(RDeb822FileTest, LargeFileOffsetsStayCorrect)
{
   string body;
   for (int i = 0; i < 3000; i++) {
      if (i % 2)
         body += "# comment " + to_string(i) + "\n";
      body += "Types: deb\nURIs: http://m" + to_string(i) + ".example/debian\nSuites: s" +
              to_string(i) + "\nComponents: main\n\n";
   }
   ASSERT_GT(body.size(), 200000u);

   string expected = body;
   expected.insert(expected.rfind("Types: deb\n"), "Enabled: no\n");
   const size_t mid = expected.find("Suites: s1500\n");
   expected.replace(mid, 14, "Suites: changed\n");
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) {
                EXPECT_EQ(f.StanzaCount(), 3000u);
                EXPECT_TRUE(f.SetField(1500, "Suites", "changed"));
                EXPECT_TRUE(f.SetEnabled(2999, false));
             }),
             expected);
}

TEST_F(RDeb822FileTest, WriteKeepsAOneTimeBackup)
{
   const string rel = "sources.list.d/t.sources";
   box.put(rel, UBUNTU_FILE);
   {
      RDeb822File f(box.path(rel));
      EXPECT_TRUE(f.Read());
      EXPECT_TRUE(f.SetEnabled(0, false));
      EXPECT_TRUE(f.Write());
   }
   EXPECT_EQ(box.get(rel + ".bak"), UBUNTU_FILE);
   {
      RDeb822File f(box.path(rel));
      EXPECT_TRUE(f.Read());
      EXPECT_TRUE(f.SetEnabled(1, false));
      EXPECT_TRUE(f.Write());
   }
   EXPECT_EQ(box.get(rel + ".bak"), UBUNTU_FILE);
   EXPECT_NE(box.get(rel), UBUNTU_FILE);
}

TEST_F(RDeb822FileTest, SameValueWithDifferentSpacingIsANoop)
{
   const string body = "Types: deb\nURIs: http://a/\nSuites:  stable   testing\t\nComponents:\n main\n contrib\n";
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) {
                EXPECT_FALSE(f.SetField(0, "Suites", "stable testing"));
                EXPECT_FALSE(f.SetField(0, "Components", "main contrib"));
                EXPECT_FALSE(f.Changed());
             }),
             body);
}

TEST_F(RDeb822FileTest, RemoveFieldDropsItsLinesOnly)
{
   const string body =
      "Types: deb\nURIs: http://a/\nSuites: s\nComponents:\n main\n# keep\n contrib\n\n"
      "Types: deb\nURIs: http://b/\nSuites: t\n";
   EXPECT_EQ(roundtrip(body, [](RDeb822File &f) {
                EXPECT_TRUE(f.RemoveField(0, "Components"));
                EXPECT_FALSE(f.RemoveField(0, "Components"));
                EXPECT_FALSE(f.RemoveField(1, "Signed-By"));
                // offsets of the following stanza are still right
                EXPECT_TRUE(f.SetField(1, "Suites", "u"));
             }),
             "Types: deb\nURIs: http://a/\nSuites: s\n# keep\n\n"
             "Types: deb\nURIs: http://b/\nSuites: u\n");
}

int main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);
   return RUN_ALL_TESTS();
}
