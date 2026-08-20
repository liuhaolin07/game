// ============================================================
// Player.h — 玩家控制类（头文件）
// 功能: 玩家角色类，继承自Entity和ICollidable，
//       支持跳跃（二段跳）、滑行、无敌状态
// 知识点: 继承、多态(override)、运算符重载、友元函数
// ============================================================
#pragma once
#include "Entity.h"

// 玩家相关常量（constexpr关键字演示）
constexpr float PLAYER_WIDTH = 180.0f;     // 玩家宽度
constexpr float PLAYER_HEIGHT = 270.0f;    // 玩家高度
constexpr float PLAYER_GRAVITY = 0.46f;    // 重力加速度
constexpr float PLAYER_JUMP_SPEED = -19.0f;// 跳跃初速度（负值=向上）
constexpr int PLAYER_MAX_JUMPS = 2;        // 最大跳跃次数（二段跳）
constexpr float PLAYER_SLIDE_WIDTH = 288.0f;  // 滑行时宽度
constexpr float PLAYER_SLIDE_HEIGHT = 216.0f; // 滑行时高度

/**
 * 玩家类
 * 继承自Entity基类和ICollidable碰撞接口
 * 展示C++多重继承的用法
 */
class Player : public Entity, public ICollidable {
public:
    // 构造函数：传入起始坐标，初始化玩家属性
    Player(float startX, float startY);
    // 析构函数：释放加载的纹理资源
    ~Player();

    // 每帧更新玩家状态（重力、跳跃、滑行逻辑）
    void Update(int screenWidth, int screenHeight);
    // 绘制玩家（含纹理回退方案：有纹理则渲染纹理，否则用色块）
    void Draw() const override;
    // 获取碰撞矩形（带边距调整）
    Rectangle GetRect() const override;

    // 获取碰撞检测用的矩形（多态：覆写纯虚函数）
    Rectangle GetCollisionRect() const override { return GetRect(); }

    // 重置玩家状态（用于重新开始或复活）
    void Reset(float startX, float startY);

    // 无敌状态判断和设置
    bool IsInvincible() const;
    void SetInvincible(float duration);

    // 运算符重载：比较两个玩家是否相同位置
    bool operator==(const Player& other) const;
    bool operator!=(const Player& other) const;
    // 赋值运算符重载
    Player& operator=(const Player& other);

    // 友元函数：输出玩家信息到流
    friend std::ostream& operator<<(std::ostream& os, const Player& p);

private:
    // ---- 物理属性 ----
    Vector2 velocity;     // 速度向量
    float gravity;        // 重力值
    float jumpSpeed;      // 跳跃速度

    Color color;          // 备用颜色（无纹理时渲染用）

    // ---- 状态标记 ----
    bool isGrounded;      // 是否在地面上
    int jumpCount;        // 当前跳跃次数
    int maxJumps;         // 最大跳跃次数

    // ---- 渲染资源 ----
    Texture2D texture;          // 正常纹理
    bool hasTexture;            // 是否有正常纹理
    bool isSliding;             // 是否在滑行
    Texture2D slideTexture;     // 滑行纹理
    bool hasSlideTexture;       // 是否有滑行纹理

    // ---- 无敌状态 ----
    float invincibleTimer;      // 无敌计时器
    Texture2D protectedTexture; // 无敌护盾纹理
    bool hasProtectedTexture;   // 是否有护盾纹理
};
