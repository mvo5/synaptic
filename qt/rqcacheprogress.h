#ifndef RGCACHEPROGRESS_H
#define RGCACHEPROGRESS_H

#include <apt-pkg/progress.h>

#include <rqmainwindow.h>
#include <qprogressbar.h>

class RQCacheProgress : public OpProgress {

   QProgressBar *_prog;
   QMainWindow *_main;

 public:

   RQCacheProgress(QMainWindow *main);
   ~RQCacheProgress();

   virtual void Update();
   virtual void Done();
};

#endif

// vim:ts=3:sw=3:et
