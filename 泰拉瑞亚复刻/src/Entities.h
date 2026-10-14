// Entities.h —— 敌人 / Boss / NPC / 弹幕 / 掉落物
#pragma once

#include "Common.h"
#include "Items.h"

#include <vector>

namespace tc {

class World;
class Player;
class Game;

enum EntityType {
    E_SLIME = 0, E_ZOMBIE, E_DEMON_EYE, E_SERVANT,
    E_KING_SLIME, E_EYE_CTHULHU,
    E_NPC_GUIDE, E_NPC_MERCHANT,
    E_PROJECTILE, E_DROP
};

struct Entity {
    EntityType type = E_SLIME;
    Vector2 pos, vel, size;
    int hp = 10, maxHp = 10;
    float hurtTimer = 0, immuneTimer = 0, animTimer = 0, animSpeed = 8.0f;
    int facing = 1;
    bool onGround = false, active = true, isBoss = false, friendly = false;
    int contactDamage = 5;
    float knockResist = 0.0f;
    int variant = 0;

    // 弹幕
    int projKind = 0;          // 0 无, 1 箭, 2 魔法珠, 3 Boss 泪
    int projDamage = 0;
    int projOwner = 0;         // 0 玩家 1 敌
    float projLife = 0.0f;   // 0 = 永不超时(普通实体); 弹幕/掉落物才设
    Color projColor = WHITE;
    float projSize = 4.0f;

    // 掉落物
    int itemId = 0, itemCount = 0;
    float bob = 0.0f;

    // AI
    bool gravity = false;
    int aiState = 0;
    float aiTimer = 0.0f;
    float specialTimer = 0.0f;
    bool grounded = false;

    Rectangle rect() const { return { pos.x, pos.y, size.x, size.y }; }
    Vector2 center() const { return { pos.x + size.x * .5f, pos.y + size.y * .5f }; }
};

class Entities {
public:
    void update(World& w, Player& p, Game& g, float dt);
    void draw(World& w, float x0, float y0, float x1, float y1);
    void clear() { m_e.clear(); }
    void removeDead();

    Entity& spawn(EntityType t, float x, float y);
    std::vector<Entity>& all() { return m_e; }
    const std::vector<Entity>& all() const { return m_e; }
    size_t count() const { return m_e.size(); }
    Entity* findBoss();
    Entity* findNpc(int kind);           // 0 向导 1 商人
    Entity* findNpcNear(float x, float y, float r);
    void trySpawnAmbient(World& w, Player& p, Game& g, float dt);

private:
    void updateAI(Entity& e, World& w, Player& p, Game& g, float dt);
    void updateProjectile(Entity& e, World& w, Player& p, Game& g, float dt);
    void updateDrop(Entity& e, World& w, Player& p, Game& g, float dt);
    void drawEntity(Entity& e, World& w);
    std::vector<Entity> m_e;
};

}  // namespace tc
