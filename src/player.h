#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>

class QMediaPlayer;
class QAudioOutput;

/// 把 QMediaPlayer 包装成干净接口，界面只依赖此类
class Player : public QObject {
  Q_OBJECT

public:
  enum State { Stopped, Playing, Paused };

  explicit Player(QObject *parent = nullptr);
  ~Player() override;

  State state() const;
  qint64 position() const;
  qint64 duration() const;
  float volume() const;

public slots:
  void setSource(const QString &filePath);
  void play();
  void pause();
  void stop();
  void setVolume(float vol); // 0.0 ~ 1.0
  void setPosition(qint64 ms);

signals:
  void positionChanged(qint64 posMs);
  void durationChanged(qint64 durMs);
  void stateChanged(Player::State state);
  void endOfMedia();
  void errorOccurred(const QString &msg);

private:
  QMediaPlayer *m_player = nullptr;
  QAudioOutput *m_audioOut = nullptr;
  State m_state = Stopped;
};

#endif // PLAYER_H
