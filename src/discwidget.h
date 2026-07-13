#ifndef DISCWIDGET_H
#define DISCWIDGET_H

#include <QTimer>
#include <QWidget>

/// 旋转黑胶光碟控件
class DiscWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)

public:
    explicit DiscWidget(QWidget *parent = nullptr);

    qreal rotation() const { return m_angle; }
    void setRotation(qreal angle);

public slots:
    void startSpin();   // 开始旋转
    void pauseSpin();   // 暂停旋转（停在当前位置）
    void stopSpin();    // 停止旋转

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawDisc(QPainter &p, const QPointF &center, qreal radius);

    QTimer m_timer;
    qreal m_angle = 0;    // 当前旋转角度
    qreal m_speed = 1.5;  // 每帧旋转度数
    bool m_spinning = false;
};

#endif // DISCWIDGET_H
