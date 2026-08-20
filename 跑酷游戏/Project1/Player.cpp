// ============================================================
// Player.cpp — 玩家控制实现
// 功能: 实现玩家移动、跳跃（二段跳）、滑行、无敌状态、
//       纹理渲染、碰撞箱计算等核心逻辑
// ============================================================
#include "Player.h"
#include "SoundManager.h"

/**
 * 构造函数
 * 调用基类Entity的构造函数初始化位置和大小，
 * 加载玩家纹理资源，设置物理参数
 */
Player::Player(float startX, float startY)
    : Entity(startX, startY, PLAYER_WIDTH, PLAYER_HEIGHT)
{
    // 初始化速度
    velocity.x = 0;
    velocity.y = 0;

    // 物理参数
    gravity = PLAYER_GRAVITY;
    jumpSpeed = PLAYER_JUMP_SPEED;

    color = RED;  // 备用渲染颜色（当纹理加载失败时使用）

    isGrounded = false;
    jumpCount = 0;
    maxJumps = PLAYER_MAX_JUMPS;

    // 加载纹理（健壮性：检查纹理是否加载成功）
    texture = LoadTexture("player.png");
    hasTexture = (texture.id != 0);

    isSliding = false;
    slideTexture = LoadTexture("slide.png");
    hasSlideTexture = (slideTexture.id != 0);

    invincibleTimer = 0.0f;
    protectedTexture = LoadTexture("protected.png");
    hasProtectedTexture = (protectedTexture.id != 0);
}

/**
 * 析构函数
 * 释放构造函数中加载的所有纹理资源
 * 防止内存泄漏
 */
Player::~Player() {
    if (hasTexture) {
        UnloadTexture(texture);
    }
    if (hasSlideTexture) {
        UnloadTexture(slideTexture);
    }
    if (hasProtectedTexture) {
        UnloadTexture(protectedTexture);
    }
}

/**
 * 每帧更新玩家状态
 * 核心功能：
 * 1. 无敌计时器递减
 * 2. 重力模拟（无敌飞行阶段使用缓动效果）
 * 3. 地面碰撞检测与落地复位
 * 4. 跳跃输入检测（支持二段跳）
 * 5. 短跳/长跳判定
 * 6. 滑行检测
 */
void Player::Update(int screenWidth, int screenHeight) {
    // 递减无敌计时
    if (invincibleTimer > 0.0f) invincibleTimer -= GetFrameTime();

    velocity.x = 0;

    float groundLevel = screenHeight - 50.0f;      // 地面Y坐标
    float emptyBottomSpace = 75.0f;                  // 底部留白

    // 无敌阶段：前1.5秒进入飞行状态（向上漂浮）
    if (invincibleTimer > 1.5f) {
        float flyHeight = 250.0f;
        float targetY = groundLevel - height + emptyBottomSpace - flyHeight;
        velocity.y = (targetY - posY) * 0.10f;  // 缓动向目标位置
        isGrounded = false;
    }
    else {
        velocity.y += gravity;  // 普通受重力影响
    }

    // 更新垂直位置
    posY += velocity.y;

    // 地面碰撞检测与落地处理
    if (posY + height - emptyBottomSpace >= groundLevel) {
        posY = groundLevel - height + emptyBottomSpace;
        velocity.y = 0;
        isGrounded = true;
        jumpCount = 0;  // 落地后重置跳跃计数
    }
    else {
        isGrounded = false;
    }

    // ---- 跳跃输入处理（支持二段跳） ----
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_SPACE)) {
        if (isGrounded) {
            // 第一次跳跃（地面起跳）
            velocity.y = jumpSpeed;
            isGrounded = false;
            jumpCount++;
            SoundManager::PlayJump();
        }
        else if (jumpCount < maxJumps) {
            // 第二次跳跃（空中二段跳）
            velocity.y = 0;
            velocity.y = jumpSpeed * 0.95f;
            jumpCount++;
            SoundManager::PlayJump();
        }
    }

    // 短跳效果：松开跳跃键时增加下落速度
    if (velocity.y < 0 && !(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_SPACE))) {
        velocity.y += gravity * 1.5f;
    }

    // 滑行检测（只有在地面上才能滑行）
    if (isGrounded && (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))) {
        if (!isSliding) SoundManager::PlaySlide();
        isSliding = true;
    }
    else {
        isSliding = false;
    }
}

/**
 * 绘制玩家
 * 优先级：护盾纹理 > 滑行纹理 > 正常纹理 > 纯色块
 * 健壮性：纹理加载失败时使用简单色块渲染
 */
void Player::Draw() const {
    Texture2D currentTexture = texture;
    bool currentHasTexture = hasTexture;

    // 根据状态选择纹理
    if (invincibleTimer > 0.0f && hasProtectedTexture) {
        currentTexture = protectedTexture;
        currentHasTexture = true;
    }
    else if (isSliding && hasSlideTexture) {
        currentTexture = slideTexture;
        currentHasTexture = true;
    }

    if (currentHasTexture) {
        // 使用纹理渲染
        Rectangle source = { 0.0f, 0.0f, static_cast<float>(currentTexture.width), static_cast<float>(currentTexture.height) };

        float drawWidth = width;
        float drawHeight = height;
        float offsetX = 0.0f;
        float offsetY = 0.0f;

        // 滑行时调整绘制尺寸和位置
        if (isSliding) {
            drawWidth = 288.0f;
            drawHeight = 216.0f;
            offsetX = -(drawWidth - width) / 2.0f;
            offsetY = height - drawHeight;
        }

        Rectangle dest = { posX + offsetX, posY + offsetY, drawWidth, drawHeight };
        DrawTexturePro(currentTexture, source, dest, { 0, 0 }, 0.0f, WHITE);
    }
    else {
        // 无纹理时的备用渲染方式（纯色块）
        DrawRectangle(static_cast<int>(posX), static_cast<int>(posY),
            static_cast<int>(width), static_cast<int>(height), color);
    }
}

/**
 * 获取碰撞矩形（带边距调整）
 * 返回精确的碰撞箱，使碰撞判定更精确
 * 站立和滑行状态使用不同的碰撞箱尺寸
 */
Rectangle Player::GetRect() const {
    float marginXLeft = 45.0f;    // 左侧边距
    float marginXRight = 35.0f;   // 右侧边距
    float marginTop = 40.0f;      // 上边距
    float marginBottom = 65.0f;   // 下边距

    if (isSliding) {
        // 滑行时碰撞箱更扁更靠下
        float slideRenderWidth = 288.0f;
        float slideOffsetX = -54.0f;
        float slideMarginTop = 115.0f;
        float slideMarginLeft = 120.0f;
        float slideMarginRight = 55.0f;
        float slideMarginBottom = 75.0f;

        return {
            posX + slideOffsetX + slideMarginLeft,
            posY + slideMarginTop,
            slideRenderWidth - slideMarginLeft - slideMarginRight,
            height - slideMarginTop - slideMarginBottom
        };
    }

    return {
        posX + marginXLeft,
        posY + marginTop,
        width - marginXLeft - marginXRight,
        height - marginTop - marginBottom
    };
}

bool Player::IsInvincible() const {
    return invincibleTimer > 0.0f;
}

void Player::SetInvincible(float duration) {
    invincibleTimer = duration;
}

void Player::Reset(float startX, float startY) {
    SetPos(startX, startY);
    velocity.x = 0;
    velocity.y = 0;
    isGrounded = false;
    jumpCount = 0;
}

bool Player::operator==(const Player& other) const {
    return posX == other.posX && posY == other.posY;
}

bool Player::operator!=(const Player& other) const {
    return !(*this == other);
}

Player& Player::operator=(const Player& other) {
    if (this != &other) {
        Entity::operator=(other);
        velocity = other.velocity;
        gravity = other.gravity;
        jumpSpeed = other.jumpSpeed;
        color = other.color;
        isGrounded = other.isGrounded;
        jumpCount = other.jumpCount;
        maxJumps = other.maxJumps;
        isSliding = other.isSliding;
        invincibleTimer = other.invincibleTimer;
    }
    return *this;
}

std::ostream& operator<<(std::ostream& os, const Player& p) {
    os << "Player(" << p.posX << ", " << p.posY << ") "
       << "HP:" << (p.IsInvincible() ? "SHIELD" : "NORMAL");
    return os;
}
