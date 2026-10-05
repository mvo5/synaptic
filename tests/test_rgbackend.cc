// Drives the client against the real daemon, unprivileged: the
// transport, the GSource and the streaming reply path all get
// exercised; only the locks are missing, which is what the daemon
// must then report.
#include "rcommit.h"
#include "rfetchevent.h"
#include "rfetchstatus.h"
#include "rgbackend.h"

#include <apt-pkg/error.h>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>
#include <vector>

static std::string daemonPath;

class CountingStatus : public RFetchStatus
{
 public:
   int events = 0;
   bool handleFetchEvent(const FetchEvent &) override
   {
      events++;
      return true;
   }
   bool MediaChange(std::string, std::string) override
   {
      return false;
   }
};

TEST(RGBackend, StartReportsMissingLocks)
{
   RGBackend backend(daemonPath, /* viaPkexec */ false);
   std::string error;
   ASSERT_TRUE(backend.start(error)) << error;
   EXPECT_FALSE(backend.locked());
   EXPECT_NE(backend.lockError().find("lock"), std::string::npos)
      << backend.lockError();
}

TEST(RGBackend, UpdateCacheWithoutLocksFails)
{
   RGBackend backend(daemonPath, /* viaPkexec */ false);
   std::string error;
   ASSERT_TRUE(backend.start(error)) << error;

   CountingStatus status;
   EXPECT_FALSE(backend.updateCache(&status, error));
   EXPECT_NE(error.find("NotLocked"), std::string::npos) << error;
   EXPECT_EQ(status.events, 0);
}

class CountingInstall : public RInstallEventHandler
{
 public:
   int events = 0;
   void handleInstallEvent(const InstallEvent &) override
   {
      events++;
   }
   void attachTerminal(int fd) override
   {
      close(fd);
   }
};

// The daemon validates the parameters against its interface before
// looking at the locks, so NotLocked proves the JSON we build is right.
TEST(RGBackend, CommitWithoutLocksFails)
{
   RGBackend backend(daemonPath, /* viaPkexec */ false);
   std::string error;
   ASSERT_TRUE(backend.start(error)) << error;

   std::vector<Selection> sels(1);
   sels[0].name = "hello";
   sels[0].arch = "amd64";
   sels[0].action = Selection::Install;
   sels[0].version = "1.0";
   CommitOptions options;
   options.conffile = CommitOptions::KeepConffile;

   CountingStatus fetch;
   CountingInstall install;
   EXPECT_FALSE(backend.commit(sels, options, &fetch, &install, error));
   EXPECT_EQ(backend.lastErrorId(), "io.github.mvo5.synaptic.NotLocked")
      << error;
   EXPECT_EQ(fetch.events, 0);
   EXPECT_EQ(install.events, 0);
}

TEST(RGBackend, CommitRejectsAskWithoutTerminal)
{
   RGBackend backend(daemonPath, /* viaPkexec */ false);
   std::string error;
   ASSERT_TRUE(backend.start(error)) << error;

   CommitOptions options;
   CountingStatus fetch;
   CountingInstall install;
   EXPECT_FALSE(backend.commit({}, options, &fetch, &install, error));
   EXPECT_EQ(backend.lastErrorId(), "org.varlink.service.InvalidParameter")
      << error;
}

TEST(RGBackend, MissingDaemonFailsToStart)
{
   RGBackend backend("/nonexistent/synapticd", /* viaPkexec */ false);
   std::string error;
   EXPECT_FALSE(backend.start(error));
   EXPECT_FALSE(error.empty());
}

int main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   if (argc < 2) {
      fprintf(stderr, "usage: %s /path/to/synapticd\n", argv[0]);
      return 2;
   }
   daemonPath = argv[1];
   return RUN_ALL_TESTS();
}
