// Entities.cpp —— 敌人 / Boss / NPC / 弹幕 / 掉落物
#include "Entities.h"

#include "Assets.h"
#include "Audio.h"
#include "Game.h"
#include "Items.h"
#include "Noise.h"
#include "Player.h"
#include "Text.h"
#include "Tiles.h"
#include "World.h"

#include <algorithm>
#include <cstdio>

namespace tc {

namespace {

// 简易 AABB 瓦片碰撞
void moveEntity(World& w, Entity& e, float dt, bool gravity, bool platforms)
{
    if (gravity) {
        e.vel.y += GRAVITY * 0.72f * dt;
        if (e.vel.y > MAX_FALL) e.vel.y = MAX_FALL;
    }
    Rectangle r = e.rect();
    e.onGround = false;

    r.x += e.vel.x * dt;
    if (e.vel.x != 0.0f) {
        const int step = (int)(e.vel.x > 0 ? floorf((r.x + r.width - 0.01f) / TILE) : floorf(r.x / TILE));
        const int y0 = (int)floorf(r.y / TILE), y1 = (int)floorf((r.y + r.height - 0.01f) / TILE);
        for (int ty = y0; ty <= y1; ty++) {
            if (w.solidAt(step, ty)) {
                r.x = (e.vel.x > 0) ? (float)(step * TILE - r.width - 0.01f) : (float)((step + 1) * TILE + 0.01f);
                e.vel.x = 0;
                break;
            }
        }
    }

    r.y += e.vel.y * dt;
    if (e.vel.y != 0.0f) {
        const int x0 = (int)floorf(r.x / TILE), x1 = (int)floorf((r.x + r.width - 0.01f) / TILE);
        if (e.vel.y > 0) {
            const int step = (int)floorf((r.y + r.height - 0.01f) / TILE);
            for (int tx = x0; tx <= x1; tx++) {
                if (w.solidAt(tx, step) || (platforms && w.platformAt(tx, step))) {
                    r.y = (float)(step * TILE - r.height - 0.01f);
                    e.vel.y = 0;
                    e.onGround = true;
                    break;
                }
            }
        } else {
            const int step = (int)floorf(r.y / TILE);
            for (int tx = x0; tx <= x1; tx++) {
                if (w.solidAt(tx, step)) {
                    r.y = (float)((step + 1) * TILE + 0.01f);
                    e.vel.y = 0;
                    break;
                }
            }
        }
    }
    e.pos.x = r.x;
    e.pos.y = r.y;
}

bool overSolid(World& w, float x, float y) { return w.solidAt((int)floorf(x / TILE), (int)floorf(y / TILE)); }

const char* slimeSprite(int v)
{
    switch (v) {
    case 1: return "slime_green";
    case 2: return "slime_purple";
    case 3: return "slime_red";
    default: return "slime_blue";
    }
}

}  // namespace

Entity& Entities::spawn(EntityType t, float x, float y)
{
    Entity e;
    e.type = t;
    e.pos = { x, y };
    switch (t) {
    case E_SLIME:
        e.size = { 28, 20 }; e.hp = e.maxHp = 28; e.contactDamage = 6;
        e.knockResist = 0.0f; e.variant = 0; break;
    case E_ZOMBIE:
        e.size = { 22, 42 }; e.hp = e.maxHp = 46; e.contactDamage = 15;
        e.knockResist = 0.15f; break;
    case E_DEMON_EYE:
        e.size = { 24, 24 }; e.hp = e.maxHp = 42; e.contactDamage = 17;
        e.knockResist = 0.1f; break;
    case E_SERVANT:
        e.size = { 15, 15 }; e.hp = e.maxHp = 10; e.contactDamage = 11;
        e.knockResist = 0.0f; break;
    case E_KING_SLIME:
        e.size = { 92, 60 }; e.hp = e.maxHp = 2000; e.contactDamage = 34;
        e.knockResist = 1.0f; e.isBoss = true; break;
    case E_EYE_CTHULHU:
        e.size = { 80, 80 }; e.hp = e.maxHp = 2800; e.contactDamage = 40;
        e.knockResist = 1.0f; e.isBoss = true; break;
    case E_NPC_GUIDE:
        e.size = { 22, 38 }; e.hp = e.maxHp = 100; e.friendly = true; break;
    case E_NPC_MERCHANT:
        e.size = { 24, 40 }; e.hp = e.maxHp = 100; e.friendly = true; break;
    case E_PROJECTILE:
        e.size = { 8, 4 }; break;
    case E_DROP:
        e.size = { 12, 12 }; break;
    }
    m_e.push_back(e);
    return m_e.back();
}

void Entities::removeDead()
{
    m_e.erase(std::remove_if(m_e.begin(), m_e.end(), [](const Entity& e) { return !e.active; }), m_e.end());
}

Entity* Entities::findBoss()
{
    for (Entity& e : m_e) if (e.active && e.isBoss) return &e;
    return nullptr;
}

Entity* Entities::findNpc(int kind)
{
    const EntityType want = kind == 0 ? E_NPC_GUIDE : E_NPC_MERCHANT;
    for (Entity& e : m_e) if (e.active && e.type == want) return &e;
    return nullptr;
}

Entity* Entities::findNpcNear(float x, float y, float r)
{
    float best = r * r;
    Entity* out = nullptr;
    for (Entity& e : m_e) {
        if (!e.active || (e.type != E_NPC_GUIDE && e.type != E_NPC_MERCHANT)) continue;
        const Vector2 c = e.center();
        const float d = (c.x - x) * (c.x - x) + (c.y - y) * (c.y - y);
        if (d < best) { best = d; out = &e; }
    }
    return out;
}

void Entities::update(World& w, Player& p, Game& g, float dt)
{
    const Vector2 pc = p.center();

    for (Entity& e : m_e) {
        if (!e.active) continue;
        if (e.hurtTimer > 0) e.hurtTimer -= dt;
        if (e.immuneTimer > 0) e.immuneTimer -= dt;
        e.animTimer += dt * e.animSpeed;

        switch (e.type) {
        case E_PROJECTILE: updateProjectile(e, w, p, g, dt); break;
        case E_DROP: updateDrop(e, w, p, g, dt); break;
        default: updateAI(e, w, p, g, dt); break;
        }

        // 远离玩家则消失(首领与 NPC 除外)
        if (!e.isBoss && e.type != E_NPC_GUIDE && e.type != E_NPC_MERCHANT && e.type != E_PROJECTILE) {
            const float dx = e.center().x - pc.x, dy = e.center().y - pc.y;
            if (dx * dx + dy * dy > 1900.0f * 1900.0f) e.active = false;
        }
        if (e.projLife > 0.0f) {
            e.projLife -= dt;
            if (e.projLife <= 0) e.active = false;
        }
    }
    removeDead();
}

// ---------------------------------------------------------------- AI
void Entities::updateAI(Entity& e, World& w, Player& p, Game& g, float dt)
{
    const Vector2 pc = p.center();
    const Vector2 ec = e.center();
    const float dx = pc.x - ec.x, dy = pc.y - ec.y;
    const float dist = len(dx, dy);
    const bool inRange = dist < 1600.0f;   // 索敌范围要大于刷怪距离, 否则怪会站着不动

    switch (e.type) {

    case E_SLIME: {
        e.aiTimer -= dt;
        if (e.onGround) {
            e.vel.x = approach(e.vel.x, 0.0f, 340.0f * dt);
            if (e.aiTimer <= 0.0f && inRange) {
                e.vel.y = -238.0f - e.variant * 18.0f;
                e.vel.x = (dx > 0 ? 1 : -1) * (70.0f + e.variant * 26.0f);
                e.aiTimer = 0.85f + rand01(hash3((int)ec.x, (int)ec.y, e.maxHp, 13u)) * 0.7f;
                g.parts().spawn(PK_DUST, ec.x, e.pos.y + e.size.y, 3, rgb(180, 200, 180), 50, 2.2f, 1.8f, 0.3f);
            }
        }
        moveEntity(w, e, dt, true, false);
        e.facing = e.vel.x < -2 ? -1 : (e.vel.x > 2 ? 1 : e.facing);
        break;
    }

    case E_ZOMBIE: {
        e.facing = dx > 0 ? 1 : -1;
        const float spd = 62.0f;
        e.vel.x = approach(e.vel.x, e.facing * spd, 330.0f * dt);
        if (e.onGround) {
            // 前方有障碍就跳
            const float aheadX = e.pos.x + (e.facing > 0 ? e.size.x + 3 : -3);
            const float footY = e.pos.y - 3;
            if ((overSolid(w, aheadX, footY - 6) || overSolid(w, aheadX, footY - 20)) && fabsf(dx) < 260.0f)
                e.vel.y = -250.0f;
            else if (fabsf(e.vel.x) < 8.0f && inRange) e.vel.y = -230.0f;
        }
        moveEntity(w, e, dt, true, false);
        break;
    }

    case E_DEMON_EYE:
    case E_SERVANT: {
        const float spd = (e.type == E_SERVANT) ? 118.0f : 74.0f;
        e.aiTimer -= dt;
        if (e.aiState == 0) {
            float wob = sinf(e.animTimer * 3.4f) * 0.5f;
            const float a = atan2f(dy, dx) + wob;
            e.vel.x = approach(e.vel.x, cosf(a) * spd, 190.0f * dt);
            e.vel.y = approach(e.vel.y, sinf(a) * spd, 190.0f * dt);
            if (inRange && e.aiTimer <= 0.0f) { e.aiState = 1; e.aiTimer = 0.55f; }
        } else {
            e.vel.x = approach(e.vel.x, signf(dx) * 300.0f, 620.0f * dt);
            e.vel.y = approach(e.vel.y, signf(dy) * 250.0f, 620.0f * dt);
            if (e.aiTimer <= 0.0f) {
                e.aiState = 0;
                e.aiTimer = 1.2f + rand01(hash3((int)ec.x, e.maxHp, (int)ec.y, 91u)) * 1.4f;
            }
        }
        // 撞墙反弹
        const float aheadX = e.pos.x + (e.vel.x > 0 ? e.size.x + 2 : -2);
        if (overSolid(w, aheadX, ec.y)) e.vel.x *= -0.8f;
        if (overSolid(w, ec.x, e.pos.y + (e.vel.y > 0 ? e.size.y + 2 : -2))) e.vel.y *= -0.8f;
        e.pos.x += e.vel.x * dt;
        e.pos.y += e.vel.y * dt;
        e.facing = e.vel.x < 0 ? -1 : 1;
        break;
    }

    case E_KING_SLIME: {
        e.aiTimer -= dt;
        e.specialTimer -= dt;
        if (e.onGround) {
            e.vel.x = approach(e.vel.x, 0.0f, 420.0f * dt);
            if (e.aiTimer <= 0.0f) {
                const float kx = clampf(dx, -260.0f, 260.0f);
                e.vel.x = kx;
                e.vel.y = -430.0f;
                e.aiTimer = 1.15f + rand01(hash3((int)ec.x, (int)ec.y, 7, 33u)) * 0.5f;
                g.shake(2.2f);
            }
        }
        moveEntity(w, e, dt, true, false);
        // 召唤小史莱姆
        if (e.specialTimer <= 0.0f && e.hp < e.maxHp * 0.7f) {
            e.specialTimer = 6.0f;
            for (int k = 0; k < 2; k++) {
                Entity& s = spawn(E_SLIME, e.pos.x + (k ? 30 : -30), e.pos.y - 20);
                s.variant = 2;
                s.hp = s.maxHp = 55;
                s.contactDamage = 18;
                s.vel.y = -180.0f;
                s.vel.x = (k ? 90 : -90);
            }
            g.addMessage(loc::get(loc::S_SLIME), rgb(150, 220, 150));
        }
        break;
    }

    case E_EYE_CTHULHU: {
        const bool phase2 = e.hp < e.maxHp * 0.5f;
        e.variant = phase2 ? 1 : 0;
        e.contactDamage = phase2 ? 56 : 40;
        const float spd = phase2 ? 190.0f : 105.0f;
        e.aiTimer -= dt;
        e.specialTimer -= dt;

        if (e.aiState == 0) {
            // 悬停追击
            const float tx = pc.x, ty = pc.y - (phase2 ? 40.0f : 110.0f);
            const float ox = tx - e.center().x, oy = ty - e.center().y;
            e.vel.x = approach(e.vel.x, clampf(ox * 2.4f, -spd, spd), 420.0f * dt);
            e.vel.y = approach(e.vel.y, clampf(oy * 2.4f, -spd, spd), 420.0f * dt);
            if (e.aiTimer <= 0.0f && inRange) {
                e.aiState = 1;
                e.aiTimer = phase2 ? 0.85f : 1.15f;
                e.vel.x = signf(dx) * (phase2 ? 520.0f : 340.0f);
                e.vel.y = signf(dy) * (phase2 ? 420.0f : 260.0f);
            }
        } else {
            e.vel.x = approach(e.vel.x, 0.0f, 300.0f * dt);
            e.vel.y = approach(e.vel.y, 0.0f, 300.0f * dt);
            if (e.aiTimer <= 0.0f) { e.aiState = 0; e.aiTimer = (phase2 ? 0.5f : 1.0f) + rand01(hash3((int)ec.x, (int)ec.y, 5, 77u)) * 0.6f; }
        }
        // 召唤仆从
        if (phase2 && e.specialTimer <= 0.0f) {
            e.specialTimer = 5.5f;
            for (int k = 0; k < 2; k++) {
                Entity& s = spawn(E_SERVANT, e.pos.x + (k ? 20 : -20), e.pos.y + 10);
                s.vel = { (k ? 1 : -1) * 130.0f, -60.0f };
            }
        }
        // 撞地形则停下
        const float aheadX = e.pos.x + signf(e.vel.x) * (e.size.x * .5f + 2);
        if (overSolid(w, aheadX, e.center().y)) e.vel.x *= -0.5f;
        const float aheadY = e.pos.y + signf(e.vel.y) * (e.size.y * .5f + 2);
        if (overSolid(w, e.center().x, aheadY)) e.vel.y *= -0.5f;
        e.pos.x += e.vel.x * dt;
        e.pos.y += e.vel.y * dt;
        e.facing = e.vel.x < 0 ? -1 : 1;
        break;
    }

    case E_NPC_GUIDE:
    case E_NPC_MERCHANT: {
        // 面向玩家, 偶尔挪两步
        e.facing = dx > 0 ? 1 : -1;
        e.aiTimer -= dt;
        e.vel.x = approach(e.vel.x, 0.0f, 200.0f * dt);
        if (e.aiTimer <= 0.0f && dist < 200.0f) {
            e.aiTimer = 2.0f + rand01(hash3((int)ec.x, (int)ec.y, 3, 5u)) * 3.0f;
            e.vel.x = (rand01(hash3((int)ec.y, e.maxHp, 1, 9u)) < 0.5f ? -1 : 1) * 34.0f;
        }
        moveEntity(w, e, dt, true, false);
        break;
    }

    default: break;
    }

    // 接触伤害
    if (!e.friendly && e.contactDamage > 0 && e.immuneTimer <= 0 && !p.dead) {
        if (aabb(p.rect().x, p.rect().y, p.rect().width, p.rect().height, e.pos.x, e.pos.y, e.size.x, e.size.y)) {
            p.takeDamage(e.contactDamage, signf(p.center().x - e.center().x) * 140.0f, -110.0f, g);
            e.immuneTimer = 0.35f;
        }
    }
}

// ---------------------------------------------------------------- 弹幕
void Entities::updateProjectile(Entity& e, World& w, Player& p, Game& g, float dt)
{
    e.pos.x += e.vel.x * dt;
    e.pos.y += e.vel.y * dt;
    if (e.projKind == 1) e.vel.y += 320.0f * dt;          // 箭有重力

    // 命中地形
    if (overSolid(w, e.center().x, e.center().y)) {
        g.parts().spawn(PK_DUST, e.center().x, e.center().y, 4, rgb(210, 200, 180), 70, 3.14f, 1.8f, 0.25f);
        e.active = false;
        return;
    }
    // 命中目标
    if (e.projOwner == 0) {
        for (Entity& t : m_e) {
            if (!t.active || t.friendly) continue;
            if (t.type == E_NPC_GUIDE || t.type == E_NPC_MERCHANT || t.type == E_DROP) continue;
            if (!aabb(t.pos.x, t.pos.y, t.size.x, t.size.y, e.pos.x, e.pos.y, e.size.x, e.size.y)) continue;
            t.hp -= e.projDamage;
            t.hurtTimer = 0.24f;
            t.vel.x += signf(e.vel.x) * (t.isBoss ? 20.0f : 120.0f);
            t.vel.y -= 50.0f;
            g.damageNumber(t.center().x, t.center().y - 6, e.projDamage, false);
            g.parts().spawn(PK_SPARK, e.center().x, e.center().y, 6, e.projColor, 110, 3.14f, 2.0f, 0.3f);
            if (t.hp <= 0 && !t.isBoss) { g.dropLoot(t); t.active = false; }
            e.active = false;
            break;
        }
    } else if (!p.dead && aabb(p.rect().x, p.rect().y, p.rect().width, p.rect().height, e.pos.x, e.pos.y, e.size.x, e.size.y)) {
        p.takeDamage(e.projDamage, signf(e.vel.x) * 90.0f, -60.0f, g);
        e.active = false;
    }
}

// ---------------------------------------------------------------- 掉落物
void Entities::updateDrop(Entity& e, World& w, Player& p, Game& g, float dt)
{
    e.vel.y += GRAVITY * 0.55f * dt;
    if (e.vel.y > 400) e.vel.y = 400;
    moveEntity(w, e, dt, false, false);
    if (e.onGround) { e.vel.x = approach(e.vel.x, 0.0f, 420.0f * dt); e.vel.y = 0; }
    e.bob += dt * 3.0f;

    // 被玩家吸引
    const Vector2 pc = p.center();
    const Vector2 ec = e.center();
    const float d = len(pc.x - ec.x, pc.y - ec.y);
    if (!p.dead && d < 92.0f) {
        const float pull = 620.0f * (1.0f - d / 92.0f);
        e.vel.x += ((pc.x - ec.x) / (d + 1.0f)) * pull * dt;
        e.vel.y += ((pc.y - ec.y) / (d + 1.0f)) * pull * dt;
    }
    if (!p.dead && d < 13.0f) {
        if (p.give(e.itemId, e.itemCount)) {
            audio::play(SFX_PICKUP, 0.45f, 0.92f + rand01(hash2((int)e.pos.x, (int)e.pos.y, 17u)) * 0.22f);
            g.addMessage(std::string(itemName(e.itemId)) + " x" + std::to_string(e.itemCount), rgb(235, 235, 235));
            e.active = false;
        } else {
            g.addMessage(loc::get(loc::S_FULL), rgb(240, 140, 140));
        }
    }
}

// ---------------------------------------------------------------- 环境刷怪
void Entities::trySpawnAmbient(World& w, Player& p, Game& g, float dt)
{
    g.m_spawnAcc += dt;
    if (g.m_spawnAcc < 1.8f) return;
    g.m_spawnAcc = 0.0f;

    int enemies = 0, drops = 0;
    for (const Entity& e : m_e) {
        if (e.type == E_DROP) drops++;
        else if (!e.friendly && e.type != E_PROJECTILE && e.type != E_NPC_GUIDE && e.type != E_NPC_MERCHANT) enemies++;
    }
    if (enemies >= 16 || drops >= 60) return;

    const Vector2 pc = p.center();
    const int py = (int)(pc.y / TILE);
    const bool night = g.isNight();
    const bool underground = py > w.surfaceY((int)(pc.x / TILE)) + 12;
    const bool hell = py > UNDERWORLD_Y - 20;
    const int biome = w.biomeAt((int)(pc.x / TILE));

    // 随机挑一个屏幕外的生成点
    for (int attempt = 0; attempt < 6; attempt++) {
        const float side = rand01(hash3((int)pc.x, (int)pc.y, g.frameCounter(), 555u)) < 0.5f ? -1.0f : 1.0f;
        const float dist = 520.0f + rand01(hash3((int)pc.y, g.frameCounter(), 3, 77u)) * 420.0f;
        const float sx = pc.x + side * dist;
        const int cxs = (int)(sx / TILE);

        if (hell) return;                       // 地狱不刷普通怪
        if (underground) {
            const int gy = w.surfaceHeight(cxs) + 20;
            if (rand01(hash3(cxs, g.frameCounter(), 11, 3u)) < 0.5f) {
                Entity& e = spawn(E_ZOMBIE, sx, (float)(gy * TILE));
                (void)e;
            } else {
                Entity& e = spawn(E_DEMON_EYE, sx, (float)((gy + 6) * TILE));
                e.vel.x = -side * 60;
            }
            return;
        }

        // 地表
        const int gy = w.surfaceHeight(cxs);
        if (!night) return;                     // 白天地表安静
        if (biome == BIOME_OCEAN) return;
        const float r = rand01(hash3(cxs, g.frameCounter(), 5, 101u));
        if (biome == BIOME_JUNGLE || biome == BIOME_CORRUPTION) {
            if (r < 0.55f) {
                Entity& e = spawn(E_SLIME, sx, (float)(gy * TILE));
                e.variant = biome == BIOME_CORRUPTION ? 2 : 1;
                e.hp = e.maxHp = 24 + e.variant * 24;
                e.contactDamage = 6 + e.variant * 4;
            } else {
                Entity& e = spawn(E_DEMON_EYE, sx, (float)((gy - 3) * TILE));
                e.vel.x = -side * 50;
            }
        } else if (r < 0.62f) {
            Entity& e = spawn(E_SLIME, sx, (float)(gy * TILE));
            e.variant = night ? 0 : 1;
            e.hp = e.maxHp = 28 + e.variant * 18;
            e.contactDamage = 6 + e.variant * 3;
        } else {
            Entity& e = spawn(E_DEMON_EYE, sx, (float)((gy - 3) * TILE));
            e.vel.x = -side * 55;
        }
        return;
    }
}

// ---------------------------------------------------------------- 绘制
void Entities::draw(World& w, float x0, float y0, float x1, float y1)
{
    for (Entity& e : m_e) {
        if (!e.active) continue;
        if (e.pos.x + e.size.x < x0 || e.pos.x > x1) continue;   // 可视范围外
        if (e.pos.y + e.size.y < y0 || e.pos.y > y1) continue;
        drawEntity(e, w);
    }
}

void Entities::drawEntity(Entity& e, World& w)
{
    Color tint = WHITE;
    if (e.hurtTimer > 0) tint = rgb(255, 150, 150);
    const Vector2 c = e.center();
    const float footY = e.pos.y + e.size.y;
    const Frame* f = nullptr;

    switch (e.type) {
    case E_SLIME: {
        f = Assets::Find(slimeSprite(e.variant));
        // 压扁拉伸
        float sy = 1.0f, sx = 1.0f;
        if (!e.onGround) { sy = 1.12f; sx = 0.9f; }
        else if (e.vel.y > -30.0f) { sy = 0.78f; sx = 1.2f; }
        if (f) {
            const float h = f->rect.height * sy, wd = f->rect.width * sx;
            drawFrame(*f, c.x - wd * 0.5f, footY - h, false, tint);
        }
        break;
    }
    case E_ZOMBIE: {
        char buf[32];
        snprintf(buf, sizeof(buf), "zombie_walk_%d", ((int)(e.animTimer * 1.4f) % 4));
        f = Assets::Find(buf);
        if (f) drawFrame(*f, e.pos.x + e.size.x * .5f - f->rect.width * .5f, footY - f->rect.height, e.facing < 0, tint);
        break;
    }
    case E_DEMON_EYE: {
        const int n = (int)(e.animTimer * 1.6f) % 3;
        char buf[32];
        snprintf(buf, sizeof(buf), "demon_eye_%d", n);
        f = Assets::Find(buf);
        if (f) drawFrameC(*f, c.x, c.y, e.facing < 0, tint);
        break;
    }
    case E_SERVANT: {
        const int n = (int)(e.animTimer * 2.2f) % 3;
        char buf[32];
        snprintf(buf, sizeof(buf), "servant_%d", n);
        f = Assets::Find(buf);
        if (f) drawFrameC(*f, c.x, c.y, e.facing < 0, tint);
        break;
    }
    case E_KING_SLIME: {
        f = Assets::Find("king_slime");
        float sy = 1.0f, sx = 1.0f;
        if (e.onGround && fabsf(e.vel.x) < 12.0f) { sy = 0.82f; sx = 1.14f; }
        else if (e.vel.y < -20.0f) { sy = 1.16f; sx = 0.9f; }
        if (f) {
            const float h = f->rect.height * sy, wd = f->rect.width * sx;
            drawFrame(*f, c.x - wd * 0.5f, footY - h, false, tint);
        }
        break;
    }
    case E_EYE_CTHULHU: {
        const bool p2 = e.hp < e.maxHp * 0.5f;
        const int n = p2 ? (2 + ((int)(e.animTimer * 1.5f) % 2)) : ((int)(e.animTimer * 1.2f) % 2);
        char buf[32];
        snprintf(buf, sizeof(buf), "eye_of_cthulhu_%d", n);
        f = Assets::Find(buf);
        if (f) {
            const float bob = sinf(e.animTimer * 2.0f) * 2.0f;
            drawFrameC(*f, c.x, c.y + bob, e.facing < 0, tint);
        }
        break;
    }
    case E_NPC_GUIDE:
    case E_NPC_MERCHANT: {
        const int n = (int)(e.animTimer * 0.8f) % 4;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s_%d", e.type == E_NPC_GUIDE ? "npc_guide" : "npc_merchant", n);
        f = Assets::Find(buf);
        if (f) drawFrame(*f, e.pos.x + e.size.x * .5f - f->rect.width * .5f, footY - f->rect.height, e.facing < 0, tint);
        break;
    }
    case E_PROJECTILE: {
        if (e.projKind == 1) {
            const float a = atan2f(e.vel.y, e.vel.x);
            const Frame* af = Assets::Find("bow");
            (void)af;
            const float len = 14.0f;
            DrawRectangleRec({ c.x - len * .5f, c.y - 1.0f, len, 2.0f }, e.projColor);
            DrawRectangleRec({ c.x + signf(e.vel.x) * len * .35f, c.y - 2.0f, 5.0f, 4.0f }, rgb(230, 230, 230));
            (void)a;
        } else {
            DrawRectangleRec({ c.x - e.projSize * .5f, c.y - e.projSize * .5f, e.projSize, e.projSize }, e.projColor);
            DrawRectangleRec({ c.x - e.projSize, c.y - e.projSize, e.projSize * 2, e.projSize * 2 },
                             ColorAlpha(e.projColor, 60));
        }
        break;
    }
    case E_DROP: {
        const ItemDef& d = itemDef(e.itemId);
        const float bob = sinf(e.bob) * 2.0f;
        const Frame* it = Assets::FindItem(d.sprite);
        if (it) {
            const float y = c.y + bob;
            DrawRectangleRec({ e.pos.x - 2, y + 4, 16, 3 }, ColorAlpha(rgb(0, 0, 0), 60));
            drawFrameC(*it, c.x, y, false, WHITE);
        }
        break;
    }
    default: break;
    }
}

}  // namespace tc


