#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "playmodestrategy.h"
#include "track.h"

#include <QList>
#include <QObject>

#include <memory>

/// 播放模式
enum class PlayMode { Repeat, Shuffle, RepeatOne };

/// 管理一组 Track：增删、当前项、切歌、播放模式、搜索过滤
class Playlist : public QObject {
    Q_OBJECT

public:
    explicit Playlist(QObject *parent = nullptr);

    int count() const { return m_tracks.size(); }
    bool isEmpty() const { return m_tracks.isEmpty(); }
    int currentIndex() const { return m_currentIndex; }
    const Track &currentTrack() const;
    const Track &trackAt(int i) const { return m_tracks.at(i); }
    PlayMode playMode() const { return m_playMode; }

    /// 搜索：标题包含 keyword 的曲目索引
    QList<int> search(const QString &keyword) const;

public slots:
    /// 添加曲目（自动按 filePath 去重），返回真正新加入的列表
    QList<Track> addTracks(const QList<Track> &tracks);
    void removeTrack(int index);
    void clear();
    void setCurrentIndex(int index);
    Track next();     // 按当前播放模式返回下一首
    Track previous(); // 返回上一首
    void setPlayMode(PlayMode mode);
    void shuffle();   // 重排随机种子

signals:
    void currentIndexChanged(int index);
    void playlistChanged();

private:
    QList<Track> m_tracks;
    int m_currentIndex = -1;
    PlayMode m_playMode = PlayMode::Repeat;
    std::unique_ptr<PlayModeStrategy> m_strategy{
        std::make_unique<RepeatStrategy>()};
    QList<int> m_history; // 播放历史，用于 previous()
};

#endif // PLAYLIST_H
