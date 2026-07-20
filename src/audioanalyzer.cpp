#include "audioanalyzer.h"

#include <QAudioBuffer>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QDebug>
#include <QUrl>
#include <QtMath>

#include <cmath>

AudioAnalyzer::AudioAnalyzer(QObject *parent) : QObject(parent) {}
AudioAnalyzer::~AudioAnalyzer() { cancel(); }

void AudioAnalyzer::analyze(const QString &filePath, qint64 durationMs) {
    cancel();
    m_durationMs = durationMs;
    m_cancelled = false;
    m_rawData.clear();
    m_srcRate = 0;
    m_srcChannels = 0;
    m_srcFormat = QAudioFormat::Unknown;

    auto *dec = new QAudioDecoder(this);
    m_decoder = dec;
    dec->setSource(QUrl::fromLocalFile(filePath));

    connect(dec, &QAudioDecoder::bufferReady, this, [this]() {
        if (m_cancelled || !m_decoder) return;
        QAudioBuffer buf = m_decoder->read();
        if (!buf.isValid()) return;

        if (m_srcRate == 0) {
            m_srcRate = buf.format().sampleRate();
            m_srcChannels = buf.format().channelCount();
            m_srcFormat = buf.format().sampleFormat();
        }

        // 通过 public 模板函数获取指针（内部调用 private data()）
        const void *ptr = buf.data<int16_t>();
        m_rawData.append(reinterpret_cast<const char *>(ptr), buf.byteCount());
    });

    connect(dec, &QAudioDecoder::finished, this, [this]() {
        if (m_cancelled || !m_decoder) return;
        m_decoder->deleteLater();
        m_decoder = nullptr;
        processDone();
    });

    dec->start();
}

void AudioAnalyzer::cancel() {
    m_cancelled = true;
    if (m_decoder) {
        m_decoder->disconnect();
        m_decoder->stop();
        m_decoder->deleteLater();
        m_decoder = nullptr;
    }
    m_rawData.clear();
}

bool AudioAnalyzer::isBusy() const { return m_decoder != nullptr; }

// ──────────── 处理解码结果 ────────────

void AudioAnalyzer::processDone() {
    if (m_cancelled) return;

    qDebug() << "[AudioAnalyzer] done, bytes=" << m_rawData.size()
             << "rate=" << m_srcRate << "ch=" << m_srcChannels
             << "fmt=" << m_srcFormat;

    if (m_rawData.isEmpty() || m_srcRate == 0 || m_srcChannels == 0) {
        emit analysisFailed("no data");
        return;
    }

    // 转为单声道 float PCM（手动按字节解析）
    QVector<float> pcm;
    int bytesPerSample = 2; // default int16
    switch (m_srcFormat) {
    case QAudioFormat::Int32: bytesPerSample = 4; break;
    case QAudioFormat::Float: bytesPerSample = 4; break;
    default: bytesPerSample = 2; break;
    }
    int frameBytes = bytesPerSample * m_srcChannels;
    int totalFrames = m_rawData.size() / frameBytes;
    if (totalFrames < 256) {
        emit analysisFailed("too short");
        return;
    }

    pcm.reserve(totalFrames);
    for (int i = 0; i < totalFrames; ++i) {
        float sum = 0.0f;
        int off = i * frameBytes;
        for (int c = 0; c < m_srcChannels; ++c) {
            int bo = off + c * bytesPerSample;
            if (bytesPerSample == 4 && m_srcFormat == QAudioFormat::Float) {
                float v;
                std::memcpy(&v, m_rawData.constData() + bo, 4);
                sum += v;
            } else if (bytesPerSample == 4) {
                int32_t v;
                std::memcpy(&v, m_rawData.constData() + bo, 4);
                sum += static_cast<float>(v) / 2147483648.0f;
            } else {
                int16_t v;
                std::memcpy(&v, m_rawData.constData() + bo, 2);
                sum += static_cast<float>(v) / 32768.0f;
            }
        }
        pcm.append(sum / static_cast<float>(m_srcChannels));
    }
    m_rawData.clear();

    // 降采样到 8kHz
    int tgtRate = 8000;
    if (m_srcRate > tgtRate) {
        int ratio = qMax(1, m_srcRate / tgtRate);
        QVector<float> ds;
        ds.reserve(pcm.size() / ratio + 1);
        for (int i = 0; i < pcm.size(); i += ratio) {
            float acc = 0.0f;
            int cnt = 0;
            for (int j = 0; j < ratio && i + j < pcm.size(); ++j, ++cnt)
                acc += pcm[i + j];
            if (cnt > 0) ds.append(acc / static_cast<float>(cnt));
        }
        pcm = std::move(ds);
    }

    // FFT 频谱
    AnalysisData data;
    data.durationMs = m_durationMs > 0
                          ? m_durationMs
                          : static_cast<qint64>(pcm.size()) * 1000 / tgtRate;
    data.spectrum = computeSpectrum(pcm, tgtRate, kNumBands);
    data.windowCount = data.spectrum.size();
    data.valid = data.windowCount > 0;

    qDebug() << "[AudioAnalyzer] dur=" << data.durationMs
             << "win=" << data.windowCount;
    emit analysisComplete(data);
}

// ──────────── FFT ────────────

static void fftR2(QVector<float> &re, QVector<float> &im) {
    int n = re.size();
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { std::swap(re[i], re[j]); std::swap(im[i], im[j]); }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float a = -2.0f * float(M_PI) / float(len);
        float wr = std::cos(a), wi = std::sin(a);
        for (int i = 0; i < n; i += len) {
            float ur = 1.0f, ui = 0.0f;
            for (int k = 0; k < len / 2; ++k) {
                int p = i + k, q = i + k + len / 2;
                float tr = ur * re[q] - ui * im[q];
                float ti = ur * im[q] + ui * re[q];
                re[q] = re[p] - tr; im[q] = im[p] - ti;
                re[p] += tr; im[p] += ti;
                float nr = ur * wr - ui * wi;
                ui = ur * wi + ui * wr;
                ur = nr;
            }
        }
    }
}

QVector<QVector<float>> AudioAnalyzer::computeSpectrum(
    const QVector<float> &pcm, int sampleRate, int nBands) {
    int total = pcm.size();
    int winLen = sampleRate * int(kWindowMs) / 1000;
    if (winLen < 256) winLen = 256;

    int n = 1; while (n < winLen) n <<= 1;
    int hop = n / 2;
    int nWin = (total - n) / hop + 1;
    if (nWin < 1) nWin = 1;
    if (nWin > 2000) nWin = 2000;

    QVector<QVector<float>> spec(nWin, QVector<float>(nBands, 0.0f));
    QVector<float> re(n), im(n);
    int nBins = n / 2;
    float hzPer = float(sampleRate / 2) / float(nBins);

    float flo = 60.0f, fhi = sampleRate * 0.45f;
    QVector<float> cf(nBands);
    for (int b = 0; b < nBands; ++b)
        cf[b] = flo * std::pow(fhi / flo, float(b) / float(nBands - 1));

    for (int w = 0; w < nWin; ++w) {
        int off = w * hop;
        if (off + n > total) break;
        for (int i = 0; i < n; ++i) re[i] = pcm[off + i];
        // Hann
        for (int i = 0; i < n; ++i)
            re[i] *= 0.5f - 0.5f * std::cos(2.0f * float(M_PI) * i / (n - 1));
        im.fill(0.0f);
        fftR2(re, im);

        for (int i = 0; i < nBins; ++i)
            re[i] = std::sqrt(re[i] * re[i] + im[i] * im[i]);

        for (int b = 0; b < nBands; ++b) {
            float bw = cf[b] * 0.2f;
            int lo = qMax(0, int((cf[b] - bw) / hzPer));
            int hi = qMin(nBins - 1, int((cf[b] + bw) / hzPer));
            float acc = 0.0f;
            for (int bi = lo; bi <= hi; ++bi) acc += re[bi];
            spec[w][b] = acc / float(hi - lo + 1);
        }
    }

    for (int b = 0; b < nBands; ++b) {
        float mx = 1e-9f;
        for (int w = 0; w < nWin; ++w)
            if (spec[w][b] > mx) mx = spec[w][b];
        for (int w = 0; w < nWin; ++w)
            spec[w][b] = qBound(0.0f, spec[w][b] / mx, 1.0f);
    }
    return spec;
}
