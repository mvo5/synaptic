/****************************************************************************
** Form interface generated from reading ui file 'window_fetch.ui'
**
** Created: Tue Feb 17 17:43:48 2004
**      by: The User Interface Compiler ()
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef WINDOWFETCH_H
#define WINDOWFETCH_H

#include <qvariant.h>
#include <qdialog.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QListView;
class QListViewItem;
class QProgressBar;
class QPushButton;

class WindowFetch : public QDialog
{
    Q_OBJECT

public:
    WindowFetch( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~WindowFetch();

    QListView* _progressListView;
    QProgressBar* _progressBar;
    QPushButton* _cancelButton;

protected:
    QVBoxLayout* WindowFetchLayout;
    QVBoxLayout* layout2;
    QHBoxLayout* layout1;

protected slots:
    virtual void languageChange();

};

#endif // WINDOWFETCH_H
