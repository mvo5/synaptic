
#include <qlistview.h>
#include <rqpackageitem.h>
#include <rqpixmaps.h>

#include <rqpackagepopup.h>

#include <rpackage.h>

#include <i18n.h>

RQPackagePopup::RQPackagePopup(QWidget *parent)
   : QPopupMenu(parent)
{
   insertItem(QIconSet(RQPixmaps::find("package-available.png")),
              _("Unmark"), IdUnmark);
   insertItem(QIconSet(RQPixmaps::find("package-install.png")),
              _("Install"), IdInstall);
   insertItem(QIconSet(RQPixmaps::find("package-reinstall.png")),
              _("Reinstall"), IdReinstall);
   insertItem(QIconSet(RQPixmaps::find("package-upgrade.png")),
              _("Upgrade"), IdUpgrade);
   insertItem(QIconSet(RQPixmaps::find("package-remove.png")),
              _("Remove"), IdRemove);
}

void RQPackagePopup::update(QListView *packageListView)
{
   vector<bool> enabled;
   enabled.resize(IdLast, false);

   RQPackageItem *pkgItem = (RQPackageItem *)packageListView->firstChild();

   while (pkgItem) {
      if (pkgItem->isSelected()) {
         int flags = pkgItem->pkg->getFlags();
         if (!(flags & RPackage::FKeep))
            enabled[IdUnmark] = true;
         if (!(flags & RPackage::FInstall) &&
             !(flags & RPackage::FInstalled))
            enabled[IdInstall] = true;
         if (!(flags & RPackage::FInstall) &&
              (flags & RPackage::FInstalled) &&
             !(flags & RPackage::FOutdated) &&
             !(flags & RPackage::FNotInstallable))
            enabled[IdReinstall] = true;
         if (!(flags & RPackage::FInstall) &&
              (flags & RPackage::FInstalled) &&
              (flags & RPackage::FOutdated) &&
             !(flags & RPackage::FNotInstallable))
            enabled[IdUpgrade] = true;
         if (!(flags & RPackage::FRemove) &&
              (flags & RPackage::FInstalled))
            enabled[IdRemove] = true;
      }
      pkgItem = (RQPackageItem *) pkgItem->nextSibling();
   }

   setItemEnabled(IdUnmark, enabled[IdUnmark]);
   setItemEnabled(IdInstall, enabled[IdInstall]);
   setItemEnabled(IdReinstall, enabled[IdReinstall]);
   setItemEnabled(IdUpgrade, enabled[IdUpgrade]);
   setItemEnabled(IdRemove, enabled[IdRemove]);
}

int RQPackagePopup::firstEnabledId()
{
   int id = 0;
   for (int i = 0; i != IdLast; i++) {
      if (isItemEnabled(i)) {
         id = i;
         break;
      }
   }
   return id;
}

// vim:ts=3:sw=3:et
