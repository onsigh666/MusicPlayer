#include "mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <dwmapi.h>
#endif

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  app.setApplicationName("My Music Player");
  app.setApplicationVersion("1.0");

  // ── 全局 QSS 基础：字体、菜单、提示 ──
  app.setStyleSheet(QLatin1String(
    "* { font-family: \"Microsoft YaHei\", \"PingFang SC\", \"Segoe UI\", sans-serif; }"
    "QToolTip { background: #2a2a2a; color: #e0e0e0; border: 1px solid #ec4141;"
    " border-radius: 8px; padding: 4px 8px; font-size: 12px; }"
    "QMenu { background: #1e1e1e; border: 1px solid #ec4141; border-radius: 10px;"
    " padding: 6px; color: #e0e0e0; font-size: 13px; }"
    "QMenu::item { padding: 6px 28px; border-radius: 6px; }"
    "QMenu::item:selected { background: #ec4141; color: #fff; }"
    "QMenu::separator { height: 1px; background: #3a3a3a; margin: 4px 10px; }"
    "QMessageBox { background: #1e1e1e; }"
    "QMessageBox QLabel { color: #e0e0e0; font-size: 13px; }"
    "QMessageBox QPushButton { min-width: 80px; min-height: 28px;"
    " border: 1px solid #ec4141; border-radius: 8px;"
    " background: #282828; color: #e0e0e0; }"
    "QMessageBox QPushButton:hover { background: #383838; }"
    "QProgressDialog { background: #1e1e1e; color: #e0e0e0; }"
    "QProgressDialog QLabel { color: #e0e0e0; font-size: 13px; }"
  ));

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

#ifdef Q_OS_WIN
  // 标题栏深色模式（Windows 10 20H1+ / Windows 11）
  {
    HWND hwnd = reinterpret_cast<HWND>(window.winId());
    const int DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    BOOL useDark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                          &useDark, sizeof(useDark));
  }
#endif

  return app.exec();
}
