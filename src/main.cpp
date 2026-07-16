#include "mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  app.setOrganizationName("MusicPlayer");
  app.setApplicationName("My Music Player");
  app.setApplicationVersion("1.0");

  // 网易云红音符图标
  QPixmap pix(64, 64);
  pix.fill(Qt::transparent);
  {
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QFont f;
    f.setPixelSize(44);
    p.setFont(f);
    p.setPen(QColor("#ec4141"));
    p.drawText(pix.rect(), Qt::AlignCenter, "♫");
  }
  app.setWindowIcon(QIcon(pix));

  MainWindow window;
  window.setWindowTitle("My Music Player");
  window.resize(800, 500);
  window.show();

  return app.exec();
}
