/****************************************************************************
** Form implementation generated from reading ui file 'window_install.ui'
**
** Created: Tue Feb 17 18:06:28 2004
**      by: The User Interface Compiler ()
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "window_install.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qframe.h>
#include <qprogressbar.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

/*
 *  Constructs a WindowInstall as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
WindowInstall::WindowInstall( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "WindowInstall" );
    setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 0, 0, sizePolicy().hasHeightForWidth() ) );
    setModal( TRUE );
    WindowInstallLayout = new QVBoxLayout( this, 11, 6, "WindowInstallLayout"); 

    layout1 = new QVBoxLayout( 0, 0, 6, "layout1"); 

    _itemLabel = new QLabel( this, "_itemLabel" );
    QFont _itemLabel_font(  _itemLabel->font() );
    _itemLabel_font.setBold( TRUE );
    _itemLabel->setFont( _itemLabel_font ); 
    layout1->addWidget( _itemLabel );

    _summaryLabel = new QLabel( this, "_summaryLabel" );
    layout1->addWidget( _summaryLabel );

    line1 = new QFrame( this, "line1" );
    line1->setFrameShape( QFrame::HLine );
    line1->setFrameShadow( QFrame::Sunken );
    line1->setFrameShape( QFrame::HLine );
    layout1->addWidget( line1 );

    _itemProgress = new QProgressBar( this, "_itemProgress" );
    layout1->addWidget( _itemProgress );

    _totalProgress = new QProgressBar( this, "_totalProgress" );
    layout1->addWidget( _totalProgress );
    WindowInstallLayout->addLayout( layout1 );
    languageChange();
    resize( QSize(471, 139).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
}

/*
 *  Destroys the object and frees any allocated resources
 */
WindowInstall::~WindowInstall()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void WindowInstall::languageChange()
{
    setCaption( tr( "Install Progress" ) );
    _itemLabel->setText( QString::null );
    _summaryLabel->setText( QString::null );
}

