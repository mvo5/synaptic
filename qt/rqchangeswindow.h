#ifndef RQWINDOWCHANGES_UI
#define RQWINDOWCHANGES_UI

#include <window_changes.h>

#include <vector>

using std::vector;

class RPackage;

class RQChangesWindow : public WindowChanges
{
   Q_OBJECT

   public:

   void setLabel(QString label);

   RQChangesWindow(QWidget *parent, vector<RPackage *> &packages);
};

#endif

// vim:ts=3:sw=3:et
