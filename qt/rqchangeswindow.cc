
#include <rqchangeswindow.h>
#include <rqpackageitem.h>

#include <qpushbutton.h>
#include <qlabel.h>

RQChangesWindow::RQChangesWindow(QWidget *parent, vector<RPackage *> &packages)
   : WindowChanges(parent)
{
   setModal(true);

   for (int i = 0; i != packages.size(); i++) {
      RQPackageItem *item = new RQPackageItem(_packageListView, packages[i]);
      _packageListView->insertItem(item);
   }

   connect(_continueButton, SIGNAL(clicked()), this, SLOT(accept()));
   connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void RQChangesWindow::setLabel(QString label)
{
   _label->setText(label);
}

// vim:ts=3:sw=3:et
