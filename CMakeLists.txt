# 音乐播放器 — 基于 Qt 的桌面音乐播放器

cmake_minimum_required(VERSION 3.19)

project(MusicPlayer VERSION 1.0 LANGUAGES CXX)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 预处理工具自动运行
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt：Widgets + Multimedia（音频播放）
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets REQUIRED)

# 可执行文件
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/player.cpp
    src/player.h
    src/playlist.cpp
    src/playlist.h
    src/library.cpp
    src/library.h
    src/lrcparser.cpp
    src/lrcparser.h
    src/discwidget.cpp
    src/discwidget.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
)
