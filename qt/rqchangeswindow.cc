
#include <rqchangeswindow.h>
#include <rqpackageitem.h>

#include <qpushbutton.h>

RQChangesWindow::RQChangesWindow(QWidget *parent, vector<RPackage *> &packages)
   : WindowChanges(parent), _packageTip(_packageListView)
{
   for (int i = 0; i != packages.size(); i++) {
      RQPackageItem *item = new RQPackageItem(_packageListView, packages[i]);
      _packageListView->insertItem(item);
   }

   connect(_continueButton, SIGNAL(clicked()), this, SLOT(accept()));
   connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

// vim:ts=3:sw=3:et
