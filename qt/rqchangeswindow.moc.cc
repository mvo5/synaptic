/****************************************************************************
** RQChangesWindow meta object code from reading C++ file 'rqchangeswindow.h'
**
** Created: Sat Feb 14 21:42:28 2004
**      by: The Qt MOC ()
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "rqchangeswindow.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.2.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *RQChangesWindow::className() const
{
    return "RQChangesWindow";
}

QMetaObject *RQChangesWindow::metaObj = 0;
static QMetaObjectCleanUp cleanUp_RQChangesWindow( "RQChangesWindow", &RQChangesWindow::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString RQChangesWindow::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQChangesWindow", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString RQChangesWindow::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQChangesWindow", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* RQChangesWindow::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = WindowChanges::staticMetaObject();
    metaObj = QMetaObject::new_metaobject(
	"RQChangesWindow", parentObject,
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_RQChangesWindow.setMetaObject( metaObj );
    return metaObj;
}

void* RQChangesWindow::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "RQChangesWindow" ) )
	return this;
    return WindowChanges::qt_cast( clname );
}

bool RQChangesWindow::qt_invoke( int _id, QUObject* _o )
{
    return WindowChanges::qt_invoke(_id,_o);
}

bool RQChangesWindow::qt_emit( int _id, QUObject* _o )
{
    return WindowChanges::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool RQChangesWindow::qt_property( int id, int f, QVariant* v)
{
    return WindowChanges::qt_property( id, f, v);
}

bool RQChangesWindow::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
