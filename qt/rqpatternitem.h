
#ifndef RQPATTERNITEM_H
#define RQPATTERNITEM_H

#include <qlistview.h>

#include <rpackagefilter.h>

#include <string>

using std::string;

class RQPatternItem : public QListViewItem
{
   protected:

   static unsigned long globalSequence;
   unsigned long sequence;
      
   public:

   RPatternPackageFilter::DepType type;
   string pattern;
   bool exclude;

   QString text(int column) const;
   void setText(int column, const QString &text);

   int compare(QListViewItem *item, int col, bool ascending) const;

   RQPatternItem(QListView *parent,
                 RPatternPackageFilter::DepType type =
                                       RPatternPackageFilter::Name,
                 string pattern = "",
                 bool exclude = false);
};

extern QString TypeName[];
extern QString TypeOperationName[];

#endif

// vim:ts=3:sw=3:et
