// helper code for the swig generated bindings

#include<apt-pkg/configuration.h>
#include<apt-pkg/init.h>
#include<apt-pkg/progress.h>
#include<apt-pkg/acquire.h>
#include "rinstallprogress.h"

bool RInitSystem();

class SwigOpProgress : public OpProgress {
 protected:
   virtual void Update() { UpdateStatus(Percent); };
 public:
   virtual void UpdateStatus(float p) {};
   virtual void Done() {};
};


class SwigInstallProgress : public RInstallProgress {
 public:
   virtual void startUpdate() {
   };
   virtual void updateInterface() {
   };
   virtual void finishUpdate() {
   };
   // get a str feed to the user with the result of the install run
   virtual const char* getResultStr(pkgPackageManager::OrderResult r) {
      RInstallProgress::getResultStr(r);
   };
   virtual pkgPackageManager::OrderResult start(RPackageManager *pm,
                                                int numPackages = 0,
                                                int numPackagesTotal = 0) 
   {
      return RInstallProgress::start(pm,numPackages,numPackagesTotal);
   };
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
   virtual bool Pulse(pkgAcquire *Owner) {
      pkgAcquireStatus::Pulse(Owner);
      UpdatePulse(FetchedBytes, CurrentCPS, CurrentItems);
   };
 public:
   // Called by items when they have finished a real download
   virtual void Fetched(unsigned long Size,unsigned long ResumePoint) {
      pkgAcquireStatus::Fetched(Size, ResumePoint);
   };
   
   // Called to change media
   virtual bool MediaChange(string Media,string Drive) = 0;
   
   // Each of these is called by the workers when an event occures
   virtual void IMSHit(ItemDesc &/*Itm*/) {};
   virtual void Fetch(ItemDesc &/*Itm*/) {};
   virtual void Done(ItemDesc &/*Itm*/) {};
   virtual void Fail(ItemDesc &/*Itm*/) {};
   virtual void UpdatePulse(double FetchedBytes, double CurrentCPS, unsigned long CurrentItems) {};
   virtual void Start() {
      pkgAcquireStatus::Start();
   };
   virtual void Stop() {
      pkgAcquireStatus::Stop();
   };

};
