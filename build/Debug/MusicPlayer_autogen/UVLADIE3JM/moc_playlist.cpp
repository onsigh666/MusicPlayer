/****************************************************************************
** Meta object code from reading C++ file 'playlist.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/playlist.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'playlist.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN8PlaylistE_t {};
} // unnamed namespace

template <> constexpr inline auto Playlist::qt_create_metaobjectdata<qt_meta_tag_ZN8PlaylistE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Playlist",
        "currentIndexChanged",
        "",
        "index",
        "playlistChanged",
        "addTracks",
        "QList<Track>",
        "tracks",
        "removeTrack",
        "clear",
        "setCurrentIndex",
        "next",
        "Track",
        "previous",
        "setPlayMode",
        "PlayMode",
        "mode",
        "shuffle"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'currentIndexChanged'
        QtMocHelpers::SignalData<void(int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'playlistChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'addTracks'
        QtMocHelpers::SlotData<QList<Track>(const QList<Track> &)>(5, 2, QMC::AccessPublic, 0x80000000 | 6, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Slot 'removeTrack'
        QtMocHelpers::SlotData<void(int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'clear'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setCurrentIndex'
        QtMocHelpers::SlotData<void(int)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'next'
        QtMocHelpers::SlotData<Track()>(11, 2, QMC::AccessPublic, 0x80000000 | 12),
        // Slot 'previous'
        QtMocHelpers::SlotData<Track()>(13, 2, QMC::AccessPublic, 0x80000000 | 12),
        // Slot 'setPlayMode'
        QtMocHelpers::SlotData<void(PlayMode)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 15, 16 },
        }}),
        // Slot 'shuffle'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Playlist, qt_meta_tag_ZN8PlaylistE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Playlist::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8PlaylistE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8PlaylistE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8PlaylistE_t>.metaTypes,
    nullptr
} };

void Playlist::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Playlist *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->currentIndexChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->playlistChanged(); break;
        case 2: { QList<Track> _r = _t->addTracks((*reinterpret_cast<std::add_pointer_t<QList<Track>>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QList<Track>*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->removeTrack((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->clear(); break;
        case 5: _t->setCurrentIndex((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: { Track _r = _t->next();
            if (_a[0]) *reinterpret_cast<Track*>(_a[0]) = std::move(_r); }  break;
        case 7: { Track _r = _t->previous();
            if (_a[0]) *reinterpret_cast<Track*>(_a[0]) = std::move(_r); }  break;
        case 8: _t->setPlayMode((*reinterpret_cast<std::add_pointer_t<PlayMode>>(_a[1]))); break;
        case 9: _t->shuffle(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Playlist::*)(int )>(_a, &Playlist::currentIndexChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Playlist::*)()>(_a, &Playlist::playlistChanged, 1))
            return;
    }
}

const QMetaObject *Playlist::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Playlist::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8PlaylistE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Playlist::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void Playlist::currentIndexChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void Playlist::playlistChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
