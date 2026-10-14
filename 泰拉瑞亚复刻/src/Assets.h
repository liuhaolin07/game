// Assets.h —— 资源加载: 图集清单 / 精灵帧 / 字体
#pragma once

#include "Common.h"

#include <string>
#include <unordered_map>

namespace tc {

// 一帧精灵: 贴图 + 源矩形
struct Frame {
    Texture2D tex{};
    Rectangle rect{};
};

class Assets {
public:
    static bool Load(const char* assetDir);
    static void Unload();

    // 按名字取帧(来自 atlas.txt)
    static const Frame* Find(const char* name);
    // 方块图集 / 物品图集有同名帧, 用带命名空间前缀的名字取
    static const Frame* FindTile(const char* name);
    static const Frame* FindItem(const char* name);
    static bool Exists(const char* name);

    static void SetPixelFilter();
};

// 画一帧精灵; flip 为真时水平镜像, scale 为像素放大倍数
void drawFrame(const Frame& f, float x, float y, bool flip, Color tint, float scale = 1.0f);
void drawFrameC(const Frame& f, float cx, float cy, bool flip, Color tint, float scale = 1.0f);
// 按名字画一帧(找不到则什么都不画)
void drawSprite(const char* name, float x, float y, bool flip, Color tint, float scale = 1.0f);
void drawSpriteC(const char* name, float cx, float cy, bool flip, Color tint, float scale = 1.0f);

// 文本: 自动使用中文系统字体(若可用), 否则退回 raylib 默认字体
namespace text {

void Init();                     // 载入字体, chars 决定要打进字体的字符集
void Shutdown();
void Draw(const char* s, int x, int y, int size, Color c);
void DrawCentered(const char* s, int cx, int y, int size, Color c);
void DrawRight(const char* s, int rx, int y, int size, Color c);
void DrawEx(const char* s, Vector2 pos, float size, float spacing, Color c);
int Width(const char* s, int size);
int Height(int size);
void DrawShadow(const char* s, int x, int y, int size, Color c);
bool HasCJK();

}  // namespace text

}  // namespace tc
