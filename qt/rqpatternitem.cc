
#include <qapplication.h>
#include <qstring.h>

#include <rqpatternitem.h>

unsigned long RQPatternItem::globalSequence = 0;

static void initPatternI18N();

RQPatternItem::RQPatternItem(QListView *parent,
                             RPatternPackageFilter::DepType type,
                             string pattern,
                             bool exclude)
   : QListViewItem(parent), type(type), pattern(pattern), exclude(exclude)
{
   initPatternI18N();
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

static void initPatternI18N()
{
   static bool initialized = false;
   if (!initialized) {
      initialized = true;
      int i;
      for (i = 0; TypeName[i]; i++)
         TypeName[i] = qApp->translate(NULL, TypeName[i]);
      for (i = 0; TypeOperationName[i]; i++)
         TypeOperationName[i] = qApp->translate(NULL, TypeOperationName[i]);
   }
}

#define tr(x) x

QString TypeName[] = {
   tr("Package name"),
   tr("Version number"),
   tr("Description"),
   tr("Maintainer"),
   tr("Dependencies"),           // depends, predepends etc
   tr("Provided packages"),      // provides and name
   tr("Conflicting packages"),   // conflicts
   tr("Replaced packages"),      // replaces/obsoletes
   tr("Suggestions or recommendations"), // suggests/recommends
   tr("Reverse dependencies"),   // Reverse Depends
   NULL
};

QString TypeOperationName[] = {
   tr("Includes"),
   tr("Excludes"),
   NULL
};

// vim:ts=3:sw=3:et
