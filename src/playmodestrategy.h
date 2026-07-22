#ifndef PLAYMODESTRATEGY_H
#define PLAYMODESTRATEGY_H

/// 播放模式策略抽象基类 —— 不同模式对应不同的"选下一首"策略
class PlayModeStrategy {
public:
    virtual ~PlayModeStrategy() = default;
    /// 返回下一首的索引（纯虚接口）
    virtual int next(int currentIndex, int trackCount) const = 0;
};

/// 列表循环：顺序切到下一首
class RepeatStrategy : public PlayModeStrategy {
public:
    int next(int currentIndex, int trackCount) const override;
};

/// 单曲循环：手动切歌时前进到下一首（与 Repeat 一致），
/// 自动播完时的原地重播由 onTrackEnded 处理
class RepeatOneStrategy : public PlayModeStrategy {
public:
    int next(int currentIndex, int trackCount) const override;
};

/// 随机播放：随机选一首
class ShuffleStrategy : public PlayModeStrategy {
public:
    int next(int currentIndex, int trackCount) const override;
};

#endif // PLAYMODESTRATEGY_H
