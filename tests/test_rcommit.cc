#include "rcommit.h"

#include <gtest/gtest.h>
#include <string>
#include <systemd/sd-json.h>

static std::string format(sd_json_variant *v)
{
   char *s = nullptr;
   EXPECT_GE(sd_json_variant_format(v, (sd_json_format_flags_t)0, &s), 0);
   std::string out = s != nullptr ? s : "";
   free(s);
   return out;
}

TEST(Commit, SelectionsToJson)
{
   std::vector<Selection> sels(2);
   sels[0].name = "hello";
   sels[0].arch = "amd64";
   sels[0].action = Selection::Install;
   sels[0].version = "2.10-3";
   sels[1].name = "cruft";
   sels[1].arch = "all";
   sels[1].action = Selection::Purge;
   sels[1].automatic = true;

   sd_json_variant *v = nullptr;
   ASSERT_GE(selectionsToJson(sels, &v), 0);
   EXPECT_EQ(format(v),
             "[{\"name\":\"hello\",\"arch\":\"amd64\",\"action\":\"install\","
             "\"version\":\"2.10-3\",\"auto\":false},"
             "{\"name\":\"cruft\",\"arch\":\"all\",\"action\":\"purge\","
             "\"auto\":true}]");
   sd_json_variant_unref(v);
}

TEST(Commit, EmptySelectionsIsAnArray)
{
   sd_json_variant *v = nullptr;
   ASSERT_GE(selectionsToJson({}, &v), 0);
   EXPECT_EQ(format(v), "[]");
   sd_json_variant_unref(v);
}

TEST(Commit, OptionsToJson)
{
   CommitOptions o;
   o.conffile = CommitOptions::Replace;
   o.terminal = true;
   sd_json_variant *v = nullptr;
   ASSERT_GE(commitOptionsToJson(o, &v), 0);
   EXPECT_EQ(
      format(v),
      "{\"download_only\":false,\"fix_missing\":false,\"conffile\":\"replace\","
      "\"terminal\":true}");
   sd_json_variant_unref(v);
}

TEST(Commit, InstallEventFromJson)
{
   sd_json_variant *v = nullptr;
   ASSERT_GE(sd_json_buildo(
                &v,
                SD_JSON_BUILD_PAIR_STRING("kind", "conffile"),
                SD_JSON_BUILD_PAIR_STRING("package", "/etc/foo.conf"),
                SD_JSON_BUILD_PAIR_INTEGER("percent", 42),
                SD_JSON_BUILD_PAIR_STRING(
                   "message", "'/etc/foo.conf' '/etc/foo.conf.dpkg-new' 1 1")),
             0);
   InstallEvent ev;
   ASSERT_TRUE(installEventFromJson(v, ev));
   EXPECT_EQ(ev.kind, InstallEvent::Conffile);
   EXPECT_EQ(ev.package, "/etc/foo.conf");
   EXPECT_EQ(ev.percent, 42);
   EXPECT_NE(ev.message.find("dpkg-new"), std::string::npos);
   sd_json_variant_unref(v);

   ASSERT_GE(sd_json_buildo(&v, SD_JSON_BUILD_PAIR_STRING("kind", "terminal")),
             0);
   ASSERT_TRUE(installEventFromJson(v, ev));
   EXPECT_EQ(ev.kind, InstallEvent::Terminal);
   EXPECT_EQ(ev.percent, -1);
   sd_json_variant_unref(v);

   ASSERT_GE(sd_json_buildo(&v, SD_JSON_BUILD_PAIR_STRING("kind", "bogus")), 0);
   EXPECT_FALSE(installEventFromJson(v, ev));
   sd_json_variant_unref(v);
   EXPECT_FALSE(installEventFromJson(nullptr, ev));
}

int main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
