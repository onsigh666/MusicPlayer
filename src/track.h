#ifndef TRACK_H
#define TRACK_H

#include <QFileInfo>
#include <QString>

/// 一首歌的数据
struct Track {
    QString filePath;
    QString title;
    QString artist;
    QString album;
    qint64 duration = 0;     // 毫秒
    qint64 lyricOffset = 0;  // 歌词时间偏移（毫秒），+ 表示歌词后移

    /// 从文件路径构造，标题默认取文件名
    static Track fromFile(const QString &path) {
        Track t;
        t.filePath = path;
        QFileInfo fi(path);
        t.title = fi.completeBaseName();
        return t;
    }
};

#endif // TRACK_H
