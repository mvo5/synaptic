/****************************************************************************
** Form interface generated from reading ui file 'window_install.ui'
**
** Created: Tue Feb 17 18:06:28 2004
**      by: The User Interface Compiler ()
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef WINDOWINSTALL_H
#define WINDOWINSTALL_H

#include <qvariant.h>
#include <qdialog.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QFrame;
class QProgressBar;

class WindowInstall : public QDialog
{
    Q_OBJECT

public:
    WindowInstall( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~WindowInstall();

    QLabel* _itemLabel;
    QLabel* _summaryLabel;
    QFrame* line1;
    QProgressBar* _itemProgress;
    QProgressBar* _totalProgress;

protected:
    QVBoxLayout* WindowInstallLayout;
    QVBoxLayout* layout1;

protected slots:
    virtual void languageChange();

};

#endif // WINDOWINSTALL_H
