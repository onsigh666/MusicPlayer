#ifndef AUDIOANALYZER_H
#define AUDIOANALYZER_H

#include <QAudioFormat>
#include <QByteArray>
#include <QObject>
#include <QVector>

class QAudioDecoder;

/// 预计算的频谱数据
struct AnalysisData {
    QVector<float> waveform;
    QVector<QVector<float>> spectrum;
    qint64 durationMs = 0;
    int windowCount = 0;
    bool valid = false;
};

/// 异步音频分析器：QAudioDecoder 解码 → FFT 频谱预计算
class AudioAnalyzer : public QObject {
    Q_OBJECT
public:
    explicit AudioAnalyzer(QObject *parent = nullptr);
    ~AudioAnalyzer() override;

    void analyze(const QString &filePath, qint64 durationMs = 0);
    void cancel();
    bool isBusy() const;

signals:
    void analysisComplete(const AnalysisData &data);
    void analysisFailed(const QString &reason);

private:
    void processDone();
    static QVector<QVector<float>> computeSpectrum(const QVector<float> &pcm,
                                                    int sampleRate,
                                                    int numBands);

    QAudioDecoder *m_decoder = nullptr;
    QByteArray m_rawData;
    int m_srcRate = 0;
    int m_srcChannels = 0;
    QAudioFormat::SampleFormat m_srcFormat = QAudioFormat::Unknown;
    qint64 m_durationMs = 0;
    bool m_cancelled = false;

    static constexpr int kTargetSampleRate = 8000;
    static constexpr float kWindowMs = 100.0f;
    static constexpr int kNumBands = 16;
};

#endif // AUDIOANALYZER_H
