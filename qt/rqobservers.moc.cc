/****************************************************************************
** RQCacheObserver meta object code from reading C++ file 'rqobservers.h'
**
** Created: Wed Feb 11 16:22:21 2004
**      by: The Qt MOC ()
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "rqobservers.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.2.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *RQCacheObserver::className() const
{
    return "RQCacheObserver";
}

QMetaObject *RQCacheObserver::metaObj = 0;
static QMetaObjectCleanUp cleanUp_RQCacheObserver( "RQCacheObserver", &RQCacheObserver::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString RQCacheObserver::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQCacheObserver", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString RQCacheObserver::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQCacheObserver", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* RQCacheObserver::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QObject::staticMetaObject();
    static const QUMethod signal_0 = {"cacheOpen", 0, 0 };
    static const QUMethod signal_1 = {"cachePreChange", 0, 0 };
    static const QUMethod signal_2 = {"cachePostChange", 0, 0 };
    static const QMetaData signal_tbl[] = {
	{ "cacheOpen()", &signal_0, QMetaData::Private },
	{ "cachePreChange()", &signal_1, QMetaData::Private },
	{ "cachePostChange()", &signal_2, QMetaData::Private }
    };
    metaObj = QMetaObject::new_metaobject(
	"RQCacheObserver", parentObject,
	0, 0,
	signal_tbl, 3,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_RQCacheObserver.setMetaObject( metaObj );
    return metaObj;
}

void* RQCacheObserver::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "RQCacheObserver" ) )
	return this;
    if ( !qstrcmp( clname, "RCacheObserver" ) )
	return (RCacheObserver*)this;
    return QObject::qt_cast( clname );
}

// SIGNAL cacheOpen
void RQCacheObserver::cacheOpen()
{
    activate_signal( staticMetaObject()->signalOffset() + 0 );
}

// SIGNAL cachePreChange
void RQCacheObserver::cachePreChange()
{
    activate_signal( staticMetaObject()->signalOffset() + 1 );
}

// SIGNAL cachePostChange
void RQCacheObserver::cachePostChange()
{
    activate_signal( staticMetaObject()->signalOffset() + 2 );
}

bool RQCacheObserver::qt_invoke( int _id, QUObject* _o )
{
    return QObject::qt_invoke(_id,_o);
}

bool RQCacheObserver::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: cacheOpen(); break;
    case 1: cachePreChange(); break;
    case 2: cachePostChange(); break;
    default:
	return QObject::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool RQCacheObserver::qt_property( int id, int f, QVariant* v)
{
    return QObject::qt_property( id, f, v);
}

bool RQCacheObserver::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
