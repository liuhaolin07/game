// ============================================================
// Background.h — 游戏背景系统（头文件）
// 功能: 包含云朵、山脉、松树、星空、日月、远景飞鸟等
//       多种背景元素的类定义，以及昼夜循环控制的GameBackground
// 知识点: 类封装、构造函数重载、静态成员函数、STL vector
// ============================================================
#pragma once
#include <vector>
#include <raylib.h>

/**
 * 云朵类：天空中漂浮的云层
 * 每个云朵有随机形状种子，采用多层圆球合成
 */
class Cloud {
public:
    Cloud();                                          // 默认构造函数
    Cloud(float startX, float startY, float spd, float sz);  // 带参构造函数（函数重载）

    void Update(float speed, int screenWidth, float groundLevel);
    void Draw(float lightIntensity) const;

    float GetX() const;
    float GetY() const;

private:
    float x, y;         // 位置
    float speed;        // 移动速度
    float size;         // 大小
    int shapeSeed;      // 形状随机种子（影响云朵形状）
};

/**
 * 山脉类：带有视差效果的远景和近景山脉
 * 支持雪顶和颜色设置
 */
class Mountain {
public:
    Mountain();
    Mountain(float startX, float w, float h, Color color, bool snow);

    void Update(float speed, int screenWidth);
    void Draw(float groundLevel, float lightIntensity) const;

private:
    float x, width, height;  // 位置与尺寸
    Color baseColor;         // 基础颜色
    bool hasSnow;            // 是否有雪顶
};

/**
 * 松树类：地面上背景松树
 * 绘制为多层三角形叠加
 */
class PineTree {
public:
    PineTree();
    PineTree(float startX, float sc);

    void Update(float speed, int screenWidth);
    void Draw(float groundLevel, float lightIntensity) const;

private:
    float x;        // 位置X
    float scale;    // 缩放比例
};

/**
 * 远景飞鸟类：远处天空中的小鸟剪影
 * 带翅膀扇动动画
 */
class DistantBird {
public:
    DistantBird();
    DistantBird(float startX, float startY, float spd, float sz);

    void Update(float currentSpeed, int screenWidth);
    void Draw(float lightIntensity) const;

private:
    float x, y;          // 位置
    float speed;         // 飞行速度
    float size;          // 大小
    float wingTimer;     // 翅膀扇动计时器
};

/**
 * 月亮类：夜间天空中的月亮
 * 带辉光、月面陨石坑细节
 */
class Moon {
public:
    Moon();
    void Draw(float progress, float nightIntensity) const;

private:
    float phase;  // 月相
};

/**
 * 背景星空类：夜间背景中的闪烁星空
 */
class BgStar {
public:
    BgStar();
    BgStar(float startX, float startY, float sz);

    void Update(float speed, int screenWidth, int screenHeight);
    void Draw(float nightIntensity, float cycleTime) const;

private:
    float x, y;    // 位置
    float size;    // 大小
};

/**
 * 游戏背景总控类
 * 管理云、山脉、松树、星星、飞鸟，
 * 控制昼夜交替循环（天空颜色渐变、日月交替）
 */
class GameBackground {
public:
    GameBackground(int width, int height);
    void Update(float currentSpeed);
    void Draw() const;

private:
    int screenWidth, screenHeight;  // 屏幕尺寸

    // 背景元素容器
    std::vector<Cloud> clouds;
    std::vector<Mountain> mountainsBack;   // 远景山脉
    std::vector<Mountain> mountainsFront;  // 近景山脉
    std::vector<PineTree> pineTrees;
    std::vector<BgStar> stars;
    std::vector<DistantBird> birds;
    Moon moon;

    float cycleTimer;  // 昼夜循环计时器

    // 静态工具方法（静态成员函数）
    static Color MixColor(Color c1, Color c2, float t);           // 颜色混合
    static void DrawMountainPeak(float x, float groundLevel, float h, float w, float lightIntensity);

    // 私有绘制方法
    void DrawSky(float progress) const;      // 绘制天空渐变
    void DrawSun(float progress, float lightIntensity) const;  // 绘制太阳
};
