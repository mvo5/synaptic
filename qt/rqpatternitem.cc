
#include <qstring.h>

#include <rqpatternitem.h>

#include <i18n.h>

unsigned long RQPatternItem::globalSequence = 0;

RQPatternItem::RQPatternItem(QListView *parent,
                             RPatternPackageFilter::DepType type,
                             string pattern,
                             bool exclude)
   : QListViewItem(parent), type(type), pattern(pattern), exclude(exclude)
{
   setRenameEnabled(2, true);
   sequence = globalSequence++;
}

QString RQPatternItem::text(int column) const
{
   QString res;

   switch (column) {
      case 0:
         res = TypeName[type];
         break;

      case 1:
         res = TypeOperationName[exclude];
         break;

      case 2:
         res = pattern.c_str();
         break;

      default:
         res = QString::null;
         break;
   }

   return res;
}

void RQPatternItem::setText(int column, const QString &text)
{
   switch (column) {
      case 2:
         pattern = text.ascii();
         break;
   }
}

int RQPatternItem::compare(QListViewItem *item, int col, bool ascending) const
{
   int thisSeq = sequence;
   int itemSeq = ((RQPatternItem *)item)->sequence;
   return (thisSeq > itemSeq) ? 1 : ((thisSeq < itemSeq) ? -1 : 0);
}

char *TypeName[] = {
   _("Package name"),
   _("Version number"),
   _("Description"),
   _("Maintainer"),
   _("Dependencies"),           // depends, predepends etc
   _("Provided packages"),      // provides and name
   _("Conflicting packages"),   // conflicts
   _("Replaced packages"),      // replaces/obsoletes
   _("Suggestions or recommendations"), // suggests/recommends
   _("Reverse dependencies"),   // Reverse Depends
   NULL
};

char *TypeOperationName[] = {
   _("Includes"),
   _("Excludes"),
   NULL
};

// vim:ts=3:sw=3:et
