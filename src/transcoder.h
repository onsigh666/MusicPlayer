#ifndef TRANSCODER_H
#define TRANSCODER_H

#include <QObject>
#include <QProcess>

/// 异步转码器：QProcess 调用 ffmpeg，不阻塞 UI
class Transcoder : public QObject {
  Q_OBJECT

public:
  explicit Transcoder(QObject *parent = nullptr);
  ~Transcoder() override;

  /// 启动转码。返回 false 表示 ffmpeg 不可用
  bool start(const QString &inputPath, const QString &outputPath,
             qint64 totalMs = 0);
  /// 取消转码
  void cancel();

signals:
  void progressChanged(int percent); // 0–100
  void finished(bool success, const QString &outputPath);

private slots:
  void onReadyRead();
  void onFinished(int exitCode, QProcess::ExitStatus status);

private:
  QProcess *m_process = nullptr;
  qint64 m_totalMs = 0;
  QString m_outputPath;
  static QString ffmpegPath();
};

#endif // TRANSCODER_H
