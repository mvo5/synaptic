/****************************************************************************
** Form interface generated from reading ui file 'window_main.ui'
**
** Created: Sun Feb 15 14:07:29 2004
**      by: The User Interface Compiler ()
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef WINDOWMAIN_H
#define WINDOWMAIN_H

#include <qvariant.h>
#include <qpixmap.h>
#include <qmainwindow.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QAction;
class QActionGroup;
class QToolBar;
class QPopupMenu;
class QSplitter;
class QComboBox;
class QListBox;
class QListBoxItem;
class QListView;
class QListViewItem;
class QToolButton;
class Spacer;

class WindowMain : public QMainWindow
{
    Q_OBJECT

public:
    WindowMain( QWidget* parent = 0, const char* name = 0, WFlags fl = WType_TopLevel );
    ~WindowMain();

    QSplitter* splitter5;
    QComboBox* _viewsComboBox;
    QListBox* _subViewsListBox;
    QComboBox* _filtersComboBox;
    QListView* _packageListView;
    QToolButton* _refreshButton;
    QToolButton* _upgradeButton;
    QToolButton* _commitButton;
    Spacer* spacer2;
    QMenuBar *MenuBarEditor;
    QPopupMenu *PopupMenuEditor;
    QToolBar *Toolbar;
    QAction* fileOpen_SelectionAction;
    QAction* fileSave_SelectionAction;
    QAction* fileSave_Selection_AsAction;

protected:
    QVBoxLayout* WindowMainLayout;
    QVBoxLayout* layout9;
    QVBoxLayout* layout10;

protected slots:
    virtual void languageChange();

private:
    QPixmap image0;
    QPixmap image1;
    QPixmap image2;

};

#endif // WINDOWMAIN_H
