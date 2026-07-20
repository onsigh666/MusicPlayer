#include "spectrumwidget.h"

#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include <cmath>

// ─────────────────────── 构造 ───────────────────────

SpectrumWidget::SpectrumWidget(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);
    setMinimumHeight(40);

    m_currentBars.fill(0.0f, kNumBars);
    m_targetBars.fill(0.0f, kNumBars);

    m_timer.setInterval(33); // ~30fps
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        // 空闲时驱动相位
        if (!m_hasData)
            m_idlePhase += 0.08f;
        // 平滑插值
        for (int i = 0; i < kNumBars; ++i)
            m_currentBars[i] += (m_targetBars[i] - m_currentBars[i]) * kSmoothing;
        update();
    });
    m_timer.start();
}

// ─────────────────────── 公开接口 ───────────────────────

void SpectrumWidget::setAnalysisData(const AnalysisData &data) {
    qDebug() << "[SpectrumWidget] setAnalysisData: valid=" << data.valid
             << "windows=" << data.windowCount
             << "durationMs=" << data.durationMs;
    m_data = data;
    m_hasData = data.valid;
    m_currentFrame = -1;
    m_currentBars.fill(0.0f, kNumBars);
    m_targetBars.fill(0.0f, kNumBars);
    update();
}

void SpectrumWidget::clearData() {
    m_hasData = false;
    m_data = {};
    m_currentFrame = -1;
    m_currentBars.fill(0.0f, kNumBars);
    m_targetBars.fill(0.0f, kNumBars);
    update();
}

void SpectrumWidget::setDarkMode(bool dark) {
    if (m_darkMode == dark)
        return;
    m_darkMode = dark;
    update();
}

void SpectrumWidget::setPosition(qint64 posMs) {
    m_position = posMs;
    if (!m_hasData)
        return;

    int idx = frameIndex(posMs);
    if (idx == m_currentFrame)
        return;
    m_currentFrame = idx;

    if (idx >= 0 && idx < m_data.spectrum.size()) {
        const auto &bands = m_data.spectrum[idx];
        int n = qMin((int)bands.size(), kNumBars);
        for (int i = 0; i < n; ++i)
            m_targetBars[i] = bands[i];
        for (int i = n; i < kNumBars; ++i)
            m_targetBars[i] = 0.0f;
    } else {
        m_targetBars.fill(0.0f, kNumBars);
    }
}

void SpectrumWidget::startAnimating() {
    m_animating = true;
    update();
}

void SpectrumWidget::pauseAnimating() {
    m_animating = false;
    update();
}

void SpectrumWidget::stopAnimating() {
    m_animating = false;
    m_currentFrame = -1;
    m_currentBars.fill(0.0f, kNumBars);
    m_targetBars.fill(0.0f, kNumBars);
    update();
}

// ─────────────────────── 绘制 ───────────────────────

void SpectrumWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 自身背景 —— 保证可见
    QColor bg = m_darkMode ? QColor(0x1a, 0x1a, 0x1a)
                            : QColor(0xe8, 0xe8, 0xe8);
    p.fillRect(rect(), bg);

    // 底部细线分隔
    p.setPen(QPen(QColor(0xec, 0x41, 0x41, 80), 1));
    p.drawLine(rect().bottomLeft(), rect().bottomRight());

    QRect r = rect().adjusted(4, 4, -4, -4);

    if (m_hasData) {
        // 有数据：在每帧更新目标值（已在 sizePosition 中处理）
        drawBars(p, r);
    } else {
        drawIdle(p, r);
    }
}

void SpectrumWidget::drawIdle(QPainter &p, const QRect &r) {
    QColor idleColor = m_darkMode ? QColor(0x66, 0x50, 0x50)
                                   : QColor(0xc8, 0xa0, 0xa0);
    p.setPen(Qt::NoPen);

    float slotW = static_cast<float>(r.width()) / kNumBars;
    float barW = slotW * (1.0f - kBarGap);
    float gap = slotW * kBarGap / 2.0f;
    float maxH = static_cast<float>(r.height());

    for (int i = 0; i < kNumBars; ++i) {
        float energy = 0.08f * (0.3f + 0.7f * std::sin(m_idlePhase + i * 0.45f));
        float h = energy * maxH;
        if (h < 3.0f) h = 3.0f;

        float x = r.x() + i * slotW + gap;
        float y = r.y() + r.height() - h;
        p.setBrush(idleColor);
        p.drawRoundedRect(QRectF(x, y, barW, h), 2.0, 2.0);
    }
}

void SpectrumWidget::drawBars(QPainter &p, const QRect &r) {
    p.setPen(Qt::NoPen);
    float slotW = static_cast<float>(r.width()) / kNumBars;
    float barW = slotW * (1.0f - kBarGap);
    float gap = slotW * kBarGap / 2.0f;
    float maxH = static_cast<float>(r.height());

    QColor bottom(0xec, 0x41, 0x41);
    QColor top = m_darkMode ? QColor(0x5a, 0x00, 0x00)
                             : QColor(0xf5, 0xa0, 0xa0);

    for (int i = 0; i < kNumBars; ++i) {
        float v = m_currentBars[i];
        float scaled = logScale(v);
        float h = scaled * maxH;
        if (scaled > 0.01f && h < 3.0f) h = 3.0f;

        float x = r.x() + i * slotW + gap;
        float y = r.y() + r.height() - h;

        QLinearGradient grad(QPointF(0, r.y() + r.height()),
                             QPointF(0, r.y()));
        grad.setColorAt(0.0, bottom);
        grad.setColorAt(1.0, top);
        p.setBrush(grad);

        p.drawRoundedRect(QRectF(x, y, barW, h), 2.0, 2.0);
    }
}

// ─────────────────────── 辅助 ───────────────────────

int SpectrumWidget::frameIndex(qint64 posMs) const {
    if (!m_hasData || m_data.durationMs <= 0 || m_data.windowCount <= 0)
        return 0;
    int idx = static_cast<int>(
        (posMs * (qint64)m_data.windowCount) / m_data.durationMs);
    return qBound(0, idx, m_data.windowCount - 1);
}

float SpectrumWidget::logScale(float v) {
    if (v <= 0.0f) return 0.0f;
    return std::log10(1.0f + 3.0f * v) / std::log10(4.0f);
}
