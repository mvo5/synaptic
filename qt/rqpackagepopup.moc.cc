/****************************************************************************
** RQPackagePopup meta object code from reading C++ file 'rqpackagepopup.h'
**
** Created: Sat Feb 14 15:44:24 2004
**      by: The Qt MOC ()
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "rqpackagepopup.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.2.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *RQPackagePopup::className() const
{
    return "RQPackagePopup";
}

QMetaObject *RQPackagePopup::metaObj = 0;
static QMetaObjectCleanUp cleanUp_RQPackagePopup( "RQPackagePopup", &RQPackagePopup::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString RQPackagePopup::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQPackagePopup", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString RQPackagePopup::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQPackagePopup", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* RQPackagePopup::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QPopupMenu::staticMetaObject();
    metaObj = QMetaObject::new_metaobject(
	"RQPackagePopup", parentObject,
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_RQPackagePopup.setMetaObject( metaObj );
    return metaObj;
}

void* RQPackagePopup::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "RQPackagePopup" ) )
	return this;
    return QPopupMenu::qt_cast( clname );
}

bool RQPackagePopup::qt_invoke( int _id, QUObject* _o )
{
    return QPopupMenu::qt_invoke(_id,_o);
}

bool RQPackagePopup::qt_emit( int _id, QUObject* _o )
{
    return QPopupMenu::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool RQPackagePopup::qt_property( int id, int f, QVariant* v)
{
    return QPopupMenu::qt_property( id, f, v);
}

bool RQPackagePopup::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
