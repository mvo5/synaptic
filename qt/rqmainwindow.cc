
#include <qcombobox.h>
#include <qlistbox.h>
#include <qtoolbutton.h>

#include <rqmainwindow.h>
#include <rpackagelister.h>
#include <rqcacheprogress.h>
#include <rqobservers.h>
#include <rqchangeswindow.h>
#include <rqfetchwindow.h>
#include <rqinstallwindow.h>

#include <rqpackageitem.h>

#include <i18n.h>

RQMainWindow::RQMainWindow(RPackageLister *lister)
   : _lister(lister), _packagePopup(this), _userDialog(this)
{
   // Plug progress system into the lister.
   _cacheProg = new RQCacheProgress(this);
   _lister->setProgressMeter(_cacheProg);

   // Connect toolbar buttons.
   connect(_commitButton, SIGNAL(clicked()), this, SLOT(commitChanges()));

   // Setup package list.
   _packageListView->setAllColumnsShowFocus(true);
   _packageListView->setSelectionMode(QListView::Extended);

   // Handle clicks on the package list.
   connect(_packageListView, SIGNAL(contextMenuRequested(QListViewItem *,
                                                         const QPoint &, int)),
           this, SLOT(rightClickedOnPackageList(QListViewItem *,
                                                const QPoint &, int)));
   connect(_packageListView, SIGNAL(clicked(QListViewItem *,
                                            const QPoint &, int)),
           this, SLOT(leftClickedOnPackageList(QListViewItem *,
                                               const QPoint &, int)));
                                                         
   // Setup package popup.
   connect(&_packagePopup, SIGNAL(activated(int)),
           this, SLOT(markPackagesFromPopup(int)));

   // Reload subviews after cache opening.
   connect(&cacheObserver, SIGNAL(cacheOpen()), this, SLOT(reloadViews()));
   connect(&cacheObserver, SIGNAL(cacheOpen()), this, SLOT(reloadPackages()));

   // Handle actions on views and filters.
   connect(_viewsComboBox, SIGNAL(activated(int)), this, SLOT(changedView(int)));
   connect(_subViewsListBox, SIGNAL(highlighted(int)), this, SLOT(changedSubView(int)));
   connect(_filtersComboBox, SIGNAL(activated(int)), this, SLOT(changedFilter(int)));

   restoreState();

   // Finally, show main window.
   show();

}

RQMainWindow::~RQMainWindow()
{
   delete _cacheProg;
}


// Slots

void RQMainWindow::restoreState()
{
   _lister->restoreFilters();

   reloadViews();
   reloadFilters();
}

void RQMainWindow::saveState()
{
}

void RQMainWindow::reloadViews()
{
   // Reload combo. Only needed once, since views are hardcoded.
   if (_viewsComboBox->count() == 0) {
      vector<string> views = _lister->getViews();
      for (int i = 0; i != views.size(); i++)
         _viewsComboBox->insertItem(views[i].c_str());
   }

   // Reload list from selected view.
   _subViewsListBox->clear();
   vector<string> subViews = _lister->getSubViews();
   for (int i = 0; i != subViews.size(); i++)
      _subViewsListBox->insertItem(subViews[i].c_str());
}

void RQMainWindow::changedView(int index)
{
   _lister->setView(index);
   _lister->setSubView("none");
   reloadViews();
   reloadPackages();
}

void RQMainWindow::changedSubView(int index)
{
   _lister->setSubView(_subViewsListBox->text(index).ascii());
   reloadPackages();
}

void RQMainWindow::reloadFilters()
{
   _filtersComboBox->clear();
   _filtersComboBox->insertItem(_("All Packages"));
   vector<string> filters = _lister->getFilterNames();
   for (int i = 0; i != filters.size(); i++)
      _filtersComboBox->insertItem(filters[i].c_str());
}

void RQMainWindow::changedFilter(int index)
{
   _lister->setFilter(index-1);
   reloadPackages();
}

void RQMainWindow::reloadPackages()
{
   _packageListView->clear();
   for (int i = 0; i != _lister->viewPackagesSize(); i++) {
      RPackage *pkg = _lister->getViewPackage(i);
      RQPackageItem *item = new RQPackageItem(_packageListView, pkg);
      _packageListView->insertItem(item);
   }
}

void RQMainWindow::rightClickedOnPackageList(QListViewItem *item,
                                             const QPoint &pos, int col)
{
   _packagePopup.update(_packageListView);
   _packagePopup.popup(pos, _packagePopup.firstEnabledId());
}

void RQMainWindow::leftClickedOnPackageList(QListViewItem *item,
                                            const QPoint &pos, int col)
{
   if (col == 0) {
      _packagePopup.update(_packageListView);
      _packagePopup.popup(pos, _packagePopup.firstEnabledId());
   }
}

void RQMainWindow::markPackage(RPackage *pkg, int mark)
{
   switch (mark) {
      case MarkKeep:
         pkg->setKeep();
         break;
      case MarkInstall:
         pkg->setInstall();
         break;
      case MarkReInstall:
         pkg->setKeep();
         pkg->setReInstall(true);
         break;
      case MarkRemove:
         pkg->setRemove(false);
         break;
      case MarkPurge:
         pkg->setRemove(true);
         break;
   }
}

void RQMainWindow::markSelectedPackages(int mark)
{
   RPackageLister::pkgState state;
   _lister->saveState(state);

   vector<RPackage *> selected;
   
   RQPackageItem *pkgItem = (RQPackageItem*)_packageListView->firstChild();
   while (pkgItem) {
      if (pkgItem->isSelected()) {
         selected.push_back(pkgItem->pkg);
         markPackage(pkgItem->pkg, mark);
      }
      pkgItem = (RQPackageItem *)pkgItem->nextSibling();
   }

   if (!_lister->check())
      _lister->fixBroken();

   vector<RPackage *> changed;
   _lister->getStateChanges(state, changed, changed, changed,
                            changed, changed, changed, selected);
   
   if (!changed.empty()) {
      RQChangesWindow changes(this, changed);
      changes.setLabel("The following additional changes are needed "
                       "to perform this operation");
      changes.exec();

      if (changes.result() == QDialog::Rejected) {
         _lister->restoreState(state);
         selected.clear();
         changed.clear();
      }
   }

   // XXX Check for unsuccessful changes?

   if (!selected.empty())
      _lister->saveUndoState(state);

   _packageListView->triggerUpdate();
}

void RQMainWindow::markPackagesFromPopup(int id)
{
   switch (id) {
      case RQPackagePopup::IdUnmark:
         markSelectedPackages(MarkKeep);
         break;
      case RQPackagePopup::IdInstall:
      case RQPackagePopup::IdUpgrade:
         markSelectedPackages(MarkInstall);
         break;
      case RQPackagePopup::IdReinstall:
         markSelectedPackages(MarkReInstall);
         break;
      case RQPackagePopup::IdRemove:
         markSelectedPackages(MarkRemove);
         break;
   }
}

void RQMainWindow::commitChanges()
{
   if (!_lister->check()) {
      _userDialog.error(_("Operation not possible with broken packages.\n"
                          "Please fix them first."));
      return;
   }

   setEnabled(false);

   // Pointers to packages will become invalid.
   _packageListView->clear();

   // XXX Confirm changes here.

   // XXX Save selections to a temporary file here.

   RQFetchWindow fetch(this);
   RQDummyInstallWindow install;

   _lister->commitChanges(&fetch, &install);

   setEnabled(true);

   if (!_lister->openCache(true)) {
      _userDialog.showErrors();
      exit(1);
   }

   // XXX Restore selections from temporary file here.

   reloadViews();
   reloadPackages();
}

// vim:ts=3:sw=3:et
