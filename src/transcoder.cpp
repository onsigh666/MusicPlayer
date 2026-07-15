#include "transcoder.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

Transcoder::Transcoder(QObject *parent) : QObject(parent) {}

Transcoder::~Transcoder() { cancel(); }

QString Transcoder::ffmpegPath() {
  // 优先在同目录和上级目录查找
  QStringList candidates = {
      QCoreApplication::applicationDirPath() + "/ffmpeg.exe",
      QCoreApplication::applicationDirPath() + "/../ffmpeg.exe",
      QCoreApplication::applicationDirPath() + "/../../ffmpeg.exe",
  };
  for (const auto &path : candidates) {
    if (QFileInfo::exists(path))
      return QFileInfo(path).absoluteFilePath();
  }
  return "ffmpeg"; // fallback: 依赖 PATH
}

bool Transcoder::start(const QString &inputPath, const QString &outputPath,
                       qint64 totalMs) {
  cancel();

  m_totalMs = totalMs;
  m_outputPath = outputPath;

  m_process = new QProcess(this);
  m_process->setProcessChannelMode(QProcess::MergedChannels);

  connect(m_process, &QProcess::readyRead, this, &Transcoder::onReadyRead);
  connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, &Transcoder::onFinished);

  // ffmpeg -i in -y -progress pipe:1 -nostats out
  QStringList args;
  args << "-i" << inputPath << "-y" << "-progress"
       << "pipe:1" << "-nostats" << outputPath;

  m_process->start(ffmpegPath(), args);
  if (!m_process->waitForStarted(3000)) {
    emit finished(false, outputPath);
    return false;
  }
  return true;
}

void Transcoder::cancel() {
  if (m_process && m_process->state() != QProcess::NotRunning) {
    m_process->kill();
    m_process->waitForFinished(2000);
  }
}

// ── 解析 ffmpeg -progress 输出 ──

void Transcoder::onReadyRead() {
  static const QRegularExpression timeRe(R"(out_time_ms=(\d+))");
  while (m_process->canReadLine()) {
    QString line = QString::fromUtf8(m_process->readLine()).trimmed();
    QRegularExpressionMatch m = timeRe.match(line);
    if (m.hasMatch() && m_totalMs > 0) {
      qint64 current = m.captured(1).toLongLong() / 1000; // 转为毫秒
      int pct = static_cast<int>(qMin(current * 100 / m_totalMs, 99LL));
      emit progressChanged(pct);
    }
  }
}

void Transcoder::onFinished(int exitCode, QProcess::ExitStatus status) {
  bool ok = (status == QProcess::NormalExit && exitCode == 0);
  if (ok)
    emit progressChanged(100);
  emit finished(ok, m_outputPath);
  m_process->deleteLater();
  m_process = nullptr;
}
