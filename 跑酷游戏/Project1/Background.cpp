// ============================================================
// Background.cpp — 游戏背景系统实现
// 功能: 实现云朵渲染、山脉视差滚动、松树绘制、星空闪烁、
//       昼夜循环系统、日月轨迹动画、飞鸟剪影、大气雾霭效果
// ============================================================
#include "Background.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>

// ========== Cloud 实现 ==========

Cloud::Cloud() : x(0), y(0), speed(0), size(0), shapeSeed(0) {}

Cloud::Cloud(float startX, float startY, float spd, float sz)
    : x(startX), y(startY), speed(spd), size(sz), shapeSeed(std::rand() % 1000) {}

/**
 * 更新云朵位置（向左缓慢移动，移出屏幕后重新生成）
 */
void Cloud::Update(float currentSpeed, int screenWidth, float groundLevel) {
    x -= speed + currentSpeed * 0.03f;
    if (x + size * 4 < 0) {
        x = static_cast<float>(screenWidth + size * 4);
        y = static_cast<float>(std::rand() % (static_cast<int>(groundLevel) / 3) + 20);
    }
}

/**
 * 绘制云朵
 * 使用多个半透明圆球合成蓬松云朵效果
 * 包含阴影层、高光层和边缘弥散点
 */
void Cloud::Draw(float lightIntensity) const {
    float s = size;
    int seed = shapeSeed;

    // 根据光照强度调整云朵颜色（白天暖色，夜晚冷色）
    unsigned char warmR = static_cast<unsigned char>(255 * lightIntensity);
    unsigned char warmG = static_cast<unsigned char>(248 * lightIntensity);
    unsigned char warmB = static_cast<unsigned char>(238 * lightIntensity);

    float hSpread = 1.4f;  // 水平展开系数（云朵是宽>高）
    float vSpread = 0.55f; // 垂直展开系数

    // 生成云朵的圆球组合（使用lambda表达式和局部结构体）
    struct Puff { float dx, dy, rad; unsigned char alpha; };
    Puff puffs[20];
    int count = 0;

    auto add = [&](float dx, float dy, float r, unsigned char a) {
        if (count < 20) { puffs[count].dx = dx; puffs[count].dy = dy; puffs[count].rad = r; puffs[count].alpha = a; count++; }
    };

    // 核心圆球（3-4个主要圆球）
    add(0, 0, s * 0.9f, 220);
    add(s * 0.8f * hSpread, -s * 0.05f, s * 0.7f, 210);
    add(-s * 0.8f * hSpread, -s * 0.05f, s * 0.7f, 210);
    add(s * 0.35f * hSpread, -s * 0.25f * vSpread * 2, s * 0.5f, 200);
    add(-s * 0.35f * hSpread, -s * 0.25f * vSpread * 2, s * 0.5f, 200);

    // 中层圆球
    float mids[6][3] = {
        { 0.45f, 0.15f, 0.45f }, { -0.45f, 0.15f, 0.45f },
        { 0.2f, -0.4f, 0.35f },  { -0.2f, -0.4f, 0.35f },
        { 0.55f, -0.2f, 0.3f },  { -0.55f, -0.2f, 0.3f }
    };
    for (int i = 0; i < 6; i++) {
        add(mids[i][0] * s * hSpread, mids[i][1] * s, mids[i][2] * s, 190);
    }

    // 随机散射小圆球（由种子控制，产生多样化云朵形状）
    int scatter = 3 + (seed % 4);
    for (int i = 0; i < scatter && count < 20; i++) {
        float a = (seed * (i + 1) * 107.0f);
        float dist = s * (0.6f + ((seed + i * 11) % 15) / 18.0f) * hSpread;
        float dy = s * (((seed + i * 7) % 20) / 25.0f - 0.2f) * vSpread;
        float r = s * (0.1f + ((seed + i * 17) % 12) / 25.0f);
        add(std::cos(a) * dist, dy, r, 160);
        add(-std::cos(a) * dist * 0.8f, dy * 0.7f, r * 0.8f, 140);
    }

    // 远端毛丝（水平拉伸的淡雾）
    float wispPos[4][2] = {
        { 1.6f, 0.0f }, { -1.6f, 0.0f },
        { 1.8f, 0.15f }, { -1.8f, 0.1f }
    };
    for (int i = 0; i < 4 && count < 20; i++) {
        add(wispPos[i][0] * s, wispPos[i][1] * s, s * 0.08f, 100);
    }

    // 阴影层（向下偏移，半透明）
    for (int i = 0; i < count; i++) {
        Color sc = {
            static_cast<unsigned char>(190 * lightIntensity),
            static_cast<unsigned char>(200 * lightIntensity),
            static_cast<unsigned char>(215 * lightIntensity),
            static_cast<unsigned char>(puffs[i].alpha * 0.5f)
        };
        DrawCircleV({ x + puffs[i].dx * 1.03f, y + puffs[i].dy + 4 + puffs[i].rad * 0.1f },
                    puffs[i].rad * 1.03f, sc);
    }

    // 主体层
    for (int i = 0; i < count; i++) {
        Color cc = { warmR, warmG, warmB, puffs[i].alpha };
        DrawCircleV({ x + puffs[i].dx, y + puffs[i].dy }, puffs[i].rad, cc);
    }

    // 顶部阳光高光（仅白天）
    if (lightIntensity > 0.4f) {
        Color hl = { 255, 255, 250, static_cast<unsigned char>(60 * lightIntensity) };
        for (int i = 0; i < count; i++) {
            if (puffs[i].dy < -s * 0.1f && puffs[i].rad > s * 0.3f) {
                float hr = puffs[i].rad * 0.45f;
                DrawCircleV({ x + puffs[i].dx - hr * 0.2f, y + puffs[i].dy - hr * 0.3f }, hr, hl);
            }
        }
    }

    // 边缘弥散点（非常透明）
    for (int i = 0; i < 6; i++) {
        float fx = x + ((seed + i * 43) % 300 - 150) / 150.0f * s * hSpread * 1.2f;
        float fy = y + s * 0.15f + i * s * 0.08f - s * 0.1f;
        float fr = s * (0.04f + (i % 3) * 0.025f);
        Color fc = { warmR, warmG, warmB, static_cast<unsigned char>(80 - i * 10) };
        DrawCircleV({ fx, fy }, fr, fc);
    }
}

float Cloud::GetX() const { return x; }
float Cloud::GetY() const { return y; }

// ========== DistantBird 实现 ==========

DistantBird::DistantBird() : x(0), y(0), speed(0), size(0), wingTimer(0) {}
DistantBird::DistantBird(float startX, float startY, float spd, float sz)
    : x(startX), y(startY), speed(spd), size(sz), wingTimer(static_cast<float>(std::rand() % 1000) / 100.0f) {}

void DistantBird::Update(float currentSpeed, int screenWidth) {
    x -= speed + currentSpeed * 0.02f;
    wingTimer += GetFrameTime() * 4.0f;
    if (x + 30 < 0) {
        x = static_cast<float>(screenWidth + std::rand() % 200);
        y = 80.0f + static_cast<float>(std::rand() % 180);
    }
}

void DistantBird::Draw(float lightIntensity) const {
    float wing = std::sin(wingTimer) * size * 0.3f;
    unsigned char alpha = static_cast<unsigned char>(180 * lightIntensity);
    Color c = { 40, 40, 50, alpha };
    DrawLine(static_cast<int>(x - size), static_cast<int>(y),
             static_cast<int>(x), static_cast<int>(y - wing), c);
    DrawLine(static_cast<int>(x), static_cast<int>(y - wing),
             static_cast<int>(x + size), static_cast<int>(y), c);
}

// ========== Mountain 实现 ==========

Mountain::Mountain()
    : x(0), width(0), height(0), baseColor(), hasSnow(false) {}

Mountain::Mountain(float startX, float w, float h, Color color, bool snow)
    : x(startX), width(w), height(h), baseColor(color), hasSnow(snow) {}

void Mountain::Update(float currentSpeed, int screenWidth) {
    x -= currentSpeed * 0.1f;  // 视差滚动（速度较慢）
    if (x + width / 2 < 0) x = static_cast<float>(screenWidth + width / 2);
}

/**
 * 绘制山脉
 * 使用三角形绘制，分亮面和暗面产生立体感
 * 支持雪顶效果
 */
void Mountain::Draw(float groundLevel, float lightIntensity) const {
    Vector2 peak = { x, groundLevel - height };
    Vector2 leftBas = { x - width / 2, groundLevel };
    Vector2 rightBas = { x + width / 2, groundLevel };
    Vector2 midBas = { x, groundLevel };

    // 阳面和阴面颜色
    Color litColor = {
        static_cast<unsigned char>(std::min(255, baseColor.r + 20) * lightIntensity),
        static_cast<unsigned char>(std::min(255, baseColor.g + 20) * lightIntensity),
        static_cast<unsigned char>(std::min(255, baseColor.b + 10) * lightIntensity), 255
    };
    Color shadowColor = {
        static_cast<unsigned char>(std::max(0, baseColor.r - 20) * lightIntensity),
        static_cast<unsigned char>(std::max(0, baseColor.g - 20) * lightIntensity),
        static_cast<unsigned char>(std::max(0, baseColor.b - 20) * lightIntensity), 255
    };

    DrawTriangle(peak, leftBas, midBas, litColor);
    DrawTriangle(peak, midBas, rightBas, shadowColor);

    // 山脊细节（较小的次要山峰）
    float ridgeW = width * 0.12f;
    float ridgeH = height * 0.15f;
    for (int r = -1; r <= 1; r += 2) {
        float rx = x + r * width * 0.28f;
        float ry = groundLevel - height + ridgeH * (0.7f + std::sin(r * 2.0f) * 0.3f);
        Vector2 rPeak = { rx, ry };
        Vector2 rBotMid = { rx, groundLevel - height + ridgeH * 0.5f };
        Vector2 rOuter = { rx + r * ridgeW, groundLevel - height + ridgeH * 0.3f };
        DrawTriangle(rPeak, rOuter, rBotMid, litColor);
    }

    // 雪顶效果
    if (hasSnow) {
        float snowH = height * 0.35f;
        float snowW = width * 0.35f;
        Vector2 sLeft = { x - snowW / 2, groundLevel - height + snowH };
        Vector2 sRight = { x + snowW / 2, groundLevel - height + snowH };
        Vector2 sMid = { x, groundLevel - height + snowH };

        DrawTriangle(peak, sLeft, sMid, {
            static_cast<unsigned char>(255 * lightIntensity),
            static_cast<unsigned char>(255 * lightIntensity),
            static_cast<unsigned char>(255 * lightIntensity), 255
        });
        DrawTriangle(peak, sMid, sRight, {
            static_cast<unsigned char>(220 * lightIntensity),
            static_cast<unsigned char>(225 * lightIntensity),
            static_cast<unsigned char>(235 * lightIntensity), 255
        });
    }
}

// ========== Moon 实现 ==========

Moon::Moon() : phase(0) {}

/**
 * 绘制月亮（含辉光、月面细节）
 * 仅在夜间可见
 */
void Moon::Draw(float progress, float nightIntensity) const {
    if (nightIntensity < 0.1f) return;

    // 月出到月落：映射夜间时段（0.40-0.85）到月亮的运行轨迹
    float nightProg = (progress - 0.40f) / 0.45f;
    if (nightProg < 0.0f) nightProg = 0.0f;
    if (nightProg > 1.0f) nightProg = 1.0f;

    float moonX = -80.0f + nightProg * (static_cast<float>(GetScreenWidth()) + 160.0f);
    float moonY = 150.0f + std::sin(nightProg * PI) * 90.0f;
    float baseR = 32.0f;

    // 月亮辉光
    Color glowColor = { 200, 220, 255, static_cast<unsigned char>(40 * nightIntensity) };
    DrawCircle(static_cast<int>(moonX), static_cast<int>(moonY),
               static_cast<int>(baseR * 2.5f), glowColor);
    glowColor.a = static_cast<unsigned char>(20 * nightIntensity);
    DrawCircle(static_cast<int>(moonX), static_cast<int>(moonY),
               static_cast<int>(baseR * 4.0f), glowColor);

    // 月亮主体
    Color moonColor = { 240, 245, 255, static_cast<unsigned char>(255 * nightIntensity) };
    DrawCircle(static_cast<int>(moonX), static_cast<int>(moonY),
               static_cast<int>(baseR), moonColor);
    // 月牙阴影
    Color crescentShade = { 220, 225, 240, static_cast<unsigned char>(200 * nightIntensity) };
    DrawCircle(static_cast<int>(moonX + baseR * 0.25f), static_cast<int>(moonY - baseR * 0.15f),
               static_cast<int>(baseR * 0.85f), crescentShade);
    // 陨石坑细节
    Color craterColor = { 230, 235, 245, static_cast<unsigned char>(100 * nightIntensity) };
    DrawCircle(static_cast<int>(moonX - baseR * 0.3f), static_cast<int>(moonY + baseR * 0.2f),
               static_cast<int>(baseR * 0.15f), craterColor);
    DrawCircle(static_cast<int>(moonX + baseR * 0.2f), static_cast<int>(moonY + baseR * 0.35f),
               static_cast<int>(baseR * 0.1f), craterColor);
}

// ========== PineTree 实现 ==========

PineTree::PineTree() : x(0), scale(1.0f) {}
PineTree::PineTree(float startX, float sc) : x(startX), scale(sc) {}

void PineTree::Update(float currentSpeed, int screenWidth) {
    x -= currentSpeed * 0.6f;
    if (x + 50 < 0) x = static_cast<float>(screenWidth + 50);
}

/**
 * 绘制松树
 * 树干+三层三角形树叶叠加
 */
void PineTree::Draw(float groundLevel, float lightIntensity) const {
    float h = 100 * scale;
    float w = 40 * scale;
    // 树干
    DrawRectangle(static_cast<int>(x - 4), static_cast<int>(groundLevel - h), 8, static_cast<int>(h),
        { static_cast<unsigned char>(70 * lightIntensity), static_cast<unsigned char>(50 * lightIntensity), static_cast<unsigned char>(35 * lightIntensity), 255 });

    // 三层树叶（三角形叠加）
    for (int layer = 0; layer < 3; layer++) {
        float yOffset = groundLevel - h + layer * (h * 0.3f);
        float layerW = w + layer * (w * 0.2f);
        DrawTriangle({ x, yOffset - 30 * scale }, { x - layerW / 2, yOffset + 20 * scale }, { x, yOffset + 20 * scale },
            { static_cast<unsigned char>(40 * lightIntensity), static_cast<unsigned char>(110 * lightIntensity), static_cast<unsigned char>(60 * lightIntensity), 255 });
        DrawTriangle({ x, yOffset - 30 * scale }, { x, yOffset + 20 * scale }, { x + layerW / 2, yOffset + 20 * scale },
            { static_cast<unsigned char>(25 * lightIntensity), static_cast<unsigned char>(80 * lightIntensity), static_cast<unsigned char>(45 * lightIntensity), 255 });
    }
}

// ========== BgStar 实现 ==========

BgStar::BgStar() : x(0), y(0), size(0) {}
BgStar::BgStar(float startX, float startY, float sz) : x(startX), y(startY), size(sz) {}

void BgStar::Update(float currentSpeed, int screenWidth, int screenHeight) {
    x -= currentSpeed * 0.01f;
    if (x < 0) {
        x = static_cast<float>(screenWidth);
        y = static_cast<float>(std::rand() % (screenHeight - 300));
    }
}

/**
 * 绘制星星（仅在夜间可见，带闪烁效果）
 */
void BgStar::Draw(float nightIntensity, float cycleTime) const {
    if (nightIntensity > 0.01f) {
        Color starCol = RAYWHITE;
        float twinkle = (std::sin(cycleTime * 3.0f + x) + 1.0f) / 2.0f;
        starCol.a = static_cast<unsigned char>(255 * nightIntensity * twinkle);
        DrawCircleV({ x, y }, size, starCol);
    }
}

// ========== GameBackground 实现 ==========

/**
 * 静态方法：混合两个颜色
 * 用于天空渐变的平滑过渡
 */
Color GameBackground::MixColor(Color c1, Color c2, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return {
        static_cast<unsigned char>(c1.r + (c2.r - c1.r) * t),
        static_cast<unsigned char>(c1.g + (c2.g - c1.g) * t),
        static_cast<unsigned char>(c1.b + (c2.b - c1.b) * t),
        255
    };
}

/**
 * 构造函数
 * 初始化所有背景元素的位置和属性
 * 使用随机数产生多样化的布局
 */
GameBackground::GameBackground(int width, int height)
    : screenWidth(width), screenHeight(height), cycleTimer(0.0f)
{
    // 生成18朵云（随机大小、速度、位置）
    clouds.reserve(18);
    for (int i = 0; i < 18; i++) {
        float sz = (std::rand() % 35 + 18) * 1.0f;
        float spd = (std::rand() % 10 + 2) * 0.08f;
        float cy = std::rand() % (screenHeight / 2) + 15;
        if (sz > 40) cy = std::rand() % (screenHeight / 3) + 15;
        else cy = 40 + std::rand() % (screenHeight / 2);
        clouds.emplace_back(
            static_cast<float>(i * (screenWidth / 12) + std::rand() % 80 - 40),
            cy, spd, sz
        );
    }

    // 生成8座远景山脉（带雪顶，速度慢）
    for (int i = 0; i < 8; i++) {
        mountainsBack.emplace_back(
            static_cast<float>(i * (screenWidth / 3) + std::rand() % 100 - 50),
            static_cast<float>(std::rand() % 250 + 600),
            static_cast<float>(std::rand() % 200 + 300),
            Color{ 65, 110, 110, 255 }, true
        );
    }

    // 生成10座近景山脉（不带雪顶，速度快）
    for (int i = 0; i < 10; i++) {
        mountainsFront.emplace_back(
            static_cast<float>(i * (screenWidth / 4) + std::rand() % 50 - 50),
            static_cast<float>(std::rand() % 150 + 400),
            static_cast<float>(std::rand() % 100 + 180),
            Color{ 50, 140, 80, 255 }, false
        );
    }

    // 生成20棵松树
    for (int i = 0; i < 20; i++) {
        pineTrees.emplace_back(
            static_cast<float>(i * (screenWidth / 15) + std::rand() % 40 - 20),
            (std::rand() % 50 + 50) / 100.0f
        );
    }

    // 生成80颗星星
    stars.reserve(80);
    for (int i = 0; i < 80; i++) {
        stars.emplace_back(
            static_cast<float>(std::rand() % screenWidth),
            static_cast<float>(std::rand() % (screenHeight - 300)),
            (std::rand() % 15 + 5) / 10.0f
        );
    }

    // 生成5只飞鸟
    birds.reserve(5);
    for (int i = 0; i < 5; i++) {
        birds.emplace_back(
            static_cast<float>(std::rand() % screenWidth),
            80.0f + static_cast<float>(std::rand() % 180),
            0.3f + static_cast<float>(std::rand() % 100) / 100.0f * 0.5f,
            6.0f + static_cast<float>(std::rand() % 40) / 10.0f
        );
    }
}

/**
 * 更新所有背景元素（每帧调用）
 */
void GameBackground::Update(float currentSpeed) {
    float dt = GetFrameTime();
    cycleTimer += dt;

    float groundLevel = static_cast<float>(screenHeight - 50);

    for (auto& cloud : clouds) {
        cloud.Update(currentSpeed, screenWidth, groundLevel);
    }
    for (auto& m : mountainsBack) {
        m.Update(currentSpeed, screenWidth);
    }
    for (auto& m : mountainsFront) {
        m.Update(currentSpeed, screenWidth);
    }
    for (auto& t : pineTrees) {
        t.Update(currentSpeed, screenWidth);
    }
    for (auto& s : stars) {
        s.Update(currentSpeed, screenWidth, screenHeight);
    }
    for (auto& b : birds) {
        b.Update(currentSpeed, screenWidth);
    }
}

/**
 * 绘制天空渐变色
 * 根据昼夜进度插值天空颜色（支持日出日落暖色过渡）
 */
void GameBackground::DrawSky(float progress) const {
    struct SkyKey { float t; Color top; Color bot; };
    std::vector<SkyKey> keys = {
        { 0.00f, { 45, 115, 235, 255 }, { 160, 220, 255, 255 } },  // 白天
        { 0.30f, { 45, 115, 235, 255 }, { 160, 220, 255, 255 } },  // 白天
        { 0.40f, { 100, 60,  60, 255 }, { 255, 130,  50, 255 } },  // 日落
        { 0.50f, { 10,  15,  40, 255 }, { 20,   30,  80, 255 } },  // 夜晚
        { 0.80f, { 10,  15,  40, 255 }, { 20,   30,  80, 255 } },  // 夜晚
        { 0.90f, { 100, 50,  80, 255 }, { 250, 100,  80, 255 } },  // 日出
        { 1.00f, { 45, 115, 235, 255 }, { 160, 220, 255, 255 } }   // 白天
    };

    // 插值计算当前天空颜色
    Color currentTop = keys[0].top, currentBot = keys[0].bot;
    for (size_t i = 0; i < keys.size() - 1; i++) {
        if (progress >= keys[i].t && progress <= keys[i + 1].t) {
            float localT = (progress - keys[i].t) / (keys[i + 1].t - keys[i].t);
            currentTop = MixColor(keys[i].top, keys[i + 1].top, localT);
            currentBot = MixColor(keys[i].bot, keys[i + 1].bot, localT);
            break;
        }
    }

    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, currentTop, currentBot);
}

/**
 * 绘制太阳
 * 带多层辉光效果和低角度时的镜头光晕
 */
void GameBackground::DrawSun(float progress, float lightIntensity) const {
    bool isDay = progress < 0.48f;
    bool isDawnDusk = (progress >= 0.48f && progress < 0.50f) || (progress > 0.82f && progress < 0.90f);
    bool visible = isDay || isDawnDusk;

    if (visible) {
        int sunX, sunY;

        if (isDay) {
            // 白天：从东到西弧线运动
            float p = progress / 0.48f;
            sunX = static_cast<int>(-100 + p * (screenWidth + 200));
            sunY = static_cast<int>(180 + std::sin(p * PI) * 120);
        } else if (progress > 0.82f && progress < 0.90f) {
            // 日出：从左侧地平线升起
            float p = (progress - 0.82f) / 0.08f;
            sunX = static_cast<int>(-120 + p * 200);
            sunY = static_cast<int>(280 + std::sin(p * PI * 0.5f) * 20);
        } else {
            // 日落：在右侧地平线下沉
            float p = (progress - 0.48f) / 0.02f;
            sunX = static_cast<int>(screenWidth + 100 - p * 200);
            sunY = static_cast<int>(290 - p * 10);
        }

        // 外层辉光
        for (int i = 6; i > 0; i--) {
            unsigned char gAlpha = static_cast<unsigned char>((20 - i * 2) * lightIntensity);
            DrawCircle(sunX, sunY, 40.0f + i * 18.0f, { 255, 220, 150, gAlpha });
        }
        // 内层辉光
        for (int i = 4; i > 0; i--) {
            unsigned char gAlpha = static_cast<unsigned char>((40 - i * 5) * lightIntensity);
            DrawCircle(sunX, sunY, 35.0f + i * 8.0f, { 255, 240, 180, gAlpha });
        }
        // 太阳核心
        DrawCircle(sunX, sunY, 38, { 255, 255, 230, static_cast<unsigned char>(255 * lightIntensity) });
        // 亮核
        DrawCircle(sunX - 8, sunY - 8, 15, { 255, 255, 250, static_cast<unsigned char>(200 * lightIntensity) });

        // 低角度时的镜头光晕
        bool nearHorizon = isDay && (progress < 0.08f || progress > 0.40f);
        if (nearHorizon || isDawnDusk) {
            float streakLen = 150.0f + std::sin(progress * PI) * 80.0f;
            float streakAlpha = 0.06f * lightIntensity;
            for (int i = 0; i < 3; i++) {
                unsigned char sAlpha = static_cast<unsigned char>(streakAlpha * 255 * (1.0f - i * 0.3f));
                float sw = streakLen - i * 30.0f;
                float offY = (i - 1) * 2.0f;
                DrawRectangle(sunX - static_cast<int>(sw / 2), sunY + static_cast<int>(offY),
                              static_cast<int>(sw), 2, { 255, 230, 180, sAlpha });
            }
        }
    }
}

/**
 * 主绘制函数
 * 按层级顺序绘制：天空→星星→太阳→月亮→云朵→飞鸟→雾霭→山脉→松树
 */
void GameBackground::Draw() const {
    // 昼夜周期：120秒一个完整循环
    float dayDuration = 120.0f;
    float progress = std::fmod(cycleTimer, dayDuration) / dayDuration;

    DrawSky(progress);

    // 计算夜间强度（0=白天, 1=全黑）
    float nightIntensity = 0.0f;
    if (progress > 0.40f && progress < 0.55f) nightIntensity = (progress - 0.40f) / 0.15f;
    else if (progress >= 0.55f && progress <= 0.80f) nightIntensity = 1.0f;
    else if (progress > 0.80f && progress < 0.95f) nightIntensity = 1.0f - (progress - 0.80f) / 0.15f;

    // 星星
    for (const auto& s : stars) {
        s.Draw(nightIntensity, cycleTimer);
    }

    float lightIntensity = 1.0f - (0.7f * nightIntensity);

    // 太阳和月亮
    DrawSun(progress, lightIntensity);
    moon.Draw(progress, nightIntensity);

    // 云朵
    for (const auto& cloud : clouds) {
        cloud.Draw(lightIntensity);
    }

    // 飞鸟
    for (const auto& b : birds) {
        b.Draw(lightIntensity);
    }

    // 水平线处的大气雾霭混合
    float groundLevel = static_cast<float>(screenHeight - 50);
    float hazeAlpha = 0.15f + nightIntensity * 0.1f;
    DrawRectangleGradientV(0, static_cast<int>(groundLevel - 60), screenWidth, 60,
        Fade(GetColor(0), 0),
        Fade({ static_cast<unsigned char>(60 + 80 * nightIntensity),
               static_cast<unsigned char>(80 + 60 * nightIntensity),
               static_cast<unsigned char>(120 + 40 * nightIntensity), 255 }, hazeAlpha));

    // 山脉（先远景后近景）
    for (const auto& m : mountainsBack) {
        m.Draw(groundLevel, lightIntensity);
    }
    for (const auto& m : mountainsFront) {
        m.Draw(groundLevel, lightIntensity);
    }

    // 松树
    for (const auto& t : pineTrees) {
        t.Draw(groundLevel, lightIntensity);
    }
}
