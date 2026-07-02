// helper code for the swig generated bindings

#include<apt-pkg/configuration.h>
#include<apt-pkg/init.h>
#include<apt-pkg/progress.h>
#include<apt-pkg/acquire.h>
#include "rinstallprogress.h"

bool RInitSystem();

class SwigOpProgress : public OpProgress {
 protected:
   virtual void Update() { UpdateStatus(Percent); }
 public:
   virtual void UpdateStatus(float p) {}
   virtual void Done() {}
};


class SwigInstallProgress : public RInstallProgress {
 public:
   virtual void startUpdate() {
   }
   virtual void updateInterface() {
   }
   virtual void finishUpdate() {
   }
   // get a str feed to the user with the result of the install run
   virtual const char* getResultStr(pkgPackageManager::OrderResult r) {
      RInstallProgress::getResultStr(r);
   }
   virtual pkgPackageManager::OrderResult start(RPackageManager *pm,
                                                int numPackages = 0,
                                                int numPackagesTotal = 0)
   {
      return RInstallProgress::start(pm,numPackages,numPackagesTotal);
   }
};

class pkgAcquire;
class pkgAcquireStatus;
class Item;
struct ItemDesc
{
   string URI;
   string Description;
   string ShortDesc;
   Item *Owner;
};

class SwigAcquireStatus : public pkgAcquireStatus
{
 protected:
   virtual task<bool> Pulse(pkgAcquire *Owner) {
      bool result = co_await pkgAcquireStatus::Pulse(Owner);
      co_await UpdatePulse(FetchedBytes, CurrentCPS, CurrentItems);
      co_return result;
   }

 public:
   // Called by items when they have finished a real download
   virtual void Fetched(unsigned long Size,unsigned long ResumePoint) {
      pkgAcquireStatus::Fetched(Size, ResumePoint);
   }

   // Called to change media
   virtual task<bool> MediaChange(string Media,string Drive) = 0;

   // Each of these is called by the workers when an event occures
   virtual task<void> IMSHit(ItemDesc &/*Itm*/) { co_return; }
   virtual task<void> Fetch(ItemDesc &/*Itm*/) { co_return; }
   virtual task<void> Done(ItemDesc &/*Itm*/) { co_return; }
   virtual task<void> Fail(ItemDesc &/*Itm*/) { co_return; }
   virtual task<void> UpdatePulse(double FetchedBytes, double CurrentCPS, unsigned long CurrentItems) { co_return; }
   virtual task<void> Start() {
      co_await pkgAcquireStatus::Start();
   }
   virtual task<void> Stop() {
      co_await pkgAcquireStatus::Stop();
   }
};
