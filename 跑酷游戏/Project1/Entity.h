// ============================================================
// Entity.h — 游戏实体基类与接口
// 功能: 定义所有游戏对象的基类（Entity），包含位置、碰撞箱、
//       状态管理；提供ICollidable（碰撞接口）和IRenderable
//       （渲染接口）演示C++的纯虚函数与多重继承
// ============================================================
#pragma once
#include <raylib.h>
#include <iostream>

/**
 * 可碰撞接口（多重继承演示）
 * 定义所有可碰撞对象的公共接口
 * 纯虚函数：子类必须实现GetCollisionRect()
 */
class ICollidable {
public:
    virtual ~ICollidable() = default;
    virtual Rectangle GetCollisionRect() const = 0;  // 纯虚函数
    virtual bool CheckCollision(const ICollidable& other) const {
        return CheckCollisionRecs(GetCollisionRect(), other.GetCollisionRect());
    }
};

/**
 * 可渲染接口（多重继承演示）
 * 定义所有可渲染对象的公共接口
 * 纯虚函数：子类必须实现Draw()
 */
class IRenderable {
public:
    virtual ~IRenderable() = default;
    virtual void Draw() const = 0;  // 纯虚函数
};

/**
 * 游戏实体基类
 * 提供所有游戏对象的通用属性和行为：
 * 位置、尺寸、活动状态、运算符重载
 * 子类可重写Update()和Draw()实现多态行为
 */
class Entity : public IRenderable {
public:
    // 构造函数（含默认参数——函数重载的一种形式）
    Entity(float x = 0.0f, float y = 0.0f, float w = 0.0f, float h = 0.0f)
        : posX(x), posY(y), width(w), height(h), active(true) {}

    // 虚析构函数（确保派生类对象正确析构）
    virtual ~Entity() = default;

    // 可重写的更新函数（虚函数，体现多态）
    virtual void Update(float dt) {}

    // 绘制函数（覆写自IRenderable接口）
    void Draw() const override {
        DrawRectangleRec(GetRect(), GRAY);
    }

    // 获取碰撞矩形（虚函数，可被派生类重写）
    virtual Rectangle GetRect() const {
        return { posX, posY, width, height };
    }

    // ---- getter/setter 封装 ----
    bool IsActive() const { return active; }
    void SetActive(bool a) { active = a; }

    float GetX() const { return posX; }
    float GetY() const { return posY; }
    void SetPos(float x, float y) { posX = x; posY = y; }

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }
    void SetSize(float w, float h) { width = w; height = h; }

    // ---- 运算符重载 ----
    bool operator==(const Entity& other) const {
        Rectangle a = GetRect();
        Rectangle b = other.GetRect();
        return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
    }

    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }

    Entity& operator=(const Entity& other) {
        if (this != &other) {  // 防止自赋值
            posX = other.posX;
            posY = other.posY;
            width = other.width;
            height = other.height;
            active = other.active;
        }
        return *this;
    }

    // 友元函数：输出实体信息到输出流
    friend std::ostream& operator<<(std::ostream& os, const Entity& e) {
        Rectangle r = e.GetRect();
        os << "Entity(" << r.x << ", " << r.y << ", "
           << static_cast<int>(r.width) << "x" << static_cast<int>(r.height) << ")";
        return os;
    }

protected:
    // 使用protected而非private，允许派生类直接访问
    float posX, posY;    // 位置坐标
    float width, height; // 尺寸
    bool active;         // 活动状态
};
