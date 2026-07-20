#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include "audioanalyzer.h"

#include <QTimer>
#include <QWidget>

/// 频谱可视化控件：在进度条上方显示跳动的频谱柱
class SpectrumWidget : public QWidget {
    Q_OBJECT
public:
    explicit SpectrumWidget(QWidget *parent = nullptr);

    void setAnalysisData(const AnalysisData &data);
    void clearData();
    void setDarkMode(bool dark);

public slots:
    void setPosition(qint64 posMs);
    void startAnimating();
    void pauseAnimating();
    void stopAnimating();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawIdle(QPainter &p, const QRect &r);
    void drawBars(QPainter &p, const QRect &r);
    int frameIndex(qint64 posMs) const;
    static float logScale(float v);

    QTimer m_timer;
    AnalysisData m_data;
    qint64 m_position = 0;
    bool m_darkMode = true;
    bool m_animating = false;
    bool m_hasData = false;

    QVector<float> m_currentBars; // 当前渲染高度（平滑插值）
    QVector<float> m_targetBars;  // 目标高度
    int m_currentFrame = -1;

    float m_idlePhase = 0.0f;     // 空闲动画相位

    static constexpr int kNumBars = 16;
    static constexpr float kBarGap = 0.3f;
    static constexpr float kSmoothing = 0.2f;
};

#endif // SPECTRUMWIDGET_H
