
#ifndef RQMAINWINDOW_H
#define RQMAINWINDOW_H

#include <window_main.h>

#include <rqpackagepopup.h>
#include <rquserdialog.h>

class RPackage;
class RPackageLister;
class RQCacheProgress;

class RQMainWindow : public WindowMain
{
   Q_OBJECT

	public:

	RQMainWindow(RPackageLister *lister);
   ~RQMainWindow();

   enum {
      MarkKeep,
      MarkInstall,
      MarkReInstall,
      MarkRemove,
      MarkPurge,
   };

   protected:

   RPackageLister *_lister;
   RQCacheProgress *_cacheProg;
   RQPackagePopup _packagePopup;
   RQUserDialog _userDialog;

   public slots:

   void restoreState();
   void saveState();
   void reloadViews();
   void changedView(int index);
   void changedSubView(int index);
   void reloadFilters();
   void changedFilter(int index);
   void reloadPackages();
   void rightClickedOnPackageList(QListViewItem *item,
                                  const QPoint &pos, int col);
   void leftClickedOnPackageList(QListViewItem *item,
                                 const QPoint &pos, int col);
   void markPackage(RPackage *pkg, int mark);
   void markSelectedPackages(int mark);
   void markPackagesFromPopup(int id);
   void commitChanges();
};

#endif

// vim:ts=3:sw=3:et
