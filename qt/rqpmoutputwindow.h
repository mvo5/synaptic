#ifndef RQPMOUTPUTWINDOW_H
#define RQPMOUTPUTWINDOW_H

#include <window_pmoutput.h>

class RQPMOutputWindow : public WindowPMOutput
{
   Q_OBJECT

   protected:

   const char *_currentPackage;
   bool _hasHeader;

   public:

   void newPackage(const char *name);
   void addLine(const char *line);
   bool empty();

   RQPMOutputWindow(QWidget *parent);
};

#endif

// vim:ts=3:sw=3:et
