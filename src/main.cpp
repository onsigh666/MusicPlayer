#include "mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  app.setApplicationName("My Music Player");
  app.setApplicationVersion("1.0");

  // 普鲁士蓝音符图标
  QPixmap pix(64, 64);
  pix.fill(Qt::transparent);
  {
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QFont f;
    f.setPixelSize(44);
    p.setFont(f);
    p.setPen(QColor("#003153")); // 普鲁士蓝
    p.drawText(pix.rect(), Qt::AlignCenter, "♫");
  }
  app.setWindowIcon(QIcon(pix));

  MainWindow window;
  window.setWindowTitle("My Music Player");
  window.resize(800, 500);
  window.show();

  return app.exec();
}
