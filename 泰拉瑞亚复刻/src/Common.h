// Common.h —— 全局常量与通用工具
#pragma once

// 避免 windows.h 的 min/max 宏和 std::min/std::max 打架
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#include "raylib.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace tc {

// ---------------------------------------------------------------- 基本尺寸
constexpr int TILE = 16;            // 一格方块的像素边长
constexpr float ZOOM = 3.0f;        // 屏幕缩放(像素风格 3 倍)
constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 720;
constexpr int VIEW_W = (int)(SCREEN_W / ZOOM);   // 可视世界像素宽
constexpr int VIEW_H = (int)(SCREEN_H / ZOOM);   // 可视世界像素高

// ---------------------------------------------------------------- 世界尺度
constexpr int WORLD_H = 1400;       // 世界总高度(格), 横向无限
constexpr int CHUNK = 64;           // 区块边长(格)
constexpr int CHUNK_AREA = CHUNK * CHUNK;
constexpr int SURFACE_Y = 250;      // 平均地表高度
constexpr int OCEAN_Y = 300;        // 海平面
constexpr int UNDERWORLD_Y = 1230;  // 地狱层顶部
constexpr int SKY_TOP = -220;       // 天空生成上界

// ---------------------------------------------------------------- 物理(单位: 像素/秒)
constexpr float GRAVITY = 900.0f;
constexpr float MAX_FALL = 620.0f;
constexpr float JUMP_SPEED = 392.0f;    // 约 5 格高
constexpr float WALK_ACCEL = 950.0f;
constexpr float AIR_ACCEL = 560.0f;
constexpr float WALK_MAX = 122.0f;
constexpr float SPRINT_MAX = 168.0f;
constexpr float GROUND_FRICTION = 1150.0f;
constexpr float AIR_FRICTION = 190.0f;
constexpr float PLAYER_W = 20.0f;
constexpr float PLAYER_H = 42.0f;

// ---------------------------------------------------------------- 光照
constexpr int LIGHT_MAX = 15;
constexpr int LIGHT_PAD = 18;       // 光照计算的边缘扩张

// ---------------------------------------------------------------- 界面
constexpr int HOTBAR_SLOTS = 10;
constexpr int INV_SLOTS = 50;       // 前 10 格是快捷栏
constexpr int MAX_STACK = 999;

// ---------------------------------------------------------------- 杂项
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }
inline int signf(float v) { return v > 0.0f ? 1 : (v < 0.0f ? -1 : 0); }
inline bool aabb(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}
inline float approach(float v, float target, float delta)
{
    if (v < target) return (v + delta < target) ? (v + delta) : target;
    if (v > target) return (v - delta > target) ? (v - delta) : target;
    return v;
}
inline float len(float x, float y) { return sqrtf(x * x + y * y); }

Color rgb(int r, int g, int b, int a = 255);
Color withAlpha(Color c, int a);
Color mixColor(Color a, Color b, float t);
Color scaleColor(Color c, float f);

}  // namespace tc
