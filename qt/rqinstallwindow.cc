
#include <qapplication.h>
#include <qprogressbar.h>
#include <qlabel.h>

#include <apt-pkg/configuration.h>

#include <rqinstallwindow.h>

#include <rpackagelister.h>

#include <unistd.h>
#include <stdio.h>

#include <i18n.h>

RQInstallWindow::RQInstallWindow(QWidget *parent,
                                     RPackageLister *lister)
   : WindowInstall(parent), _startCounting(false), _pmoutWindow(this)
{
   prepare(lister);
}

RQInstallWindow::~RQInstallWindow()
{
}

void RQInstallWindow::startUpdate()
{
   show();
   qApp->processEvents();
}

void RQInstallWindow::finishUpdate()
{
   if (_startCounting) {
      _itemProgress->setProgress(100);
      _totalProgress->setProgress(100);
   }

   if (!_pmoutWindow.empty())
      _pmoutWindow.exec();

   qApp->processEvents();

   accept();
}

void RQInstallWindow::prepare(RPackageLister *lister)
{
   for (unsigned int row = 0; row < lister->packagesSize(); row++) {
      RPackage *elem = lister->getPackage(row);

      // Is it going to be seen?
      if (!(elem->getFlags() & RPackage::FInstall))
         continue;

      const char *name = elem->name();
      const char *ver = elem->availableVersion();
      const char *pos = strchr(ver, ':');
      if (pos)
         ver = pos + 1;
      string namever = string(name) + "-" + string(ver);
      _summaryMap[namever] = elem->summary();
   }
}

void RQInstallWindow::updateInterface()
{
   char buf[2];
   static char line[1024] = "";

   while (1) {
      // This algorithm should be improved.
      int len = read(_childin, buf, 1);
      if (len < 1)
         break;
      if (buf[0] == '\n') {
         float val;
         if (line[0] != '%') {
            map<string, string>::const_iterator I = _summaryMap.find(line);
            if (I == _summaryMap.end()) {
               if (_startCounting == false) {
                  _itemLabel->setText(line);
                  _itemProgress->setProgress(0);
               } else {
                  _pmoutWindow.addLine(line);
               }
            } else {
               // Get from the map, so that _pmoutWindow doesn't have
               // to keep an internal copy.
               _pmoutWindow.newPackage(I->first.c_str());
               _itemLabel->setText(line);
               _summaryLabel->setText(I->second.c_str());
               _itemProgress->setProgress(0);
               _donePackages += 1;
               val = ((float)_donePackages) / _numPackages;
               _totalProgress->setProgress((int)((float)_donePackages*100)
                                           /_numPackages);
               _donePackagesTotal += 1;
            }
         } else {
            sscanf(line + 3, "%f", &val);
            _itemProgress->setProgress((int)val);
            if (_startCounting == false) {
               // This will happen when the "Preparing..." progress
               // is shown and its progress percentage starts.
               _startCounting = true;
            }
         }
         line[0] = 0;
      } else {
         buf[1] = 0;
         strcat(line, buf);
      }
   }

   qApp->processEvents();
}

// vim:ts=3:sw=3:et
