/****************************************************************************
** RQFetchWindow meta object code from reading C++ file 'rqfetchwindow.h'
**
** Created: Tue Feb 17 12:17:52 2004
**      by: The Qt MOC ()
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "rqfetchwindow.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.2.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *RQFetchWindow::className() const
{
    return "RQFetchWindow";
}

QMetaObject *RQFetchWindow::metaObj = 0;
static QMetaObjectCleanUp cleanUp_RQFetchWindow( "RQFetchWindow", &RQFetchWindow::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString RQFetchWindow::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQFetchWindow", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString RQFetchWindow::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQFetchWindow", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* RQFetchWindow::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = WindowFetch::staticMetaObject();
    metaObj = QMetaObject::new_metaobject(
	"RQFetchWindow", parentObject,
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_RQFetchWindow.setMetaObject( metaObj );
    return metaObj;
}

void* RQFetchWindow::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "RQFetchWindow" ) )
	return this;
    if ( !qstrcmp( clname, "pkgAcquireStatus" ) )
	return (pkgAcquireStatus*)this;
    return WindowFetch::qt_cast( clname );
}

bool RQFetchWindow::qt_invoke( int _id, QUObject* _o )
{
    return WindowFetch::qt_invoke(_id,_o);
}

bool RQFetchWindow::qt_emit( int _id, QUObject* _o )
{
    return WindowFetch::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool RQFetchWindow::qt_property( int id, int f, QVariant* v)
{
    return WindowFetch::qt_property( id, f, v);
}

bool RQFetchWindow::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
