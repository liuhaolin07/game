// Player.h —— 玩家角色
#pragma once

#include "Common.h"
#include "Items.h"

namespace tc {

class World;
class Game;

struct ItemStack {
    int id = 0;
    int count = 0;
    ItemStack() {}
    ItemStack(int i, int c) : id(i), count(c) {}
    bool empty() const { return id == 0 || count <= 0; }
    void clear() { id = 0; count = 0; }
};

class Player {
public:
    void init();

    void update(World& w, Game& g, float dt);
    void draw(World& w, Game& g);
    void releaseBow(Game& g);

    // ---- 状态 ----
    Vector2 pos{ 0, 0 }, vel{ 0, 0 };    // pos = 脚底中心
    int hp = 100, maxHp = 100;
    int mana = 20, maxMana = 20;
    int coins = 0;
    bool onGround = false, dead = false;
    int facing = 1;      // 1 右 / -1 左
    float hurtTimer = 0, invulnTimer = 0;
    float animTimer = 0;
    int animFrame = 0;
    bool inWater = false, inLava = false;
    bool m_wasInWater = false;

    ItemStack inv[INV_SLOTS];
    int selected = 0;
    int equip[4] = { 0, 0, 0, 0 };       // EQ_HELM / EQ_ARMOR / EQ_ACC1 / EQ_ACC2

    // ---- 交互 ----
    int mineX = -1, mineY = -1, mineWall = 0;
    float mineProgress = 0;
    float useTimer = 0;                   // 药水/召唤蓄力
    int useItem = 0;
    float swingTimer = 0;                 // 挥剑动画
    int swingDir = 1;
    bool bowCharging = false;
    float bowCharge = 0;
    bool itemUsedThisTick = false;

    Rectangle rect() const { return { pos.x - PLAYER_W * .5f, pos.y - PLAYER_H, PLAYER_W, PLAYER_H }; }
    Rectangle headRect() const { return { pos.x - 7, pos.y - PLAYER_H, 14, 14 }; }
    float top() const { return pos.y - PLAYER_H; }
    Vector2 center() const { return { pos.x, pos.y - PLAYER_H * .5f }; }

    // ---- 背包 ----
    int held() const { return inv[selected].id; }
    int heldCount() const { return inv[selected].count; }
    int countItem(int id) const;
    bool hasItem(int id, int n = 1) const { return countItem(id) >= n; }
    bool give(int id, int count);
    void consume(int id, int count = 1);
    void addToSlot(int slot, const ItemStack& s);
    void swapEquip(int slotIndex, int itemId);

    // ---- 属性 ----
    int totalDefense() const;
    float moveSpeed() const;
    float jumpPower() const;
    int baseDamage() const;
    float critChance() const;
    float critMult() const { return 2.0f; }

    // ---- 工具 ----
    bool hasTool() const;
    int toolPower() const;
    float toolSpeed() const;
    bool isAxe() const;
    bool isPick() const;

    void heal(int n);
    void restoreMana(int n);
    void takeDamage(int dmg, float kbx, float kby, Game& g);
    void respawn(World& w);

private:
    void physics(World& w, Game& g, float dt);
    void mining(World& w, Game& g, float dt);
    void useRight(World& w, Game& g);
    void useLeft(World& w, Game& g);
    void attackSwing(World& w, Game& g);
    void interact(World& w, Game& g, int tx, int ty);

    float m_regen = 0.0f;
    float m_stepSound = 0.0f;
    int m_frameJ = 0;
    float m_animState = 0;    // 0 站立 1 走 2 跳
    float m_stepTimer = 0;
    bool m_jumpHeld = false;
};

}  // namespace tc
