# My Music Player

基于 Qt 6 / C++17 的桌面音乐播放器。支持播放列表持久化、歌词同步与交互、频谱可视化、多种播放模式、转码、深色/浅色主题切换。

## 编译

### 依赖

- **Qt 6**（Core, Gui, Widgets, Multimedia, MultimediaWidgets）
- **CMake** ≥ 3.19
- **MinGW**（推荐 Qt 官方安装包自带的 MinGW。如果使用 MSVC，将下面命令中的 `-G "MinGW Makefiles"` 替换为 `-G "Visual Studio 2022"` 等对应版本即可）

### 步骤

```bash
# 进入项目目录
cd music-player

# 创建并进入构建目录
mkdir build && cd build

# CMake 配置（注意将路径替换为你自己的 Qt 安装路径）
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="D:/Qt/6.11.1/mingw_64"

# 编译
cmake --build .
```

编译完成后，可执行文件位于 `build/MusicPlayer.exe`。

### 可能遇到的编译问题

| 问题 | 解决办法 |
|------|----------|
| `Could NOT find Qt6` | `CMAKE_PREFIX_PATH` 指向的路径不正确，确认 Qt 实际安装位置 |
| `fatal error: QAudioDecoder: No such file or directory` | 检查 CMake 是否链接了 `Qt::Multimedia` |
| `undefined reference to ...` | 清理 build 目录后重新 cmake + cmake --build |
| 中文乱码 | 确保源文件以 UTF-8 编码保存 |

## 运行

双击 `build/MusicPlayer.exe` 即可启动。

首次启动时播放列表为空，可通过以下方式添加歌曲：
- **扫描文件夹**：点击左下角"扫描文件夹"按钮，选择音乐目录，程序会递归扫描其中所有 `.mp3` 文件
- **打开文件**：点击"打开文件"按钮，手动选择单个或多个文件

程序关闭时自动保存播放列表，下次启动自动恢复。

### ffmpeg（转码功能用）

转码功能依赖 `ffmpeg.exe`。如果不使用转码，可以不准备它，其他功能不受影响。

- **获取**：从 [ffmpeg.org](https://ffmpeg.org/download.html) 下载 Windows 构建版本。推荐选择 `ffmpeg-release-full.7z` 或 `ffmpeg-master-latest-win64-gpl.zip`
- **放置**：将 `ffmpeg.exe` 放在以下任一路径（程序按优先级依次搜索）：
  1. 与 `MusicPlayer.exe` 同目录
  2. 上级目录（`../ffmpeg.exe`）
  3. 上上级目录（`../../ffmpeg.exe`）
  4. 系统 `PATH` 环境变量中

## 使用说明

### 主界面布局

```
┌──────────────────────────────────────────────────┐
│                  歌曲标题栏                        │
├────────────┬─────────────────────────────────────┤
│            │                                     │
│  搜索框    │       歌手 / 专辑信息               │
│            │                                     │
│  播放列表  │    ┌─────────────────────┐          │
│  (曲库)    │    │  黑胶光碟 或 歌词    │          │
│            │    │                     │          │
│            │    └─────────────────────┘          │
│            │       [歌词] 切换按钮               │
│  清空列表  │                                     │
├────────────┴─────────────────────────────────────┤
│           ▎▎▎▎▎▎▎▎▎ 频谱柱 (16根) ▎▎▎▎▎        │
│  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 进度条 ━━━━    │
│   00:01 / 04:05                                  │
│  [换肤]  ←⏮ ▶ ⏹ ⏭→  ↻ 列表循环  音量 [===]   │
│               [扫描][打开]                      │
└──────────────────────────────────────────────────┘
```

### 基本操作

- **播放/暂停**：点击 ▶ / ⏸ 按钮
- **切歌**：点击 ⏮（上一首）/ ⏭（下一首）
- **停止**：点击 ⏹
- **进度跳转**：拖动进度条滑块
- **音量**：拖动音量滑块
- **从列表选歌**：双击列表中任意歌曲
- **删除歌曲**：右键歌曲 → "从列表中删除"

### 切换播放模式

点击 ↻ / ⇄ / ↺ 按钮，三种模式循环切换：
- **列表循环 (↻)**：默认模式，播完最后一首回到第一首继续
- **随机播放 (⇄)**：随机从列表中选一首
- **单曲循环 (↺)**：播完自动重头播放当前歌曲

### 歌词操作

- **自动加载**：播放歌曲时自动搜索同名 `.lrc` 文件（先搜同目录，再递归搜索子目录）
- **时间微调**：点击 `-0.5s` / `+0.5s` 按钮整体偏移歌词时间，偏移量会记住
- **拖动浏览**：在歌词区域上下拖动可翻看前后歌词，此时暂停自动跟随
- **跳转播放**：拖动浏览时，屏幕中间行右侧会出现红色跳转按钮，点击跳转到该行时间播放
- **自动回弹**：拖动后 3 秒内未点击跳转按钮，自动滚回当前播放行，恢复跟随

### 主题切换

点击左下角"切换浅色模式" / "切换深色模式"按钮切换主题，下次启动会记住上次选择。

### 转码

1. 在播放列表右键点击歌曲
2. 选择"转码"
3. 选择输出格式（MP3 / WAV / FLAC / AAC / OGG）和保存路径
4. 等待进度条走完，转码完成后会弹出提示

### 其他

- **搜索过滤**：在搜索框输入关键字，播放列表实时过滤，仅显示标题匹配的歌曲
- **光碟/歌词切换**：点击右下角"歌词" / "唱片"按钮，切换右侧面板显示内容

## 文件结构

```
music-player/
├── CMakeLists.txt              # CMake 构建配置
├── README.md                   # 本文件
└── src/
    ├── main.cpp                # 程序入口
    ├── mainwindow.h / .cpp     # 主窗口：UI 布局 + 信号接线
    ├── player.h / .cpp         # QMediaPlayer 封装
    ├── playlist.h / .cpp       # 播放列表管理
    ├── playmodestrategy.h / .cpp # 播放模式策略（多态核心）
    ├── track.h                 # Track 数据结构
    ├── library.h / .cpp        # 曲库 JSON 持久化
    ├── lrcparser.h / .cpp      # .lrc 歌词解析
    ├── discwidget.h / .cpp     # 黑胶光碟控件
    ├── transcoder.h / .cpp     # ffmpeg 异步转码
    ├── audioanalyzer.h / .cpp  # 频谱分析：QAudioDecoder + FFT
    └── spectrumwidget.h / .cpp # 频谱柱可视化控件
```

## 必做功能

| 功能 | 说明 | 验收要点 |
|------|------|----------|
| 扫描歌曲 + 播放列表持久化 | 选择文件夹递归扫描 `.mp3`，保存到 JSON 文件 | 关闭程序再打开列表还在；添加新文件夹，新歌能并进来 |
| 基本播放控制 | 播放 / 暂停 / 停止 / 上一首 / 下一首 / 音量调节 | 各按钮功能正常 |
| 进度条 + 时间显示 | 进度条随播放前进、可拖动 seek；显示 `01:23 / 04:05` | 拖动进度条能跳转；时间显示正确 |
| 歌曲信息 | 读取 ID3 标签（标题、歌手、专辑），无标签时退回到文件名 | 切歌时信息更新；带标签的歌曲显示标签标题，不是文件名 |
| 歌词同步 | 自动加载同名 `.lrc`，播放时逐行高亮滚动 | 歌词跟着音乐一行行高亮走 |
| 歌词时间微调 | ±0.5s 整体偏移，偏移量持久化 | +/− 能把对不齐的歌词调到对齐，切回来还在 |
| 歌词拖动 + 跳转 | 拖动浏览、中间行跳转按钮、3 秒自动回弹 | 能拖动翻看；点了按钮跳到该句播放；不点自动回到当前句 |
| 三种播放模式 | 列表循环 / 单曲循环 / 随机 | 三种模式都影响自动播下一首，界面能看出当前模式 |
| 搜索过滤 | 搜索框实时过滤播放列表 | 输入关键字只剩匹配项；清空恢复全部 |
| 转码 | QProcess 异步调用 ffmpeg，有进度条 | 能生成目标格式文件，过程不卡界面 |
| 界面美化（QSS） | 暗色主题 + 圆角 + 悬停 + 红色 accent | 看着不是一堆默认灰色控件 |

## 加分功能

| 功能 | 说明 |
|------|------|
| 音频可视化 / 频谱 | 进度条上方 16 根频谱柱跳动，QAudioDecoder 解码 PCM → FFT → 对数频段归一化 → 30fps 平滑渲染 |
| 多主题 / 换肤 | 深色 / 浅色两套完整主题，运行时一键切换，记住上次选择 |
| 记住上次播放 | 下次启动自动恢复上次播放的歌曲和进度位置 |
| 转码增强 | 支持 MP3 / WAV / FLAC / AAC / OGG 五种输出格式 |
| 封面与标签 | 从元数据读取标题、歌手、专辑并显示在界面上 |
| 旋转光碟动画 | 30fps 黑胶唱片控件，播放时匀速旋转，暂停时停在原地，停止后归位 |

## 设计说明

### 整体架构

整个播放器遵循**组合优先于继承**的设计原则，围绕 `MainWindow` 聚合所有子组件，组件之间通过 Qt 信号/槽进行通信，不存在直接的跨组件耦合。

```
MainWindow
  ├── Player          ← 封装 QMediaPlayer + QAudioOutput
  ├── Playlist        ← 曲目管理 + 策略对象（多态）
  │    └── PlayModeStrategy  ← 抽象基类
  │         ├── RepeatStrategy
  │         ├── RepeatOneStrategy
  │         └── ShuffleStrategy
  ├── LrcParser       ← 纯数据类，.lrc 解析
  ├── Library         ← JSON 持久化
  ├── Transcoder      ← ffmpeg QProcess 封装
  ├── DiscWidget      ← 自定义绘制 QWidget
  ├── AudioAnalyzer   ← QAudioDecoder + FFT 异步频谱分析
  └── SpectrumWidget  ← 频谱可视化自定义 QWidget
```

### 各模块职责详解

#### Player（播放核心）

封装 `QMediaPlayer` 和 `QAudioOutput`，对外暴露简洁接口：

```cpp
// 对外接口（完整，无冗余）
void setSource(const QString &filePath);
void play();
void pause();
void stop();
void setVolume(float vol);   // 0.0 ~ 1.0
void setPosition(qint64 ms);

// 信号
positionChanged, durationChanged, stateChanged
endOfMedia, errorOccurred, metaDataReady
```

内部维护自定义状态机（`Stopped / Playing / Paused`），监听 `QMediaPlayer::playbackStateChanged` 做状态同步。`endOfMedia` 信号由 `QMediaPlayer::mediaStatusChanged == EndOfMedia` 触发，驱动 `MainWindow::onTrackEnded` 自动切歌。

#### Playlist + PlayModeStrategy（多态的核心）

**播放列表管理**：增删曲目（自动去重）、当前索引管理、播放历史记录（用于 "上一首" 回退）。

**策略模式实现**：三种播放模式的"选下一首"逻辑被抽取为 `PlayModeStrategy` 纯虚基类：

```cpp
class PlayModeStrategy {
public:
    virtual ~PlayModeStrategy() = default;
    virtual int next(int currentIndex, int trackCount) const = 0;
};
```

三个派生类各自实现 `next()`：
- `RepeatStrategy`：`(currentIndex + 1) % trackCount`（列表循环）
- `RepeatOneStrategy`：同上（因为单曲播完重播由 `onTrackEnded` 处理，手动切歌时仍前进到下一首）
- `ShuffleStrategy`：`QRandomGenerator::bounded(trackCount)`（真随机）

`Playlist` 持有 `std::unique_ptr<PlayModeStrategy>`，调用 `setPlayMode()` 时替换策略对象。`next()` 方法内部调用 `m_strategy->next(...)`，完全消除 `switch` 分支。

```cpp
Track Playlist::next() {
    if (m_tracks.isEmpty()) return Track();
    int nextIdx = m_strategy->next(m_currentIndex, m_tracks.size());
    if (m_currentIndex >= 0) m_history.prepend(m_currentIndex);
    m_currentIndex = nextIdx;
    emit currentIndexChanged(nextIdx);
    return m_tracks.at(nextIdx);
}
```

#### LrcParser（歌词解析）

纯数据类（继承自 `QObject`），解析 `.lrc` 格式的时间戳文本行：

```cpp
struct Line {
    qint64 timestamp;  // 毫秒
    QString text;
};
```

核心算法 `lineAt(positionMs, offsetMs)`：二分查找 `[mm:ss.xx]` 时间戳列表中最后一个 ≤ 给定位置的索引，O(log n) 时间复杂度。支持通过 `offsetMs` 参数整体偏移时间轴。

#### DiscWidget（光碟动画）

自定义 `QWidget`，用 `QPainter` 绘制光碟的所有视觉层：
1. 盘面底色（暗色/浅色各一套灰度）
2. 同心圆凹槽（从半径 32% 到 92%，步长 2.5%）
3. 反光亮线（线性渐变制造高光反射效果）
4. 红色中心标签 + "MUSIC" 字样
5. 中心孔

`QTimer` 以 ~30fps 驱动角度累加，`std::fmod` 保持角度在 0-360° 范围。暂停时停止定时器（停在当前位置），停止时角度归零。

#### AudioAnalyzer + SpectrumWidget（频谱可视化）

频谱功能的工作流程：

```
歌曲文件 → QAudioDecoder（异步解码）
  → QByteArray 原始 PCM 字节
  → 按格式解析（memcpy 逐帧转换成 float，支持 Float/Int32/Int16）
  → 混合声道（多声道平均值）
  → 降采样（44100Hz → 8000Hz，邻值平均）
  → 分窗 FFT（窗口 100ms，50% 重叠，Hann 窗）
  → 对数频段分组（60Hz ~ Nyquist，16 个频段，每频段 ±20% 带宽平均）
  → 每频段独立归一化（0.0 ~ 1.0）
  → SpectrumWidget 接收，按播放位置查表渲染
```

**音频解码**：不设 `QAudioDecoder` 的 target format，使用文件原生格式解码。在 `bufferReady` 信号中通过 `buf.data<int16_t>()`（唯一公开的模板方法）获取数据指针，追加到 `QByteArray`。后续在 `processDone()` 中用 `memcpy` 逐帧解析，避免了 `QAudioBuffer::data()` 和 `constData()` 在 Qt 6 中为私有方法的限制。

**FFT 实现**：Cooley-Tukey 基-2 迭代 FFT（`fftR2`），位逆序重排 + 蝶形运算。输入为实数序列，虚部填 0，原地计算。每窗口计算 FFT 后取幅值 `sqrt(re² + im²)`。

**对数频段**：中心频率从 60Hz 到采样率的 45%，按对数刻度均匀分布。每个频段以其中心频率 ±20% 为带宽，取范围内 FFT bin 的平均值。

**频谱渲染**：`SpectrumWidget` 接收预计算频谱数据（约 5 分钟歌曲 ~2000 个窗口 × 16 个频段 = ~150KB），在 `setPosition()` 中根据当前播放位置映射到窗口索引。`QTimer` 以 ~30fps 驱动平滑插值：每帧 `currentBars[i] += (targetBars[i] - currentBars[i]) * 0.2`，产生平滑的柱状跳动动画。柱高经过 `log10(1 + 3v) / log10(4)` 非线性缩放，使低能量的细节也能被看到。

**无数据降级**：分析完成前或分析失败时，`SpectrumWidget` 显示空闲动画——所有柱子以正弦波驱动微弱波动（高度 3-8%），避免空白区域。

#### Transcoder（异步转码）

通过 `QProcess` 启动 `ffmpeg.exe`：

```
ffmpeg -i "input.mp3" -y -progress pipe:1 -nostats "output.flac"
```

从 ffmpeg 的 `-progress pipe:1` 输出中正则匹配 `out_time_ms=(\d+)` 提取当前进度，以 `当前时间 / 总时长 × 100` 计算百分比，通过 `progressChanged` 信号发送给 `QProgressDialog`。`cancel()` 时 `kill` 子进程。

#### MainWindow（总控）

- **`setupUI()`**：搭建完整的 UI 层次结构（标题栏 → 左右分栏 → 频谱 → 进度条 → 控制栏），配置 QSS 样式
- **`wireSignals()`**：连接所有信号/槽，包括 UI 按钮 → 操作函数、Player 状态 → UI 刷新、歌词位置 → 歌词高亮、频谱位置 → 频谱更新等
- **`applyTheme()`**：深色/浅色主题切换，应用全局 QSS 样式表和 Windows 标题栏模式（DWM API）

### 数据流

以一次"双击播放歌曲"为例：

```
用户双击列表 → onPlaylistDoubleClicked()
  → Playlist::setCurrentIndex()  [更新索引]
  → loadCurrentTrack()           [设置 Player 源、加载歌词、启动频谱分析]
  → Player::play()               [开始播放]
  → refreshLyric()               [位置变化 → 歌词高亮]
  → SpectrumWidget::setPosition()  [位置变化 → 频谱跳动]
  → endOfMedia → onTrackEnded()  [播完自动下一首]
```

### 持久化

- **曲库**：`Library` 类用 JSON 格式保存到 `QStandardPaths::AppDataLocation` 下，包含每首歌的路径、标题、歌词偏移量等信息
- **设置**：`QSettings` 保存主题偏好、上次播放曲目路径和进度、音量
- **歌词偏移量**：存储在 `Track::lyricOffset` 中，随曲库一起持久化

### 遇到的最大难点

频谱功能的实现是开发过程中最困难的部分。技术挑战和解决方案如下：

1. **QMediaPlayer 不暴露音频采样**：`QMediaPlayer` 是高级播放接口，不提供访问原始 PCM 数据的途径。必须使用 `QAudioDecoder` 对音频文件进行离线解码。

2. **QAudioBuffer 的私有 API 问题**：在 Qt 6 中，`QAudioBuffer::data()` 和 `QAudioBuffer::constData()` 被标记为私有方法，无法直接获取采样数据指针。查阅 Qt 源码后发现**唯一**公开的访问途径是模板方法 `buf.data<T>()`。

3. **模板方法只特化了 int16 版本**：`buf.data<int16_t>()` 返回 `const QByteArray_view`，无论原始采样格式是什么。不能直接将其当作 float* 或 int32_t* 来读取，否则会得到错误的结果或崩溃。

4. **最终解决方案**：通过 `buf.data<int16_t>()` 获取原始字节指针，追加到 `QByteArray` 累积存储。在所有解码完成后，在 `processDone()` 中根据 `QAudioFormat::sampleFormat()` 报告的格式，用 `memcpy` 逐帧将字节解释为正确的类型（Float / Int32 / Int16），手动归一化到 -1.0 ~ 1.0 范围，然后混合声道到单声道。这样做绕过了 Qt 的所有模板 ABI 问题，同时保证了所有采样格式的正确处理。

5. **Track::duration 为 0 的问题**：起初在 `triggerAudioAnalysis()` 中检查 `duration < 1000` 时跳过分析，但 `Track::duration` 实际始终为 0（仅在构造时未填充）。解决方式：移除 duration 检查，改用 PCM 采样数反推音频时长，确保分析始终进行。
