
#ifndef RQFETCHITEM_H
#define RQFETCHITEM_H

#include <qlistview.h>
#include <qstring.h>
#include <qprogressbar.h>
#include <qpixmap.h>

class pkgAcquire::ItemDesc;

class QPainter;
class QColorGroup;

class RQFetchItem : public QListViewItem
{

   protected:

   pkgAcquire::ItemDesc _itemDesc;
   QProgressBar _progressBar;
   QPixmap _progressBarPixmap;
   QPixmap _statusPixmap;

   bool _progressBarUpdated;

   void paintCell(QPainter *p, const QColorGroup &cg,
                  int column, int width, int align);
   
   public:

   bool setProgress(int progress);
   void setDone();
   void setFailed();

   QString text(int column) const;
   const QPixmap *pixmap(int column) const;
   int width(const QFontMetrics &fm, const QListView *lv, int c) const;
   int compare(QListViewItem *item, int col, bool ascending) const;

   RQFetchItem(QListView *parent, pkgAcquire::ItemDesc &itemDesc);
};

#endif

// vim:ts=3:sw=3:et
