
#ifndef RQPACKAGEITEM_H
#define RQPACKAGEITEM_H

#include <qlistview.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qtooltip.h>

class RPackage;

class QPainter;
class QColorGroup;

class RQPackageItem : public QListViewItem
{
   protected:

   void paintCell(QPainter *p, const QColorGroup &cg,
                  int column, int width, int align);
   
   QPixmap _pm;

   int _showicon;

   public:

   RPackage *pkg;

   QString text(int column) const;
   const QPixmap *pixmap(int column) const;

   RQPackageItem(QListView *parent, RPackage *pkg, bool showicon=true)
      : QListViewItem(parent), pkg(pkg), _showicon(showicon)
   {};

   RQPackageItem(QListViewItem *parent, RPackage *pkg, bool showicon=true)
      : QListViewItem(parent), pkg(pkg), _showicon(showicon)
   {};
};

class RQPackageTip : public QToolTip
{
   protected:

   QListView *_packageListView;

   void maybeTip(const QPoint &p);

   public:

   static QString packageTip(RPackage *pkg);

   RQPackageTip(QListView *parent)
      : QToolTip(parent->viewport()), _packageListView(parent)
   {};
};

#endif

// vim:ts=3:sw=3:et
