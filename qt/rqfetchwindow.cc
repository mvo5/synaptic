
#include <qapplication.h>

#include <rqfetchwindow.h>
#include <rqfetchitem.h>

#include <apt-pkg/acquire-item.h>
#include <apt-pkg/acquire-worker.h>

RQFetchWindow::RQFetchWindow(QWidget *parent)
   :  WindowFetch(parent)
{
}

void RQFetchWindow::Start()
{
   pkgAcquireStatus::Start();
   show();
   qApp->processEvents();
}

void RQFetchWindow::Stop()
{
   pkgAcquireStatus::Stop();
   hide();
   qApp->processEvents();
}

void RQFetchWindow::Fetch(pkgAcquire::ItemDesc &itemDesc)
{
   if (itemDesc.Owner->Complete)
      return;

   itemDesc.Owner->ID = _items.size()+1;

   printf("Fetch (%d)!\n", itemDesc.Owner->ID);

   RQFetchItem *item = new RQFetchItem(_progressListView, itemDesc);
   _items.push_back(item);

   // Redraw the new item now.
   _progressListView->triggerUpdate();
   qApp->processEvents();
}

void RQFetchWindow::Done(pkgAcquire::ItemDesc &itemDesc)
{
   if (itemDesc.Owner->ID == 0)
      Fetch(itemDesc);

   printf("Done (%d)!\n", itemDesc.Owner->ID);
   RQFetchItem *item = _items[itemDesc.Owner->ID-1];
   item->setDone();
   item->repaint();
}

void RQFetchWindow::IMSHit(pkgAcquire::ItemDesc &itemDesc)
{
   if (itemDesc.Owner->ID == 0)
      Fetch(itemDesc);

   printf("IMSHit (%d)!\n", itemDesc.Owner->ID);
   RQFetchItem *item = _items[itemDesc.Owner->ID-1];
   item->setDone();
   item->repaint();
}

void RQFetchWindow::Fail(pkgAcquire::ItemDesc &itemDesc)
{
   if (itemDesc.Owner->ID == 0)
      Fetch(itemDesc);

   if (itemDesc.Owner->Status != pkgAcquire::Item::StatIdle) {
      printf("Fail (%d/%d)!\n", itemDesc.Owner->ID-1, _items.size());
      RQFetchItem *item = _items[itemDesc.Owner->ID-1];
      item->setFailed();
      item->repaint();
   }
}

bool RQFetchWindow::Pulse(pkgAcquire *Owner)
{
   pkgAcquireStatus::Pulse(Owner);

   pkgAcquire::Worker *I = Owner->WorkersBegin();

   for (; I != NULL; I = Owner->WorkerStep(I)) {
      if (I->CurrentItem == 0)
         continue;
      RQFetchItem *item = _items[I->CurrentItem->Owner->ID-1];
      if (item->setProgress(I->CurrentSize, I->TotalSize))
         item->repaint();
      printf("SetProgress (%d)!\n", I->CurrentItem->Owner->ID);

   }

   qApp->processEvents();

   return true;
}

// vim:ts=3:sw=3:et
