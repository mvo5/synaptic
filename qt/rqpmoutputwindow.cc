
#include <qtextedit.h>
#include <qpushbutton.h>

#include <rqpmoutputwindow.h>

#include <i18n.h>

RQPMOutputWindow::RQPMOutputWindow(QWidget *parent)
   : WindowPMOutput(parent), _currentPackage(0), _hasHeader(0)
{
   connect(_closeButton, SIGNAL(clicked()), this, SLOT(accept()));
}

void RQPMOutputWindow::addLine(const char *text)
{
   if (!_hasHeader) {
      _textEdit->setBold(true);
      if (!_currentPackage)
         _textEdit->append(QString(_("\nWhile preparing for transaction:\n\n")));
      else
         _textEdit->append(QString(_("\nWhile installing package %1:\n\n"))
                           .arg(_currentPackage));
      _textEdit->setBold(false);
      _hasHeader = true;
   }
   _textEdit->append(text);
   _textEdit->append("\n");
}

void RQPMOutputWindow::newPackage(const char *name)
{
   _currentPackage = name;
   _hasHeader = false;
}

bool RQPMOutputWindow::empty()
{
   return (_textEdit->length() == 0);
}



// vim:ts=3:sw=3:et
