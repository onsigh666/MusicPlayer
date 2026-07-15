#include "playlist.h"
#include <QRandomGenerator>
#include <QSet>

Playlist::Playlist(QObject *parent) : QObject(parent) {}

const Track &Playlist::currentTrack() const {
    static Track empty;
    if (m_currentIndex < 0 || m_currentIndex >= m_tracks.size()) return empty;
    return m_tracks.at(m_currentIndex);
}

QList<Track> Playlist::addTracks(const QList<Track> &tracks) {
    // 构建已有路径集合，去重
    QSet<QString> existing;
    for (const auto &t : m_tracks) existing.insert(t.filePath);

    QList<Track> fresh;
    for (const auto &t : tracks) {
        if (!existing.contains(t.filePath)) {
            fresh.append(t);
        }
    }
    if (fresh.isEmpty()) return fresh;

    int oldSize = m_tracks.size();
    m_tracks.append(fresh);
    emit playlistChanged();

    // 首次添加时自动选中第一首
    if (oldSize == 0 && m_currentIndex == -1) {
        m_currentIndex = 0;
        emit currentIndexChanged(0);
    }
    return fresh;
}

void Playlist::removeTrack(int index) {
    if (index < 0 || index >= m_tracks.size()) return;
    m_tracks.removeAt(index);
    m_history.removeAll(index);
    // 修正历史索引
    for (int &i : m_history) {
        if (i > index) --i;
    }

    if (m_tracks.isEmpty()) {
        m_currentIndex = -1;
    } else if (index < m_currentIndex) {
        --m_currentIndex;
    } else if (index == m_currentIndex) {
        m_currentIndex = qMin(m_currentIndex, m_tracks.size() - 1);
    }
    emit playlistChanged();
    emit currentIndexChanged(m_currentIndex);
}

void Playlist::clear() {
    m_tracks.clear();
    m_history.clear();
    m_currentIndex = -1;
    emit playlistChanged();
    emit currentIndexChanged(-1);
}

void Playlist::setCurrentIndex(int index) {
    if (index < 0 || index >= m_tracks.size() || index == m_currentIndex) return;
    if (m_currentIndex >= 0) m_history.prepend(m_currentIndex);
    m_currentIndex = index;
    emit currentIndexChanged(index);
}

Track Playlist::next() {
    if (m_tracks.isEmpty()) return Track();

    int nextIdx = m_currentIndex;
    switch (m_playMode) {
    case PlayMode::RepeatOne:
        // 手动切歌：前进到下一首（原地循环由 onTrackEnded 处理）
    case PlayMode::Repeat:
        nextIdx = (m_currentIndex + 1) % m_tracks.size();
        break;
    case PlayMode::Shuffle:
        nextIdx = QRandomGenerator::global()->bounded(m_tracks.size());
        break;
    }

    if (m_currentIndex >= 0) m_history.prepend(m_currentIndex);
    m_currentIndex = nextIdx;
    emit currentIndexChanged(nextIdx);
    return m_tracks.at(nextIdx);
}

Track Playlist::previous() {
    if (m_tracks.isEmpty()) return Track();
    // 有历史则回退，无历史则回到列表末尾
    int prevIdx = m_history.isEmpty()
        ? qMax(m_currentIndex - 1, 0)
        : m_history.takeFirst();
    m_currentIndex = prevIdx;
    emit currentIndexChanged(prevIdx);
    return m_tracks.at(prevIdx);
}

void Playlist::setPlayMode(PlayMode mode) { m_playMode = mode; }

void Playlist::shuffle() {
    for (int i = m_tracks.size() - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        m_tracks.swapItemsAt(i, j);
    }
    m_currentIndex = 0;
    m_history.clear();
    emit playlistChanged();
    emit currentIndexChanged(0);
}

QList<int> Playlist::search(const QString &keyword) const {
    QList<int> result;
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks.at(i).title.contains(keyword, Qt::CaseInsensitive))
            result.append(i);
    }
    return result;
}
