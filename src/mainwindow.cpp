#include "mainwindow.h"
#include "library.h"
#include "lrcparser.h"
#include "player.h"
#include "playlist.h"
#include "track.h"
#include "transcoder.h"

#include <QApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QPainter>
#include <QPolygonF>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollBar>
#include <QSlider>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

// ─────────────────────── 实心红色右三角按钮 ───────────────────────

class TriangleButton : public QPushButton {
public:
  using QPushButton::QPushButton;

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#ec4141"));
    p.setPen(Qt::NoPen);
    qreal w = width(), h = height();
    QPolygonF tri;
    tri << QPointF(w * 0.15, h * 0.2)
        << QPointF(w * 0.85, h * 0.5)
        << QPointF(w * 0.15, h * 0.8);
    p.drawPolygon(tri);
  }
};

// ─────────────────────── 构造 / 析构 ───────────────────────

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  m_player = new Player(this);
  m_playlist = new Playlist(this);
  m_library = new Library(this);
  m_lrc = new LrcParser();
  m_transcoder = new Transcoder(this);

  setupUI();
  wireSignals();

  // 启动时恢复上次扫描的曲库
  loadLibraryTracks();
}

MainWindow::~MainWindow() = default;

// ─────────────────────── UI 构建 ───────────────────────

void MainWindow::setupUI() {
  auto *central = new QWidget(this);
  // 网易云风格黑底
  central->setStyleSheet("background: #161616;");
  auto *mainLayout = new QVBoxLayout(central);

  // 歌曲标题
  m_titleLabel = new QLabel("未在播放", central);
  m_titleLabel->setAlignment(Qt::AlignCenter);
  m_titleLabel->setStyleSheet(
      "font-size: 16px; font-weight: bold; margin: 8px; color: #e5e5e5;"
      "background: transparent;");
  mainLayout->addWidget(m_titleLabel);

  // 中间区域：左侧曲库列表 (1/3) + 右侧面板 (2/3)
  auto *middleLayout = new QHBoxLayout;

  // 左侧：曲库列表 + 清空按钮
  auto *leftPanel = new QWidget(central);
  auto *leftLayout = new QVBoxLayout(leftPanel);
  leftLayout->setContentsMargins(0, 0, 0, 0);

  // 搜索框
  m_searchBox = new QLineEdit(leftPanel);
  m_searchBox->setPlaceholderText("搜索歌曲...");
  m_searchBox->setClearButtonEnabled(true);
  m_searchBox->setStyleSheet(
      "QLineEdit { border-radius: 6px; padding: 4px 8px;"
      "border: 1px solid #444; color: #ccc; background: #1e1e1e; }"
      "QLineEdit:focus { border-color: #ec4141; }");
  leftLayout->addWidget(m_searchBox);

  m_playlistWidget = new QListWidget(leftPanel);
  m_playlistWidget->setMinimumWidth(180);
  m_playlistWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  m_playlistWidget->setStyleSheet(
      "QListWidget { border-radius: 8px; border: 1px solid #333;"
      " background: #1e1e1e; color: #c0c0c0; }"
      "QListWidget::item { padding: 2px 4px; }"
      "QListWidget::item:hover { background: #2a2a2a; }");
  leftLayout->addWidget(m_playlistWidget, 1);

  auto *clearBtn = new QPushButton("🗑 一键清空", leftPanel);
  clearBtn->setStyleSheet(
      "border-radius: 6px; padding: 4px;"
      "border: 1px solid #444; color: #aaa;"
      "background: #252525;");
  connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearAll);
  leftLayout->addWidget(clearBtn);

  middleLayout->addWidget(leftPanel, 1); // 占 1/3

  // 右侧面板：光碟 / 歌词双层切换
  m_rightPanel = new QWidget(central);
  m_rightPanel->setStyleSheet("background: #1e1e1e; border-radius: 8px;");
  auto *rightLayout = new QVBoxLayout(m_rightPanel);

  // 歌曲信息（标签元数据）
  m_infoArtist = new QLabel(m_rightPanel);
  m_infoArtist->setAlignment(Qt::AlignCenter);
  m_infoArtist->setStyleSheet("font-size: 13px; color: #ccc; background: transparent;");
  m_infoAlbum = new QLabel(m_rightPanel);
  m_infoAlbum->setAlignment(Qt::AlignCenter);
  m_infoAlbum->setStyleSheet("font-size: 12px; color: #888; background: transparent;");
  rightLayout->addWidget(m_infoArtist);
  rightLayout->addWidget(m_infoAlbum);

  // 双层容器
  m_stack = new QStackedWidget(m_rightPanel);
  m_stack->setStyleSheet("background: transparent;");

  // 第 0 页：光碟
  auto *discPage = new QWidget;
  auto *discPageLayout = new QVBoxLayout(discPage);
  discPageLayout->setContentsMargins(0, 0, 0, 0);
  discPageLayout->setAlignment(Qt::AlignCenter);
  m_disc = new DiscWidget(discPage);
  m_disc->setMinimumSize(240, 240);
  discPageLayout->addWidget(m_disc);
  m_stack->addWidget(discPage);

  // 第 1 页：歌词 + 偏移控制
  m_lyricPage = new QWidget;
  auto *lyricPageLayout = new QVBoxLayout(m_lyricPage);
  lyricPageLayout->setContentsMargins(0, 0, 0, 0);

  m_lyricWidget = new QListWidget(m_lyricPage);
  m_lyricWidget->setStyleSheet(
      "QListWidget { border: none; background: transparent; font-size: 13px;"
      " color: #999; }"
      "QListWidget::item { padding: 3px 6px; }");
  m_lyricWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_lyricWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  lyricPageLayout->addWidget(m_lyricWidget, 1);

  // 歌词偏移微调
  auto *lyricOffsetLayout = new QHBoxLayout;
  m_lyricMinusBtn = new QPushButton("−0.5s", m_lyricPage);
  m_lyricMinusBtn->setFixedSize(48, 26);
  m_lyricMinusBtn->setStyleSheet(
      "border-radius: 4px; font-size: 11px; color: #aaa;"
      "border: 1px solid #444; background: #2a2a2a;");
  m_lyricPlusBtn = new QPushButton("+0.5s", m_lyricPage);
  m_lyricPlusBtn->setFixedSize(48, 26);
  m_lyricPlusBtn->setStyleSheet(
      "border-radius: 4px; font-size: 11px; color: #aaa;"
      "border: 1px solid #444; background: #2a2a2a;");
  m_lyricOffsetLabel = new QLabel("0.0s", m_lyricPage);
  m_lyricOffsetLabel->setAlignment(Qt::AlignCenter);
  m_lyricOffsetLabel->setStyleSheet("font-size: 12px; color: #888; background: transparent;");
  lyricOffsetLayout->addWidget(m_lyricMinusBtn);
  lyricOffsetLayout->addStretch();
  lyricOffsetLayout->addWidget(m_lyricOffsetLabel);
  lyricOffsetLayout->addStretch();
  lyricOffsetLayout->addWidget(m_lyricPlusBtn);
  lyricPageLayout->addLayout(lyricOffsetLayout);

  // 歌词跳转按钮（实心红色右三角，挂在 lyricPage 上）
  m_lyricJumpBtn = new TriangleButton(m_lyricPage);
  m_lyricJumpBtn->setFixedSize(18, 14);
  m_lyricJumpBtn->setCursor(Qt::PointingHandCursor);
  m_lyricJumpBtn->setToolTip("跳转到此处播放");
  m_lyricJumpBtn->setStyleSheet("border: none; background: transparent;");
  m_lyricJumpBtn->hide();

  // 3 秒自动回弹定时器
  m_lyricReturnTimer = new QTimer(this);
  m_lyricReturnTimer->setSingleShot(true);
  m_lyricReturnTimer->setInterval(3000);

  // 200ms 吸附防抖定时器（拖拽松手后对齐中心行）
  m_lyricSnapTimer = new QTimer(this);
  m_lyricSnapTimer->setSingleShot(true);
  m_lyricSnapTimer->setInterval(200);

  m_stack->addWidget(m_lyricPage);
  rightLayout->addWidget(m_stack, 1);

  // 切换按钮（右下角）
  auto *toggleRow = new QHBoxLayout;
  toggleRow->addStretch();
  m_toggleBtn = new QPushButton("📝 歌词", m_rightPanel);
  m_toggleBtn->setStyleSheet(
      "border-radius: 6px; padding: 3px 8px; font-size: 11px;"
      "border: 1px solid #444; color: #aaa;"
      "background: #2a2a2a;");
  toggleRow->addWidget(m_toggleBtn);
  rightLayout->addLayout(toggleRow);

  middleLayout->addWidget(m_rightPanel, 3); // 占 3/4

  mainLayout->addLayout(middleLayout);

  // 进度条 + 时间
  auto *progressLayout = new QHBoxLayout;
  m_progressSlider = new QSlider(Qt::Horizontal, central);
  m_progressSlider->setEnabled(false);
  m_progressSlider->setStyleSheet(
      "QSlider::groove:horizontal { background: #333; height: 4px;"
      " border-radius: 2px; }"
      "QSlider::sub-page:horizontal { background: #ec4141;"
      " border-radius: 2px; }"
      "QSlider::handle:horizontal { background: #ec4141; width: 12px;"
      " margin: -4px 0; border-radius: 6px; }"
      "QSlider::handle:horizontal:hover { background: #ff5252; }");
  m_timeLabel = new QLabel("00:00 / 00:00", central);
  m_timeLabel->setStyleSheet("color: #999; background: transparent;"
                              " font-size: 12px;");
  progressLayout->addWidget(m_progressSlider, 1);
  progressLayout->addWidget(m_timeLabel);
  mainLayout->addLayout(progressLayout);

  // 控制栏
  auto *controlLayout = new QHBoxLayout;

  m_prevBtn = new QPushButton("⏮", central);
  m_playBtn = new QPushButton("▶", central);
  m_stopBtn = new QPushButton("⏹", central);
  m_nextBtn = new QPushButton("⏭", central);
  m_modeBtn = new QPushButton("🔁", central);

  for (auto *btn : {m_prevBtn, m_playBtn, m_stopBtn, m_nextBtn, m_modeBtn}) {
    btn->setFixedSize(44, 44);
    btn->setStyleSheet(
        "font-size: 18px;"
        "border-radius: 10px;"
        "border: 1px solid #444;"
        "color: #ccc;"
        "background: #2a2a2a;");
  }

  m_volumeSlider = new QSlider(Qt::Horizontal, central);
  m_volumeSlider->setRange(0, 100);
  m_volumeSlider->setValue(70);
  m_volumeSlider->setMaximumWidth(120);
  m_volumeSlider->setStyleSheet(
      "QSlider::groove:horizontal { background: #333; height: 3px;"
      " border-radius: 1px; }"
      "QSlider::sub-page:horizontal { background: #ec4141;"
      " border-radius: 1px; }"
      "QSlider::handle:horizontal { background: #ec4141; width: 10px;"
      " margin: -3px 0; border-radius: 5px; }");

  m_modeLabel = new QLabel("列表循环", central);
  m_modeLabel->setStyleSheet("color: #999; background: transparent;");

  auto *openBtn = new QPushButton("📂 打开文件", central);
  openBtn->setStyleSheet(
      "border-radius: 8px; padding: 4px 10px;"
      "border: 1px solid #444; color: #ccc;"
      "background: #2a2a2a;");
  connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpenFiles);

  auto *scanBtn = new QPushButton("📁 扫描文件夹", central);
  scanBtn->setStyleSheet(openBtn->styleSheet());
  connect(scanBtn, &QPushButton::clicked, this, &MainWindow::onScanFolder);

  auto *volIcon = new QLabel("🔊", central);
  volIcon->setStyleSheet("background: transparent;");

  controlLayout->addStretch();
  controlLayout->addWidget(m_prevBtn);
  controlLayout->addWidget(m_playBtn);
  controlLayout->addWidget(m_stopBtn);
  controlLayout->addWidget(m_nextBtn);
  controlLayout->addWidget(m_modeBtn);
  controlLayout->addWidget(m_modeLabel);
  controlLayout->addStretch();
  controlLayout->addWidget(volIcon);
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
  connect(m_lyricMinusBtn, &QPushButton::clicked, this,
          &MainWindow::onLyricOffsetMinus);
  connect(m_lyricPlusBtn, &QPushButton::clicked, this,
          &MainWindow::onLyricOffsetPlus);
  connect(m_toggleBtn, &QPushButton::clicked, this,
          &MainWindow::onTogglePanel);
  connect(m_searchBox, &QLineEdit::textChanged, this,
          &MainWindow::onSearchTextChanged);

  // 歌词拖动浏览
  connect(m_lyricWidget->verticalScrollBar(), &QScrollBar::valueChanged,
          this, &MainWindow::onLyricScrollChanged);
  connect(m_lyricJumpBtn, &QPushButton::clicked, this,
          &MainWindow::onLyricJump);

  // 中心行吸附（拖拽松手 / 滚动停止 200ms 后对齐）
  auto doSnap = [this]() {
    if (m_lyricAutoFollow || m_lrc->isEmpty()) return;
    int vw = m_lyricWidget->viewport()->width();
    int vh = m_lyricWidget->viewport()->height();
    QListWidgetItem *item = m_lyricWidget->itemAt(QPoint(vw / 2, vh / 2));
    if (item) {
      m_lyricCenterRow = m_lyricWidget->row(item);
      m_lyricProgramScroll = true;
      m_lyricWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }
  };
  connect(m_lyricWidget->verticalScrollBar(), &QScrollBar::sliderReleased,
          this, doSnap);
  connect(m_lyricSnapTimer, &QTimer::timeout, this, doSnap);

  // 3 秒自动回弹
  connect(m_lyricReturnTimer, &QTimer::timeout, this, [this]() {
    // 自动回弹：恢复跟随播放
    m_lyricAutoFollow = true;
    m_lyricJumpBtn->hide();
    m_lyricSnapTimer->stop();
    if (m_lyricLine >= 0 && m_lyricLine < m_lyricWidget->count()) {
      m_lyricProgramScroll = true;
      m_lyricWidget->scrollToItem(m_lyricWidget->item(m_lyricLine),
                                  QAbstractItemView::PositionAtCenter);
    }
  });

  // Player → UI
  connect(m_player, &Player::stateChanged, this,
          &MainWindow::refreshPlayPauseBtn);
  connect(m_player, &Player::positionChanged, this,
          &MainWindow::refreshTimeLabel);
  connect(m_player, &Player::durationChanged, this,
          &MainWindow::refreshProgressRange);
  connect(m_player, &Player::endOfMedia, this, &MainWindow::onTrackEnded);
  connect(m_player, &Player::errorOccurred, this, &MainWindow::onPlayerError);
  connect(m_player, &Player::metaDataReady, this, &MainWindow::onMetaDataReady);
  connect(m_player, &Player::positionChanged, this, &MainWindow::refreshLyric);

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

  // 去重添加
  QList<Track> fresh = m_playlist->addTracks(tracks);
  if (!fresh.isEmpty())
    applySearchFilter();

  // 保存曲库
  saveLibraryTracks();

  // 如果是第一批曲目，自动播放第一首
  if (m_player->state() == Player::Stopped &&
      m_playlist->count() == fresh.size()) {
    loadCurrentTrack();
    m_player->play();
  }
}

void MainWindow::onClearAll() {
  if (m_playlist->isEmpty()) return;

  m_player->stop();
  m_playlist->clear();
  m_playlistWidget->clear();
  m_highlightedRow = -1;
  m_titleLabel->setText("未在播放");
  m_lyricWidget->clear();
  m_lrc->clear();
  m_lyricLine = -1;

  saveLibraryTracks();
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
  // 清空旧标签信息，等待新歌曲元数据
  m_tagTitle.clear();
  m_infoArtist->setText(QString());
  m_infoAlbum->setText(QString());
  autoLoadLyric(t.filePath);
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
  m_tagTitle.clear();
  m_infoArtist->setText(QString());
  m_infoAlbum->setText(QString());
  autoLoadLyric(t.filePath);
}

void MainWindow::onPlaylistDoubleClicked(const QModelIndex &index) {
  int playlistIdx = m_playlistWidget->item(index.row())->data(Qt::UserRole).toInt();
  m_playlist->setCurrentIndex(playlistIdx);
  loadCurrentTrack();
  m_player->play();
}

void MainWindow::onPlaylistContextMenu(const QPoint &pos) {
  QListWidgetItem *item = m_playlistWidget->itemAt(pos);
  if (!item) return;

  int playlistIdx = item->data(Qt::UserRole).toInt();

  QMenu menu(this);
  QAction *delAction = menu.addAction("从列表中删除");
  QAction *transAction = menu.addAction("转码");

  QAction *chosen = menu.exec(m_playlistWidget->viewport()->mapToGlobal(pos));
  if (!chosen) return;

  if (chosen == delAction) {
    // ── 删除 ──
    if (playlistIdx == m_playlist->currentIndex()) {
      m_player->stop();
    }
    m_playlist->removeTrack(playlistIdx);
    if (playlistIdx == m_playlist->currentIndex()) {
      m_highlightedRow = -1;
    }
    applySearchFilter();
    saveLibraryTracks();

  } else if (chosen == transAction) {
    // ── 转码 ──
    const Track &track = m_playlist->trackAt(playlistIdx);
    QString srcName = QFileInfo(track.filePath).completeBaseName();

    // 选择保存路径和格式
    QString filter = "MP3 (*.mp3);;WAV (*.wav);;FLAC (*.flac);;AAC (*.aac);;OGG (*.ogg)";
    QString savePath = QFileDialog::getSaveFileName(this, "保存转码文件",
                                                     srcName, filter);
    if (savePath.isEmpty()) return;

    // 进度对话框
    auto *progress = new QProgressDialog("正在转码...", "取消", 0, 100, this);
    progress->setWindowTitle("转码进度");
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setValue(0);

    // 连接转码器信号
    connect(m_transcoder, &Transcoder::progressChanged,
            progress, &QProgressDialog::setValue);
    connect(progress, &QProgressDialog::canceled,
            m_transcoder, &Transcoder::cancel);
    connect(m_transcoder, &Transcoder::finished, this,
            [this, progress](bool ok, const QString &outPath) {
              progress->close();
              if (ok) {
                QMessageBox::information(this, "转码完成",
                    QString("文件已保存到：\n%1").arg(outPath));
              } else {
                QMessageBox::warning(this, "转码失败",
                    "ffmpeg 转码失败，请检查文件格式。");
              }
              progress->deleteLater();
            });

    // 启动转码
    if (!m_transcoder->start(track.filePath, savePath, track.duration)) {
      progress->close();
      progress->deleteLater();
      QMessageBox::warning(this, "转码失败",
          "无法启动 ffmpeg，请确认 ffmpeg.exe 存在于程序目录。");
    }
  }
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
    next = PlayMode::Repeat;
    label = "列表循环";
    icon = "🔁";
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
  if (!fresh.isEmpty())
    applySearchFilter();

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
  if (!fresh.isEmpty())
    applySearchFilter();
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

void MainWindow::refreshCurrentRow(int playlistIndex) {
  // 清除旧曲目的高亮
  if (m_highlightedRow >= 0 && m_highlightedRow < m_playlistWidget->count()) {
    auto *item = m_playlistWidget->item(m_highlightedRow);
    item->setBackground(QBrush());
    item->setForeground(QBrush());
    QFont f = item->font();
    f.setBold(false);
    item->setFont(f);
  }

  // 在过滤后的显示列表中查找对应的 display row
  int displayRow = -1;
  for (int i = 0; i < m_playlistWidget->count(); ++i) {
    if (m_playlistWidget->item(i)->data(Qt::UserRole).toInt() == playlistIndex) {
      displayRow = i;
      break;
    }
  }

  // 设置新曲目的高亮（网易云红字 + 浅暗红底）
  if (displayRow >= 0) {
    auto *item = m_playlistWidget->item(displayRow);
    item->setBackground(QColor("#2d2525"));
    item->setForeground(QColor("#ec4141"));
    QFont f = item->font();
    f.setBold(true);
    item->setFont(f);
    m_highlightedRow = displayRow;
  }

  m_playlistWidget->setCurrentRow(displayRow);
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

void MainWindow::onMetaDataReady(const QString &title, const QString &artist,
                                 const QString &album) {
  // 有标签标题则覆盖文件名
  if (!title.isEmpty()) {
    m_tagTitle = title;
    m_titleLabel->setText(title);
  } else if (!m_tagTitle.isEmpty()) {
    // 元数据回调可能多次触发，保留之前读到的标题
    return;
  }
  m_infoArtist->setText(artist.isEmpty() ? QString() : "🎤 " + artist);
  m_infoAlbum->setText(album.isEmpty() ? QString() : "💿 " + album);
}

// ─────────────────────── 辅助 ───────────────────────

void MainWindow::loadCurrentTrack() {
  const Track &t = m_playlist->currentTrack();
  if (t.filePath.isEmpty())
    return;
  m_player->setSource(t.filePath);
  m_titleLabel->setText(t.title);
  m_tagTitle.clear();
  m_infoArtist->setText(QString());
  m_infoAlbum->setText(QString());
  autoLoadLyric(t.filePath);
}

// ─────────────────────── 歌词 ───────────────────────

void MainWindow::autoLoadLyric(const QString &audioPath) {
  m_lyricWidget->clear();
  m_lrc->clear();
  m_lyricLine = -1;

  // 重置拖动浏览状态
  m_lyricAutoFollow = true;
  m_lyricJumpBtn->hide();
  m_lyricReturnTimer->stop();
  m_lyricSnapTimer->stop();

  QString base = QFileInfo(audioPath).completeBaseName();
  QString audioDir = QFileInfo(audioPath).absolutePath();

  // 1) 同目录直接替换扩展名（最快）
  QString direct = audioDir + "/" + base + ".lrc";
  if (!QFile::exists(direct))
    direct = audioDir + "/" + base + ".LRC";
  if (QFile::exists(direct)) {
    m_lrc->load(direct);
  } else {
    // 2) 递归搜索子目录
    QDirIterator it(audioDir, {"*.lrc", "*.LRC"}, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
      it.next();
      if (QFileInfo(it.filePath()).completeBaseName().compare(
              base, Qt::CaseInsensitive) == 0) {
        m_lrc->load(it.filePath());
        break;
      }
    }
  }

  if (m_lrc->isEmpty()) return;

  // 填入歌词列表
  for (const auto &line : m_lrc->lines())
    m_lyricWidget->addItem(line.text);

  // 底部占位行：确保末行歌词也能滚动到屏幕中央
  for (int i = 0; i < 10; ++i) {
    auto *pad = new QListWidgetItem();
    pad->setFlags(Qt::NoItemFlags); // 不可选中、不可交互
    m_lyricWidget->addItem(pad);
  }

  // 显示偏移量
  qint64 offset = m_playlist->currentTrack().lyricOffset;
  m_lyricOffsetLabel->setText(
      QString("%1%2s").arg(offset >= 0 ? "+" : "").arg(offset / 1000.0, 0, 'f', 1));
}

void MainWindow::refreshLyric(qint64 posMs) {
  if (m_lrc->isEmpty()) return;

  qint64 offset = m_playlist->currentTrack().lyricOffset;
  int line = m_lrc->lineAt(posMs, offset);
  if (line == m_lyricLine) return; // 没变化

  // 清除旧行的高亮
  if (m_lyricLine >= 0 && m_lyricLine < m_lyricWidget->count()) {
    auto *item = m_lyricWidget->item(m_lyricLine);
    item->setForeground(QBrush());
    QFont f = item->font();
    f.setBold(false);
    item->setFont(f);
  }

  // 设置新行的高亮（网易云红字）
  if (line >= 0 && line < m_lyricWidget->count()) {
    auto *item = m_lyricWidget->item(line);
    item->setForeground(QColor("#ec4141"));
    QFont f = item->font();
    f.setBold(true);
    item->setFont(f);
    // 仅在自动跟随模式下自动滚动
    if (m_lyricAutoFollow) {
      m_lyricProgramScroll = true;
      m_lyricWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }
  }

  m_lyricLine = line;
}

void MainWindow::onLyricOffsetPlus() {
  if (m_lyricWidget->count() == 0) return;

  auto &t = const_cast<Track &>(m_playlist->currentTrack());
  t.lyricOffset += 500; // +0.5s → ms
  m_lyricOffsetLabel->setText(
      QString("+%1s").arg(t.lyricOffset / 1000.0, 0, 'f', 1));
  saveLyricOffset();
}

void MainWindow::onLyricOffsetMinus() {
  if (m_lyricWidget->count() == 0) return;

  auto &t = const_cast<Track &>(m_playlist->currentTrack());
  t.lyricOffset -= 500; // -0.5s → ms
  m_lyricOffsetLabel->setText(
      QString("%1%2s")
          .arg(t.lyricOffset >= 0 ? "+" : "")
          .arg(t.lyricOffset / 1000.0, 0, 'f', 1));
  saveLyricOffset();
}

void MainWindow::saveLyricOffset() {
  // 当前 Track 已经更新，直接全量保存整个曲库
  saveLibraryTracks();
}

// ─────────────────────── 歌词拖动浏览 ───────────────────────

void MainWindow::onLyricScrollChanged(int value) {
  Q_UNUSED(value);
  // 程序自动滚动不触发浏览模式
  if (m_lyricProgramScroll) {
    m_lyricProgramScroll = false;
    return;
  }

  // 用户手动滚动 → 进入浏览模式
  if (m_lyricAutoFollow) {
    m_lyricAutoFollow = false;
  }

  // 找出视口正中央对应的歌词行
  int vw = m_lyricWidget->viewport()->width();
  int vh = m_lyricWidget->viewport()->height();
  QListWidgetItem *item = m_lyricWidget->itemAt(QPoint(vw / 2, vh / 2));
  if (item) {
    int row = m_lyricWidget->row(item);
    if (row != m_lyricCenterRow) {
      m_lyricCenterRow = row;
      // 坐标换算：按钮放在视口内 70% 宽度处，Y 对齐中央行
      QPoint lwPos = m_lyricWidget->pos();
      QPoint vpOff = m_lyricWidget->viewport()->pos();
      QRect r = m_lyricWidget->visualItemRect(item);
      int bx = lwPos.x() + vpOff.x() + static_cast<int>(vw * 0.70);
      int by = lwPos.y() + vpOff.y() + r.center().y() - m_lyricJumpBtn->height() / 2;
      m_lyricJumpBtn->move(bx, by);
      m_lyricJumpBtn->show();
      m_lyricJumpBtn->raise();
    }
  }
  // 重置定时器
  m_lyricSnapTimer->start();    // 滚动停止 200ms 后吸附中心行
  m_lyricReturnTimer->start();  // 3 秒后回弹
}

void MainWindow::onLyricJump() {
  if (m_lyricCenterRow < 0 || m_lyricCenterRow >= m_lrc->lines().size())
    return;

  qint64 timestamp = m_lrc->lines().at(m_lyricCenterRow).timestamp;
  qint64 offset = m_playlist->currentTrack().lyricOffset;
  m_player->setPosition(timestamp - offset);

  // 恢复自动跟随
  m_lyricAutoFollow = true;
  m_lyricJumpBtn->hide();
  m_lyricReturnTimer->stop();
  m_lyricSnapTimer->stop();
}

// ─────────────────────── 搜索过滤 ───────────────────────

void MainWindow::applySearchFilter() {
  m_playlistWidget->clear();
  QString kw = m_searchBox->text().trimmed();
  for (int i = 0; i < m_playlist->count(); ++i) {
    const Track &t = m_playlist->trackAt(i);
    if (kw.isEmpty() || t.title.contains(kw, Qt::CaseInsensitive)) {
      auto *item = new QListWidgetItem(t.title);
      item->setData(Qt::UserRole, i); // 存储 playlist 中的真实索引
      m_playlistWidget->addItem(item);
    }
  }
  refreshCurrentRow(m_playlist->currentIndex());
}

void MainWindow::onSearchTextChanged(const QString &text) {
  Q_UNUSED(text);
  applySearchFilter();
}

void MainWindow::onTogglePanel() {
  if (m_stack->currentIndex() == 0) {
    m_stack->setCurrentIndex(1);
    m_toggleBtn->setText("💿 唱片");
  } else {
    m_stack->setCurrentIndex(0);
    m_toggleBtn->setText("📝 歌词");
  }
}

QString MainWindow::formatTime(qint64 ms) {
  int secs = static_cast<int>(ms / 1000);
  return QString("%1:%2")
      .arg(secs / 60, 2, 10, QChar('0'))
      .arg(secs % 60, 2, 10, QChar('0'));
}
