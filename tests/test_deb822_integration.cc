#include <gtest/gtest.h>
#include "../common/rsources.h"
#include "../common/rsource_deb822.h"
#include <apt-pkg/error.h>
#include <fstream>
#include <sstream>
#include <cstdio>

class Deb822Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary file for testing
        char tmpname[] = "/tmp/synaptic_test_XXXXXX";
        int fd = mkstemp(tmpname);
        ASSERT_NE(fd, -1);
        close(fd);
        testFile = tmpname;
    }

    void TearDown() override {
        if (!testFile.empty()) {
            remove(testFile.c_str());
        }
    }

    std::string testFile;
};

TEST_F(Deb822Test, ParseSimpleDeb822Source) {
    // Write a test source file
    std::ofstream ofs(testFile.c_str());
    ASSERT_TRUE(ofs.is_open());
    ofs << "Types: deb\n"
        << "URIs: http://deb.debian.org/debian\n"
        << "Suites: trixie\n"
        << "Components: main contrib\n"
        << "Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg\n\n";
    ofs.close();

    std::vector<RDeb822Source::Deb822Entry> entries;
    ASSERT_TRUE(RDeb822Source::ParseDeb822File(testFile, entries));
    ASSERT_EQ(entries.size(), 1);
    
    const auto& entry = entries[0];
    EXPECT_EQ(entry.Types, "deb");
    EXPECT_EQ(entry.URIs, "http://deb.debian.org/debian");
    EXPECT_EQ(entry.Suites, "trixie");
    EXPECT_EQ(entry.Components, "main contrib");
    EXPECT_EQ(entry.SignedBy, "/usr/share/keyrings/debian-archive-keyring.gpg");
    EXPECT_TRUE(entry.Enabled);
}

TEST_F(Deb822Test, ParseMultipleEntries) {
    std::ofstream ofs(testFile.c_str());
    ASSERT_TRUE(ofs.is_open());
    ofs << "Types: deb\n"
        << "URIs: http://security.debian.org/debian-security\n"
        << "Suites: trixie-security\n"
        << "Components: main\n\n"
        << "Types: deb deb-src\n"
        << "URIs: http://deb.debian.org/debian\n"
        << "Suites: trixie trixie-updates\n"
        << "Components: main contrib non-free\n"
        << "Enabled: no\n\n";
    ofs.close();

    std::vector<RDeb822Source::Deb822Entry> entries;
    ASSERT_TRUE(RDeb822Source::ParseDeb822File(testFile, entries));
    ASSERT_EQ(entries.size(), 2);
    
    EXPECT_EQ(entries[0].Types, "deb");
    EXPECT_TRUE(entries[0].Enabled);
    
    EXPECT_EQ(entries[1].Types, "deb deb-src");
    EXPECT_FALSE(entries[1].Enabled);
}

TEST_F(Deb822Test, ConvertBetweenFormats) {
    // Create a Deb822 entry
    RDeb822Source::Deb822Entry deb822Entry;
    deb822Entry.Types = "deb deb-src";
    deb822Entry.URIs = "http://deb.debian.org/debian";
    deb822Entry.Suites = "trixie";
    deb822Entry.Components = "main contrib";
    deb822Entry.SignedBy = "/usr/share/keyrings/debian-archive-keyring.gpg";
    deb822Entry.Enabled = true;

    // Convert to SourceRecord
    SourcesList::SourceRecord sourceRecord;
    ASSERT_TRUE(RDeb822Source::ConvertToSourceRecord(deb822Entry, sourceRecord));

    // Verify conversion
    EXPECT_TRUE(sourceRecord.Type & SourcesList::Deb);
    EXPECT_TRUE(sourceRecord.Type & SourcesList::DebSrc);
    EXPECT_EQ(sourceRecord.URI, "http://deb.debian.org/debian");
    EXPECT_EQ(sourceRecord.Dist, "trixie");
    ASSERT_EQ(sourceRecord.NumSections, 2);
    EXPECT_EQ(sourceRecord.Sections[0], "main");
    EXPECT_EQ(sourceRecord.Sections[1], "contrib");

    // Convert back to Deb822
    RDeb822Source::Deb822Entry convertedEntry;
    ASSERT_TRUE(RDeb822Source::ConvertFromSourceRecord(sourceRecord, convertedEntry));

    // Verify round-trip conversion
    EXPECT_EQ(convertedEntry.Types, "deb deb-src");
    EXPECT_EQ(convertedEntry.URIs, "http://deb.debian.org/debian");
    EXPECT_EQ(convertedEntry.Suites, "trixie");
    EXPECT_EQ(convertedEntry.Components, "main contrib");
    EXPECT_TRUE(convertedEntry.Enabled);
}

TEST_F(Deb822Test, WriteAndReadBack) {
    // Create test entries
    std::vector<RDeb822Source::Deb822Entry> entries;
    RDeb822Source::Deb822Entry entry1;
    entry1.Types = "deb";
    entry1.URIs = "http://example.com/debian";
    entry1.Suites = "stable";
    entry1.Components = "main";
    entry1.Enabled = true;
    entries.push_back(entry1);

    // Write to file
    ASSERT_TRUE(RDeb822Source::WriteDeb822File(testFile, entries));

    // Read back
    std::vector<RDeb822Source::Deb822Entry> readEntries;
    ASSERT_TRUE(RDeb822Source::ParseDeb822File(testFile, readEntries));

    // Compare
    ASSERT_EQ(readEntries.size(), entries.size());
    EXPECT_EQ(readEntries[0].Types, entries[0].Types);
    EXPECT_EQ(readEntries[0].URIs, entries[0].URIs);
    EXPECT_EQ(readEntries[0].Suites, entries[0].Suites);
    EXPECT_EQ(readEntries[0].Components, entries[0].Components);
    EXPECT_EQ(readEntries[0].Enabled, entries[0].Enabled);
}

TEST_F(Deb822Test, HandleComments) {
    std::ofstream ofs(testFile.c_str());
    ASSERT_TRUE(ofs.is_open());
    ofs << "# This is a comment\n"
        << "Types: deb\n"
        << "URIs: http://example.com/debian\n"
        << "# Another comment\n"
        << "Suites: stable\n"
        << "Components: main\n\n";
    ofs.close();

    std::vector<RDeb822Source::Deb822Entry> entries;
    ASSERT_TRUE(RDeb822Source::ParseDeb822File(testFile, entries));
    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].Comment, "# This is a comment\n# Another comment");
}

TEST_F(Deb822Test, HandleInvalidFile) {
    std::ofstream ofs(testFile.c_str());
    ASSERT_TRUE(ofs.is_open());
    ofs << "Types: deb\n"
        << "URIs: http://example.com/debian\n"
        << "# Missing Suites field\n"
        << "Components: main\n\n";
    ofs.close();

    std::vector<RDeb822Source::Deb822Entry> entries;
    EXPECT_FALSE(RDeb822Source::ParseDeb822File(testFile, entries));
}

TEST_F(Deb822Test, SourcesListIntegration) {
    // Write a test Deb822 source file
    std::ofstream ofs(testFile.c_str());
    ASSERT_TRUE(ofs.is_open());
    ofs << "Types: deb deb-src\n"
        << "URIs: http://deb.debian.org/debian\n"
        << "Suites: trixie\n"
        << "Components: main contrib\n"
        << "Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg\n\n";
    ofs.close();

    // Create SourcesList and read the file
    SourcesList sources;
    ASSERT_TRUE(sources.ReadDeb822SourcePart(testFile));

    // Verify the source was read correctly
    ASSERT_FALSE(sources.SourceRecords.empty());
    auto record = sources.SourceRecords.front();
    EXPECT_TRUE(record->Type & SourcesList::Deb822);
    EXPECT_TRUE(record->Type & SourcesList::Deb);
    EXPECT_TRUE(record->Type & SourcesList::DebSrc);
    EXPECT_EQ(record->URI, "http://deb.debian.org/debian");
    EXPECT_EQ(record->Dist, "trixie");
    ASSERT_EQ(record->NumSections, 2);
    EXPECT_EQ(record->Sections[0], "main");
    EXPECT_EQ(record->Sections[1], "contrib");

    // Write back to a new file
    std::string newFile = testFile + ".new";
    ASSERT_TRUE(sources.WriteDeb822Source(record, newFile));

    // Read the new file and verify contents
    std::vector<RDeb822Source::Deb822Entry> entries;
    ASSERT_TRUE(RDeb822Source::ParseDeb822File(newFile, entries));
    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].Types, "deb deb-src");
    EXPECT_EQ(entries[0].URIs, "http://deb.debian.org/debian");
    EXPECT_EQ(entries[0].Suites, "trixie");
    EXPECT_EQ(entries[0].Components, "main contrib");

    // Clean up
    remove(newFile.c_str());
} 

TEST_F(Deb822Test, ParseAdditionalFields) {
    std::ofstream ofs(testFile.c_str());
    ASSERT_TRUE(ofs.is_open());
    ofs << "Types: deb deb-src\n"
        << "URIs: http://deb.debian.org/debian\n"
        << "Suites: trixie\n"
        << "Components: main contrib\n"
        << "Architectures: amd64 arm64\n"
        << "Languages: en fr de\n"
        << "Targets: stable-updates\n"
        << "Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg\n\n";
    ofs.close();

    std::vector<RDeb822Source::Deb822Entry> entries;
    ASSERT_TRUE(RDeb822Source::ParseDeb822File(testFile, entries));
    ASSERT_EQ(entries.size(), 1);
    
    const auto& entry = entries[0];
    EXPECT_EQ(entry.Types, "deb deb-src");
    EXPECT_EQ(entry.URIs, "http://deb.debian.org/debian");
    EXPECT_EQ(entry.Suites, "trixie");
    EXPECT_EQ(entry.Components, "main contrib");
    EXPECT_EQ(entry.Architectures, "amd64 arm64");
    EXPECT_EQ(entry.Languages, "en fr de");
    EXPECT_EQ(entry.Targets, "stable-updates");
    EXPECT_EQ(entry.SignedBy, "/usr/share/keyrings/debian-archive-keyring.gpg");
    EXPECT_TRUE(entry.Enabled);
}

TEST_F(Deb822Test, ConvertAdditionalFields) {
    // Create a Deb822 entry with additional fields
    RDeb822Source::Deb822Entry deb822Entry;
    deb822Entry.Types = "deb deb-src";
    deb822Entry.URIs = "http://deb.debian.org/debian";
    deb822Entry.Suites = "trixie";
    deb822Entry.Components = "main contrib";
    deb822Entry.Architectures = "amd64 arm64";
    deb822Entry.Languages = "en fr de";
    deb822Entry.Targets = "stable-updates";
    deb822Entry.SignedBy = "/usr/share/keyrings/debian-archive-keyring.gpg";
    deb822Entry.Enabled = true;

    // Convert to SourceRecord
    SourcesList::SourceRecord sourceRecord;
    ASSERT_TRUE(RDeb822Source::ConvertToSourceRecord(deb822Entry, sourceRecord));

    // Verify conversion
    EXPECT_TRUE(sourceRecord.Type & SourcesList::Deb);
    EXPECT_TRUE(sourceRecord.Type & SourcesList::DebSrc);
    EXPECT_EQ(sourceRecord.URI, "http://deb.debian.org/debian");
    EXPECT_EQ(sourceRecord.Dist, "trixie");
    ASSERT_EQ(sourceRecord.NumSections, 2);
    EXPECT_EQ(sourceRecord.Sections[0], "main");
    EXPECT_EQ(sourceRecord.Sections[1], "contrib");

    // Check that additional fields are stored in Comment
    EXPECT_TRUE(sourceRecord.Comment.find("Architectures: amd64 arm64") != std::string::npos);
    EXPECT_TRUE(sourceRecord.Comment.find("Languages: en fr de") != std::string::npos);
    EXPECT_TRUE(sourceRecord.Comment.find("Targets: stable-updates") != std::string::npos);
    EXPECT_TRUE(sourceRecord.Comment.find("Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg") != std::string::npos);

    // Convert back to Deb822
    RDeb822Source::Deb822Entry convertedEntry;
    ASSERT_TRUE(RDeb822Source::ConvertFromSourceRecord(sourceRecord, convertedEntry));

    // Verify round-trip conversion
    EXPECT_EQ(convertedEntry.Types, "deb deb-src");
    EXPECT_EQ(convertedEntry.URIs, "http://deb.debian.org/debian");
    EXPECT_EQ(convertedEntry.Suites, "trixie");
    EXPECT_EQ(convertedEntry.Components, "main contrib");
    EXPECT_EQ(convertedEntry.Architectures, "amd64 arm64");
    EXPECT_EQ(convertedEntry.Languages, "en fr de");
    EXPECT_EQ(convertedEntry.Targets, "stable-updates");
    EXPECT_EQ(convertedEntry.SignedBy, "/usr/share/keyrings/debian-archive-keyring.gpg");
    EXPECT_TRUE(convertedEntry.Enabled);
}

TEST_F(Deb822Test, WriteAndReadAdditionalFields) {
    // Create temporary file
    std::string tempFile = "temp_test.sources";
    std::ofstream out(tempFile);
    out << "Types: deb deb-src\n"
        << "URIs: http://deb.debian.org/debian\n"
        << "Suites: trixie\n"
        << "Components: main contrib\n"
        << "Architectures: amd64 arm64\n"
        << "Languages: en fr de\n"
        << "Targets: stable-updates\n"
        << "Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg\n";
    out.close();

    // Read it back
    std::vector<RDeb822Source::Deb822Entry> entries;
    std::string error;
    EXPECT_TRUE(RDeb822Source::ParseDeb822File(tempFile, entries, error)) << error;
    EXPECT_EQ(entries.size(), 1);

    // Verify fields
    const auto& entry = entries[0];
    EXPECT_EQ(entry.Types, "deb deb-src");
    EXPECT_EQ(entry.URIs, "http://deb.debian.org/debian");
    EXPECT_EQ(entry.Suites, "trixie");
    EXPECT_EQ(entry.Components, "main contrib");
    EXPECT_EQ(entry.Architectures, "amd64 arm64");
    EXPECT_EQ(entry.Languages, "en fr de");
    EXPECT_EQ(entry.Targets, "stable-updates");
    EXPECT_EQ(entry.SignedBy, "/usr/share/keyrings/debian-archive-keyring.gpg");

    // Clean up
    std::remove(tempFile.c_str());
} 