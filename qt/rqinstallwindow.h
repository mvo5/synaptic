#ifndef RQINSTALLWINDOW_H
#define RQINSTALLWINDOW_H

#include <rinstallprogress.h>
#include <qapplication.h>

class RQDummyInstallWindow : public RInstallProgress {

   protected:

   virtual void startUpdate() { qApp->processEvents(); };
   virtual void updateInterface() { qApp->processEvents(); };
   virtual void finishUpdate() { qApp->processEvents(); };

   public:

   RQDummyInstallWindow() : RInstallProgress() {};
   virtual ~RQDummyInstallWindow() {};
};

#endif
