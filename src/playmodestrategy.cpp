#include "playmodestrategy.h"
#include <QRandomGenerator>

// ──── 列表循环 ────
int RepeatStrategy::next(int currentIndex, int trackCount) const {
    return (currentIndex + 1) % trackCount;
}

// ──── 单曲循环 ────
// 手动切歌时前进到下一首（自动播完的原地重播由 MainWindow::onTrackEnded 处理）
int RepeatOneStrategy::next(int currentIndex, int trackCount) const {
    return (currentIndex + 1) % trackCount;
}

// ──── 随机播放 ────
int ShuffleStrategy::next(int /*currentIndex*/, int trackCount) const {
    return QRandomGenerator::global()->bounded(trackCount);
}
