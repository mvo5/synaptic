
#include <rqsummarywindow.h>
#include <rqpackageitem.h>

#include <rpackagelister.h>

#include <qpushbutton.h>
#include <qstring.h>
#include <qlabel.h>

#include <apt-pkg/strutl.h>

class RQSummaryTopicItem : public QListViewItem {

   protected:

   int _order;

   public:

   RQSummaryTopicItem(QListView *parent, int order)
      : QListViewItem(parent), _order(order)
   {
      setExpandable(true);
   };
   
   int compare(QListViewItem *item, int col, bool ascending) const
   {
      int a = _order, b = ((RQSummaryTopicItem *)item)->_order;
      return (a > b) ? -1 : ((b < a) ? 1 : 0);
   };
};

RQSummaryWindow::RQSummaryWindow(QWidget *parent, RPackageLister *lister)
   : WindowSummary(parent), _packageTip(_packageListView)
{
   connect(_continueButton, SIGNAL(clicked()), this, SLOT(accept()));
   connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

   const char *topic[] = {
      _("Held packages (%1)"),
      _("Kept packages (%1)"),
      _("ESSENTIAL packages being REMOVED (%1)"),
      _("Installed packages (%1)"),
      _("Reinstalled packages (%1)"),
      _("Upgraded packages (%1)"),
      _("Removed packages (%1)"),
      _("Downgraded packages (%1)"),
      NULL,
   };
   int order[] = { 2, 6, 7, 3, 4, 5, 0, -1 };
   vector<RPackage *> changed[8];
   double sizeChange;

   lister->getDetailedSummary(changed[0], changed[1], changed[2], changed[3],
                              changed[4], changed[5], changed[6], changed[7],
                              sizeChange);

   for (int i = 0; order[i] != -1; i++) {
      vector<RPackage *> &packages = changed[order[i]];
      if (packages.size() == 0)
         continue;
      QListViewItem *parent = new RQSummaryTopicItem(_packageListView,
                                                     order[i]);
      parent->setText(0, QString(topic[order[i]]).arg(packages.size()));
      for (int j = 0; j != packages.size(); j++)
         (void)new RQPackageItem(parent, packages[j], false);
   }


   QString info;
   QString tmp;

   if (sizeChange > 0)
      tmp.sprintf(_("<b>%s</b> of extra disk space will be used.<br>"),
                  SizeToStr(sizeChange).c_str());
   else if (sizeChange < 0)
      tmp.sprintf(_("<b>%s</b> of extra disk space will be freed.<br>"),
                  SizeToStr(sizeChange).c_str());
   else
      tmp = _("No extra disk space will be used nor freed.<br>");
   info += tmp;

   int dlCount;
   double dlSize;
   lister->getDownloadSummary(dlCount, dlSize);
   if (dlSize != 0)
      tmp.sprintf(_("<b>%s</b> will be downloaded.<br>"),
                  SizeToStr(dlSize).c_str());
   else
      tmp.sprintf(_("No downloads are needed.<br>"),
                  SizeToStr(dlSize).c_str());
   info += tmp;
   
   _infoLabel->setText(info);
}

// vim:ts=3:sw=3:et
