// ============================================================
// SoundManager.h — 音效管理系统（头文件）
// 功能: 通过波形合成算法生成游戏音效，使用单例模式管理
// 知识点: 单例模式、静态成员、私有构造/析构函数
// ============================================================
#pragma once
#include <raylib.h>

/**
 * 音效管理器 - 单例模式
 * 使用静态方法访问，内部维护唯一实例
 * 所有音效通过波形合成生成，无需外部音频文件
 */
class SoundManager {
public:
    static void Init();       // 初始化音效管理器
    static void Shutdown();   // 关闭并释放所有音效资源

    // 播放各种游戏音效
    static void PlayJump();      // 播放跳跃音效（方波，300→800Hz）
    static void PlayCoin();      // 播放收集金币音效（正弦波，1000→1800Hz）
    static void PlayGameOver();  // 播放游戏结束音效（锯齿波，400→60Hz下降音调）
    static void PlaySlide();     // 播放滑行音效（方波，120Hz恒定）
    static void PlayCombo();     // 播放连击音效（正弦波，800→1200Hz）
    static void PlayPerfect();   // 播放完美连击音效（正弦波，1000→2000Hz）

private:
    static SoundManager* instance;   // 单例实例指针（静态成员）

    // 音效资源（每个音效对应一个Sound对象）
    Sound jumpSound;      // 跳跃音效
    Sound coinSound;      // 金币音效
    Sound gameOverSound;  // 游戏结束音效
    Sound slideSound;     // 滑行音效
    Sound comboSound;     // 连击音效
    Sound perfectSound;   // 完美连击音效

    // 私有构造/析构（防止外部创建/销毁实例）
    SoundManager();
    ~SoundManager();

    /**
     * 波形合成方法
     * @param freqStart 起始频率(Hz)
     * @param freqEnd   结束频率(Hz)
     * @param duration  持续时间(秒)
     * @param amplitude 振幅(0~1)
     * @param waveType  波形类型(0=正弦波, 1=方波, 2=锯齿波)
     * @return 生成的Sound对象
     */
    static Sound GenerateSFX(float freqStart, float freqEnd, float duration, float amplitude, int waveType);
};
