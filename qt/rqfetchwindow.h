#ifndef RQFETCHWINDOW_H
#define RQFETCHWINDOW_H

#include <vector>

#include <window_fetch.h>

#include <apt-pkg/acquire.h>

using std::vector;

class QWidget;

class RQFetchItem;

class RQFetchWindow : public WindowFetch, public pkgAcquireStatus
{
   Q_OBJECT

   protected:

   vector<RQFetchItem *> _items;

   public:

   RQFetchWindow(QWidget *parent);

   public:

   // pkgAcquireStatus interface.

   virtual bool MediaChange(string Media, string Drive) {};
   virtual void IMSHit(pkgAcquire::ItemDesc &Itm);
   virtual void Fetch(pkgAcquire::ItemDesc &Itm);
   virtual void Done(pkgAcquire::ItemDesc &Itm);
   virtual void Fail(pkgAcquire::ItemDesc &Itm);
   virtual bool Pulse(pkgAcquire *Owner);
   virtual void Start();
   virtual void Stop();
};

#endif

// vim:ts=3:sw=3:et
