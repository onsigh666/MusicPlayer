/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/mainwindow.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "onOpenFiles",
        "",
        "onScanFolder",
        "onClearAll",
        "onPlayPause",
        "onStop",
        "onNext",
        "onPrevious",
        "onPlaylistDoubleClicked",
        "QModelIndex",
        "index",
        "onPlaylistContextMenu",
        "QPoint",
        "pos",
        "onSeek",
        "position",
        "onVolumeChanged",
        "volume",
        "onTogglePlayMode",
        "onLyricOffsetPlus",
        "onLyricOffsetMinus",
        "onTogglePanel",
        "onSearchTextChanged",
        "text",
        "onLyricScrollChanged",
        "value",
        "onLyricJump",
        "onToggleTheme",
        "onAnalysisComplete",
        "AnalysisData",
        "data",
        "onAnalysisFailed",
        "reason",
        "refreshPlayPauseBtn",
        "Player::State",
        "state",
        "refreshTimeLabel",
        "posMs",
        "refreshProgressRange",
        "durMs",
        "refreshCurrentRow",
        "onTrackEnded",
        "onPlayerError",
        "msg",
        "onMetaDataReady",
        "title",
        "artist",
        "album"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onOpenFiles'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onScanFolder'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onClearAll'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPlayPause'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStop'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNext'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPrevious'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPlaylistDoubleClicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Slot 'onPlaylistContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Slot 'onSeek'
        QtMocHelpers::SlotData<void(int)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 16 },
        }}),
        // Slot 'onVolumeChanged'
        QtMocHelpers::SlotData<void(int)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 18 },
        }}),
        // Slot 'onTogglePlayMode'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onLyricOffsetPlus'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onLyricOffsetMinus'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTogglePanel'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSearchTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 24 },
        }}),
        // Slot 'onLyricScrollChanged'
        QtMocHelpers::SlotData<void(int)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 26 },
        }}),
        // Slot 'onLyricJump'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToggleTheme'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAnalysisComplete'
        QtMocHelpers::SlotData<void(const AnalysisData &)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 30, 31 },
        }}),
        // Slot 'onAnalysisFailed'
        QtMocHelpers::SlotData<void(const QString &)>(32, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 33 },
        }}),
        // Slot 'refreshPlayPauseBtn'
        QtMocHelpers::SlotData<void(Player::State)>(34, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 35, 36 },
        }}),
        // Slot 'refreshTimeLabel'
        QtMocHelpers::SlotData<void(qint64)>(37, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 38 },
        }}),
        // Slot 'refreshProgressRange'
        QtMocHelpers::SlotData<void(qint64)>(39, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 40 },
        }}),
        // Slot 'refreshCurrentRow'
        QtMocHelpers::SlotData<void(int)>(41, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 11 },
        }}),
        // Slot 'onTrackEnded'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPlayerError'
        QtMocHelpers::SlotData<void(const QString &)>(43, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 44 },
        }}),
        // Slot 'onMetaDataReady'
        QtMocHelpers::SlotData<void(const QString &, const QString &, const QString &)>(45, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 46 }, { QMetaType::QString, 47 }, { QMetaType::QString, 48 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onOpenFiles(); break;
        case 1: _t->onScanFolder(); break;
        case 2: _t->onClearAll(); break;
        case 3: _t->onPlayPause(); break;
        case 4: _t->onStop(); break;
        case 5: _t->onNext(); break;
        case 6: _t->onPrevious(); break;
        case 7: _t->onPlaylistDoubleClicked((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 8: _t->onPlaylistContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 9: _t->onSeek((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->onVolumeChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->onTogglePlayMode(); break;
        case 12: _t->onLyricOffsetPlus(); break;
        case 13: _t->onLyricOffsetMinus(); break;
        case 14: _t->onTogglePanel(); break;
        case 15: _t->onSearchTextChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 16: _t->onLyricScrollChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 17: _t->onLyricJump(); break;
        case 18: _t->onToggleTheme(); break;
        case 19: _t->onAnalysisComplete((*reinterpret_cast<std::add_pointer_t<AnalysisData>>(_a[1]))); break;
        case 20: _t->onAnalysisFailed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 21: _t->refreshPlayPauseBtn((*reinterpret_cast<std::add_pointer_t<Player::State>>(_a[1]))); break;
        case 22: _t->refreshTimeLabel((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 23: _t->refreshProgressRange((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 24: _t->refreshCurrentRow((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 25: _t->onTrackEnded(); break;
        case 26: _t->onPlayerError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 27: _t->onMetaDataReady((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 28)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 28;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 28)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 28;
    }
    return _id;
}
QT_WARNING_POP
