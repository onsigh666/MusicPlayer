#ifndef LRCPARSER_H
#define LRCPARSER_H

#include <QList>
#include <QString>

/// 解析 .lrc 歌词文件，根据播放时间返回当前歌词行
class LrcParser {
public:
    struct Line {
        qint64 timestamp = 0; // 毫秒
        QString text;
    };

    bool load(const QString &filePath);
    void clear();
    bool isEmpty() const { return m_lines.isEmpty(); }
    const QList<Line> &lines() const { return m_lines; }

    /// 根据播放位置（毫秒）+ 偏移量返回歌词行索引，未找到返回 -1
    int lineAt(qint64 positionMs, qint64 offsetMs = 0) const;

private:
    qint64 parseTimestamp(const QStringView s) const;

    QList<Line> m_lines;
};

#endif // LRCPARSER_H
