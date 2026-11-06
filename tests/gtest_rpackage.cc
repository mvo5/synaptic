#include "rpackage.h"

#include <gtest/gtest.h>
#include <string>

TEST(RPackageTest, debParser)
{
   // Each line of the long description must start with a space, and blank lines should contain a single '.' following the space.

   {  // Empty string.
      const std::string value = "";
      const std::string expected = "";
      EXPECT_EQ(RPackage::debParser(value), expected);
   }

   {  // One line, no line feed.
      const std::string value = "1234 5678 90";
      const std::string expected = "";
      EXPECT_EQ(RPackage::debParser(value), expected);
   }

   {  // One line, with line feed.
      const std::string value = "1234 5678 90/n";
      const std::string expected = "";
      EXPECT_EQ(RPackage::debParser(value), expected);
   }

   {  // One line, with line feed and space.
      const std::string value = "1234 5678 90/n ";
      const std::string expected = "";
      EXPECT_EQ(RPackage::debParser(value), expected);
   }

   {  // Description string
      const std::string value =
         "Description: GNU grep, egrep and fgrep.\n"
         " The GNU family of grep utilities may be the \"fastest grep in the west\".\n"
         " .\n"
         " GNU grep is based on a fast lazy-state deterministic matcher\n"
         "   (about twice as fast as stock Unix egrep)\n"
         " hybridized with a Boyer-Moore-Gosper...";

      const std::string expected =
         "The GNU family of grep utilities may be the \"fastest grep in the west\".\n"
         "\n"
         "GNU grep is based on a fast lazy-state deterministic matcher\n"
         "  (about twice as fast as stock Unix egrep)\n"
         "hybridized with a Boyer-Moore-Gosper...";

      EXPECT_EQ(RPackage::debParser(value), expected);
   }
}

TEST(RPackageTest, stripWsParser)
{
   // Each line of the long description must start with a space, and blank lines should contain a single '.' following the space.

   // stripWsParser() is expected to return a single line description.

   {  // Empty string.
      const std::string value = "";
      const std::string expected = "";
      EXPECT_EQ(RPackage::stripWsParser(value), expected);
   }

   {  // Description string
      const std::string value =
         "Description: GNU grep, egrep and fgrep.\n"
         " The GNU family of grep utilities may be the \"fastest grep in the west\".\n"
         " .\n"
         " GNU grep is based on a fast lazy-state deterministic matcher\n"
         "   (about twice as fast as stock Unix egrep)\n"
         " hybridized with a Boyer-Moore-Gosper...";

      const std::string expected =
         "Description: GNU grep, egrep and fgrep. The GNU family of grep utilities may be the "
         "\"fastest grep in the west\". GNU grep is based on a fast lazy-state deterministic matcher"
         "   (about twice as fast as stock Unix egrep) hybridized with a Boyer-Moore-Gosper...";

      // p.s. Notice the extra spaces at "...deterministic matcher   (about..."
      // This matches the behaviour of the original parser implementation.

      EXPECT_EQ(RPackage::stripWsParser(value), expected);
   }
}
