// Player.cpp —— 玩家: 移动 / 挖掘 / 放置 / 战斗
#include "Player.h"

#include "Assets.h"
#include "Audio.h"
#include "Entities.h"
#include "Game.h"
#include "Text.h"
#include "Tiles.h"

#include "Noise.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace tc {

namespace {
constexpr float MINE_SCALE = 0.32f;    // 硬度 -> 基础挖掘秒数系数

bool woodLike(uint16_t t)
{
    return t == T_WOOD || t == T_WOOD_PANEL || t == T_LEAVES || t == T_LEAVES_JUNGLE ||
           t == T_VINE || t == T_FENCE || t == T_PLATFORM || t == T_DOOR;
}
}  // namespace

void Player::init()
{
    pos = { 0, 0 };
    vel = { 0, 0 };
    hp = maxHp = 100;
    mana = maxMana = 20;
    dead = false;
    for (int i = 0; i < INV_SLOTS; i++) inv[i].clear();
    for (int i = 0; i < 4; i++) equip[i] = 0;
    selected = 0;
    // 初始物资
    give(IT_WOOD, 40);
    give(IT_TORCH, 20);
    give(IT_PICK_COPPER, 1);
    give(IT_AXE_COPPER, 1);
    give(IT_SWORD_COPPER, 1);
    give(IT_POTION_HEALTH, 3);
}

int Player::countItem(int id) const
{
    if (!id) return 0;
    int n = 0;
    for (int i = 0; i < INV_SLOTS; i++) if (inv[i].id == id) n += inv[i].count;
    return n;
}

bool Player::give(int id, int count)
{
    if (!id || count <= 0) return false;
    const int max = itemStackMax(id);
    for (int i = 0; i < INV_SLOTS; i++) {
        if (inv[i].id == id && inv[i].count < max) {
            const int add = std::min(max - inv[i].count, count);
            inv[i].count += add;
            count -= add;
            if (count <= 0) return true;
        }
    }
    for (int i = 0; i < INV_SLOTS; i++) {
        if (inv[i].empty()) {
            const int add = std::min(max, count);
            inv[i].id = id;
            inv[i].count = add;
            count -= add;
            if (count <= 0) return true;
        }
    }
    return count <= 0;
}

void Player::consume(int id, int count)
{
    if (!id || count <= 0) return;
    for (int i = INV_SLOTS - 1; i >= 0 && count > 0; i--) {
        if (inv[i].id != id) continue;
        const int take = std::min(inv[i].count, count);
        inv[i].count -= take;
        count -= take;
        if (inv[i].count <= 0) inv[i].clear();
    }
}

void Player::addToSlot(int slot, const ItemStack& s)
{
    if (slot < 0 || slot >= INV_SLOTS) return;
    inv[slot] = s;
}

int Player::totalDefense() const
{
    int d = 0;
    for (int i = 0; i < 4; i++) if (equip[i]) d += itemDef(equip[i]).defense;
    return d;
}

float Player::moveSpeed() const
{
    float s = WALK_MAX;
    for (int i = 0; i < 4; i++) if (equip[i]) s *= itemDef(equip[i]).speedMult;
    return s;
}

float Player::jumpPower() const
{
    float j = JUMP_SPEED;
    for (int i = 0; i < 4; i++) if (equip[i]) j *= 1.0f + (itemDef(equip[i]).speedMult - 1.0f) * 0.9f;
    return j;
}

bool Player::hasTool() const { return isPick() || isAxe(); }
bool Player::isPick() const { return itemDef(held()).kind == IK_PICK; }
bool Player::isAxe() const { return itemDef(held()).kind == IK_AXE; }
int Player::toolPower() const { return itemDef(held()).pickPower; }
float Player::toolSpeed() const { return itemDef(held()).pickSpeed; }

int Player::baseDamage() const
{
    const ItemDef& d = itemDef(held());
    int dmg = d.damage;
    for (int i = 0; i < 4; i++) {
        if (!equip[i]) continue;
        const ItemDef& e = itemDef(equip[i]);
        dmg += (int)(e.damage * 0.5f);
    }
    return dmg;
}

float Player::critChance() const
{
    float c = 0.04f;
    for (int i = 0; i < 4; i++) if (equip[i]) c += itemDef(equip[i]).crit;
    return c;
}

void Player::heal(int n)
{
    hp = std::min(hp + n, maxHp);
    mana = std::min(mana + n, maxMana);
}
void Player::restoreMana(int n) { mana = std::min(mana + n, maxMana); }

void Player::takeDamage(int dmg, float kbx, float kby, Game& g)
{
    if (dead || invulnTimer > 0) return;
    const int real = std::max(1, dmg - totalDefense());
    hp -= real;
    hurtTimer = 0.42f;
    invulnTimer = 0.95f;
    vel.x += kbx;
    vel.y = std::min(vel.y, 0.0f) + kby;
    audio::play(SFX_HURT, 0.8f, 1.0f - clampf((float)real / 260.0f, 0.0f, 0.25f));
    g.shake(3.0f);
    g.damageNumber(center().x, top() - 6, real, false);
    g.parts().spawn(PK_BLOOD, center().x, center().y, 8, rgb(190, 30, 30), 90.0f, 3.14f, 2.6f, 0.5f);
    if (hp <= 0) { hp = 0; dead = true; }
}

void Player::respawn(World& w)
{
    hp = maxHp;
    mana = maxMana / 2;
    dead = false;
    invulnTimer = 2.0f;
    hurtTimer = 0;
    // 出生点: 用纯函数 surfaceY() 定位, 即使区块还没生成也能算出正确位置
    int sx = 340;
    for (int t = 0; t < 900; t++) {
        const int x = sx + t;
        if (w.biomeAt(x) != BIOME_OCEAN && w.surfaceY(x) < OCEAN_Y) { sx = x; break; }
    }
    pos = { (float)(sx * TILE + TILE * 0.5f), (float)(w.surfaceY(sx) * TILE) };
    vel = { 0, 0 };
}

// ---------------------------------------------------------------- 主循环
void Player::update(World& w, Game& g, float dt)
{
    itemUsedThisTick = false;
    if (hurtTimer > 0) hurtTimer -= dt;
    if (invulnTimer > 0) invulnTimer -= dt;
    if (swingTimer > 0) swingTimer -= dt;
    if (dead) return;

    // 岩浆灼烧
    inLava = w.lavaAt((int)(pos.x / TILE), (int)((pos.y - 8) / TILE));
    if (inLava) {
        if (invulnTimer <= 0.0f) {
            invulnTimer = 0.45f;
            takeDamage(26, 0, -60, g);
        }
        g.parts().spawn(PK_FLAME, pos.x, pos.y - 20, 3, rgb(255, 170, 60), 60, 3.14f, 3.0f, 0.4f);
    }
    // 缓慢回血(脱战后)
    m_regen += dt;
    if (m_regen > 2.4f && hp < maxHp && !inLava) {
        m_regen = 0.0f;
        hp = std::min(maxHp, hp + 1);
    }

    if (inWater && !m_wasInWater) audio::play(SFX_SPLASH, 0.6f, 0.95f + rand01(hash2((int)pos.x, 5, 9u)) * 0.15f);
    m_wasInWater = inWater;

    physics(w, g, dt);
    mining(w, g, dt);

    // 快捷栏
    for (int i = 0; i < 10; i++) {
        if (IsKeyPressed(KEY_ZERO + i)) g.player().selected = i;
    }
    float wheel = GetMouseWheelMove();
    if (wheel > 0) g.player().selected = (g.player().selected + HOTBAR_SLOTS - 1) % HOTBAR_SLOTS;
    else if (wheel < 0) g.player().selected = (g.player().selected + 1) % HOTBAR_SLOTS;

    // 使用类物品的蓄力进度
    if (useTimer > 0.0f) {
        useTimer -= dt;
        if (useTimer <= 0.0f) {
            const ItemDef& u = itemDef(useItem);
            if (u.kind == IK_CONSUMABLE) {
                if (useItem == IT_POTION_HEALTH) {
                    int before = hp;
                    hp = std::min(hp + 60, maxHp);
                    g.damageNumber(center().x, top() - 4, hp - before, false);
                } else {
                    mana = std::min(mana + 60, maxMana);
                }
                g.parts().spawn(PK_SPARK, center().x, center().y, 8, rgb(120, 255, 160), 80, 3.14f, 2.0f, 0.5f);
                consume(useItem, 1);
            }
            useItem = 0;
        }
    }

    // 攻击 / 使用
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) useLeft(w, g);
    else releaseBow(g);
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) useRight(w, g);
}

// ---------------------------------------------------------------- 物理
void Player::physics(World& w, Game& g, float dt)
{
    const bool left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    const bool right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    const bool down = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);

    // 水中判定
    const int cx = (int)(pos.x / TILE);
    inWater = w.liquidAt(cx, (int)((pos.y - PLAYER_H * 0.75f) / TILE)) ||
              w.liquidAt(cx, (int)((pos.y - 8) / TILE));
    if (inWater) inLava = w.lavaAt(cx, (int)((pos.y - 8) / TILE));

    const float speed = moveSpeed() * (inWater ? 0.62f : 1.0f);
    float target = 0.0f;
    if (left && !right) { target = -speed; facing = -1; }
    else if (right && !left) { target = speed; facing = 1; }

    const float accel = (onGround ? WALK_ACCEL : AIR_ACCEL) * dt;
    if (target != 0.0f) vel.x = approach(vel.x, target, accel);
    else vel.x = approach(vel.x, 0.0f, (onGround ? GROUND_FRICTION : AIR_FRICTION) * dt);

    // 跳跃
    if (IsKeyDown(KEY_SPACE)) {
        if (onGround && !m_jumpHeld && !inWater) {
            vel.y = -jumpPower();
            m_jumpHeld = true;
            audio::play(SFX_JUMP, 0.45f, 0.95f + rand01(hash2((int)pos.x, m_frameJ, 31u)) * 0.14f);
            m_frameJ++;
        } else if (inWater) {
            vel.y = approach(vel.y, -jumpPower() * 0.42f, 900.0f * dt);
        }
    } else {
        m_jumpHeld = false;
    }
    // 松开空格缩短跳跃
    if (!IsKeyDown(KEY_SPACE) && vel.y < -90.0f && !inWater) vel.y = -90.0f;

    vel.y += GRAVITY * (inWater ? 0.42f : 1.0f) * dt;
    if (vel.y > MAX_FALL) vel.y = MAX_FALL;

    // ---- 分轴碰撞 ----
    Rectangle r = rect();
    const bool wasOnGround = onGround;
    const float fallV = vel.y;
    onGround = false;

    // X 轴
    r.x += vel.x * dt;
    if (vel.x != 0.0f) {
        const int step = (int)(vel.x > 0 ? floorf((r.x + r.width - 0.01f) / TILE) : floorf(r.x / TILE));
        const int y0 = (int)floorf(r.y / TILE);
        const int y1 = (int)floorf((r.y + r.height - 0.01f) / TILE);
        for (int ty = y0; ty <= y1; ty++) {
            if (w.solidAt(step, ty)) {
                r.x = (vel.x > 0) ? (float)(step * TILE - r.width - 0.01f) : (float)((step + 1) * TILE + 0.01f);
                vel.x = 0;
                break;
            }
        }
    }

    // Y 轴
    r.y += vel.y * dt;
    if (vel.y != 0.0f) {
        const int x0 = (int)floorf(r.x / TILE);
        const int x1 = (int)floorf((r.x + r.width - 0.01f) / TILE);
        if (vel.y > 0) {
            const int step = (int)floorf((r.y + r.height - 0.01f) / TILE);
            for (int tx = x0; tx <= x1; tx++) {
                if (w.solidAt(tx, step) || (w.platformAt(tx, step) && !down)) {
                    r.y = (float)(step * TILE - r.height - 0.01f);
                    vel.y = 0;
                    onGround = true;
                    break;
                }
            }
        } else {
            const int step = (int)floorf(r.y / TILE);
            for (int tx = x0; tx <= x1; tx++) {
                if (w.solidAt(tx, step)) {
                    r.y = (float)((step + 1) * TILE + 0.01f);
                    vel.y = 0;
                    break;
                }
            }
        }
    }

    // 贴地检测(站上平台)
    if (!onGround && vel.y >= 0) {
        const int x0 = (int)floorf(r.x / TILE), x1 = (int)floorf((r.x + r.width - 0.01f) / TILE);
        const int step = (int)floorf((r.y + r.height + 1.0f) / TILE);
        for (int tx = x0; tx <= x1; tx++)
            if (w.solidAt(tx, step) || (w.platformAt(tx, step) && !down)) { onGround = true; break; }
    }

    pos.x = r.x + r.width * 0.5f;
    pos.y = r.y + r.height;
    if (pos.y > (float)(WORLD_H * TILE)) { pos.y = (float)(WORLD_H * TILE); takeDamage(500, 0, 0, g); }
    if (pos.y < (float)(SKY_TOP * TILE)) { pos.y = (float)(SKY_TOP * TILE); vel.y = 0; }

    // 走路动画
    if (onGround && fabsf(vel.x) > 8.0f) {
        m_animState = 1;
        m_stepTimer += fabsf(vel.x) * dt * 0.055f;
        m_stepSound -= fabsf(vel.x) * dt;
        if (m_stepSound <= 0.0f) {
            m_stepSound = 1.9f;                       // 步距(像素)
            const bool snowish = w.tileAt((int)(pos.x / TILE), (int)((pos.y - 1) / TILE)) == T_SNOW;
            audio::playThrottled(SFX_STEP, 0.20f, 0.30f,
                                 (snowish ? 0.82f : 1.0f) * (0.92f + rand01(hash2(animFrame, m_frameJ, 7u)) * 0.16f));
        }
    }
    else if (!onGround) m_animState = (vel.y < 0 ? 2.0f : 3.0f);
    else m_animState = 0;
    animFrame = ((int)m_stepTimer) % 4;

    if (onGround && !wasOnGround) {
        g.parts().spawn(PK_DUST, pos.x - 6, pos.y, 4, rgb(180, 170, 160), 70, 2.0f, 2.2f, 0.35f);
        if (fallV > 300.0f) audio::play(SFX_LAND, clampf(fallV / 700.0f, 0.2f, 0.8f), 0.95f);
    }
}

// ---------------------------------------------------------------- 挖掘
void Player::mining(World& w, Game& g, float dt)
{
    // 必须按住左键才挖
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) { mineProgress = 0; mineX = mineY = -1; }
        return;
    }
    const Vector2 m = GetMousePosition();
    const Vector2 mw = Game::screenToWorld(m, g);
    const int tx = (int)floorf(mw.x / TILE);
    const int ty = (int)floorf(mw.y / TILE);
    const bool withinReach = (mw.x - pos.x) * (mw.x - pos.x) + (mw.y - center().y) * (mw.y - center().y)
                             < 34.0f * 34.0f;
    const uint16_t t = w.tileAt(tx, ty);
    const uint16_t wl = w.wallAt(tx, ty);
    const bool wantWall = (t == T_AIR) && (wl != W_NONE);

    if (!withinReach || (!wantWall && t == T_AIR)) {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) { mineProgress = 0; mineX = mineY = -1; }
        return;
    }

    if (mineX != tx || mineY != ty) { mineX = tx; mineY = ty; mineProgress = 0; }
    if (t != T_AIR) mineWall = 0; else mineWall = 1;

    const ItemDef& it = itemDef(held());
    float speed = 0.55f;                       // 空手
    if (isPick() || isAxe()) speed = toolSpeed();

    if (!wantWall) {
        const TileDef& d = tileInfo(t);
        if (d.hardness <= 0.0f) { mineProgress = 0; return; }   // 不可破坏(工作台/箱子等)
        if (d.pickPower > 0 && (!isPick() || it.pickPower < d.pickPower)) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                g.addMessage(loc::get(loc::S_NEED_PICK), rgb(240, 200, 120));
            mineProgress = 0;
            return;
        }
        if (isAxe() && !woodLike(t)) speed *= 0.55f;
        if (!isAxe() && woodLike(t)) speed *= 0.45f;
        const float need = d.hardness * MINE_SCALE / speed;
        mineProgress += dt / need;
        Sfx dsfx = SFX_DIG_DIRT;
        if (woodLike(t) || t == T_VINE) dsfx = SFX_DIG_WOOD;
        else if (t == T_STONE || t == T_STONE_JUNGLE || t == T_EBONSTONE || t == T_SANDSTONE ||
                 t == T_CLAY || t == T_CLAY_BRICK || t == T_BRICKS || t == T_BRICKS_STONE ||
                 t == T_MUD_BRICK || t == T_SNOW_BRICK || t == T_OBSIDIAN || t == T_HELLSTONE) dsfx = SFX_DIG_STONE;
        audio::playThrottled(dsfx, 0.19f, 0.42f,
                             0.9f + rand01(hash2(mineX, mineY, (int)(mineProgress * 53.0f))) * 0.22f);
    } else {
        const float need = 0.30f * MINE_SCALE / speed;
        mineProgress += dt / need;
    }

    if (mineProgress >= 1.0f) {
        mineProgress = 0;
        if (!wantWall) {
            const TileDef& d = tileInfo(t);
            if (d.itemDrop) {
                g.spawnDrop(d.itemDrop, 1, tx * TILE + TILE * 0.5f, ty * TILE + TILE * 0.5f);
                if (rand01(hash3(tx, ty, (int)(g.player().animTimer * 60), 991u)) < 0.22f)
                    g.spawnDrop(d.itemDrop, 1, tx * TILE + TILE * 0.5f, ty * TILE + TILE * 0.5f);
            }
            w.setTile(tx, ty, T_AIR);
            g.parts().spawn(PK_DUST, tx * TILE + 8, ty * TILE + 8, 7,
                            tileInfo(t).mapColor, 78.0f, 3.14f, 2.4f, 0.42f);
        } else {
            w.setWall(tx, ty, W_NONE);
            g.parts().spawn(PK_DUST, tx * TILE + 8, ty * TILE + 8, 5,
                            wallInfo(wl).mapColor, 62.0f, 3.14f, 2.2f, 0.38f);
        }
        audio::play(SFX_BREAK, 0.7f, 0.88f + rand01(hash2(tx, ty, 991u)) * 0.26f);
        g.shake(0.7f);
    }
}

// ---------------------------------------------------------------- 左键: 攻击
void Player::useLeft(World& w, Game& g)
{
    const ItemDef& d = itemDef(held());
    if (!held()) return;

    if (d.kind == IK_BOW) {
        if (!countItem(IT_ARROW)) return;
        bowCharging = true;
        bowCharge = std::min(1.0f, bowCharge + GetFrameTime() * 1.1f);
        return;
    }
    if (d.kind == IK_SWORD || d.kind == IK_MAGIC) {
        if (swingTimer > 0) return;
        swingTimer = d.useTime;
        swingDir = facing;
        audio::play(SFX_SWING, 0.45f, 0.95f + rand01(hash2((int)pos.x, m_frameJ, 77u)) * 0.2f);
        attackSwing(w, g);
    }
}

void Player::attackSwing(World& w, Game& g)
{
    const ItemDef& d = itemDef(held());
    const Vector2 c = center();
    const float reach = 34.0f;
    bool hit = false;
    for (Entity& e : g.ents().all()) {
        if (!e.active || e.friendly || e.type == E_DROP || e.type == E_PROJECTILE) continue;
        if (e.type == E_NPC_GUIDE || e.type == E_NPC_MERCHANT) continue;
        const Rectangle r = e.rect();
        if (!aabb(c.x - reach, c.y - reach, reach * 2, reach * 2, r.x, r.y, r.width, r.height)) continue;
        // 只打朝向的一侧
        if ((e.center().x - c.x) * facing < -6.0f) continue;

        bool crit = rand01(hash3((int)c.x, (int)c.y, g.frameCounter(), 7717u)) < critChance();
        int dmg = baseDamage();
        audio::play(SFX_HIT_ENEMY, crit ? 0.8f : 0.6f, 0.9f + rand01(hash2((int)c.x, (int)c.y, 55u)) * 0.25f);
        if (crit) dmg = (int)(dmg * critMult());
        e.hp -= dmg;
        e.hurtTimer = 0.26f;
        const float kb = (d.kind == IK_MAGIC ? 40.0f : 150.0f) / (1.0f + e.knockResist);
        e.vel.x += ((e.center().x - c.x) > 0 ? 1 : -1) * kb;
        e.vel.y -= 60.0f * (1.0f - e.knockResist);
        g.damageNumber(e.center().x, e.center().y - 8, dmg, crit);
        g.shake(crit ? 3.2f : 1.6f);
        g.parts().spawn(crit ? PK_SPARK : PK_BLOOD, e.center().x, e.center().y, crit ? 12 : 7,
                        crit ? rgb(255, 220, 120) : rgb(200, 40, 40), 120, 3.14f, 2.4f, 0.4f);
        hit = true;
        if (e.hp <= 0 && !e.isBoss) {
            audio::play(SFX_ENEMY_DIE, 0.65f, 0.9f + rand01(hash2((int)e.pos.x, (int)e.pos.y, 3u)) * 0.2f);
            g.dropLoot(e);
            e.active = false;
        }
    }
    if (hit) g.parts().spawn(PK_DUST, c.x + facing * 22, c.y, 4, rgb(220, 220, 230), 90, 1.6f, 2.0f, 0.25f);
}

// ---------------------------------------------------------------- 右键: 放置 / 使用
void Player::useRight(World& w, Game& g)
{
    const Vector2 m = GetMousePosition();
    const Vector2 mw = Game::screenToWorld(m, g);
    const int tx = (int)floorf(mw.x / TILE);
    const int ty = (int)floorf(mw.y / TILE);
    const uint16_t t = w.tileAt(tx, ty);
    const ItemDef& d = itemDef(held());

    // 1) 空手: 与方块/实体交互
    if (!held()) { interact(w, g, tx, ty); return; }

    // 2) Boss 召唤物
    if (d.kind == IK_SUMMON) {
        g.summonBoss(held() == IT_SLIME_CROWN ? 0 : 1);
        consume(held(), 1);
        return;
    }
    // 3) 消耗品
    if (d.kind == IK_CONSUMABLE) {
        useItem = held();
        useTimer = d.useTime;
        return;
    }
    // 4) 魔法
    if (d.kind == IK_MAGIC) {
        if (mana < d.manaCost) { g.addMessage(loc::get(loc::S_NEED_MANA), rgb(120, 170, 255)); return; }
        mana -= d.manaCost;
        swingTimer = 0.3f;
        for (int k = -1; k <= 1; k++) {
            const float a = (k) * 0.16f;
            const float sp = 420.0f;
            g.spawnProjectile(pos.x + facing * 12, pos.y - 24,
                              facing * sp * cosf(a), sp * sinf(a) * 0.35f,
                              2, d.damage, 0, false, 1.6f, rgb(150, 90, 255), 5.0f);
        }
        g.parts().spawn(PK_MAGIC, pos.x + facing * 12, pos.y - 24, 6, rgb(170, 110, 255), 90, 3.14f, 2.6f, 0.35f);
        return;
    }
    // 5) 镐/斧: 快速挖掘
    if (hasTool()) {
        if (t == T_AIR) return;
        const TileDef& td = tileInfo(t);
        if (td.hardness <= 0) return;
        if (td.pickPower > 0 && (!isPick() || d.pickPower < td.pickPower)) return;
        if (w.tileAt(tx, ty) == t) {
            w.setTile(tx, ty, T_AIR);
            if (td.itemDrop) g.spawnDrop(td.itemDrop, 1, tx * TILE + 8, ty * TILE + 8);
            g.parts().spawn(PK_DUST, tx * TILE + 8, ty * TILE + 8, 6, td.mapColor, 90, 3.14f, 2.4f, 0.35f);
            swingTimer = 0.2f;
        }
        return;
    }
    // 6) 放置方块 / 家具
    if (d.placeTile != T_AIR) {
        if (!g.canPlaceAt(tx, ty)) return;
        // 需要镐等级?
        const TileDef& td = tileInfo(d.placeTile);
        if (td.pickPower > 0 && (!isPick() || d.pickPower < td.pickPower)) {
            g.addMessage(loc::get(loc::S_NEED_PICK), rgb(240, 200, 120));
            return;
        }
        w.setTile(tx, ty, d.placeTile);
        if (d.placeTile == T_FURNACE) w.setTile(tx, ty, T_FURNACE);
        consume(held(), 1);
        audio::play(SFX_PLACE, 0.55f, 0.92f + rand01(hash2(tx, ty, 313u)) * 0.2f);
        g.parts().spawn(PK_DUST, tx * TILE + 8, ty * TILE + 8, 3, td.mapColor, 50, 3.14f, 1.8f, 0.25f);
        swingTimer = 0.16f;
        return;
    }
    // 7) 其它: 当作交互
    interact(w, g, tx, ty);
}

// 弓: 松开左键射出
void Player::releaseBow(Game& g)
{
    if (!bowCharging) return;
    bowCharging = false;
    const float power = 0.35f + bowCharge * 0.65f;
    bowCharge = 0;
    if (!countItem(IT_ARROW)) return;
    const ItemDef& bow = itemDef(held());
    consume(IT_ARROW, 1);
    const float sp = 520.0f * power;
    g.spawnProjectile(pos.x + facing * 10, pos.y - 24, facing * sp, -18.0f,
                      1, bow.damage + itemDamage(IT_ARROW), 0, true, 3.0f, rgb(220, 200, 160), 3.0f);
    audio::play(SFX_SWING, 0.4f, 1.15f);
    g.parts().spawn(PK_DUST, pos.x + facing * 12, pos.y - 24, 4, rgb(240, 240, 240), 70, 1.2f, 1.6f, 0.2f);
    swingTimer = 0.24f;
}

void Player::interact(World& w, Game& g, int tx, int ty)
{
    const uint16_t t = w.tileAt(tx, ty);
    if (t == T_DOOR) { w.setTile(tx, ty, T_DOOR_OPEN); return; }
    if (t == T_DOOR_OPEN) { w.setTile(tx, ty, T_DOOR); return; }
    if (t == T_CHEST) {
        g.chestAt(tx, ty);
        g.m_openChest = ((int64_t)(uint32_t)tx << 32) | (uint32_t)ty;
        g.invOpen = true;
        g.craftingPanel = false;
        return;
    }
    if (t == T_WORKBENCH) { g.craftStation = ST_BENCH; g.invOpen = true; g.craftingPanel = true; return; }
    if (t == T_FURNACE || t == T_FURNACE_LIT) { g.craftStation = ST_FURNACE; g.invOpen = true; g.craftingPanel = false; return; }
    if (t == T_ANVIL) { g.craftStation = ST_ANVIL; g.invOpen = true; g.craftingPanel = true; return; }
    // NPC
    Entity* npc = g.ents().findNpcNear(pos.x, pos.y, 46.0f);
    if (npc) {
        g.shopOpen = true;
        g.shopNpc = (npc->type == E_NPC_MERCHANT) ? 1 : 0;
    }
}

void Player::swapEquip(int slotIndex, int itemId_)
{
    // 找到该装备当前所在槽, 交换
    int cur = equip[slotIndex];
    if (itemId_ == 0) { equip[slotIndex] = 0; if (cur) give(cur, 1); return; }
    if (itemDef(itemId_).equip != (EquipSlot)(slotIndex + 1)) return;
    for (int i = 0; i < INV_SLOTS; i++) {
        if (inv[i].id == itemId_) { inv[i].clear(); break; }
    }
    if (cur) give(cur, 1);
    equip[slotIndex] = itemId_;
}

// ---------------------------------------------------------------- 绘制
void Player::draw(World& w, Game& g)
{
    const char* name = "player_idle";
    if (m_animState == 1) {
        char buf[32];
        snprintf(buf, sizeof(buf), "player_walk_%d", animFrame);
        name = buf;
    } else if (m_animState == 2) name = "player_jump";
    else if (m_animState == 3) name = "player_fall";

    Color tint = WHITE;
    if (hurtTimer > 0 && ((int)(hurtTimer * 22) % 2 == 0)) tint = rgb(255, 120, 120);
    else if (invulnTimer > 0) tint = rgb(220, 220, 235, 200);

    const Frame* f = Assets::Find(name);
    if (f) {
        const float w2 = f->rect.width, h2 = f->rect.height;
        drawFrame(*f, pos.x - w2 * 0.5f, pos.y - h2, facing < 0, tint);
    }

    // 手上的物品
    const int hid = held();
    if (hid && inv[selected].count > 0) {
        const ItemDef& d = itemDef(hid);
        if (d.kind != IK_BLOCK && d.kind != IK_PLACE && d.kind != IK_MATERIAL) {
            const Frame* it = Assets::FindItem(d.sprite);
            if (it) {
                float rot = 0.0f;
                if (swingTimer > 0) {
                    const float t = 1.0f - clampf(swingTimer / std::max(0.01f, d.useTime), 0.0f, 1.0f);
                    rot = (-0.9f + t * 2.2f) * swingDir;
                }
                const float hw = it->rect.width * 0.5f, hh = it->rect.height * 0.5f;
                const float hxp = pos.x + facing * 9.0f, hyp = pos.y - 26.0f;
                const Vector2 org{ hw, hh };
                const Rectangle dst{ hxp, hyp, it->rect.width, it->rect.height };
                DrawTexturePro(it->tex, it->rect, dst, org, rot, WHITE);
            }
        }
    }
}

}  // namespace tc

