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
};

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

int main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);
   return RUN_ALL_TESTS();
}
