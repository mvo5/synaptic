/****************************************************************************
** Form implementation generated from reading ui file 'window_fetch.ui'
**
** Created: Mon Feb 16 19:48:33 2004
**      by: The User Interface Compiler ()
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "window_fetch.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qheader.h>
#include <qlistview.h>
#include <qprogressbar.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

/*
 *  Constructs a WindowFetch as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
WindowFetch::WindowFetch( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "WindowFetch" );
    WindowFetchLayout = new QVBoxLayout( this, 11, 6, "WindowFetchLayout"); 

    layout2 = new QVBoxLayout( 0, 0, 6, "layout2"); 

    _progressListView = new QListView( this, "_progressListView" );
    _progressListView->addColumn( tr( "Status" ) );
    _progressListView->header()->setClickEnabled( FALSE, _progressListView->header()->count() - 1 );
    _progressListView->header()->setResizeEnabled( FALSE, _progressListView->header()->count() - 1 );
    _progressListView->addColumn( tr( "Size" ) );
    _progressListView->header()->setClickEnabled( FALSE, _progressListView->header()->count() - 1 );
    _progressListView->header()->setResizeEnabled( FALSE, _progressListView->header()->count() - 1 );
    _progressListView->addColumn( tr( "Description" ) );
    _progressListView->header()->setClickEnabled( FALSE, _progressListView->header()->count() - 1 );
    _progressListView->header()->setResizeEnabled( FALSE, _progressListView->header()->count() - 1 );
    _progressListView->setFocusPolicy( QListView::NoFocus );
    _progressListView->setSelectionMode( QListView::NoSelection );
    _progressListView->setResizeMode( QListView::NoColumn );
    layout2->addWidget( _progressListView );

    _progressBar = new QProgressBar( this, "_progressBar" );
    layout2->addWidget( _progressBar );

    layout1 = new QHBoxLayout( 0, 0, 6, "layout1"); 
    QSpacerItem* spacer = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    layout1->addItem( spacer );

    _cancelButton = new QPushButton( this, "_cancelButton" );
    layout1->addWidget( _cancelButton );
    layout2->addLayout( layout1 );
    WindowFetchLayout->addLayout( layout2 );
    languageChange();
    resize( QSize(570, 338).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
}

/*
 *  Destroys the object and frees any allocated resources
 */
WindowFetch::~WindowFetch()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void WindowFetch::languageChange()
{
    setCaption( tr( "Fetch Progress" ) );
    _progressListView->header()->setLabel( 0, tr( "Status" ) );
    _progressListView->header()->setLabel( 1, tr( "Size" ) );
    _progressListView->header()->setLabel( 2, tr( "Description" ) );
    _cancelButton->setText( tr( "&Cancel" ) );
    _cancelButton->setAccel( QKeySequence( tr( "Alt+C" ) ) );
}

