#ifndef RQFILTERSWINDOW_H
#define RQFILTERSWINDOW_H

#include "window_filters.h"

class RPackageLister;
class RFilter;

class RQFiltersWindow : public WindowFilters
{
   Q_OBJECT
   
   protected:
   
   RPackageLister *_lister;
   RFilter *_filter;

   public:

   RQFiltersWindow(QWidget *parent, RPackageLister *lister);

   public slots:

   void changeFilter(int i);
   void saveFilter();
   void insertNewFilter();
   void deleteSelectedFilter();
   void patternClicked(QListViewItem *item, const QPoint &pos, int column);
   void insertNewPattern();
   void deleteSelectedPattern();
   void changeFilterNameOnList(const QString &name);
};

#endif

// vim:ts=3:sw=3:et
