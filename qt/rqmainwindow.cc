
#include <qcombobox.h>
#include <qlistbox.h>
#include <qtoolbutton.h>
#include <qaction.h>

#include <rpackagelister.h>
#include <rconfiguration.h>

#include <rqmainwindow.h>
#include <rqcacheprogress.h>
#include <rqobservers.h>
#include <rqchangeswindow.h>
#include <rqsummarywindow.h>
#include <rqfetchwindow.h>
#include <rqinstallwindow.h>
#include <rqpixmaps.h>
#include <rqfilterswindow.h>
#include <rqpackageitem.h>

#include <apt-pkg/configuration.h>

#include <config.h>
#include <i18n.h>

RQMainWindow::RQMainWindow(RPackageLister *lister)
   : _lister(lister), _packagePopup(this), _userDialog(this),
     _packageTip(_packageListView)
{
   // Fix buttons in the toolbar. Right now qt-designer is
   // saving buttons in XPM format.
   _refreshButton->setPixmap(RQPixmaps::find("update.png"));
   _upgradeButton->setPixmap(RQPixmaps::find("distupgrade.png"));
   _commitButton->setPixmap(RQPixmaps::find("proceed.png"));

   // Plug progress system into the lister.
   _cacheProg = new RQCacheProgress(this);
   _lister->setProgressMeter(_cacheProg);

   // Connect menubar actions.
   connect(fileCommit_ChangesAction, SIGNAL(activated()),
           this, SLOT(commitChanges()));
   connect(fileUpgrade_All_PackagesAction, SIGNAL(activated()),
           this, SLOT(distUpgrade()));
   connect(fileRefresh_Package_InformationAction, SIGNAL(activated()),
           this, SLOT(refreshCache()));
   connect(fileFix_Broken_PackagesAction, SIGNAL(activated()),
           this, SLOT(fixBroken()));
   connect(editFiltersAction, SIGNAL(activated()), this, SLOT(editFilters()));

   // Connect toolbar buttons.
   connect(_commitButton, SIGNAL(clicked()), this, SLOT(commitChanges()));
   connect(_upgradeButton, SIGNAL(clicked()), this, SLOT(distUpgrade()));
   connect(_refreshButton, SIGNAL(clicked()), this, SLOT(refreshCache()));

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
   saveState();
   RWriteConfigFile(*_config);
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
   _viewsComboBox->setCurrentItem(_config->FindI("Synaptic::ViewMode", 0));


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
      (void)new RQPackageItem(_packageListView, pkg);
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

void RQMainWindow::distUpgrade()
{
   if (!_lister->check()) {
      _userDialog.error(_("Operation not possible with broken packages.\n"
                          "Please fix them first."));
      return;
   }

   RPackageLister::pkgState state;
   _lister->saveState(state);

   _lister->distUpgrade();

   vector<RPackage *> selected;
   vector<RPackage *> changed;
   _lister->getStateChanges(state, changed, changed, changed,
                            changed, changed, changed, selected);
   
   if (!changed.empty()) {
      RQChangesWindow changes(this, changed);
      changes.exec();
      if (changes.result() == QDialog::Rejected) {
         _lister->restoreState(state);
         changed.clear();
      } else {
         _lister->saveUndoState(state);
      }
   } else {
      _userDialog.proceed(_("Your system is up-to-date!"));
   }

   _packageListView->triggerUpdate();
}

void RQMainWindow::fixBroken()
{
   if (_lister->check()) {
      _userDialog.proceed(_("There are no broken packages."));
      return;
   }
   
   RPackageLister::pkgState state;
   _lister->saveState(state);

   _lister->fixBroken();

   vector<RPackage *> selected;
   vector<RPackage *> changed;
   _lister->getStateChanges(state, changed, changed, changed,
                            changed, changed, changed, selected);
   
   if (!changed.empty()) {
      RQChangesWindow changes(this, changed);
      changes.exec();
      if (changes.result() == QDialog::Rejected) {
         _lister->restoreState(state);
         selected.clear();
         changed.clear();
      } else {
         _lister->saveUndoState(state);
      }
   } else {
      _userDialog.error(_("Can't fix broken packages!"));
   }

   _packageListView->triggerUpdate();
}

void RQMainWindow::commitChanges()
{
   if (!_lister->check()) {
      _userDialog.error(_("Operation not possible with broken packages.\n"
                          "Please fix them first."));
      return;
   }

   int installed;
   int broken;
   int toInstall;
   int toReInstall;
   int toRemove;
   double sizeChange;
   _lister->getStats(installed, broken, toInstall, toReInstall, toRemove,
                     sizeChange);
   if (!toInstall && !toRemove)
      return;

   RQSummaryWindow summary(this, _lister);
   summary.exec();
   if (summary.result() == QDialog::Rejected)
      return;

   setEnabled(false);

   // Pointers to packages will become invalid.
   _packageListView->clear();

   // XXX Save selections to a temporary file here.

   RQFetchWindow fetch(this);
#ifdef HAVE_RPM
   RQInstallWindow install(this, _lister);
#else
   RQDummyInstallWindow install;
#endif

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

void RQMainWindow::refreshCache()
{
   // Pointers to packages will become invalid.
   _packageListView->clear();

   RQFetchWindow fetch(this);

   // XXX Save selections to a temporary file here.

   if (!_lister->updateCache(&fetch))
      _userDialog.showErrors();
   
   // Forget new packages here.

   if (!_lister->openCache(true))
      _userDialog.showErrors();

   // XXX Restore selections from temporary file here.

   reloadViews();
   reloadPackages();
}

void RQMainWindow::editFilters()
{
   RQFiltersWindow filters(this, _lister);
   filters.exec();
   reloadFilters();
   _lister->storeFilters();
}

// vim:ts=3:sw=3:et
