/****************************************************************************
** Form interface generated from reading ui file 'window_changes.ui'
**
** Created: Tue Feb 17 15:06:06 2004
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.2.3   edited May 19 14:22 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef WINDOWCHANGES_H
#define WINDOWCHANGES_H

#include <qvariant.h>
#include <qdialog.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QListView;
class QListViewItem;
class QPushButton;

class WindowChanges : public QDialog
{
    Q_OBJECT

public:
    WindowChanges( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~WindowChanges();

    QLabel* _label;
    QListView* _packageListView;
    QPushButton* _continueButton;
    QPushButton* _cancelButton;

protected:
    QVBoxLayout* WindowChangesLayout;
    QVBoxLayout* layout3;
    QHBoxLayout* layout2;

protected slots:
    virtual void languageChange();

};

#endif // WINDOWCHANGES_H
