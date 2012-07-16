/****************************************************************************
** Meta object code from reading C++ file 'gui.h'
**
** Created: Mon 16. Jul 18:28:56 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../qt/gui.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gui.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Gui[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
       5,    4,    4,    4, 0x0a,
      25,    4,    4,    4, 0x0a,
      47,   45,    4,    4, 0x0a,
      75,    4,    4,    4, 0x0a,
     109,  107,    4,    4, 0x0a,
     134,    4,    4,    4, 0x0a,
     156,    4,    4,    4, 0x0a,
     178,    4,    4,    4, 0x0a,
     198,    4,    4,    4, 0x0a,
     250,  227,  222,    4, 0x0a,
     284,    4,    4,    4, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Gui[] = {
    "Gui\0\0playButtonPressed()\0stopButtonPressed()\0"
    "p\0updateProgressSlider(float)\0"
    "progressSliderValueChanged(int)\0v\0"
    "timeScaleChanged(double)\0showClockChanged(int)\0"
    "showScoreChanged(int)\0showHudChanged(int)\0"
    "reverseTimeChanged(int)\0bool\0"
    "scalingPath,origin,end\0"
    "getScalingData(QString,int*,int*)\0"
    "demoFinished()\0"
};

void Gui::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Gui *_t = static_cast<Gui *>(_o);
        switch (_id) {
        case 0: _t->playButtonPressed(); break;
        case 1: _t->stopButtonPressed(); break;
        case 2: _t->updateProgressSlider((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 3: _t->progressSliderValueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->timeScaleChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 5: _t->showClockChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->showScoreChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->showHudChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->reverseTimeChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: { bool _r = _t->getScalingData((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< int*(*)>(_a[2])),(*reinterpret_cast< int*(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 10: _t->demoFinished(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Gui::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Gui::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_Gui,
      qt_meta_data_Gui, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Gui::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Gui::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Gui::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Gui))
        return static_cast<void*>(const_cast< Gui*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int Gui::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
