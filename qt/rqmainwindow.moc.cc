/****************************************************************************
** RQMainWindow meta object code from reading C++ file 'rqmainwindow.h'
**
** Created: Sun Feb 15 13:51:49 2004
**      by: The Qt MOC ()
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "rqmainwindow.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.2.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *RQMainWindow::className() const
{
    return "RQMainWindow";
}

QMetaObject *RQMainWindow::metaObj = 0;
static QMetaObjectCleanUp cleanUp_RQMainWindow( "RQMainWindow", &RQMainWindow::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString RQMainWindow::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQMainWindow", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString RQMainWindow::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "RQMainWindow", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* RQMainWindow::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = WindowMain::staticMetaObject();
    static const QUMethod slot_0 = {"restoreState", 0, 0 };
    static const QUMethod slot_1 = {"saveState", 0, 0 };
    static const QUMethod slot_2 = {"reloadViews", 0, 0 };
    static const QUParameter param_slot_3[] = {
	{ "index", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_3 = {"changedView", 1, param_slot_3 };
    static const QUParameter param_slot_4[] = {
	{ "index", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_4 = {"changedSubView", 1, param_slot_4 };
    static const QUMethod slot_5 = {"reloadFilters", 0, 0 };
    static const QUParameter param_slot_6[] = {
	{ "index", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_6 = {"changedFilter", 1, param_slot_6 };
    static const QUMethod slot_7 = {"reloadPackages", 0, 0 };
    static const QUParameter param_slot_8[] = {
	{ "item", &static_QUType_ptr, "QListViewItem", QUParameter::In },
	{ "pos", &static_QUType_varptr, "\x0e", QUParameter::In },
	{ "col", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_8 = {"rightClickedOnPackageList", 3, param_slot_8 };
    static const QUParameter param_slot_9[] = {
	{ "item", &static_QUType_ptr, "QListViewItem", QUParameter::In },
	{ "pos", &static_QUType_varptr, "\x0e", QUParameter::In },
	{ "col", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_9 = {"leftClickedOnPackageList", 3, param_slot_9 };
    static const QUParameter param_slot_10[] = {
	{ "pkg", &static_QUType_ptr, "RPackage", QUParameter::In },
	{ "mark", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_10 = {"markPackage", 2, param_slot_10 };
    static const QUParameter param_slot_11[] = {
	{ "mark", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_11 = {"markSelectedPackages", 1, param_slot_11 };
    static const QUParameter param_slot_12[] = {
	{ "id", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_12 = {"markPackagesFromPopup", 1, param_slot_12 };
    static const QUMethod slot_13 = {"commitChanges", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "restoreState()", &slot_0, QMetaData::Public },
	{ "saveState()", &slot_1, QMetaData::Public },
	{ "reloadViews()", &slot_2, QMetaData::Public },
	{ "changedView(int)", &slot_3, QMetaData::Public },
	{ "changedSubView(int)", &slot_4, QMetaData::Public },
	{ "reloadFilters()", &slot_5, QMetaData::Public },
	{ "changedFilter(int)", &slot_6, QMetaData::Public },
	{ "reloadPackages()", &slot_7, QMetaData::Public },
	{ "rightClickedOnPackageList(QListViewItem*,const QPoint&,int)", &slot_8, QMetaData::Public },
	{ "leftClickedOnPackageList(QListViewItem*,const QPoint&,int)", &slot_9, QMetaData::Public },
	{ "markPackage(RPackage*,int)", &slot_10, QMetaData::Public },
	{ "markSelectedPackages(int)", &slot_11, QMetaData::Public },
	{ "markPackagesFromPopup(int)", &slot_12, QMetaData::Public },
	{ "commitChanges()", &slot_13, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"RQMainWindow", parentObject,
	slot_tbl, 14,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_RQMainWindow.setMetaObject( metaObj );
    return metaObj;
}

void* RQMainWindow::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "RQMainWindow" ) )
	return this;
    return WindowMain::qt_cast( clname );
}

bool RQMainWindow::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: restoreState(); break;
    case 1: saveState(); break;
    case 2: reloadViews(); break;
    case 3: changedView((int)static_QUType_int.get(_o+1)); break;
    case 4: changedSubView((int)static_QUType_int.get(_o+1)); break;
    case 5: reloadFilters(); break;
    case 6: changedFilter((int)static_QUType_int.get(_o+1)); break;
    case 7: reloadPackages(); break;
    case 8: rightClickedOnPackageList((QListViewItem*)static_QUType_ptr.get(_o+1),(const QPoint&)*((const QPoint*)static_QUType_ptr.get(_o+2)),(int)static_QUType_int.get(_o+3)); break;
    case 9: leftClickedOnPackageList((QListViewItem*)static_QUType_ptr.get(_o+1),(const QPoint&)*((const QPoint*)static_QUType_ptr.get(_o+2)),(int)static_QUType_int.get(_o+3)); break;
    case 10: markPackage((RPackage*)static_QUType_ptr.get(_o+1),(int)static_QUType_int.get(_o+2)); break;
    case 11: markSelectedPackages((int)static_QUType_int.get(_o+1)); break;
    case 12: markPackagesFromPopup((int)static_QUType_int.get(_o+1)); break;
    case 13: commitChanges(); break;
    default:
	return WindowMain::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool RQMainWindow::qt_emit( int _id, QUObject* _o )
{
    return WindowMain::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool RQMainWindow::qt_property( int id, int f, QVariant* v)
{
    return WindowMain::qt_property( id, f, v);
}

bool RQMainWindow::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
