#include "player.h"

#include <QAudioOutput>
#include <QMediaMetaData>
#include <QMediaPlayer>

Player::Player(QObject *parent) : QObject(parent) {
    m_audioOut = new QAudioOutput(this);
    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(m_audioOut);

    connect(m_player, &QMediaPlayer::positionChanged,
            this, &Player::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &Player::durationChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) emit endOfMedia();
    });
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState s) {
        State prev = m_state;
        switch (s) {
        case QMediaPlayer::PlayingState: m_state = Playing;  break;
        case QMediaPlayer::PausedState:  m_state = Paused;   break;
        case QMediaPlayer::StoppedState: m_state = Stopped;  break;
        }
        if (m_state != prev) emit stateChanged(m_state);
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &msg) {
        emit errorOccurred(msg);
    });
    connect(m_player, &QMediaPlayer::metaDataChanged, this, [this]() {
        auto md = m_player->metaData();
        emit metaDataReady(
            md.stringValue(QMediaMetaData::Title),
            md.stringValue(QMediaMetaData::ContributingArtist),
            md.stringValue(QMediaMetaData::AlbumTitle));
    });
}

Player::~Player() = default;

Player::State Player::state() const { return m_state; }
qint64 Player::position() const { return m_player->position(); }
qint64 Player::duration() const { return m_player->duration(); }
float Player::volume() const { return m_audioOut->volume(); }

void Player::setSource(const QString &filePath) {
    m_player->setSource(QUrl::fromLocalFile(filePath));
}
void Player::play()  { m_player->play(); }
void Player::pause() { m_player->pause(); }
void Player::stop()  { m_player->stop(); }
void Player::setVolume(float vol) { m_audioOut->setVolume(vol); }
void Player::setPosition(qint64 ms) { m_player->setPosition(ms); }
