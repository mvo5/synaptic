
#include <qlistbox.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qpopupmenu.h>
#include <qlineedit.h>

#include <rqfilterswindow.h>
#include <rqpatternitem.h>

#include <rpackagelister.h>
#include <rpackagefilter.h>

RQFiltersWindow::RQFiltersWindow(QWidget *parent, RPackageLister *lister)
	: WindowFilters(parent), _lister(lister), _filter(0)
{
	connect(_closeButton, SIGNAL(clicked()), this, SLOT(accept()));

   connect(_namesListBox, SIGNAL(highlighted(int)),
           this, SLOT(changeFilter(int)));
   connect(_nameLineEdit, SIGNAL(textChanged(const QString &)),
           this, SLOT(changeFilterNameOnList(const QString &)));

   connect(_patternsListView, SIGNAL(contextMenuRequested(QListViewItem *, const QPoint &, int)),
           this, SLOT(patternClicked(QListViewItem *, const QPoint &, int)));

   connect(_newFilterButton, SIGNAL(clicked()),
           this, SLOT(insertNewFilter()));
   connect(_deleteFilterButton, SIGNAL(clicked()),
           this, SLOT(deleteSelectedFilter()));

   connect(_newPatternButton, SIGNAL(clicked()),
           this, SLOT(insertNewPattern()));
   connect(_deletePatternButton, SIGNAL(clicked()),
           this, SLOT(deleteSelectedPattern()));

   vector<string> filters = _lister->getFilterNames();
   for (int i = 0; i != filters.size(); i++)
      _namesListBox->insertItem(filters[i].c_str(), i);

   vector<string> sections = _lister->getSections();
   for (int i = 0; i != sections.size(); i++)
      _sectionsListBox->insertItem(sections[i].c_str(), i);
}

void RQFiltersWindow::changeFilter(int i)
{
   if (_filter)
      saveFilter();

   _filter = _lister->findFilter(i);
   if (!_filter)
      return;

   _nameLineEdit->setText(_filter->getName().c_str());

   // Setup status tab.
   int status = _filter->status.status();
   _installedCheckBox->setChecked(
                        status & RStatusPackageFilter::Installed);
   _upgradableCheckBox->setChecked(
                        status & RStatusPackageFilter::Upgradable);
   _notInstalledCheckBox->setChecked(
                        status & RStatusPackageFilter::NotInstalled);
   _residualConfigCheckBox->setChecked(
                        status & RStatusPackageFilter::ResidualConfig);
   _markInstallCheckBox->setChecked(
                        status & RStatusPackageFilter::MarkInstall);
   _markRemoveCheckBox->setChecked(
                        status & RStatusPackageFilter::MarkRemove);
   _markKeepCheckBox->setChecked(
                        status & RStatusPackageFilter::MarkKeep);
   _newInReposCheckBox->setChecked(
                        status & RStatusPackageFilter::NewPackage);
   _pinnedCheckBox->setChecked(
                        status & RStatusPackageFilter::PinnedPackage);
   _orphanedCheckBox->setChecked(
                        status & RStatusPackageFilter::OrphanedPackage);
   _notInstallableCheckBox->setChecked(
                        status & RStatusPackageFilter::NotInstallable);
   _brokenCheckBox->setChecked(
                        status & RStatusPackageFilter::Broken);

   // Setup sections tab.
   _sectionsListBox->clearSelection();
   for (int i = 0; i != _filter->section.count(); i++) {
      string section = _filter->section.section(i);
      QListBoxItem *item = _sectionsListBox->findItem(section.c_str());
      _sectionsListBox->setSelected(item, true);
   }
   if (_filter->section.inclusive())
      _includeSectionsRadioButton->setChecked(true);
   else
      _excludeSectionsRadioButton->setChecked(true);

   // Setup patterns tab.
   _patternsListView->clear();
   RPatternPackageFilter::DepType type;
   string pattern;
   bool exclude;
   for (int i = 0; i != _filter->pattern.count(); i++) {
      _filter->pattern.getPattern(i, type, pattern, exclude);
      (void)new RQPatternItem(_patternsListView, type, pattern, exclude);
   }
}

void RQFiltersWindow::saveFilter()
{
   if (!_filter)
      return;

   _filter->setName(_nameLineEdit->text().ascii());

   // Save status tab.
   int status = 0;
   if (_installedCheckBox->isChecked())
      status |= RStatusPackageFilter::Installed;
   if (_upgradableCheckBox->isChecked())
      status |= RStatusPackageFilter::Upgradable;
   if (_notInstalledCheckBox->isChecked())
      status |= RStatusPackageFilter::NotInstalled;
   if (_residualConfigCheckBox->isChecked())
      status |= RStatusPackageFilter::ResidualConfig;
   if (_markInstallCheckBox->isChecked())
      status |= RStatusPackageFilter::MarkInstall;
   if (_markRemoveCheckBox->isChecked())
      status |= RStatusPackageFilter::MarkRemove;
   if (_markKeepCheckBox->isChecked())
      status |= RStatusPackageFilter::MarkKeep;
   if (_newInReposCheckBox->isChecked())
      status |= RStatusPackageFilter::NewPackage;
   if (_pinnedCheckBox->isChecked())
      status |= RStatusPackageFilter::PinnedPackage;
   if (_orphanedCheckBox->isChecked())
      status |= RStatusPackageFilter::OrphanedPackage;
   if (_notInstallableCheckBox->isChecked())
      status |= RStatusPackageFilter::NotInstallable;
   if (_brokenCheckBox->isChecked())
      status |= RStatusPackageFilter::Broken;
   _filter->status.setStatus(status);

   // Save sections tab.
   _filter->section.clear();
   QListBoxItem *sectitem = _sectionsListBox->firstItem();
   while (sectitem) {
      if (sectitem->isSelected())
         _filter->section.addSection(sectitem->text().ascii());
      sectitem = sectitem->next();
   }
   _filter->section.setInclusive(_includeSectionsRadioButton->isChecked());

   // Save patterns tab.
   _filter->pattern.reset();
   RQPatternItem *patitem = (RQPatternItem *)_patternsListView->firstChild();
   while (patitem) {
      _filter->pattern.addPattern(patitem->type, patitem->pattern,
                                  patitem->exclude);
      patitem = (RQPatternItem *)patitem->nextSibling();
   }
}

void RQFiltersWindow::insertNewFilter()
{
   // Save filter now, since indexes might get corrupted
   // during the new filter insertion.
   saveFilter();
   _filter = NULL;

   QString name;
   int i = 1;
   RFilter *filter;
   do {
      filter = new RFilter(_lister);
      name.sprintf(_("New Filter %i"), i++);
      filter->setName(name.ascii());
   } while (!_lister->registerFilter(filter));
   
   int index = _lister->getFilterIndex(filter);
   _namesListBox->insertItem(filter->getName().c_str(), index);
   _namesListBox->setCurrentItem(index);
   changeFilter(index);
}

void RQFiltersWindow::changeFilterNameOnList(const QString &name)
{
   // A little delicated to avoid infinite recursion.
   _namesListBox->blockSignals(true);
   int current = _namesListBox->currentItem();
   _namesListBox->changeItem(_nameLineEdit->text(),
                             _lister->getFilterIndex(_filter));
   _namesListBox->setCurrentItem(current);
   _namesListBox->blockSignals(false);
   _namesListBox->update();
}

void RQFiltersWindow::deleteSelectedFilter()
{
   int index = _namesListBox->currentItem();
   if (index == -1) return;
   RFilter *filter = _lister->findFilter(index);
   if (!filter) return;
   _lister->unregisterFilter(filter);
   delete filter;
   _filter = NULL;
   _namesListBox->removeItem(index);
}

void RQFiltersWindow::patternClicked(QListViewItem *_item, const QPoint &pos,
                                     int column)
{
   QPopupMenu popup(this);
   RQPatternItem *item = (RQPatternItem *)_item;
   
   if (!item) return;

   if (column == 0) {
      for (int i = 0; TypeName[i]; i++)
         popup.insertItem(TypeName[i], i);
      int id = popup.exec(pos, item->type);
      if (id != -1)
         item->type = (RPatternPackageFilter::DepType)id;
   } else if (column == 1) {
      for (int i = 0; TypeOperationName[i]; i++)
         popup.insertItem(TypeOperationName[i], i);
      int id = popup.exec(pos, item->exclude);
      if (id != -1)
         item->exclude = id;
   } else if (column == 2) {
      item->startRename(2);
   }
}

void RQFiltersWindow::insertNewPattern()
{
   RQPatternItem *item = new RQPatternItem(_patternsListView);
   _patternsListView->setCurrentItem(item);
}

void RQFiltersWindow::deleteSelectedPattern()
{
   QListViewItem *item = _patternsListView->currentItem();
   delete item;
}

// vim:ts=3:sw=3:et
