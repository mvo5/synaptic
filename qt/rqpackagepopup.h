#ifndef RQPACKAGEPOPUP_H
#define RQPACKAGEPOPUP_H

#include <qpopupmenu.h>

class RPackage;
class QListView;

class RQPackagePopup : public QPopupMenu
{
   Q_OBJECT

   public:

   enum {
      IdUnmark,
      IdInstall,
      IdReinstall,
      IdUpgrade,
      IdRemove,
      IdLast
   };

   void update(QListView *packageListView);
   int firstEnabledId();

   RQPackagePopup(QWidget *parent);
};

#endif
