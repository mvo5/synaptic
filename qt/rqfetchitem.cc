
#include <qlabel.h>
#include <qheader.h>

#include <qapplication.h>
#include <qprogressbar.h>
#include <qsizepolicy.h>
#include <qstring.h>
#include <qpainter.h>

#include <apt-pkg/acquire.h>
#include <apt-pkg/acquire-item.h>
#include <apt-pkg/strutl.h>

#include <rqfetchitem.h>

#include <iostream>

RQFetchItem::RQFetchItem(QListView *parent, pkgAcquire::ItemDesc &itemDesc)
   : QListViewItem(parent), _itemDesc(itemDesc)
{
   _progressBar.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
   _progressBar.setFixedSize(100, height());
   _progressBarUpdated = true;
}

bool RQFetchItem::setProgress(int progress)
{
   if (progress != _progressBar.progress()) {
      _progressBar.setProgress(progress);
      _progressBarUpdated = true;
   }
   return _progressBarUpdated;
}

void RQFetchItem::setDone()
{
   _progressBar.setProgress(1, 1);
   _progressBar.setEnabled(false);
   _progressBarUpdated = true;
}

void RQFetchItem::setFailed()
{
   _progressBar.setEnabled(false);
   _progressBarUpdated = true;
}

QString RQFetchItem::text(int column) const
{
   QString res;

   switch (column) {

      case 1:
         res = SizeToStr(_itemDesc.Owner->FileSize);
         break;

      case 2:
         res = _itemDesc.Description.c_str();
         break;

   }

   return res;
}

void RQFetchItem::paintCell(QPainter *p, const QColorGroup &cg,
                             int column, int width, int align)
{
   if (column == 0 && _progressBarUpdated) {
      _progressBarUpdated = false;
      _pm = QPixmap::grabWidget(&_progressBar);
   }
   QListViewItem::paintCell(p, cg, column, width, align);
}

const QPixmap *RQFetchItem::pixmap(int column) const
{
   return (column == 0) ? &_pm : NULL;
}

int RQFetchItem::width(const QFontMetrics &fm, const QListView *lv, int c) const
{
   if (c == 0)
      return 100+1+lv->itemMargin()*2;
   return QListViewItem::width(fm, lv, c);
}

int RQFetchItem::compare(QListViewItem *item, int col, bool ascending) const
{
   int thisID = _itemDesc.Owner->ID;
   int itemID = ((RQFetchItem *)item)->_itemDesc.Owner->ID;
   return (thisID > itemID) ? -1 : ((thisID < itemID) ? 1 : 0);
}

// vim:ts=3:sw=3:et
