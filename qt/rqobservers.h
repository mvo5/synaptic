
#ifndef RQOBSERVERS_H
#define RQOBSERVERS_H

#include <qobject.h>

#include <rpackagelister.h>

class RQCacheObserver : public QObject, public RCacheObserver
{
   Q_OBJECT

   signals:

   void cacheOpen();
   void cachePreChange();
   void cachePostChange();

   public:

   void notifyCacheOpen() { emit cacheOpen(); };
   void notifyCachePreChange() { emit cachePreChange(); };
   void notifyCachePostChange() { emit cachePostChange(); };

};

extern RQCacheObserver cacheObserver;

#endif

// vim:ts=3:sw=3:et
