#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "audioanalyzer.h"
#include "discwidget.h"
#include "player.h"
#include "spectrumwidget.h"
#include "track.h"

#include <QMainWindow>

/// 主题模式
enum class Theme { Dark, Light };

class Playlist;
class Library;
class LrcParser;
class Transcoder;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSlider;
class QStackedWidget;
class QTimer;
class QWidget;

/// 主窗口：仅负责 UI 构建和信号接线，不直接操作 QMediaPlayer
class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

protected:
  void closeEvent(QCloseEvent *event) override;

private slots:
  // UI → 逻辑
  void onOpenFiles();
  void onScanFolder();
  void onClearAll();
  void onPlayPause();
  void onStop();
  void onNext();
  void onPrevious();
  void onPlaylistDoubleClicked(const QModelIndex &index);
  void onPlaylistContextMenu(const QPoint &pos);
  void onSeek(int position);
  void onVolumeChanged(int volume);
  void onTogglePlayMode();
  void onLyricOffsetPlus();
  void onLyricOffsetMinus();
  void onTogglePanel();  // 切换光碟/歌词视图
  void onSearchTextChanged(const QString &text); // 搜索过滤
  void onLyricScrollChanged(int value); // 用户拖动歌词
  void onLyricJump();                   // 跳转到浏览行
  void onToggleTheme();                // 深色/浅色模式切换
  void onAnalysisComplete(const AnalysisData &data);
  void onAnalysisFailed(const QString &reason);

  // 逻辑 → UI
  void refreshPlayPauseBtn(Player::State state);
  void refreshTimeLabel(qint64 posMs);
  void refreshProgressRange(qint64 durMs);
  void refreshCurrentRow(int index);
  void onTrackEnded();
  void onPlayerError(const QString &msg);
  void onMetaDataReady(const QString &title, const QString &artist,
                       const QString &album);

private:
  void setupUI();
  void wireSignals();
  void loadCurrentTrack();
  void loadLibraryTracks();                    // 启动时加载持久化曲库
  void saveLibraryTracks();                    // 保存当前曲库到文件
  QList<Track> scanDir(const QString &dirPath) const;  // 递归扫描文件夹
  void autoLoadLyric(const QString &audioPath); // 搜索并加载同名 .lrc
  void refreshLyric(qint64 posMs);              // 根据播放位置高亮当前歌词行
  void saveLyricOffset();                       // 持久化当前曲目的偏移量
  void applySearchFilter();                     // 根据搜索框刷新播放列表显示
  void applyTheme();                             // 根据 m_theme 刷新全部样式
  QString formatTime(qint64 ms);
  void triggerAudioAnalysis(const QString &filePath, qint64 durationMs);

  Theme m_theme = Theme::Dark;
  QColor m_highlightBg{0x2d, 0x25, 0x25};  // 当前播放行高亮背景
  QColor m_highlightFg{0xec, 0x41, 0x41};  // 当前播放行高亮文字

  // 核心组件
  Player *m_player = nullptr;
  Playlist *m_playlist = nullptr;
  Library *m_library = nullptr;
  LrcParser *m_lrc = nullptr;
  Transcoder *m_transcoder = nullptr;
  AudioAnalyzer *m_analyzer = nullptr;
  SpectrumWidget *m_spectrum = nullptr;
  AnalysisData m_currentAnalysis;

  // UI 控件
  QWidget *m_rightPanel = nullptr;
  QStackedWidget *m_stack = nullptr;
  DiscWidget *m_disc = nullptr;
  QLabel *m_titleLabel = nullptr;
  QLabel *m_infoArtist = nullptr;
  QLabel *m_infoAlbum = nullptr;
  QString m_tagTitle;  // 从标签读到的标题（为空则退回文件名）
  QWidget *m_lyricPage = nullptr;           // 歌词页容器（按钮挂载点）
  QListWidget *m_lyricWidget = nullptr;     // 歌词显示列表
  QLabel *m_lyricOffsetLabel = nullptr;     // "±0.0s"
  QPushButton *m_lyricMinusBtn = nullptr;
  QPushButton *m_lyricPlusBtn = nullptr;
  int m_lyricLine = -1;  // 当前播放高亮的歌词行
  // 歌词拖动浏览
  bool m_lyricAutoFollow = true;          // 自动跟随播放位置
  bool m_lyricProgramScroll = false;      // 标记程序自动滚动
  int m_lyricCenterRow = -1;              // 拖动时屏幕中央的行号
  QTimer *m_lyricReturnTimer = nullptr;   // 3 秒自动回弹
  QTimer *m_lyricSnapTimer = nullptr;     // 200ms 吸附防抖
  QPushButton *m_lyricJumpBtn = nullptr;  // 跳转按钮
  qint64 m_restorePosition = -1;          // 启动时待恢复的播放位置
  QLabel *m_timeLabel = nullptr;
  QLabel *m_modeLabel = nullptr;
  QSlider *m_progressSlider = nullptr;
  QSlider *m_volumeSlider = nullptr;
  QListWidget *m_playlistWidget = nullptr;
  QLineEdit *m_searchBox = nullptr;       // 搜索框
  int m_highlightedRow = -1;  // 当前播放曲目行
  QPushButton *m_playBtn = nullptr;
  QPushButton *m_stopBtn = nullptr;
  QPushButton *m_nextBtn = nullptr;
  QPushButton *m_prevBtn = nullptr;
  QPushButton *m_modeBtn = nullptr;
  QPushButton *m_toggleBtn = nullptr;   // 光碟/歌词切换
  QPushButton *m_themeBtn = nullptr;    // 深色/浅色模式切换
  QWidget *m_central = nullptr;         // 中央控件（applyTheme 需要）
};

#endif // MAINWINDOW_H
