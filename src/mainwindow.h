#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "discwidget.h"
#include "player.h"
#include "track.h"

#include <QMainWindow>

class Playlist;
class Library;
class LrcParser;
class QLabel;
class QListWidget;
class QPushButton;
class QSlider;
class QWidget;

/// 主窗口：仅负责 UI 构建和信号接线，不直接操作 QMediaPlayer
class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

private slots:
  // UI → 逻辑
  void onOpenFiles();
  void onScanFolder();
  void onPlayPause();
  void onStop();
  void onNext();
  void onPrevious();
  void onPlaylistDoubleClicked(const QModelIndex &index);
  void onPlaylistContextMenu(const QPoint &pos);
  void onSeek(int position);
  void onVolumeChanged(int volume);
  void onTogglePlayMode();

  // 逻辑 → UI
  void refreshPlayPauseBtn(Player::State state);
  void refreshTimeLabel(qint64 posMs);
  void refreshProgressRange(qint64 durMs);
  void refreshCurrentRow(int index);
  void onTrackEnded();
  void onPlayerError(const QString &msg);

private:
  void setupUI();
  void wireSignals();
  void loadCurrentTrack();
  void loadLibraryTracks();                    // 启动时加载持久化曲库
  void saveLibraryTracks();                    // 保存当前曲库到文件
  QList<Track> scanDir(const QString &dirPath) const;  // 递归扫描文件夹
  static QString formatTime(qint64 ms);

  // 核心组件
  Player *m_player = nullptr;
  Playlist *m_playlist = nullptr;
  Library *m_library = nullptr;
  LrcParser *m_lrc = nullptr;

  // UI 控件
  QWidget *m_rightPanel = nullptr;
  DiscWidget *m_disc = nullptr;
  QLabel *m_titleLabel = nullptr;
  QLabel *m_timeLabel = nullptr;
  QLabel *m_modeLabel = nullptr;
  QSlider *m_progressSlider = nullptr;
  QSlider *m_volumeSlider = nullptr;
  QListWidget *m_playlistWidget = nullptr;
  int m_highlightedRow = -1;  // 当前播放曲目行（金色背景）
  QPushButton *m_playBtn = nullptr;
  QPushButton *m_stopBtn = nullptr;
  QPushButton *m_nextBtn = nullptr;
  QPushButton *m_prevBtn = nullptr;
  QPushButton *m_modeBtn = nullptr;
};

#endif // MAINWINDOW_H
