
#ifndef RQPACKAGEITEM_H
#define RQPACKAGEITEM_H

#include <qlistview.h>
#include <qstring.h>
#include <qpixmap.h>

class RPackage;

class QPainter;
class QColorGroup;

class RQPackageItem : public QListViewItem
{
   protected:

   void paintCell(QPainter *p, const QColorGroup &cg,
                  int column, int width, int align);
   
   QPixmap _pm;

   public:

   RPackage *pkg;

   QString text(int column) const;
   const QPixmap *pixmap(int column) const;

   RQPackageItem(QListView *parent, RPackage *pkg);
};

#endif

// vim:ts=3:sw=3:et
