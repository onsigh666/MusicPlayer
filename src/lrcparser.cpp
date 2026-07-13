#include "lrcparser.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

bool LrcParser::load(const QString &filePath) {
    clear();

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    static const QRegularExpression re(R"(^\[(\d+):(\d+(?:\.\d+)?)\](.*))");
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QRegularExpressionMatch m = re.match(line);
        if (!m.hasMatch()) continue;

        Line entry;
        entry.timestamp = parseTimestamp(m.captured(1) + ":" + m.captured(2));
        entry.text = m.captured(3).trimmed();
        m_lines.append(entry);
    }
    return !m_lines.isEmpty();
}

void LrcParser::clear() { m_lines.clear(); }

int LrcParser::lineAt(qint64 positionMs) const {
    if (m_lines.isEmpty()) return -1;

    // 二分查找最后一个 timestamp <= positionMs 的行
    int lo = 0, hi = m_lines.size() - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (m_lines[mid].timestamp <= positionMs) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return hi; // hi == -1 表示还未到第一行歌词
}

qint64 LrcParser::parseTimestamp(const QStringView s) const {
    // 格式: mm:ss.xx 或 mm:ss
    int colon = s.indexOf(':');
    int min = s.left(colon).toInt();
    double sec = s.mid(colon + 1).toDouble();
    return static_cast<qint64>((min * 60 + sec) * 1000);
}
