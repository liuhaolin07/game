// ============================================================
// ObstacleManager.h — 障碍物与道具管理器（头文件）
// 功能: 管理障碍物、金币、护盾的生成、移动、碰撞与渲染，
//       包含 Obstacle、Coin、Shield、FloatingText 等多个类
// 知识点: 继承、枚举类、模板函数、运算符重载、STL容器
// ============================================================
#pragma once
#include <vector>
#include <string>
#include "Entity.h"
#include "Pool.h"

class Player;

/**
 * 障碍物类型枚举
 * 使用 enum class（C++11强类型枚举）
 */
enum class ObstacleType {
    Box,            // 普通箱子
    TwoBox,         // 双倍高箱子
    StoneWall,      // 石墙
    HighStoneWall,  // 高石墙
    Bird,           // 飞鸟（空中障碍）
    Hammer,         // 锤子（高空障碍）
    Laser,          // 激光（地面障碍）
    Spine           // 地刺（地面障碍）
};

/**
 * 模板函数：获取障碍物类型名称
 * 演示C++函数模板的用法
 */
template<typename T>
std::string ObstacleTypeName(T type) {
    switch (static_cast<ObstacleType>(type)) {
    case ObstacleType::Box:           return "Box";
    case ObstacleType::TwoBox:        return "TwoBox";
    case ObstacleType::StoneWall:     return "StoneWall";
    case ObstacleType::HighStoneWall: return "HighStoneWall";
    case ObstacleType::Bird:          return "Bird";
    case ObstacleType::Hammer:        return "Hammer";
    case ObstacleType::Laser:         return "Laser";
    case ObstacleType::Spine:         return "Spine";
    default:                          return "Unknown";
    }
}

/**
 * 障碍物类
 * 继承自Entity（位置/尺寸）和ICollidable（碰撞接口）
 */
class Obstacle : public Entity, public ICollidable {
public:
    Obstacle();
    Obstacle(float x, float y, float w, float h, ObstacleType t);

    void Draw() const override;

    Rectangle GetCollisionRect() const override { return GetRect(); }

    ObstacleType GetType() const;
    bool HasPassed() const;   // 是否已被玩家越过
    void MarkPassed();        // 标记为已越过

    bool operator==(const Obstacle& other) const;
    bool operator!=(const Obstacle& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Obstacle& o);

private:
    ObstacleType type;  // 障碍物类型
    bool passed;        // 是否已通过（用于计分）
};

/**
 * 金币类
 * 继承Entity，支持收集状态和旋转绘制
 */
class Coin : public Entity {
public:
    Coin();
    Coin(float x, float y, float r);

    void Draw() const override;

    bool IsCollected() const;
    void Collect();
    float GetRadius() const;

    bool operator==(const Coin& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Coin& c);

private:
    float radius;    // 金币半径
    bool collected;  // 是否已被收集
};

/**
 * 护盾道具类
 * 收集后为玩家提供无敌状态
 */
class Shield : public Entity {
public:
    Shield();
    Shield(float x, float y, float w, float h);

    void Draw() const override;

    bool IsCollected() const;
    void Collect();

    bool operator==(const Shield& other) const;
    bool operator!=(const Shield& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Shield& s);

private:
    bool collected;  // 是否已被收集
};

/**
 * 浮动文字类
 * 用于显示得分增加等临时文字效果
 * 自动随时间上移并淡出
 */
class FloatingText : public Entity {
public:
    FloatingText();
    FloatingText(float x, float y, const std::string& t, Color c);

    void Draw() const override;
    void Update(float dt) override;

    bool IsExpired() const;  // 是否已过期（自动移除）

    bool operator==(const FloatingText& other) const;
    bool operator!=(const FloatingText& other) const;

    friend std::ostream& operator<<(std::ostream& os, const FloatingText& ft);

private:
    std::string text;     // 显示文本
    float lifeTime;       // 剩余显示时间
    Color color;          // 文本颜色
    float moveOffsetY;    // 上移偏移量
};

/**
 * 障碍物管理器类
 * 负责所有障碍物、金币、护盾、浮动文字的生命周期管理
 * 核心功能：生成、移动、碰撞检测、得分计算
 */
class ObstacleManager {
public:
    ObstacleManager();
    ~ObstacleManager();

    // 更新所有对象状态（移动、碰撞检测），返回true表示游戏结束
    bool Update(float speed, int screenWidth, int screenHeight, Player& player, int& score);
    void Draw() const;   // 绘制所有对象
    void Reset();        // 重置所有状态

    int GetObstacleCount() const;
    int GetCoinCount() const;

    // 运算符重载
    ObstacleManager& operator+=(const Obstacle& o);
    ObstacleManager& operator-=(const ObstacleType& t);

    bool operator==(const ObstacleManager& other) const;
    bool operator!=(const ObstacleManager& other) const;

    friend std::ostream& operator<<(std::ostream& os, const ObstacleManager& om);

private:
    // 对象容器（使用STL vector动态管理）
    std::vector<Obstacle> obstacles;       // 障碍物列表
    std::vector<Coin> coins;               // 金币列表
    std::vector<FloatingText> floatingTexts; // 浮动文字列表
    std::vector<Shield> shields;           // 护盾列表

    // 生成计时器
    float shieldSpawnTimer;   // 护盾生成计时
    float spawnTimer;         // 障碍物生成计时
    float spawnInterval;      // 障碍物生成间隔
    float coinSpawnTimer;     // 金币生成计时

    // 连击系统
    int coinCombo;            // 金币连击计数
    float comboDisplayTimer;  // 连击显示计时

    // 纹理资源
    Texture2D texBox, texTwoBox, texStoneWall, texHighStoneWall;
    Texture2D texBird, texHammer, texLaser, texSpine;
    Texture2D texShield, texCoin;

    // 私有生成方法
    void SpawnObstacle(int screenWidth, int screenHeight, float groundLevel);
    void SpawnCoin(int screenWidth, int screenHeight, float groundLevel);
    void SpawnShield(int screenWidth, float groundLevel);

    // 私有绘制方法
    void DrawObstacles() const;
    void DrawCoins() const;
    void DrawShields() const;
    void DrawFloatingTexts() const;
    void DrawCombo() const;
};
