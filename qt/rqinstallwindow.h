#ifndef RQINSTALLWINDOW_H
#define RQINSTALLWINDOW_H

#include <rinstallprogress.h>
#include <qapplication.h>

#include <window_install.h>

#include <map>
#include <string>

using namespace std;

class RPackageLister;

class RQDummyInstallWindow : public RInstallProgress {

   protected:

   virtual void startUpdate() { qApp->processEvents(); };
   virtual void updateInterface() { qApp->processEvents(); };
   virtual void finishUpdate() { qApp->processEvents(); };

   public:

   RQDummyInstallWindow() : RInstallProgress() {};
   virtual ~RQDummyInstallWindow() {};
};

class RQInstallWindow : public WindowInstall, public RInstallProgress
{
   Q_OBJECT

   protected:

   bool _startCounting;

   map<string, string> _summaryMap;

   virtual void startUpdate();
   virtual void updateInterface();
   virtual void finishUpdate();

   virtual void prepare(RPackageLister *lister);

   public:

   RQInstallWindow(QWidget *parent, RPackageLister *lister);
   ~RQInstallWindow();
};


#endif

// vim:ts=3:sw=3:et
