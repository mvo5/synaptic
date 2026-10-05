#include "rfetchevent.h"

#include <gtest/gtest.h>
#include <systemd/sd-json.h>

static FetchEvent roundTrip(const FetchEvent &in)
{
   sd_json_variant *v = nullptr;
   EXPECT_GE(fetchEventToJson(in, &v), 0);
   FetchEvent out;
   EXPECT_TRUE(fetchEventFromJson(v, out));
   sd_json_variant_unref(v);
   return out;
}

TEST(FetchEvent, KindNames)
{
   FetchEvent::Kind kind;
   EXPECT_TRUE(FetchEvent::kindFromName("pulse", kind));
   EXPECT_EQ(kind, FetchEvent::Pulse);
   EXPECT_FALSE(FetchEvent::kindFromName("bogus", kind));
}

TEST(FetchEvent, ItemRoundTrip)
{
   FetchEvent in;
   in.kind = FetchEvent::Fail;
   in.hasItem = true;
   in.item.id = 7;
   in.item.uri = "https://deb.debian.org/debian/dists/sid/InRelease";
   in.item.description = "https://deb.debian.org/debian sid InRelease";
   in.item.shortDescription = "InRelease";
   in.item.size = 12345;
   in.error = "no public key";

   FetchEvent out = roundTrip(in);
   EXPECT_EQ(out.kind, FetchEvent::Fail);
   ASSERT_TRUE(out.hasItem);
   EXPECT_EQ(out.item.id, 7u);
   EXPECT_EQ(out.item.uri, in.item.uri);
   EXPECT_EQ(out.item.description, in.item.description);
   EXPECT_EQ(out.item.shortDescription, in.item.shortDescription);
   EXPECT_EQ(out.item.size, 12345u);
   EXPECT_EQ(out.error, "no public key");
   EXPECT_TRUE(out.workers.empty());
}

TEST(FetchEvent, PulseRoundTrip)
{
   FetchEvent in;
   in.kind = FetchEvent::Pulse;
   in.currentBytes = 10;
   in.totalBytes = 100;
   in.currentItems = 1;
   in.totalItems = 3;
   in.currentCPS = 42;
   FetchWorker w;
   w.item.id = 2;
   w.item.uri = "http://x/y.deb";
   w.item.description = "x y";
   w.item.shortDescription = "y";
   w.current = 5;
   w.total = 50;
   in.workers.push_back(w);

   FetchEvent out = roundTrip(in);
   EXPECT_EQ(out.kind, FetchEvent::Pulse);
   EXPECT_FALSE(out.hasItem);
   EXPECT_EQ(out.currentBytes, 10u);
   EXPECT_EQ(out.totalBytes, 100u);
   EXPECT_EQ(out.currentItems, 1u);
   EXPECT_EQ(out.totalItems, 3u);
   EXPECT_EQ(out.currentCPS, 42u);
   ASSERT_EQ(out.workers.size(), 1u);
   EXPECT_EQ(out.workers[0].item.id, 2u);
   EXPECT_EQ(out.workers[0].current, 5u);
   EXPECT_EQ(out.workers[0].total, 50u);
}

TEST(FetchEvent, RejectsGarbage)
{
   sd_json_variant *v = nullptr;
   ASSERT_GE(sd_json_buildo(&v, SD_JSON_BUILD_PAIR_STRING("kind", "nonsense")),
             0);
   FetchEvent out;
   EXPECT_FALSE(fetchEventFromJson(v, out));
   sd_json_variant_unref(v);
   EXPECT_FALSE(fetchEventFromJson(nullptr, out));
}

int main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
