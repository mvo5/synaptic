/****************************************************************************
** WindowMain meta object code from reading C++ file 'window_main.h'
**
** Created: Sun Feb 15 14:08:12 2004
**      by: The Qt MOC ()
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "window_main.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.2.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *WindowMain::className() const
{
    return "WindowMain";
}

QMetaObject *WindowMain::metaObj = 0;
static QMetaObjectCleanUp cleanUp_WindowMain( "WindowMain", &WindowMain::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString WindowMain::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "WindowMain", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString WindowMain::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "WindowMain", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* WindowMain::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QMainWindow::staticMetaObject();
    static const QUMethod slot_0 = {"languageChange", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "languageChange()", &slot_0, QMetaData::Protected }
    };
    metaObj = QMetaObject::new_metaobject(
	"WindowMain", parentObject,
	slot_tbl, 1,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_WindowMain.setMetaObject( metaObj );
    return metaObj;
}

void* WindowMain::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "WindowMain" ) )
	return this;
    return QMainWindow::qt_cast( clname );
}

bool WindowMain::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: languageChange(); break;
    default:
	return QMainWindow::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool WindowMain::qt_emit( int _id, QUObject* _o )
{
    return QMainWindow::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool WindowMain::qt_property( int id, int f, QVariant* v)
{
    return QMainWindow::qt_property( id, f, v);
}

bool WindowMain::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
