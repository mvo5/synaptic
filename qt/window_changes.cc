/****************************************************************************
** Form implementation generated from reading ui file 'window_changes.ui'
**
** Created: Tue Feb 17 15:39:47 2004
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.2.3   edited May 19 14:22 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "window_changes.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qheader.h>
#include <qlistview.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

/*
 *  Constructs a WindowChanges as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
WindowChanges::WindowChanges( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "WindowChanges" );
    WindowChangesLayout = new QVBoxLayout( this, 11, 6, "WindowChangesLayout"); 

    layout3 = new QVBoxLayout( 0, 0, 6, "layout3"); 

    _label = new QLabel( this, "_label" );
    layout3->addWidget( _label );

    _packageListView = new QListView( this, "_packageListView" );
    _packageListView->addColumn( tr( "S" ) );
    _packageListView->header()->setClickEnabled( FALSE, _packageListView->header()->count() - 1 );
    _packageListView->addColumn( tr( "Name" ) );
    _packageListView->header()->setClickEnabled( FALSE, _packageListView->header()->count() - 1 );
    _packageListView->addColumn( tr( "Current" ) );
    _packageListView->header()->setClickEnabled( FALSE, _packageListView->header()->count() - 1 );
    _packageListView->addColumn( tr( "Available" ) );
    _packageListView->header()->setClickEnabled( FALSE, _packageListView->header()->count() - 1 );
    layout3->addWidget( _packageListView );

    layout2 = new QHBoxLayout( 0, 0, 6, "layout2"); 
    QSpacerItem* spacer = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    layout2->addItem( spacer );

    _continueButton = new QPushButton( this, "_continueButton" );
    layout2->addWidget( _continueButton );

    _cancelButton = new QPushButton( this, "_cancelButton" );
    layout2->addWidget( _cancelButton );
    layout3->addLayout( layout2 );
    WindowChangesLayout->addLayout( layout3 );
    languageChange();
    resize( QSize(482, 399).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
}

/*
 *  Destroys the object and frees any allocated resources
 */
WindowChanges::~WindowChanges()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void WindowChanges::languageChange()
{
    setCaption( tr( "Confirm Changes" ) );
    _label->setText( tr( "Label with description" ) );
    _packageListView->header()->setLabel( 0, tr( "S" ) );
    _packageListView->header()->setLabel( 1, tr( "Name" ) );
    _packageListView->header()->setLabel( 2, tr( "Current" ) );
    _packageListView->header()->setLabel( 3, tr( "Available" ) );
    _continueButton->setText( tr( "C&ontinue" ) );
    _continueButton->setAccel( QKeySequence( tr( "Alt+O" ) ) );
    _cancelButton->setText( tr( "&Cancel" ) );
    _cancelButton->setAccel( QKeySequence( tr( "Alt+C" ) ) );
}

