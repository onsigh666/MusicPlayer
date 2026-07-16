#include "mainwindow.h"
#include "library.h"
#include "lrcparser.h"
#include "player.h"
#include "playlist.h"
#include "track.h"
#include "transcoder.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDirIterator>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <dwmapi.h>
#endif
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPolygonF>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSlider>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <limits>

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
    tri << QPointF(w * 0.15, h * 0.2) << QPointF(w * 0.85, h * 0.5)
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

  // 恢复上次主题设置
  QSettings settings;
  if (settings.value("theme", "dark").toString() == "light")
    m_theme = Theme::Light;

  setupUI();
  wireSignals();

  // 启动时恢复上次扫描的曲库
  loadLibraryTracks();

  // 恢复上次播放的曲目和进度
  {
    QSettings settings;
    QString lastPath = settings.value("lastTrack").toString();
    if (!lastPath.isEmpty()) {
      for (int i = 0; i < m_playlist->count(); ++i) {
        if (m_playlist->trackAt(i).filePath == lastPath) {
          qint64 pos = settings.value("lastPosition", 0).toLongLong();
          if (pos > 0)
            m_restorePosition = pos; // 必须在 loadCurrentTrack 之前赋值
          m_playlist->setCurrentIndex(i);
          loadCurrentTrack();
          // 重试定时器：Qt6 WMF 后端在 setSource() 后可能需要一段时间
          // 才能接受 setPosition()，每 150ms 重试，最多 20 次（3 秒）
          if (m_restorePosition >= 0) {
            auto *retryTimer = new QTimer(this);
            retryTimer->setInterval(150);
            auto *attempts = new int(0);
            connect(retryTimer, &QTimer::timeout, this,
                    [this, retryTimer, attempts]() {
                      ++(*attempts);
                      if (m_player->duration() > 0) {
                        qint64 target =
                            qMin(m_restorePosition, m_player->duration());
                        m_player->setPosition(target);
                        // 验证 seek 是否生效（位置误差 < 500ms）
                        if (std::abs(m_player->position() - target) < 500) {
                          m_restorePosition = -1;
                          retryTimer->stop();
                          retryTimer->deleteLater();
                          delete attempts;
                          return;
                        }
                      }
                      if (*attempts >= 20) {
                        m_restorePosition = -1;
                        retryTimer->stop();
                        retryTimer->deleteLater();
                        delete attempts;
                      }
                    });
            retryTimer->start();
          }
          break;
        }
      }
    }
    int vol = settings.value("lastVolume", -1).toInt();
    if (vol >= 0) {
      m_volumeSlider->setValue(vol);
      m_player->setVolume(vol / 100.0f);
    }
  }
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event) {
  const Track &t = m_playlist->currentTrack();
  if (!t.filePath.isEmpty()) {
    QSettings settings;
    settings.setValue("lastTrack", t.filePath);
    settings.setValue("lastPosition", m_player->position());
    settings.setValue("lastVolume", m_volumeSlider->value());
  }
  QMainWindow::closeEvent(event);
}

// ─────────────────────── UI 构建 ───────────────────────

void MainWindow::setupUI() {
  m_central = new QWidget(this);
  auto *mainLayout = new QVBoxLayout(m_central);

  // 歌曲标题
  m_titleLabel = new QLabel("未在播放", m_central);
  m_titleLabel->setAlignment(Qt::AlignCenter);
  m_titleLabel->setStyleSheet(
      "font-size: 16px; font-weight: bold; margin: 8px; color: #f0f0f0;");
  mainLayout->addWidget(m_titleLabel);

  // 中间区域：左侧曲库列表 (1/3) + 右侧面板 (2/3)
  auto *middleLayout = new QHBoxLayout;

  // 左侧：曲库列表 + 清空按钮
  auto *leftPanel = new QWidget(m_central);
  auto *leftLayout = new QVBoxLayout(leftPanel);
  leftLayout->setContentsMargins(0, 0, 0, 0);

  // 搜索框
  m_searchBox = new QLineEdit(leftPanel);
  m_searchBox->setPlaceholderText("搜索歌曲...");
  m_searchBox->setClearButtonEnabled(true);
  leftLayout->addWidget(m_searchBox);

  m_playlistWidget = new QListWidget(leftPanel);
  m_playlistWidget->setMinimumWidth(180);
  m_playlistWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  leftLayout->addWidget(m_playlistWidget, 1);

  auto *clearBtn = new QPushButton("清空列表", leftPanel);
  connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearAll);
  leftLayout->addWidget(clearBtn);

  middleLayout->addWidget(leftPanel, 1); // 占 1/3

  // 右侧面板：光碟 / 歌词双层切换
  m_rightPanel = new QWidget(m_central);
  m_rightPanel->setStyleSheet(
      "background: #1e1e1e; border: 1px solid #ec4141; border-radius: 10px;");
  auto *rightLayout = new QVBoxLayout(m_rightPanel);

  // 歌曲信息（标签元数据）
  m_infoArtist = new QLabel(m_rightPanel);
  m_infoArtist->setAlignment(Qt::AlignCenter);
  m_infoArtist->setStyleSheet("font-size: 13px; color: #e0e0e0;");
  m_infoAlbum = new QLabel(m_rightPanel);
  m_infoAlbum->setAlignment(Qt::AlignCenter);
  m_infoAlbum->setStyleSheet("font-size: 12px; color: #888;");
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
      "QListWidget { border: none; background: transparent;"
      " font-size: 13px; color: #999; }"
      "QListWidget::item { padding: 3px 6px; border-radius: 6px; }");
  m_lyricWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_lyricWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  lyricPageLayout->addWidget(m_lyricWidget, 1);

  // 歌词偏移微调
  auto *lyricOffsetLayout = new QHBoxLayout;
  m_lyricMinusBtn = new QPushButton("-0.5s", m_lyricPage);
  m_lyricMinusBtn->setFixedSize(60, 28);
  m_lyricPlusBtn = new QPushButton("+0.5s", m_lyricPage);
  m_lyricPlusBtn->setFixedSize(60, 28);
  m_lyricOffsetLabel = new QLabel("0.0s", m_lyricPage);
  m_lyricOffsetLabel->setAlignment(Qt::AlignCenter);
  m_lyricOffsetLabel->setStyleSheet("color: #888; font-size: 12px;");
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
  m_toggleBtn = new QPushButton("歌词", m_rightPanel);
  toggleRow->addWidget(m_toggleBtn);
  rightLayout->addLayout(toggleRow);

  middleLayout->addWidget(m_rightPanel, 3); // 占 3/4

  mainLayout->addLayout(middleLayout);

  // 进度条 + 时间
  auto *progressLayout = new QHBoxLayout;
  m_progressSlider = new QSlider(Qt::Horizontal, m_central);
  m_progressSlider->setEnabled(false);
  m_timeLabel = new QLabel("00:00 / 00:00", m_central);
  m_timeLabel->setStyleSheet("color: #888; font-size: 12px;");
  progressLayout->addWidget(m_progressSlider, 1);
  progressLayout->addWidget(m_timeLabel);
  mainLayout->addLayout(progressLayout);

  // 控制栏
  auto *controlLayout = new QHBoxLayout;

  // 用 Segoe UI Symbol 字体 — 文字符号而非 emoji，CSS 颜色才生效
  m_prevBtn = new QPushButton("⏮", m_central);
  m_playBtn = new QPushButton("▶", m_central);
  m_stopBtn = new QPushButton("⏹", m_central);
  m_nextBtn = new QPushButton("⏭", m_central);
  m_modeBtn = new QPushButton("↻", m_central);

  // 播放按钮 — 红色圆形
  m_playBtn->setFixedSize(48, 48);
  m_playBtn->setStyleSheet(
      "QPushButton { font-family: \"Segoe UI Symbol\"; font-size: 20px;"
      "border-radius: 24px; color: #fff; border: none; background: #ec4141; }"
      "QPushButton:hover { background: #ff5252; }"
      "QPushButton:pressed { background: #c62828; }");
  m_playBtn->setCursor(Qt::PointingHandCursor);

  // 上下首、停止 — 透明底
  for (auto *btn : {m_prevBtn, m_stopBtn, m_nextBtn}) {
    btn->setFixedSize(40, 40);
    btn->setStyleSheet(
        "QPushButton { font-family: \"Segoe UI Symbol\"; font-size: 16px;"
        "border-radius: 20px; color: #ec4141; border: none;"
        "background: transparent; }"
        "QPushButton:hover { color: #ff5252;"
        "background: rgba(236,65,65,0.10); }");
  }

  // 模式按钮 — 更小巧
  m_modeBtn->setFixedSize(36, 36);
  m_modeBtn->setStyleSheet(
      "QPushButton { font-family: \"Segoe UI Symbol\"; font-size: 15px;"
      "border-radius: 18px; color: #888; border: none;"
      "background: transparent; }"
      "QPushButton:hover { color: #ccc; background: rgba(255,255,255,0.06); }");

  m_volumeSlider = new QSlider(Qt::Horizontal, m_central);
  m_volumeSlider->setRange(0, 100);
  m_volumeSlider->setValue(70);
  m_volumeSlider->setMaximumWidth(120);

  m_modeLabel = new QLabel("列表循环", m_central);
  m_modeLabel->setStyleSheet("color: #888;");

  auto *openBtn = new QPushButton("打开文件", m_central);
  connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpenFiles);

  auto *scanBtn = new QPushButton("扫描文件夹", m_central);
  connect(scanBtn, &QPushButton::clicked, this, &MainWindow::onScanFolder);

  auto *volIcon = new QLabel("音量", m_central);

  // 换肤按钮（左下角）
  m_themeBtn = new QPushButton(m_central);
  connect(m_themeBtn, &QPushButton::clicked, this, &MainWindow::onToggleTheme);

  controlLayout->addWidget(m_themeBtn);
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

  setCentralWidget(m_central);
  applyTheme(); // 初次应用默认深色主题
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
  connect(m_toggleBtn, &QPushButton::clicked, this, &MainWindow::onTogglePanel);
  connect(m_searchBox, &QLineEdit::textChanged, this,
          &MainWindow::onSearchTextChanged);

  // 歌词拖动浏览
  connect(m_lyricWidget->verticalScrollBar(), &QScrollBar::valueChanged, this,
          &MainWindow::onLyricScrollChanged);
  connect(m_lyricJumpBtn, &QPushButton::clicked, this,
          &MainWindow::onLyricJump);

  // 中心行吸附（拖拽松手 / 滚动停止 200ms 后对齐）
  auto doSnap = [this]() {
    if (m_lyricAutoFollow || m_lrc->isEmpty())
      return;
    int vw = m_lyricWidget->viewport()->width();
    int vh = m_lyricWidget->viewport()->height();
    QListWidgetItem *item = m_lyricWidget->itemAt(QPoint(vw / 2, vh / 2));
    if (item) {
      m_lyricCenterRow = m_lyricWidget->row(item);
      m_lyricProgramScroll = true;
      m_lyricWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }
  };
  connect(m_lyricWidget->verticalScrollBar(), &QScrollBar::sliderReleased, this,
          doSnap);
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
  if (m_playlist->isEmpty())
    return;

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
  int playlistIdx =
      m_playlistWidget->item(index.row())->data(Qt::UserRole).toInt();
  m_playlist->setCurrentIndex(playlistIdx);
  loadCurrentTrack();
  m_player->play();
}

void MainWindow::onPlaylistContextMenu(const QPoint &pos) {
  QListWidgetItem *item = m_playlistWidget->itemAt(pos);
  if (!item)
    return;

  int playlistIdx = item->data(Qt::UserRole).toInt();

  QMenu menu(this);
  QAction *delAction = menu.addAction("从列表中删除");
  QAction *transAction = menu.addAction("转码");

  QAction *chosen = menu.exec(m_playlistWidget->viewport()->mapToGlobal(pos));
  if (!chosen)
    return;

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
    QString filter =
        "MP3 (*.mp3);;WAV (*.wav);;FLAC (*.flac);;AAC (*.aac);;OGG (*.ogg)";
    QString savePath =
        QFileDialog::getSaveFileName(this, "保存转码文件", srcName, filter);
    if (savePath.isEmpty())
      return;

    // 进度对话框
    auto *progress = new QProgressDialog("正在转码...", "取消", 0, 100, this);
    progress->setWindowTitle("转码进度");
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setValue(0);

    // 连接转码器信号
    connect(m_transcoder, &Transcoder::progressChanged, progress,
            &QProgressDialog::setValue);
    connect(progress, &QProgressDialog::canceled, m_transcoder,
            &Transcoder::cancel);
    connect(m_transcoder, &Transcoder::finished, this,
            [this, progress](bool ok, const QString &outPath) {
              progress->close();
              if (ok) {
                QMessageBox::information(
                    this, "转码完成",
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
      QMessageBox::warning(
          this, "转码失败",
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
    icon = "⇄";
    break;
  case PlayMode::Shuffle:
    next = PlayMode::RepeatOne;
    label = "单曲循环";
    icon = "↺";
    break;
  case PlayMode::RepeatOne:
    next = PlayMode::Repeat;
    label = "列表循环";
    icon = "↻";
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
  // 启动时恢复上次播放位置（首次尝试，重试定时器兜底）
  if (m_restorePosition >= 0 && durMs > 0) {
    m_player->setPosition(qMin(m_restorePosition, durMs));
    // 不在此清除 m_restorePosition，由重试定时器验证并清除
  }
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
    if (m_playlistWidget->item(i)->data(Qt::UserRole).toInt() ==
        playlistIndex) {
      displayRow = i;
      break;
    }
  }

  // 设置新曲目的高亮（网易云红字 + 浅暗红底）
  if (displayRow >= 0) {
    auto *item = m_playlistWidget->item(displayRow);
    item->setBackground(m_highlightBg);
    item->setForeground(m_highlightFg);
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
      if (QFileInfo(it.filePath())
              .completeBaseName()
              .compare(base, Qt::CaseInsensitive) == 0) {
        m_lrc->load(it.filePath());
        break;
      }
    }
  }

  if (m_lrc->isEmpty())
    return;

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
  m_lyricOffsetLabel->setText(QString("%1%2s")
                                  .arg(offset >= 0 ? "+" : "")
                                  .arg(offset / 1000.0, 0, 'f', 1));
}

void MainWindow::refreshLyric(qint64 posMs) {
  if (m_lrc->isEmpty())
    return;

  qint64 offset = m_playlist->currentTrack().lyricOffset;
  int line = m_lrc->lineAt(posMs, offset);
  if (line == m_lyricLine)
    return; // 没变化

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
  if (m_lyricWidget->count() == 0)
    return;

  auto &t = const_cast<Track &>(m_playlist->currentTrack());
  t.lyricOffset += 500; // +0.5s → ms
  m_lyricOffsetLabel->setText(
      QString("%1%2s")
          .arg(t.lyricOffset >= 0 ? "+" : "")
          .arg(t.lyricOffset / 1000.0, 0, 'f', 1));
  saveLyricOffset();
}

void MainWindow::onLyricOffsetMinus() {
  if (m_lyricWidget->count() == 0)
    return;

  auto &t = const_cast<Track &>(m_playlist->currentTrack());
  t.lyricOffset -= 500; // -0.5s → ms
  m_lyricOffsetLabel->setText(QString("%1%2s")
                                  .arg(t.lyricOffset >= 0 ? "+" : "")
                                  .arg(t.lyricOffset / 1000.0, 0, 'f', 1));
  saveLyricOffset();
}

void MainWindow::saveLyricOffset() {
  // 当前 Track 已经更新，直接全量保存整个曲库
  saveLibraryTracks();
}

// ─────────────────────── 换肤 ───────────────────────

void MainWindow::applyTheme() {
  bool dark = (m_theme == Theme::Dark);

  // ── 颜色变量 ──
  QString winBg = dark ? "#121212" : "#f0f0f0";
  QString ctrlBg = dark ? "#282828" : "#e0e0e0";
  QString ctrlHov = dark ? "#383838" : "#d0d0d0";
  QString ctrlPrs = dark ? "#1e1e1e" : "#c0c0c0";
  QString inputBg = dark ? "#1c1c1c" : "#ffffff";
  QString txtPri = dark ? "#e0e0e0" : "#333333";
  QString txtSec = dark ? "#b0b0b0" : "#555555";
  QString txtMut = dark ? "#888888" : "#999999";
  QString titleClr = dark ? "#f0f0f0" : "#222222";
  QString listBg = dark ? "#1c1c1c" : "#ffffff";
  QString listHov = dark ? "#2a2a2a" : "#eeeeee";
  QString listSel = dark ? "#333333" : "#e0e0e0";
  QString rpBg = dark ? "#1e1e1e" : "#ffffff";
  QString sliderBg = dark ? "#3a3a3a" : "#d0d0d0";
  QString progBg = dark ? "#333333" : "#e0e0e0";
  QString lyricClr = dark ? "#999999" : "#777777";
  QString menuBg = dark ? "#1e1e1e" : "#ffffff";
  QString menuSep = dark ? "#3a3a3a" : "#e0e0e0";
  QString tooltipBg = dark ? "#2a2a2a" : "#ffffff";
  QString msgBg = dark ? "#1e1e1e" : "#ffffff";
  QString msgBtnHov = dark ? "#383838" : "#e0e0e0";

  // ── 主窗口 ──
  setStyleSheet(QString("QMainWindow { background: %1; }").arg(winBg));

#ifdef Q_OS_WIN
  // 标题栏深色/浅色模式
  {
    HWND hwnd = reinterpret_cast<HWND>(winId());
    const int DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    BOOL useDark = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                          &useDark, sizeof(useDark));
  }
#endif

  // ── Central 控件样式表 ──
  m_central->setStyleSheet(QStringLiteral(
      "QWidget { background: %1; }"
      "QPushButton { border: 1px solid #ec4141; border-radius: 10px;"
      " padding: 5px 12px; background: %2; color: %3; font-size: 13px; }"
      "QPushButton:hover { background: %4; }"
      "QPushButton:pressed { background: %5; }"
      "QPushButton:focus { outline: none; }"
      "QLineEdit { border: 1px solid #ec4141; border-radius: 10px;"
      " padding: 6px 10px; background: %6; color: %3; font-size: 13px;"
      " selection-background-color: #ec4141; }"
      "QLineEdit:focus { border-color: #ff5252; }"
      "QListWidget { background: %7; border: 1px solid #ec4141;"
      " border-radius: 10px; padding: 4px; outline: none; color: %8; }"
      "QListWidget::item { border-radius: 6px; padding: 5px 8px; }"
      "QListWidget::item:hover { background: %9; }"
      "QListWidget::item:selected { background: %10; }"
      "QLabel { background: transparent; color: %8; }"
      "QSlider::groove:horizontal { background: %11; height: 6px;"
      " border-radius: 3px; border: 1px solid #ec4141; }"
      "QSlider::sub-page:horizontal { background: #ec4141; border-radius: 3px; }"
      "QSlider::handle:horizontal { background: #ec4141; width: 14px;"
      " height: 14px; margin: -5px 0; border-radius: 7px;"
      " border: 1px solid #cc0000; }"
      "QSlider::handle:horizontal:hover { background: #ff5252; }"
      "QScrollBar:vertical { background: transparent; width: 6px; margin: 0; }"
      "QScrollBar::handle:vertical { background: #ec4141; border-radius: 3px;"
      " min-height: 30px; }"
      "QScrollBar::handle:vertical:hover { background: #ff5252; }"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
      "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
      " background: transparent; }"
      "QScrollBar:horizontal { height: 0; }"
      "QProgressBar { border: 1px solid #ec4141; border-radius: 6px;"
      " background: %12; height: 8px; text-align: center; }"
      "QProgressBar::chunk { background: #ec4141; border-radius: 4px; }")
      .arg(winBg, ctrlBg, txtPri, ctrlHov, ctrlPrs, inputBg,
           listBg, txtSec, listHov, listSel, sliderBg, progBg));

  // ── 内联控件 ──
  m_titleLabel->setStyleSheet(
      QString("font-size: 16px; font-weight: bold; margin: 8px; color: %1;")
          .arg(titleClr));
  m_rightPanel->setStyleSheet(
      QString("background: %1; border: 1px solid #ec4141; border-radius: 10px;")
          .arg(rpBg));
  m_infoArtist->setStyleSheet(
      QString("font-size: 13px; color: %1;").arg(txtPri));
  m_infoAlbum->setStyleSheet(
      QString("font-size: 12px; color: %1;").arg(txtMut));
  m_lyricWidget->setStyleSheet(
      QString("QListWidget { border: none; background: transparent;"
              " font-size: 13px; color: %1; }"
              "QListWidget::item { padding: 3px 6px; border-radius: 6px; }")
          .arg(lyricClr));
  m_lyricOffsetLabel->setStyleSheet(
      QString("color: %1; font-size: 12px;").arg(txtMut));
  m_timeLabel->setStyleSheet(
      QString("color: %1; font-size: 12px;").arg(txtMut));
  m_modeLabel->setStyleSheet(QString("color: %1;").arg(txtMut));

  // ── 导航按钮（红字透明底，颜色不变） ──
  for (auto *btn : {m_prevBtn, m_stopBtn, m_nextBtn}) {
    btn->setStyleSheet(
        "QPushButton { font-family: \"Segoe UI Symbol\"; font-size: 16px;"
        "border-radius: 20px; color: #ec4141; border: none;"
        "background: transparent; }"
        "QPushButton:hover { color: #ff5252; background: rgba(236,65,65,0.10); }");
  }

  // ── 模式按钮 ──
  m_modeBtn->setStyleSheet(
      QString("QPushButton { font-family: \"Segoe UI Symbol\"; font-size: 15px;"
              "border-radius: 18px; color: %1; border: none;"
              "background: transparent; }"
              "QPushButton:hover { color: %2; background: %3; }")
          .arg(dark ? "#888" : "#aaa", dark ? "#ccc" : "#666",
               dark ? "rgba(255,255,255,0.06)" : "rgba(0,0,0,0.06)"));

  // ── 高亮颜色 ──
  m_highlightBg = dark ? QColor(0x2d, 0x25, 0x25) : QColor(0xfc, 0xe8, 0xe8);
  m_highlightFg = QColor(0xec, 0x41, 0x41); // 红色不变

  // ── 全局 QSS（菜单、提示、对话框） ──
  qApp->setStyleSheet(QStringLiteral(
      "* { font-family: \"Microsoft YaHei\", \"PingFang SC\","
      " \"Segoe UI\", sans-serif; }"
      "QToolTip { background: %1; color: %2; border: 1px solid #ec4141;"
      " border-radius: 8px; padding: 4px 8px; font-size: 12px; }"
      "QMenu { background: %3; border: 1px solid #ec4141; border-radius: 10px;"
      " padding: 6px; color: %2; font-size: 13px; }"
      "QMenu::item { padding: 6px 28px; border-radius: 6px; }"
      "QMenu::item:selected { background: #ec4141; color: #fff; }"
      "QMenu::separator { height: 1px; background: %4; margin: 4px 10px; }"
      "QMessageBox { background: %5; }"
      "QMessageBox QLabel { color: %2; font-size: 13px; }"
      "QMessageBox QPushButton { min-width: 80px; min-height: 28px;"
      " border: 1px solid #ec4141; border-radius: 8px;"
      " background: %6; color: %2; }"
      "QMessageBox QPushButton:hover { background: %7; }"
      "QProgressDialog { background: %5; color: %2; }"
      "QProgressDialog QLabel { color: %2; font-size: 13px; }")
      .arg(tooltipBg, txtPri, menuBg, menuSep, msgBg, ctrlBg, msgBtnHov));

  // ── 光碟 ──
  m_disc->setDarkMode(dark);

  // ── 换肤按钮文字 ──
  m_themeBtn->setText(dark ? "切换浅色模式" : "切换深色模式");

  // ── 刷新当前播放高亮 ──
  refreshCurrentRow(m_playlist->currentIndex());
}

void MainWindow::onToggleTheme() {
  m_theme = (m_theme == Theme::Dark) ? Theme::Light : Theme::Dark;
  QSettings settings;
  settings.setValue("theme", m_theme == Theme::Dark ? "dark" : "light");
  applyTheme();
}

// ─────────────────────── 歌词拖动浏览 ───────────────────────

void MainWindow::onLyricScrollChanged(int value) {
  Q_UNUSED(value);
  if (m_lyricProgramScroll) {
    m_lyricProgramScroll = false;
    return;
  }

  // 用户手动滚动 → 进入浏览模式，显示固定位置的跳转按钮
  if (m_lyricAutoFollow) {
    m_lyricAutoFollow = false;
    // 按钮固定在歌词区域右侧、垂直居中，位置不随滚动变化
    int bx = m_lyricPage->width() - m_lyricJumpBtn->width() - 12;
    int by = (m_lyricWidget->pos().y() + m_lyricWidget->height() / 2)
             - m_lyricJumpBtn->height() / 2;
    m_lyricJumpBtn->move(bx, by);
    m_lyricJumpBtn->show();
    m_lyricJumpBtn->raise();
  }

  // 跟踪视口中心行（供 snap 吸附使用）
  int vw = m_lyricWidget->viewport()->width();
  int vh = m_lyricWidget->viewport()->height();
  QListWidgetItem *item = m_lyricWidget->itemAt(QPoint(vw / 2, vh / 2));
  if (item)
    m_lyricCenterRow = m_lyricWidget->row(item);

  m_lyricSnapTimer->start();
  m_lyricReturnTimer->start();
}

void MainWindow::onLyricJump() {
  // 将按钮中心点映射到歌词 viewport 坐标系，找距离最近的歌词行
  QPoint btnCenter = m_lyricJumpBtn->geometry().center();
  QPoint vpPt = m_lyricWidget->viewport()->mapFrom(m_lyricPage, btnCenter);

  int closestRow = -1;
  int closestDist = std::numeric_limits<int>::max();
  for (int i = 0; i < m_lyricWidget->count(); ++i) {
    QRect r = m_lyricWidget->visualItemRect(m_lyricWidget->item(i));
    if (!r.isValid()) continue;
    int dist = std::abs(r.center().y() - vpPt.y());
    if (dist < closestDist) {
      closestDist = dist;
      closestRow = i;
    }
  }

  if (closestRow < 0 || closestRow >= m_lrc->lines().size())
    return;

  qint64 timestamp = m_lrc->lines().at(closestRow).timestamp;
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
    m_toggleBtn->setText("唱片");
  } else {
    m_stack->setCurrentIndex(0);
    m_toggleBtn->setText("歌词");
  }
}

QString MainWindow::formatTime(qint64 ms) {
  int secs = static_cast<int>(ms / 1000);
  return QString("%1:%2")
      .arg(secs / 60, 2, 10, QChar('0'))
      .arg(secs % 60, 2, 10, QChar('0'));
}
