// Audio.h —— 音效系统
#pragma once

#include "Common.h"

namespace tc {

enum Sfx {
    SFX_DIG_DIRT = 0, SFX_DIG_WOOD, SFX_DIG_STONE, SFX_BREAK, SFX_PLACE,
    SFX_JUMP, SFX_LAND, SFX_HURT, SFX_PICKUP, SFX_CRAFT,
    SFX_OPEN, SFX_CLOSE, SFX_HIT_ENEMY, SFX_ENEMY_DIE, SFX_COIN,
    SFX_BOSS, SFX_MENU, SFX_SPLASH, SFX_STEP, SFX_SWING, SFX_LEVELUP,
    SFX_COUNT
};

namespace audio {

// 载入 assets/sfx/ 下的 wav; 目录不存在或音频设备不可用时自动静音, 不影响游戏
bool init();
void shutdown();
bool ready();
bool enabled();
void setEnabled(bool on);

// 播放一次; pitch 做随机微调可以避免重复感
void play(Sfx s, float volume = 1.0f, float pitch = 1.0f);
// 节流播放: 同一音效在 minInterval 秒内只响一次(挖掘/脚步用)
void playThrottled(Sfx s, float minInterval, float volume = 1.0f, float pitch = 1.0f);

void setMasterVolume(float v);
float masterVolume();

}  // namespace audio
}  // namespace tc
