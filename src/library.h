#ifndef LIBRARY_H
#define LIBRARY_H

#include "track.h"

#include <QList>
#include <QObject>

/// 曲库持久化：保存 / 加载歌单到 JSON 文件
class Library : public QObject {
    Q_OBJECT

public:
    explicit Library(QObject *parent = nullptr);

    bool savePlaylist(const QString &name, const QList<Track> &tracks) const;
    QList<Track> loadPlaylist(const QString &name) const;
    QStringList playlistNames() const;

private:
    QString dataDir() const;
    QString filePath(const QString &name) const;
};

#endif // LIBRARY_H
