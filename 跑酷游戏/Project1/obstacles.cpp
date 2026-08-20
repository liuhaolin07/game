// ============================================================
// obstacles.cpp — 障碍物、金币、护盾实现
// 功能: 实现障碍物生成与移动、金币链生成、护盾道具、
//       碰撞检测、连击系统、影子绘制等游戏核心机制
// ============================================================
#include "ObstacleManager.h"
#include "SoundManager.h"
#include "Player.h"
#include "Pool.h"
#include <cstdlib>
#include <cmath>

// ========== 常量定义（constexpr编译期常量） ==========
constexpr float COIN_RADIUS = 28.0f;     // 金币半径
constexpr float COIN_SPACING = 55.0f;    // 金币链间距
constexpr float SHIELD_WIDTH = 140.0f;   // 护盾宽度
constexpr float SHIELD_HEIGHT = 140.0f;  // 护盾高度

// ========== Obstacle 类实现 ==========

Obstacle::Obstacle() : Entity(), type(ObstacleType::Box), passed(false) {}

Obstacle::Obstacle(float x, float y, float w, float h, ObstacleType t)
    : Entity(x, y, w, h), type(t), passed(false) {}

void Obstacle::Draw() const {
    // 实际绘制由 ObstacleManager::DrawObstacles() 统一完成
}

ObstacleType Obstacle::GetType() const { return type; }
bool Obstacle::HasPassed() const { return passed; }
void Obstacle::MarkPassed() { passed = true; }

bool Obstacle::operator==(const Obstacle& other) const {
    return Entity::operator==(other) && type == other.type;
}

bool Obstacle::operator!=(const Obstacle& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const Obstacle& o) {
    Rectangle r = o.GetRect();
    os << "Obstacle(type=" << static_cast<int>(o.type)
       << ", pos=(" << r.x << "," << r.y << ")"
       << ", size=" << static_cast<int>(r.width) << "x" << static_cast<int>(r.height) << ")";
    return os;
}

// ========== Coin 类实现 ==========

Coin::Coin() : Entity(), radius(COIN_RADIUS), collected(false) {}
Coin::Coin(float x, float y, float r) : Entity(x, y, r * 2, r * 2), radius(r), collected(false) {}

void Coin::Draw() const {
    DrawCircleV({ posX, posY }, radius, GOLD);
    DrawCircleV({ posX, posY }, radius * 0.7f, YELLOW);
}

bool Coin::IsCollected() const { return collected; }
void Coin::Collect() { collected = true; }
float Coin::GetRadius() const { return radius; }

bool Coin::operator==(const Coin& other) const {
    return posX == other.posX && posY == other.posY;
}

std::ostream& operator<<(std::ostream& os, const Coin& c) {
    os << "Coin(" << c.posX << ", " << c.posY << ", r=" << c.radius << ")";
    return os;
}

// ========== Shield 类实现 ==========

Shield::Shield() : Entity(), collected(false) {}
Shield::Shield(float x, float y, float w, float h) : Entity(x, y, w, h), collected(false) {}

void Shield::Draw() const {
    DrawRectangleRec(GetRect(), SKYBLUE);
}

bool Shield::IsCollected() const { return collected; }
void Shield::Collect() { collected = true; }

bool Shield::operator==(const Shield& other) const {
    return Entity::operator==(other) && collected == other.collected;
}

bool Shield::operator!=(const Shield& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const Shield& s) {
    os << "Shield(" << s.GetX() << ", " << s.GetY()
       << ", collected=" << (s.collected ? "yes" : "no") << ")";
    return os;
}

// ========== FloatingText 类实现 ==========

FloatingText::FloatingText()
    : Entity(), text(""), lifeTime(0.0f), color(WHITE), moveOffsetY(0.0f) {}

FloatingText::FloatingText(float x, float y, const std::string& t, Color c)
    : Entity(x, y, 0, 0), text(t), lifeTime(1.0f), color(c), moveOffsetY(0.0f) {}

/**
 * 绘制浮动文字（随时间淡出并上移）
 */
void FloatingText::Draw() const {
    unsigned char alpha = static_cast<unsigned char>(255 * lifeTime);
    Color fadeColor = color;
    fadeColor.a = alpha;
    int fontSize = 30;
    DrawText(text.c_str(),
        static_cast<int>(posX - MeasureText(text.c_str(), fontSize) / 2),
        static_cast<int>(posY - moveOffsetY),
        fontSize, fadeColor);
}

bool FloatingText::IsExpired() const { return lifeTime <= 0; }

/**
 * 更新浮动文字（递减生命值，上移）
 */
void FloatingText::Update(float dt) {
    lifeTime -= dt;
    moveOffsetY += 80.0f * dt;
}

bool FloatingText::operator==(const FloatingText& other) const {
    return text == other.text && lifeTime == other.lifeTime;
}

bool FloatingText::operator!=(const FloatingText& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const FloatingText& ft) {
    os << "FloatingText(\"" << ft.text << "\", life=" << ft.lifeTime << ")";
    return os;
}

// ========== ObstacleManager 类实现 ==========

/**
 * 构造函数
 * 初始化所有计时器，加载障碍物/道具纹理资源
 */
ObstacleManager::ObstacleManager() {
    spawnTimer = 0.0f;
    spawnInterval = 2.0f;
    coinSpawnTimer = 0.0f;
    shieldSpawnTimer = 5.0f;
    coinCombo = 0;
    comboDisplayTimer = 0.0f;

    // 加载所有纹理（若文件缺失，id会被设为0）
    texBox = LoadTexture("box.png");
    texTwoBox = LoadTexture("twobox.png");
    texStoneWall = LoadTexture("stonewall.png");
    texHighStoneWall = LoadTexture("highstonewall.png");
    texBird = LoadTexture("bird.png");
    texHammer = LoadTexture("hammer.png");
    texLaser = LoadTexture("laser.png");
    texSpine = LoadTexture("spine.png");
    texShield = LoadTexture("shield.png");
    texCoin = LoadTexture("coin.png");
}

/**
 * 析构函数
 * 释放所有纹理资源，防止内存泄漏
 */
ObstacleManager::~ObstacleManager() {
    UnloadTexture(texBox);
    UnloadTexture(texTwoBox);
    UnloadTexture(texStoneWall);
    UnloadTexture(texHighStoneWall);
    UnloadTexture(texBird);
    UnloadTexture(texHammer);
    UnloadTexture(texLaser);
    UnloadTexture(texSpine);
    UnloadTexture(texShield);
    UnloadTexture(texCoin);
}

/**
 * 随机生成障碍物
 * 根据类型设置不同的尺寸和位置
 * 健壮性：生成前检查是否与金币/护盾重叠
 */
void ObstacleManager::SpawnObstacle(int screenWidth, int screenHeight, float groundLevel) {
    int typeInt = GetRandomValue(0, 7);
    ObstacleType type = static_cast<ObstacleType>(typeInt);

    float w = 80.0f;
    float h = 80.0f;
    float y = groundLevel;

    // 每种障碍物有独特的尺寸和高度
    switch (type) {
    case ObstacleType::Box:
        w = 140; h = 140;
        y = groundLevel - h;
        break;
    case ObstacleType::TwoBox:
        w = 140; h = 263;
        y = groundLevel - h + 10;
        break;
    case ObstacleType::StoneWall:
        w = 120; h = 150;
        y = groundLevel - h;
        break;
    case ObstacleType::HighStoneWall:
        w = 100; h = 250;
        y = groundLevel - h + 10;
        break;
    case ObstacleType::Spine:
        w = 220; h = 50;
        y = groundLevel - h;
        break;
    case ObstacleType::Laser:
        w = 300; h = 30;
        y = groundLevel - h;
        break;
    case ObstacleType::Bird:
        w = 175; h = 120;
        y = groundLevel - 245;
        break;
    case ObstacleType::Hammer:
        w = 120; h = 190;
        y = groundLevel - 325;
        break;
    }

    Rectangle obsRect = { static_cast<float>(screenWidth), y, w, h };

    // 避免与未收集的金币重叠
    for (const auto& coin : coins) {
        if (!coin.IsCollected()) {
            Rectangle coinBox = { coin.GetX() - coin.GetRadius(), coin.GetY() - coin.GetRadius(),
                                  coin.GetRadius() * 2, coin.GetRadius() * 2 };
            if (CheckCollisionRecs(obsRect, coinBox)) return;
        }
    }

    // 避免与未收集的护盾重叠
    for (const auto& s : shields) {
        if (!s.IsCollected() && CheckCollisionRecs(obsRect, s.GetRect())) return;
    }

    obstacles.emplace_back(static_cast<float>(screenWidth), y, w, h, type);
    spawnInterval = static_cast<float>(GetRandomValue(15, 30)) / 10.0f;
}

/**
 * 生成金币链
 * 支持6种排列模式（弧形、直线、波浪形）
 * 避免与已存在的障碍物重叠
 */
void ObstacleManager::SpawnCoin(int screenWidth, int screenHeight, float groundLevel) {
    int chainCount = GetRandomValue(4, 8);
    float startX = static_cast<float>(screenWidth) + 50.0f;
    float baseY = static_cast<float>(GetRandomValue(screenHeight - 350, screenHeight - 200));
    float spacing = COIN_SPACING;
    int pattern = GetRandomValue(0, 5);
    float radius = COIN_RADIUS;

    for (int i = 0; i < chainCount; i++) {
        float x = startX + i * spacing;
        float t = static_cast<float>(i) / (chainCount - 1);
        float yOffset = 0.0f;

        // 6种不同的金币排列图案
        switch (pattern) {
        case 0: yOffset = -90.0f * std::sin(t * PI); break;       // 弧形
        case 1: yOffset = -120.0f * std::sin(t * PI); break;      // 大弧形
        case 2: yOffset = -60.0f * std::sin(t * 2.0f * PI); break;// 波浪形
        case 3: yOffset = -100.0f * t; break;                     // 斜线上升
        case 4: yOffset = -100.0f * (1.0f - t); break;            // 斜线下降
        case 5: yOffset = 80.0f * std::sin(t * PI); break;        // 倒弧形
        }

        float cy = baseY + yOffset;
        Rectangle coinRect = { x - radius, cy - radius, radius * 2, radius * 2 };

        // 检查金币是否与障碍物重叠
        bool overlaps = false;
        for (const auto& obs : obstacles) {
            if (CheckCollisionRecs(coinRect, obs.GetRect())) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            coins.emplace_back(x, cy, radius);
        }
    }
}

/**
 * 生成护盾道具
 * 避免与障碍物位置重叠
 */
void ObstacleManager::SpawnShield(int screenWidth, float groundLevel) {
    float sx = static_cast<float>(screenWidth);
    float sy = groundLevel - 200.0f;
    float sw = SHIELD_WIDTH, sh = SHIELD_HEIGHT;
    Rectangle shieldRect = { sx, sy, sw, sh };

    // 检查是否与已存在的障碍物重叠
    for (const auto& obs : obstacles) {
        if (CheckCollisionRecs(shieldRect, obs.GetRect())) return;
    }

    shields.emplace_back(sx, sy, sw, sh);
}

/**
 * 核心更新函数（每帧调用）
 * 功能：更新所有对象位置、检测碰撞、计分、移除出屏对象
 * @param speed 当前游戏速度
 * @param player 玩家引用（用于碰撞检测）
 * @param score 得分引用
 * @return true 表示玩家碰到障碍物，游戏结束
 */
bool ObstacleManager::Update(float speed, int screenWidth, int screenHeight, Player& player, int& score) {
    float dt = GetFrameTime();
    // 更新所有计时器
    spawnTimer += dt;
    coinSpawnTimer += dt;
    shieldSpawnTimer += dt;
    if (comboDisplayTimer > 0) comboDisplayTimer -= dt;

    float groundLevel = screenHeight - 50.0f;

    // ---- 按计时生成新对象 ----
    // 护盾：每8~15秒生成一个
    if (shieldSpawnTimer >= 8.0f + GetRandomValue(0, 7)) {
        shieldSpawnTimer = 0.0f;
        SpawnShield(screenWidth, groundLevel);
    }

    // 障碍物：按间隔生成
    if (spawnTimer >= spawnInterval) {
        spawnTimer = 0.0f;
        SpawnObstacle(screenWidth, screenHeight, groundLevel);
    }

    // 金币链：每隔1.2秒生成一组
    if (coinSpawnTimer > 1.2f) {
        coinSpawnTimer = 0.0f;
        SpawnCoin(screenWidth, screenHeight, groundLevel);
    }

    Rectangle playerRect = player.GetRect();

    // ---- 障碍物更新与碰撞检测 ----
    for (auto it = obstacles.begin(); it != obstacles.end(); ) {
        it->SetPos(it->GetX() - speed, it->GetY());  // 左移

        // 飞鸟的碰撞箱稍作调整（略向上偏移）
        Rectangle checkRect = it->GetRect();
        if (it->GetType() == ObstacleType::Bird) checkRect.y -= 25;

        // 玩家与障碍物碰撞检测
        if (CheckCollisionRecs(playerRect, checkRect)) {
            if (!player.IsInvincible()) return true;  // 非无敌状态则游戏结束
        }

        // 玩家成功越过障碍物：加10分
        if (!it->HasPassed() && playerRect.x > (it->GetX() + it->GetWidth())) {
            it->MarkPassed();
            score += 10;
        }

        // 移除已出屏幕的障碍物
        if (it->GetX() + it->GetWidth() < 0) {
            it = obstacles.erase(it);
        }
        else {
            ++it;
        }
    }

    // ---- 金币更新与碰撞检测 ----
    for (auto it = coins.begin(); it != coins.end(); ) {
        it->SetPos(it->GetX() - speed, it->GetY());

        // 金币碰撞检测（圆形与矩形碰撞）
        if (!it->IsCollected() && CheckCollisionCircleRec(
            { it->GetX(), it->GetY() }, it->GetRadius(), playerRect)) {
            it->Collect();

            // 连击计数
            coinCombo++;
            comboDisplayTimer = 1.2f;

            SoundManager::PlayCoin();
            if (coinCombo >= 5) SoundManager::PlayPerfect();
            else if (coinCombo >= 3) SoundManager::PlayCombo();

            // 连击得分计算（上限5倍）
            int multiplier = coinCombo;
            if (multiplier > 5) multiplier = 5;
            int addedScore = 50 * multiplier;
            score += addedScore;

            // 显示加分浮动文字
            Color ftColor = (coinCombo >= 5) ? RED : YELLOW;
            floatingTexts.emplace_back(it->GetX(), it->GetY() - 30,
                "+" + std::to_string(addedScore), ftColor);
        }

        // 金币出屏幕或已被收集则移除
        if (it->GetX() + it->GetRadius() < 0) {
            if (!it->IsCollected()) coinCombo = 0;  // 错失金币重置连击
            it = coins.erase(it);
        }
        else if (it->IsCollected()) {
            it = coins.erase(it);
        }
        else {
            ++it;
        }
    }

    // ---- 护盾更新与碰撞检测 ----
    for (auto it = shields.begin(); it != shields.end(); ) {
        it->SetPos(it->GetX() - speed, it->GetY());

        if (!it->IsCollected() && CheckCollisionRecs(playerRect, it->GetRect())) {
            it->Collect();
            player.SetInvincible(5.0f);  // 获得5秒无敌
            SoundManager::PlayCoin();
            floatingTexts.emplace_back(it->GetX(), it->GetY() - 30, "SHIELD!", SKYBLUE);
        }

        if (it->GetX() + it->GetWidth() < 0 || it->IsCollected()) {
            it = shields.erase(it);
        }
        else {
            ++it;
        }
    }

    // ---- 浮动文字更新 ----
    for (auto it = floatingTexts.begin(); it != floatingTexts.end(); ) {
        it->Update(dt);
        if (it->IsExpired()) {
            it = floatingTexts.erase(it);
        }
        else {
            ++it;
        }
    }

    return false;  // 游戏继续
}

/**
 * 绘制所有对象
 */
void ObstacleManager::Draw() const {
    DrawObstacles();
    DrawCoins();
    DrawShields();
    DrawFloatingTexts();
    DrawCombo();
}

/**
 * 绘制障碍物
 * 为每个障碍物绘制地面阴影，然后渲染对应纹理
 * 健壮性：纹理不存在时使用橙色色块替代
 */
void ObstacleManager::DrawObstacles() const {
    float groundLevel = static_cast<float>(GetScreenHeight()) - 50.0f;

    for (const auto& obs : obstacles) {
        // 绘制地面阴影（椭圆形）
        float shadowX = obs.GetX() + obs.GetWidth() / 2;
        float shadowW = obs.GetWidth() * 0.45f;
        DrawEllipse(static_cast<int>(shadowX), static_cast<int>(groundLevel - 4),
                    static_cast<int>(shadowW), 5, Fade(BLACK, 0.2f));

        // 根据类型选择对应纹理
        const Texture2D* currentTex = nullptr;
        switch (obs.GetType()) {
        case ObstacleType::Box:           currentTex = &texBox; break;
        case ObstacleType::TwoBox:        currentTex = &texTwoBox; break;
        case ObstacleType::StoneWall:     currentTex = &texStoneWall; break;
        case ObstacleType::HighStoneWall: currentTex = &texHighStoneWall; break;
        case ObstacleType::Bird:          currentTex = &texBird; break;
        case ObstacleType::Hammer:        currentTex = &texHammer; break;
        case ObstacleType::Laser:         currentTex = &texLaser; break;
        case ObstacleType::Spine:         currentTex = &texSpine; break;
        }

        if (currentTex && currentTex->id != 0) {
            // 使用纹理渲染
            float drawW = static_cast<float>(currentTex->width);
            float drawH = static_cast<float>(currentTex->height);

            if (obs.GetType() == ObstacleType::Box || obs.GetType() == ObstacleType::TwoBox ||
                obs.GetType() == ObstacleType::StoneWall || obs.GetType() == ObstacleType::HighStoneWall) {
                drawW *= 0.8f;
            }

            if (obs.GetType() == ObstacleType::Spine) {
                drawH *= 0.5f;
            }

            float offsetX = (obs.GetWidth() - drawW) / 2.0f;
            float offsetY = (obs.GetHeight() - drawH) / 2.0f;

            if (obs.GetType() == ObstacleType::Hammer || obs.GetType() == ObstacleType::Bird ||
                obs.GetType() == ObstacleType::TwoBox || obs.GetType() == ObstacleType::HighStoneWall) {
                offsetY = obs.GetHeight() - drawH;
            }

            if (obs.GetType() == ObstacleType::HighStoneWall) {
                offsetY += 10;
            }

            Rectangle source = { 0.0f, 0.0f, static_cast<float>(currentTex->width), static_cast<float>(currentTex->height) };
            Rectangle dest = { obs.GetX() + offsetX, obs.GetY() + offsetY, drawW, drawH };
            DrawTexturePro(*currentTex, source, dest, { 0, 0 }, 0.0f, WHITE);
        }
        else {
            // 纹理加载失败时的备用渲染
            DrawRectangleRec(obs.GetRect(), ORANGE);
        }
    }
}

/**
 * 绘制金币（带旋转动画效果和阴影）
 */
void ObstacleManager::DrawCoins() const {
    float groundLevel = static_cast<float>(GetScreenHeight()) - 50.0f;

    for (const auto& coin : coins) {
        // 阴影（根据高度缩放）
        float shadowDist = groundLevel - coin.GetY();
        float shadowScale = std::max(0.3f, 1.0f - shadowDist / 400.0f);
        DrawEllipse(static_cast<int>(coin.GetX()), static_cast<int>(groundLevel - 3),
                    static_cast<int>(coin.GetRadius() * shadowScale), static_cast<int>(3 * shadowScale),
                    Fade(BLACK, 0.15f));

        if (!coin.IsCollected() && texCoin.id != 0) {
            // 3D旋转效果（通过水平缩放模拟）
            float spinAngle = GetTime() * 6.0f;
            float scaleX = std::abs(std::cos(spinAngle));
            float baseSize = coin.GetRadius() * 2.2f;
            float sizeX = baseSize * std::max(0.15f, scaleX);
            float sizeY = baseSize;
            Rectangle src = { 0, 0, static_cast<float>(texCoin.width), static_cast<float>(texCoin.height) };
            Rectangle dst = { coin.GetX() - sizeX / 2, coin.GetY() - sizeY / 2, sizeX, sizeY };
            DrawTexturePro(texCoin, src, dst, { 0, 0 }, 0.0f, WHITE);
            // 正面高光
            if (scaleX > 0.7f) {
                DrawCircleV({ coin.GetX(), coin.GetY() }, coin.GetRadius() * 0.15f,
                            Fade(RAYWHITE, 0.4f * scaleX));
            }
        } else {
            coin.Draw();
        }
    }
}

/**
 * 绘制护盾（带阴影和纹理）
 */
void ObstacleManager::DrawShields() const {
    float groundLevel = static_cast<float>(GetScreenHeight()) - 50.0f;

    for (const auto& s : shields) {
        float shadowX = s.GetX() + s.GetWidth() / 2;
        DrawEllipse(static_cast<int>(shadowX), static_cast<int>(groundLevel - 4),
                    static_cast<int>(s.GetWidth() * 0.35f), 4, Fade(BLACK, 0.2f));

        if (texShield.id != 0) {
            Rectangle source = { 0.0f, 0.0f, static_cast<float>(texShield.width), static_cast<float>(texShield.height) };
            Rectangle dest = { s.GetX(), s.GetY(), s.GetWidth(), s.GetHeight() };
            DrawTexturePro(texShield, source, dest, { 0, 0 }, 0.0f, WHITE);
        }
        else {
            s.Draw();
        }
    }
}

/**
 * 绘制所有浮动文字
 */
void ObstacleManager::DrawFloatingTexts() const {
    for (const auto& ft : floatingTexts) {
        ft.Draw();
    }
}

/**
 * 绘制连击提示
 * 3连击显示COMBO，5连击显示PERFECT
 * 带脉冲缩放动画效果
 */
void ObstacleManager::DrawCombo() const {
    if (coinCombo > 1 && comboDisplayTimer > 0.0f) {
        int screenW = GetScreenWidth();
        std::string comboStr;
        Color comboColor;
        float baseFontSize = 40.0f;

        if (coinCombo >= 5) {
            comboStr = "PERFECT! x" + std::to_string(coinCombo);
            comboColor = RED;
            baseFontSize = 60.0f;
        }
        else {
            comboStr = "COMBO! x" + std::to_string(coinCombo);
            comboColor = ORANGE;
        }

        // 脉冲缩放动画
        float scale = 1.0f + 0.3f * std::sin(comboDisplayTimer * 10.0f);
        int finalFontSize = static_cast<int>(baseFontSize * scale);
        unsigned char alpha = 255;
        if (comboDisplayTimer < 0.5f) alpha = static_cast<unsigned char>(255 * (comboDisplayTimer / 0.5f));
        comboColor.a = alpha;
        int textW = MeasureText(comboStr.c_str(), finalFontSize);
        DrawText(comboStr.c_str(), screenW / 2 - textW / 2, 100, finalFontSize, comboColor);
    }
}

void ObstacleManager::Reset() {
    obstacles.clear();
    coins.clear();
    shields.clear();
    floatingTexts.clear();
    spawnTimer = 0.0f;
    coinSpawnTimer = 0.0f;
    shieldSpawnTimer = 5.0f;
    coinCombo = 0;
    comboDisplayTimer = 0.0f;
}

int ObstacleManager::GetObstacleCount() const { return static_cast<int>(obstacles.size()); }
int ObstacleManager::GetCoinCount() const { return static_cast<int>(coins.size()); }

ObstacleManager& ObstacleManager::operator+=(const Obstacle& o) {
    obstacles.push_back(o);
    return *this;
}

ObstacleManager& ObstacleManager::operator-=(const ObstacleType& t) {
    for (auto it = obstacles.begin(); it != obstacles.end(); ) {
        if (it->GetType() == t) {
            it = obstacles.erase(it);
        }
        else {
            ++it;
        }
    }
    return *this;
}

bool ObstacleManager::operator==(const ObstacleManager& other) const {
    return obstacles.size() == other.obstacles.size() && coins.size() == other.coins.size();
}

bool ObstacleManager::operator!=(const ObstacleManager& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const ObstacleManager& om) {
    os << "ObstacleManager(obstacles=" << om.obstacles.size()
       << ", coins=" << om.coins.size()
       << ", shields=" << om.shields.size()
       << ", combo=" << om.coinCombo << ")";
    return os;
}
