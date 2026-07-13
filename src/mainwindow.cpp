#include "mainwindow.h"
#include "library.h"
#include "lrcparser.h"
#include "player.h"
#include "playlist.h"
#include "track.h"

#include <QApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

// ─────────────────────── 构造 / 析构 ───────────────────────

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  m_player = new Player(this);
  m_playlist = new Playlist(this);
  m_library = new Library(this);
  m_lrc = new LrcParser();

  setupUI();
  wireSignals();

  // 启动时恢复上次扫描的曲库
  loadLibraryTracks();
}

MainWindow::~MainWindow() = default;

// ─────────────────────── UI 构建 ───────────────────────

void MainWindow::setupUI() {
  auto *central = new QWidget(this);
  // 玻璃质感半透明紫色背景
  central->setStyleSheet(
      "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
      " stop:0 rgba(225, 210, 240, 190),"
      " stop:0.5 rgba(215, 200, 235, 180),"
      " stop:1 rgba(205, 190, 230, 190));");
  auto *mainLayout = new QVBoxLayout(central);

  // 歌曲标题
  m_titleLabel = new QLabel("未在播放", central);
  m_titleLabel->setAlignment(Qt::AlignCenter);
  m_titleLabel->setStyleSheet(
      "font-size: 16px; font-weight: bold; margin: 8px;");
  mainLayout->addWidget(m_titleLabel);

  // 中间区域：左侧曲库列表 (1/3) + 右侧面板 (2/3)
  auto *middleLayout = new QHBoxLayout;
  m_playlistWidget = new QListWidget(central);
  m_playlistWidget->setMinimumWidth(180);
  m_playlistWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  m_playlistWidget->setStyleSheet(
      "QListWidget { border-radius: 8px; border: 1px solid rgba(150,130,180,80);"
      " background: rgba(255,255,255,50); }");
  middleLayout->addWidget(m_playlistWidget, 1); // 占 1/3

  // 右侧面板：旋转光碟
  m_rightPanel = new QWidget(central);
  m_rightPanel->setStyleSheet("background: rgba(255,255,255,40); border-radius: 8px;");
  auto *rightLayout = new QVBoxLayout(m_rightPanel);
  m_disc = new DiscWidget(m_rightPanel);
  m_disc->setMinimumSize(200, 200);
  rightLayout->addWidget(m_disc);
  middleLayout->addWidget(m_rightPanel, 2); // 占 2/3

  mainLayout->addLayout(middleLayout);

  // 进度条 + 时间
  auto *progressLayout = new QHBoxLayout;
  m_progressSlider = new QSlider(Qt::Horizontal, central);
  m_progressSlider->setEnabled(false);
  m_timeLabel = new QLabel("00:00 / 00:00", central);
  progressLayout->addWidget(m_progressSlider, 1);
  progressLayout->addWidget(m_timeLabel);
  mainLayout->addLayout(progressLayout);

  // 控制栏
  auto *controlLayout = new QHBoxLayout;

  m_prevBtn = new QPushButton("⏮", central);
  m_playBtn = new QPushButton("▶", central);
  m_stopBtn = new QPushButton("⏹", central);
  m_nextBtn = new QPushButton("⏭", central);
  m_modeBtn = new QPushButton("→", central); // Sequential 图标

  for (auto *btn : {m_prevBtn, m_playBtn, m_stopBtn, m_nextBtn, m_modeBtn}) {
    btn->setFixedSize(44, 44);
    btn->setStyleSheet(
        "font-size: 18px;"
        "border-radius: 10px;"
        "border: 1px solid rgba(150,130,180,100);"
        "background: rgba(255,255,255,60);");
  }

  m_volumeSlider = new QSlider(Qt::Horizontal, central);
  m_volumeSlider->setRange(0, 100);
  m_volumeSlider->setValue(70);
  m_volumeSlider->setMaximumWidth(120);

  m_modeLabel = new QLabel("顺序播放", central);

  auto *openBtn = new QPushButton("📂 打开文件", central);
  openBtn->setStyleSheet(
      "border-radius: 8px; padding: 4px 10px;"
      "border: 1px solid rgba(150,130,180,100);"
      "background: rgba(255,255,255,60);");
  connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpenFiles);

  auto *scanBtn = new QPushButton("📁 扫描文件夹", central);
  scanBtn->setStyleSheet(openBtn->styleSheet());
  connect(scanBtn, &QPushButton::clicked, this, &MainWindow::onScanFolder);

  controlLayout->addStretch();
  controlLayout->addWidget(m_prevBtn);
  controlLayout->addWidget(m_playBtn);
  controlLayout->addWidget(m_stopBtn);
  controlLayout->addWidget(m_nextBtn);
  controlLayout->addWidget(m_modeBtn);
  controlLayout->addWidget(m_modeLabel);
  controlLayout->addStretch();
  controlLayout->addWidget(new QLabel("🔊", central));
  controlLayout->addWidget(m_volumeSlider);
  controlLayout->addWidget(scanBtn);
  controlLayout->addWidget(openBtn);
  mainLayout->addLayout(controlLayout);

  setCentralWidget(central);
}

// ─────────────────────── 信号接线 ───────────────────────

void MainWindow::wireSignals() {
  // UI 按钮 → 逻辑
  connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
  connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
  connect(m_nextBtn, &QPushButton::clicked, this, &MainWindow::onNext);
  connect(m_prevBtn, &QPushButton::clicked, this, &MainWindow::onPrevious);
  connect(m_modeBtn, &QPushButton::clicked, this,
          &MainWindow::onTogglePlayMode);
  connect(m_playlistWidget, &QListWidget::doubleClicked, this,
          &MainWindow::onPlaylistDoubleClicked);
  connect(m_playlistWidget, &QListWidget::customContextMenuRequested, this,
          &MainWindow::onPlaylistContextMenu);
  connect(m_progressSlider, &QSlider::sliderMoved, this, &MainWindow::onSeek);
  connect(m_volumeSlider, &QSlider::valueChanged, this,
          &MainWindow::onVolumeChanged);

  // Player → UI
  connect(m_player, &Player::stateChanged, this,
          &MainWindow::refreshPlayPauseBtn);
  connect(m_player, &Player::positionChanged, this,
          &MainWindow::refreshTimeLabel);
  connect(m_player, &Player::durationChanged, this,
          &MainWindow::refreshProgressRange);
  connect(m_player, &Player::endOfMedia, this, &MainWindow::onTrackEnded);
  connect(m_player, &Player::errorOccurred, this, &MainWindow::onPlayerError);

  // Player → 光碟
  connect(m_player, &Player::stateChanged, m_disc, [this](Player::State s) {
    switch (s) {
    case Player::Playing:
      m_disc->startSpin();
      break;
    case Player::Paused:
      m_disc->pauseSpin();
      break;
    case Player::Stopped:
      m_disc->stopSpin();
      break;
    }
  });

  // Playlist → UI
  connect(m_playlist, &Playlist::currentIndexChanged, this,
          &MainWindow::refreshCurrentRow);

  // 初始音量
  m_player->setVolume(m_volumeSlider->value() / 100.0f);
}

// ─────────────────────── UI → 逻辑 ───────────────────────

void MainWindow::onOpenFiles() {
  QStringList files = QFileDialog::getOpenFileNames(
      this, "选择音乐文件", QString(),
      "音频文件 (*.mp3 *.wav *.flac *.ogg *.aac *.m4a *.wma);;所有文件 (*)");
  if (files.isEmpty())
    return;

  QList<Track> tracks;
  for (const auto &path : files)
    tracks.append(Track::fromFile(path));

  // 去重添加，只把真正新增的加入界面
  QList<Track> fresh = m_playlist->addTracks(tracks);
  for (const auto &t : fresh)
    m_playlistWidget->addItem(t.title);

  // 高亮当前曲目（addTracks 信号触发时列表尚未填充）
  refreshCurrentRow(m_playlist->currentIndex());

  // 保存曲库
  saveLibraryTracks();

  // 如果是第一批曲目，自动播放第一首
  if (m_player->state() == Player::Stopped &&
      m_playlist->count() == fresh.size()) {
    loadCurrentTrack();
    m_player->play();
  }
}

void MainWindow::onPlayPause() {
  if (m_playlist->isEmpty())
    return;
  if (m_player->state() == Player::Playing) {
    m_player->pause();
  } else {
    // 如果是停止状态且没设置过源，先加载当前曲目
    if (m_player->state() == Player::Stopped && m_player->duration() == 0) {
      loadCurrentTrack();
    }
    m_player->play();
  }
}

void MainWindow::onStop() { m_player->stop(); }

void MainWindow::onNext() {
  if (m_playlist->isEmpty())
    return;
  Track t = m_playlist->next();
  if (t.filePath.isEmpty())
    return; // Sequential 模式到了末尾
  m_player->setSource(t.filePath);
  m_player->play();
  m_titleLabel->setText(t.title);
}

void MainWindow::onPrevious() {
  if (m_playlist->isEmpty())
    return;
  Track t = m_playlist->previous();
  if (t.filePath.isEmpty())
    return;
  m_player->setSource(t.filePath);
  m_player->play();
  m_titleLabel->setText(t.title);
}

void MainWindow::onPlaylistDoubleClicked(const QModelIndex &index) {
  int row = index.row();
  m_playlist->setCurrentIndex(row);
  loadCurrentTrack();
  m_player->play();
}

void MainWindow::onPlaylistContextMenu(const QPoint &pos) {
  QListWidgetItem *item = m_playlistWidget->itemAt(pos);
  if (!item) return;

  int row = m_playlistWidget->row(item);
  QMenu menu(this);
  QAction *delAction = menu.addAction("从列表中删除");
  menu.addSeparator();

  QAction *chosen = menu.exec(m_playlistWidget->viewport()->mapToGlobal(pos));
  if (chosen != delAction) return;

  // 如果要删除的是正在播放的曲目，先停止
  if (row == m_playlist->currentIndex()) {
    m_player->stop();
  }

  // 先从视图移除，再从模型移除（顺序重要：removeTrack 会发信号触发 refreshCurrentRow）
  delete m_playlistWidget->takeItem(row);

  // 修正高亮索引
  if (row < m_highlightedRow)
    --m_highlightedRow;
  else if (row == m_highlightedRow)
    m_highlightedRow = -1;

  m_playlist->removeTrack(row);

  saveLibraryTracks();
}

void MainWindow::onSeek(int position) { m_player->setPosition(position); }

void MainWindow::onVolumeChanged(int volume) {
  m_player->setVolume(volume / 100.0f);
}

void MainWindow::onTogglePlayMode() {
  PlayMode current = m_playlist->playMode();
  PlayMode next;
  QString label, icon;
  switch (current) {
  case PlayMode::Sequential:
    next = PlayMode::Repeat;
    label = "列表循环";
    icon = "🔁";
    break;
  case PlayMode::Repeat:
    next = PlayMode::Shuffle;
    label = "随机播放";
    icon = "🔀";
    break;
  case PlayMode::Shuffle:
    next = PlayMode::RepeatOne;
    label = "单曲循环";
    icon = "🔂";
    break;
  case PlayMode::RepeatOne:
    next = PlayMode::Sequential;
    label = "顺序播放";
    icon = "→";
    break;
  }
  m_playlist->setPlayMode(next);
  m_modeLabel->setText(label);
  m_modeBtn->setText(icon);
}

// ─────────────────────── 扫描曲库 ───────────────────────

void MainWindow::onScanFolder() {
  QString dir = QFileDialog::getExistingDirectory(this, "选择音乐文件夹");
  if (dir.isEmpty())
    return;

  QList<Track> found = scanDir(dir);
  if (found.isEmpty()) {
    QMessageBox::information(this, "扫描结果", "未找到音频文件");
    return;
  }

  // 去重添加
  QList<Track> fresh = m_playlist->addTracks(found);
  for (const auto &t : fresh)
    m_playlistWidget->addItem(t.title);
  refreshCurrentRow(m_playlist->currentIndex());

  // 持久化
  saveLibraryTracks();

  // 如果此前列表为空，自动播放第一首
  if (m_player->state() == Player::Stopped &&
      m_playlist->count() == fresh.size()) {
    loadCurrentTrack();
    m_player->play();
  }
}

QList<Track> MainWindow::scanDir(const QString &dirPath) const {
  QList<Track> result;
  QDirIterator it(dirPath, {"*.mp3"}, QDir::Files,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    result.append(Track::fromFile(it.filePath()));
  }
  return result;
}

void MainWindow::loadLibraryTracks() {
  QList<Track> saved = m_library->loadPlaylist("music_library");
  if (saved.isEmpty())
    return;

  QList<Track> fresh = m_playlist->addTracks(saved);
  for (const auto &t : fresh)
    m_playlistWidget->addItem(t.title);
  refreshCurrentRow(m_playlist->currentIndex());
}

void MainWindow::saveLibraryTracks() {
  QList<Track> all;
  for (int i = 0; i < m_playlist->count(); ++i)
    all.append(m_playlist->trackAt(i));
  m_library->savePlaylist("music_library", all);
}

// ─────────────────────── 逻辑 → UI ───────────────────────

void MainWindow::refreshPlayPauseBtn(Player::State state) {
  m_playBtn->setText(state == Player::Playing ? "⏸" : "▶");
}

void MainWindow::refreshTimeLabel(qint64 posMs) {
  if (!m_progressSlider->isSliderDown())
    m_progressSlider->setValue(static_cast<int>(posMs));
  qint64 dur = m_player->duration();
  m_timeLabel->setText(formatTime(posMs) + " / " + formatTime(dur));
}

void MainWindow::refreshProgressRange(qint64 durMs) {
  m_progressSlider->setRange(0, static_cast<int>(durMs));
  m_progressSlider->setEnabled(durMs > 0);
}

void MainWindow::refreshCurrentRow(int index) {
  // 清除旧曲目的金色背景和加粗
  if (m_highlightedRow >= 0 && m_highlightedRow < m_playlistWidget->count()) {
    auto *item = m_playlistWidget->item(m_highlightedRow);
    item->setBackground(QBrush());
    QFont f = item->font();
    f.setBold(false);
    item->setFont(f);
  }

  // 设置新曲目的金色背景 + 加粗
  if (index >= 0 && index < m_playlistWidget->count()) {
    auto *item = m_playlistWidget->item(index);
    item->setBackground(QColor("#FFD700"));
    QFont f = item->font();
    f.setBold(true);
    item->setFont(f);
    m_highlightedRow = index;
  }

  // 鼠标选中仅显示焦点框（默认行为）
  m_playlistWidget->setCurrentRow(index);
}

void MainWindow::onTrackEnded() {
  // RepeatOne: 从头重播同一首，不重新设置源
  if (m_playlist->playMode() == PlayMode::RepeatOne) {
    m_player->setPosition(0);
    m_player->play();
    return;
  }
  onNext();
}

void MainWindow::onPlayerError(const QString &msg) {
  QMessageBox::warning(this, "播放错误", msg);
}

// ─────────────────────── 辅助 ───────────────────────

void MainWindow::loadCurrentTrack() {
  const Track &t = m_playlist->currentTrack();
  if (t.filePath.isEmpty())
    return;
  m_player->setSource(t.filePath);
  m_titleLabel->setText(t.title);
}

QString MainWindow::formatTime(qint64 ms) {
  int secs = static_cast<int>(ms / 1000);
  return QString("%1:%2")
      .arg(secs / 60, 2, 10, QChar('0'))
      .arg(secs % 60, 2, 10, QChar('0'));
}
