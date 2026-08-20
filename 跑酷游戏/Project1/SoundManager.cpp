// ============================================================
// SoundManager.cpp — 音效管理实现
// 作者: C
// 功能: 波形合成生成音效（正弦/方波/锯齿波），
//       单例模式管理音效资源生命周期
// ============================================================
#include "SoundManager.h"
#include <cmath>

SoundManager* SoundManager::instance = nullptr;   // 单例实例初始化为空

// 生成音效的静态方法 - 通过合成波形创建音效
// freqStart: 起始频率, freqEnd: 结束频率, duration: 持续时间, amplitude: 振幅, waveType: 波形类型
Sound SoundManager::GenerateSFX(float freqStart, float freqEnd, float duration, float amplitude, int waveType) {
    unsigned int sampleRate = 44100;   // 采样率44.1kHz
    unsigned int sampleCount = static_cast<unsigned int>(sampleRate * duration);   // 总采样数

    Wave wave = { 0 };
    wave.frameCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;    // 16位采样
    wave.channels = 1;       // 单声道

    short* data = static_cast<short*>(MemAlloc(sampleCount * sizeof(short)));

    // 生成波形数据
    for (unsigned int i = 0; i < sampleCount; i++) {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);   // 当前时间
        float progress = t / duration;         // 进度0~1
        float freq = freqStart + (freqEnd - freqStart) * progress;   // 频率渐变
        float value = 0.0f;

        // 根据波形类型生成不同波形
        switch (waveType) {
        case 0:   // 正弦波
            value = sinf(2.0f * PI * freq * t);
            break;
        case 1:   // 方波
            value = (sinf(2.0f * PI * freq * t) >= 0.0f) ? 1.0f : -1.0f;
            break;
        case 2:   // 锯齿波
            value = 2.0f * fmodf(freq * t, 1.0f) - 1.0f;
            break;
        }

        // 音量包络（淡入淡出，防止爆音）
        float envelope = 1.0f;
        float fadeIn = 0.005f;
        float fadeOut = 0.005f;
        if (t < fadeIn) envelope = t / fadeIn;
        if (t > duration - fadeOut) envelope = (duration - t) / fadeOut;

        data[i] = static_cast<short>(value * envelope * amplitude * 32767.0f);
    }

    wave.data = data;
    Sound sound = LoadSoundFromWave(wave);   // 从波形数据加载音效
    UnloadWave(wave);   // 释放波形数据（音效已复制）
    return sound;
}

// 构造函数 - 创建所有游戏音效
SoundManager::SoundManager() {
    // 跳跃音效：频率300->800Hz, 方波, 0.12秒
    jumpSound = GenerateSFX(300.0f, 800.0f, 0.12f, 0.4f, 1);
    SetSoundVolume(jumpSound, 0.5f);

    // 金币音效：频率1000->1800Hz, 正弦波, 0.08秒
    coinSound = GenerateSFX(1000.0f, 1800.0f, 0.08f, 0.35f, 0);
    SetSoundVolume(coinSound, 0.4f);

    // 游戏结束音效：频率400->60Hz, 锯齿波, 0.8秒（下降音调）
    gameOverSound = GenerateSFX(400.0f, 60.0f, 0.8f, 0.5f, 2);
    SetSoundVolume(gameOverSound, 0.6f);

    // 滑行音效：频率120Hz恒定, 方波, 0.15秒
    slideSound = GenerateSFX(120.0f, 120.0f, 0.15f, 0.2f, 1);
    SetSoundVolume(slideSound, 0.3f);

    // 连击音效：频率800->1200Hz, 正弦波, 0.1秒
    comboSound = GenerateSFX(800.0f, 1200.0f, 0.1f, 0.4f, 0);
    SetSoundVolume(comboSound, 0.5f);

    // 完美连击音效：频率1000->2000Hz, 正弦波, 0.15秒
    perfectSound = GenerateSFX(1000.0f, 2000.0f, 0.15f, 0.5f, 0);
    SetSoundVolume(perfectSound, 0.6f);
}

// 析构函数 - 释放所有音效资源
SoundManager::~SoundManager() {
    UnloadSound(jumpSound);
    UnloadSound(coinSound);
    UnloadSound(gameOverSound);
    UnloadSound(slideSound);
    UnloadSound(comboSound);
    UnloadSound(perfectSound);
}

// 初始化音效管理器（创建单例）
void SoundManager::Init() {
    if (!instance) {
        instance = new SoundManager();
    }
}

// 关闭音效管理器（销毁单例）
void SoundManager::Shutdown() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

// 播放各种音效的静态方法
void SoundManager::PlayJump() { if (instance) PlaySound(instance->jumpSound); }
void SoundManager::PlayCoin() { if (instance) PlaySound(instance->coinSound); }
void SoundManager::PlayGameOver() { if (instance) PlaySound(instance->gameOverSound); }
void SoundManager::PlaySlide() { if (instance) PlaySound(instance->slideSound); }
void SoundManager::PlayCombo() { if (instance) PlaySound(instance->comboSound); }
void SoundManager::PlayPerfect() { if (instance) PlaySound(instance->perfectSound); }
