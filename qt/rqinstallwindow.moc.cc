/****************************************************************************
** RQInstallWindow meta object code from reading C++ file 'rqinstallwindow.h'
**
** Created: Tue Feb 17 18:00:05 2004
**      by: The Qt MOC ()
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "rqinstallwindow.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.2.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *RQInstallWindow::className() const
{
    return "RQInstallWindow";
}

QMetaObject *RQInstallWindow::metaObj = 0;
static QMetaObjectCleanUp cleanUp_RQInstallWindow( "RQInstallWindow", &RQInstallWindow::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString RQInstallWindow::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQInstallWindow", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString RQInstallWindow::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQInstallWindow", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* RQInstallWindow::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = WindowInstall::staticMetaObject();
    metaObj = QMetaObject::new_metaobject(
	"RQInstallWindow", parentObject,
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_RQInstallWindow.setMetaObject( metaObj );
    return metaObj;
}

void* RQInstallWindow::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "RQInstallWindow" ) )
	return this;
    if ( !qstrcmp( clname, "RInstallProgress" ) )
	return (RInstallProgress*)this;
    return WindowInstall::qt_cast( clname );
}

bool RQInstallWindow::qt_invoke( int _id, QUObject* _o )
{
    return WindowInstall::qt_invoke(_id,_o);
}

bool RQInstallWindow::qt_emit( int _id, QUObject* _o )
{
    return WindowInstall::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool RQInstallWindow::qt_property( int id, int f, QVariant* v)
{
    return WindowInstall::qt_property( id, f, v);
}

bool RQInstallWindow::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
