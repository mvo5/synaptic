
#include <qapplication.h>
#include <qpushbutton.h>

#include <rqfetchwindow.h>
#include <rqfetchitem.h>

#include <apt-pkg/acquire-item.h>
#include <apt-pkg/acquire-worker.h>

RQFetchWindow::RQFetchWindow(QWidget *parent)
   :  WindowFetch(parent)
{
   setModal(true);

   connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void RQFetchWindow::Start()
{
   pkgAcquireStatus::Start();
   setResult(Accepted);
}

void RQFetchWindow::Stop()
{
   pkgAcquireStatus::Stop();
   accept();
   qApp->processEvents();
}

void RQFetchWindow::Fetch(pkgAcquire::ItemDesc &itemDesc)
{
   if (itemDesc.Owner->Complete)
      return;

   if (!isVisible() && result() != Rejected)
      show();

   itemDesc.Owner->ID = _items.size()+1;

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

   RQFetchItem *item = _items[itemDesc.Owner->ID-1];
   item->setDone();
   item->repaint();
}

void RQFetchWindow::IMSHit(pkgAcquire::ItemDesc &itemDesc)
{
   if (itemDesc.Owner->ID == 0)
      Fetch(itemDesc);

   RQFetchItem *item = _items[itemDesc.Owner->ID-1];
   item->setDone();
   item->repaint();
}

void RQFetchWindow::Fail(pkgAcquire::ItemDesc &itemDesc)
{
   if (itemDesc.Owner->ID == 0)
      Fetch(itemDesc);

   if (itemDesc.Owner->Status != pkgAcquire::Item::StatIdle) {
      RQFetchItem *item = _items[itemDesc.Owner->ID-1];
      item->setFailed();
      item->repaint();
   }
}

bool RQFetchWindow::Pulse(pkgAcquire *Owner)
{
   pkgAcquireStatus::Pulse(Owner);

   if (!isVisible() && result() != Rejected)
      show();

   pkgAcquire::Worker *I = Owner->WorkersBegin();

   for (; I != NULL; I = Owner->WorkerStep(I)) {
      if (I->CurrentItem == 0)
         continue;
      RQFetchItem *item = _items[I->CurrentItem->Owner->ID-1];

      // Based on 100 to reduce the number of repaints.
      if (item->setProgress((int)((float)I->CurrentSize*100)/I->TotalSize))
         item->repaint();
   }

   _progressBar->setProgress((int)CurrentBytes + CurrentItems,
                             (int)TotalBytes + TotalItems);

   qApp->processEvents();

   return (result() != Rejected);
}

// vim:ts=3:sw=3:et
