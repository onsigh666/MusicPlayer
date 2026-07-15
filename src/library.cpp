#include "library.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

Library::Library(QObject *parent) : QObject(parent) {
    // 确保数据目录存在
    QDir().mkpath(dataDir());
}

QString Library::dataDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString Library::filePath(const QString &name) const {
    return dataDir() + "/" + name + ".json";
}

bool Library::savePlaylist(const QString &name, const QList<Track> &tracks) const {
    QJsonArray arr;
    for (const auto &t : tracks) {
        QJsonObject obj;
        obj["filePath"] = t.filePath;
        obj["title"]    = t.title;
        obj["artist"]   = t.artist;
        obj["album"]    = t.album;
        obj["duration"]    = t.duration;
        obj["lyricOffset"] = t.lyricOffset;
        arr.append(obj);
    }
    QJsonDocument doc(arr);

    QFile f(filePath(name));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(doc.toJson());
    return true;
}

QList<Track> Library::loadPlaylist(const QString &name) const {
    QList<Track> tracks;

    QFile f(filePath(name));
    if (!f.open(QIODevice::ReadOnly)) return tracks;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return tracks;

    for (const auto &val : doc.array()) {
        QJsonObject obj = val.toObject();
        Track t;
        t.filePath = obj["filePath"].toString();
        t.title    = obj["title"].toString();
        t.artist   = obj["artist"].toString();
        t.album    = obj["album"].toString();
        t.duration    = static_cast<qint64>(obj["duration"].toDouble());
        t.lyricOffset = static_cast<qint64>(obj["lyricOffset"].toDouble());
        tracks.append(t);
    }
    return tracks;
}

QStringList Library::playlistNames() const {
    QStringList names;
    QDir dir(dataDir());
    for (const auto &fi : dir.entryInfoList({"*.json"}, QDir::Files)) {
        names.append(fi.baseName());
    }
    return names;
}
