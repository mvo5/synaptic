#ifndef RQWINDOWSUMMARY_UI
#define RQWINDOWSUMMARY_UI

#include <window_summary.h>

#include <rqpackageitem.h>

class RPackageLister;

class RQSummaryWindow : public WindowSummary
{
   Q_OBJECT

   protected:

   RQPackageTip _packageTip;

   public:

   void setSummary(QString summary);
   void setInformation(QString information);
   void hideInformation();

   RQSummaryWindow(QWidget *parent, RPackageLister *lister);
};

#endif

// vim:ts=3:sw=3:et
