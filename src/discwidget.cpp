#include "discwidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QtMath>

DiscWidget::DiscWidget(QWidget *parent) : QWidget(parent) {
    m_timer.setInterval(33); // ~30 fps
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        m_angle = std::fmod(m_angle + m_speed, 360.0);
        update();
    });
}

void DiscWidget::setRotation(qreal angle) {
    m_angle = angle;
    update();
}

void DiscWidget::startSpin() {
    m_spinning = true;
    m_timer.start();
}

void DiscWidget::pauseSpin() {
    m_spinning = false;
    m_timer.stop();
}

void DiscWidget::stopSpin() {
    m_spinning = false;
    m_timer.stop();
    m_angle = 0;
    update();
}

void DiscWidget::setDarkMode(bool dark) {
    if (m_darkMode == dark) return;
    m_darkMode = dark;
    update();
}

void DiscWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

void DiscWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 计算适合当前控件大小的光碟半径
    int side = qMin(width(), height());
    qreal radius = side * 0.44;
    QPointF center(width() / 2.0, height() / 2.0);

    p.save();
    p.translate(center);
    p.rotate(m_angle);
    drawDisc(p, QPointF(0, 0), radius);
    p.restore();
}

void DiscWidget::drawDisc(QPainter &p, const QPointF &center, qreal r) {
    // ── 盘面底色 ──
    p.setPen(Qt::NoPen);
    p.setBrush(m_darkMode ? QColor(55, 55, 55) : QColor(210, 210, 210));
    p.drawEllipse(center, r, r);

    // ── 同心纹理凹槽 ──
    p.setPen(QPen(m_darkMode ? QColor(75, 75, 75) : QColor(185, 185, 185), 0.5));
    p.setBrush(Qt::NoBrush);
    for (qreal rr = r * 0.32; rr < r * 0.92; rr += r * 0.025) {
        p.drawEllipse(center, rr, rr);
    }

    // ── 反光亮线 ──
    p.setPen(Qt::NoPen);
    QLinearGradient grad(-r * 0.85, 0, r * 0.3, 0);
    grad.setColorAt(0.0, QColor(255, 255, 255, 0));
    grad.setColorAt(0.48, QColor(255, 255, 255, m_darkMode ? 15 : 40));
    grad.setColorAt(0.52, QColor(255, 255, 255, m_darkMode ? 15 : 40));
    grad.setColorAt(1.0, QColor(255, 255, 255, 0));
    p.setBrush(grad);
    p.drawEllipse(center, r * 0.95, r * 0.95);

    // ── 中心标签（红底白字，双模式相同） ──
    qreal labelR = r * 0.3;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(236, 65, 65));
    p.drawEllipse(center, labelR, labelR);

    p.setPen(QColor(255, 255, 255, 220));
    QFont font("sans-serif", labelR * 0.28);
    font.setBold(true);
    p.setFont(font);
    p.drawText(QRectF(-labelR * 0.9, -labelR * 0.5, labelR * 1.8, labelR),
               Qt::AlignCenter, "MUSIC");

    // ── 中心孔 ──
    p.setPen(Qt::NoPen);
    p.setBrush(m_darkMode ? QColor(30, 30, 30) : QColor(160, 160, 160));
    p.drawEllipse(center, r * 0.05, r * 0.05);
}
