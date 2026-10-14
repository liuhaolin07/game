// Game.cpp —— 主控: 初始化 / 更新 / 绘制 / 存档
#include "Game.h"

#include "Assets.h"
#include "Audio.h"
#include "Noise.h"
#include "Text.h"
#include "UI.h"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace tc {

namespace {
const char* kSaveDir = "save";
}

Vector2 Game::screenToWorld(Vector2 screen, const Game& g)
{
    // screen = (world - target) * zoom + offset, 反推时要把 offset 减掉
    return { (screen.x - (float)GetScreenWidth() * 0.5f) / ZOOM + g.m_camTarget.x,
             (screen.y - (float)GetScreenHeight() * 0.5f) / ZOOM + g.m_camTarget.y };
}

Vector2 Game::mouseWorld() const { return screenToWorld(GetMousePosition(), *this); }

// ---------------------------------------------------------------- 初始化
Game::Game() = default;
Game::~Game() = default;

bool Game::init()
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_W, SCREEN_H, "Terraria Clone");
    SetTargetFPS(60);
    SetExitKey(KEY_ESCAPE);

    if (!Assets::Load("assets")) {
        TraceLog(LOG_ERROR, "无法载入 assets/ 目录下的素材");
        return false;
    }
    Assets::SetPixelFilter();
    text::Init();
    audio::init();
    m_ui.reset(new UI());
    return true;
}

void Game::shutdown()
{
    audio::shutdown();
    text::Shutdown();
    Assets::Unload();
    CloseWindow();
}

void Game::newWorld(uint32_t seedIn)
{
    m_seed = seedIn ? seedIn : (uint32_t)(GetFrameTime() * 100000.0) + 20240501u;
    m_world.init(m_seed);
    m_ents.clear();
    m_parts.clear();
    m_chests.clear();
    m_dmg.clear();
    m_msgs.clear();
    m_time = 0.30f;
    m_shake = 0;

    m_player.init();
    m_player.respawn(m_world);      // 出生点只依赖纯函数 surfaceY(), 不需要区块已加载

    // 预热视野: 必须先把玩家放到位, 否则会在错误的位置生成一圈区块
    for (int i = 0; i < 140; i++) m_world.stream(m_player.pos.x, m_player.pos.y, 0.016f);
    m_player.pos.y = (float)(m_world.surfaceHeight((int)(m_player.pos.x / TILE)) * TILE);
    m_player.vel = { 0, 0 };
    m_camTarget = m_player.center();
    spawnNPCs();
}

void Game::spawnNPCs()
{
    // 向导: 出生点
    if (!m_ents.findNpc(0)) {
        const int x = (int)(m_player.pos.x / TILE);
        const int y = m_world.surfaceHeight(x);
        m_ents.spawn(E_NPC_GUIDE, (float)(x * TILE - 11), (float)(y * TILE));
    }
    // 商人: 需要一间合适的房子
    if (!m_ents.findNpc(1)) {
        float hx, hy;
        if (findHouse(m_player.pos.x, m_player.pos.y, hx, hy))
            m_ents.spawn(E_NPC_MERCHANT, hx, hy);
    }
}

bool Game::findHouse(float px, float py, float& outX, float& outY) const
{
    // 在玩家附近找: 脚下是实心, 头顶 6 格空气, 有背景墙, 光照够亮, 且有空间站人
    const int cx = (int)(px / TILE);
    for (int r = 0; r < 90; r++) {
        for (int s = -1; s <= 1; s += 2) {
            const int x = cx + r * s;
            for (int y = m_world.surfaceY(x) - 1; y < m_world.surfaceY(x) + 40 && y < WORLD_H - 2; y++) {
                if (!m_world.solidAt(x, y + 1)) continue;
                if (m_world.solidAt(x, y) || m_world.solidAt(x, y - 1) || m_world.solidAt(x, y - 2)) continue;
                if (m_world.wallAt(x, y) == W_NONE) continue;
                if (m_world.liquidAt(x, y)) continue;
                if (m_world.lightAt(x, y - 1) < 9) continue;
                outX = (float)(x * TILE - 10);
                outY = (float)((y + 1) * TILE);
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------- 存档
namespace {
void wu16(FILE* f, uint16_t v) { fwrite(&v, sizeof(v), 1, f); }
void wu32(FILE* f, uint32_t v) { fwrite(&v, sizeof(v), 1, f); }
void wi32(FILE* f, int32_t v) { fwrite(&v, sizeof(v), 1, f); }
void wf(FILE* f, float v) { fwrite(&v, sizeof(v), 1, f); }
uint16_t ru16(FILE* f) { uint16_t v = 0; if (fread(&v, sizeof(v), 1, f) != 1) v = 0; return v; }
uint32_t ru32(FILE* f) { uint32_t v = 0; if (fread(&v, sizeof(v), 1, f) != 1) v = 0; return v; }
int32_t ri32(FILE* f) { int32_t v = 0; if (fread(&v, sizeof(v), 1, f) != 1) v = 0; return v; }
float rf(FILE* f) { float v = 0; if (fread(&v, sizeof(v), 1, f) != 1) v = 0; return v; }
}  // namespace

bool Game::hasSave() const
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s.wld", kSaveDir, m_worldName.c_str());
    return FileExists(path);
}

bool Game::saveWorld(const char*)
{
    char path[256];
    snprintf(path, sizeof(path), "%s", kSaveDir);
#ifdef _WIN32
    system("mkdir save 2>nul");
#else
    system("mkdir -p save 2>/dev/null");
#endif
    snprintf(path, sizeof(path), "%s/%s.wld", kSaveDir, m_worldName.c_str());
    FILE* f = fopen(path, "wb");
    if (!f) return false;

    char magic[4] = { 'T', 'C', 'L', '1' };
    fwrite(magic, 1, 4, f);
    wu32(f, m_seed);
    wf(f, m_time);
    wf(f, m_player.pos.x);
    wf(f, m_player.pos.y);
    wi32(f, m_player.hp);
    wi32(f, m_player.maxHp);
    wi32(f, m_player.mana);
    wi32(f, m_player.maxMana);
    wi32(f, m_player.coins);
    wi32(f, m_player.selected);
    for (int i = 0; i < 4; i++) wi32(f, m_player.equip[i]);
    for (int i = 0; i < INV_SLOTS; i++) { wi32(f, m_player.inv[i].id); wi32(f, m_player.inv[i].count); }

    // 箱子
    wi32(f, (int32_t)m_chests.size());
    for (auto& kv : m_chests) {
        wi32(f, (int32_t)(kv.first & 0xFFFFFFFF));
        wi32(f, (int32_t)(kv.first >> 32));
        wi32(f, (int32_t)kv.second.slots.size());
        for (const ItemStack& s : kv.second.slots) { wi32(f, s.id); wi32(f, s.count); }
    }

    // 区块(只存改动过的), RLE
    const std::vector<World::SaveChunk> chunks = m_world.collectModified();
    wi32(f, (int32_t)chunks.size());
    for (const World::SaveChunk& sc : chunks) {
        wi32(f, sc.cx);
        wi32(f, sc.cy);
        for (int pass = 0; pass < 2; pass++) {
            const std::vector<uint16_t>& arr = (pass == 0) ? sc.tile : sc.wall;
            wi32(f, (int32_t)arr.size());
            int i = 0;
            while (i < (int)arr.size()) {
                const uint16_t v = arr[i];
                int run = 1;
                while (i + run < (int)arr.size() && arr[i + run] == v && run < 65000) run++;
                wu16(f, v);
                wu16(f, (uint16_t)run);
                i += run;
            }
        }
    }
    fclose(f);
    return true;
}

bool Game::loadWorld(const char*)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s.wld", kSaveDir, m_worldName.c_str());
    if (!FileExists(path)) return false;
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    char magic[4] = { 0 };
    if (fread(magic, 1, 4, f) != 4 || magic[0] != 'T') { fclose(f); return false; }
    m_seed = ru32(f);
    m_time = rf(f);
    const float px = rf(f), py = rf(f);
    m_world.init(m_seed);
    m_player.init();
    m_player.hp = ri32(f); m_player.maxHp = ri32(f);
    m_player.mana = ri32(f); m_player.maxMana = ri32(f);
    m_player.coins = ri32(f);
    m_player.selected = ri32(f);
    for (int i = 0; i < 4; i++) m_player.equip[i] = ri32(f);
    for (int i = 0; i < INV_SLOTS; i++) { m_player.inv[i].id = ri32(f); m_player.inv[i].count = ri32(f); }

    m_chests.clear();
    const int nChest = ri32(f);
    for (int i = 0; i < nChest; i++) {
        const int32_t lo = ri32(f), hi = ri32(f);
        const int n = ri32(f);
        Chest c;
        c.slots.resize(n);
        for (int k = 0; k < n; k++) { c.slots[k].id = ri32(f); c.slots[k].count = ri32(f); }
        m_chests[(int64_t)(((uint64_t)(uint32_t)lo) | ((uint64_t)(uint32_t)hi << 32))] = c;
    }

    const int nChunk = ri32(f);
    for (int i = 0; i < nChunk; i++) {
        World::SaveChunk sc;
        sc.cx = ri32(f);
        sc.cy = ri32(f);
        for (int pass = 0; pass < 2; pass++) {
            std::vector<uint16_t>& arr = (pass == 0) ? sc.tile : sc.wall;
            const int n = ri32(f);
            arr.resize(n);
            int p = 0;
            while (p < n) {
                const uint16_t v = ru16(f);
                const uint16_t run = ru16(f);
                for (int k = 0; k < run && p < n; k++) arr[p++] = v;
            }
        }
        if (sc.tile.size() == (size_t)CHUNK_AREA && sc.wall.size() == (size_t)CHUNK_AREA)
            m_world.restoreChunk(sc);
    }
    fclose(f);

    m_ents.clear();
    m_parts.clear();
    m_dmg.clear();
    m_msgs.clear();
    m_player.pos = { px, py };
    m_player.dead = false;
    for (int i = 0; i < 120; i++) m_world.stream(m_player.pos.x, m_player.pos.y, 0.016f);
    m_camTarget = m_player.center();
    spawnNPCs();
    return true;
}

// ---------------------------------------------------------------- 工具
Chest* Game::chestAt(int x, int y)
{
    int64_t key = ((int64_t)(uint32_t)x << 32) | (uint32_t)y;
    auto it = m_chests.find(key);
    if (it == m_chests.end()) { Chest c; c.slots.resize(20); m_chests[key] = c; }
    return &m_chests[key];
}

void Game::spawnProjectile(float x, float y, float vx, float vy, int kind, int damage,
                           int owner, bool gravity, float life, Color color, float size)
{
    Entity& e = m_ents.spawn(E_PROJECTILE, x - size * .5f, y - size * .5f);
    e.size = { size, size };
    if (kind == 1) e.size = { 10, 4 };
    e.vel = { vx, vy };
    e.projKind = kind;
    e.projDamage = damage;
    e.projOwner = owner;
    e.projLife = life;
    e.projColor = color;
    e.projSize = size;
    e.gravity = gravity;
    e.contactDamage = 0;
}

void Game::spawnDrop(int itemId, int count, float x, float y)
{
    if (!itemId || count <= 0) return;
    Entity& e = m_ents.spawn(E_DROP, x - 6, y - 6);
    e.size = { 12, 12 };
    e.itemId = itemId;
    e.itemCount = count;
    e.vel = { (rand01(hash2((int)x, (int)y, m_frame)) - 0.5f) * 90.0f, -110.0f };
    e.projLife = 240.0f;
}

void Game::damageNumber(float x, float y, int value, bool crit)
{
    DamageText d;
    d.pos = { x, y };
    d.vel = { (rand01(hash2((int)x, m_frame, 7)) - 0.5f) * 30.0f, -95.0f };
    d.value = value;
    d.crit = crit;
    d.life = crit ? 1.1f : 0.8f;
    m_dmg.push_back(d);
}

void Game::dropLoot(const Entity& e)
{
    const Vector2 c = e.center();
    const uint32_t h = hash3((int)c.x, (int)c.y, m_frame, 6151u);
    const int cnt = e.isBoss ? 0 : 1;
    switch (e.type) {
    case E_SLIME:
        spawnDrop(IT_GEL, 1 + (int)(rand01(h) * (2 + e.variant)), c.x, c.y);
        if (cnt && rand01(h * 3u) < 0.35f) {
            m_player.coins += 4 + (int)(rand01(h * 7u) * 12.0f);
            audio::playThrottled(SFX_COIN, 0.05f, 0.35f, 0.95f + rand01(h * 9u) * 0.15f);
        }
        break;
    case E_ZOMBIE:
        spawnDrop(IT_BONE, 1 + (int)(rand01(h) * 2.0f), c.x, c.y);
        if (rand01(h * 5u) < 0.4f) spawnDrop(IT_STRING, 1, c.x, c.y);
        m_player.coins += 12 + (int)(rand01(h * 11u) * 26.0f);
        audio::playThrottled(SFX_COIN, 0.05f, 0.4f, 1.0f + rand01(h * 13u) * 0.12f);
        break;
    case E_DEMON_EYE:
        spawnDrop(IT_LENS, 1, c.x, c.y);
        m_player.coins += 18 + (int)(rand01(h * 13u) * 24.0f);
        audio::playThrottled(SFX_COIN, 0.05f, 0.45f, 1.05f + rand01(h * 17u) * 0.12f);
        break;
    case E_SERVANT:
        m_player.coins += 3;
        break;
    case E_KING_SLIME:
        spawnDrop(IT_GEL, 20 + (int)(rand01(h) * 22.0f), c.x, c.y);
        m_player.coins += 500 + (int)(rand01(h * 17u) * 400.0f);
        break;
    case E_EYE_CTHULHU:
        spawnDrop(IT_ORE_DEMONITE, 18 + (int)(rand01(h) * 14.0f), c.x, c.y);
        spawnDrop(IT_LENS, 12 + (int)(rand01(h * 3u) * 12.0f), c.x, c.y);
        m_player.coins += 800 + (int)(rand01(h * 19u) * 500.0f);
        break;
    default: break;
    }
}

void Game::addMessage(const std::string& s, Color c)
{
    m_msgs.push_back({ s, 3.0f, c });
    if (m_msgs.size() > 6) m_msgs.erase(m_msgs.begin());
}

void Game::updateMessages(float dt)
{
    for (auto& m : m_msgs) m.life -= dt;
    m_msgs.erase(std::remove_if(m_msgs.begin(), m_msgs.end(), [](const FloatMsg& m) { return m.life <= 0; }),
                 m_msgs.end());
}

void Game::alertBoss(const char* name)
{
    audio::play(SFX_BOSS, 0.9f);
    m_bossName = name;
    m_bossAlertTime = 3.2f;
    shake(5.0f);
}

void Game::summonBoss(int which)
{
    if (m_ents.findBoss()) { addMessage("已经有首领在场了", rgb(240, 200, 120)); return; }
    const int x = (int)(m_player.pos.x / TILE);
    if (which == 0) {
        if (m_player.pos.y / TILE > UNDERWORLD_Y - 60) {
            addMessage(loc::get(loc::S_SLIME_CROWN_USE), rgb(240, 200, 120));
            return;
        }
        const int y = m_world.surfaceHeight(x);
        m_ents.spawn(E_KING_SLIME, (float)((x - 5) * TILE), (float)(y * TILE - 62));
        alertBoss(loc::zh(loc::S_SLIME));
    } else {
        if (!isNight() && m_player.pos.y / TILE < m_world.surfaceY(x) - 40) {
            addMessage(loc::get(loc::S_EYE_USE), rgb(240, 200, 120));
            return;
        }
        const int y = m_world.surfaceHeight(x);
        m_ents.spawn(E_EYE_CTHULHU, (float)((x - 6) * TILE), (float)(y * TILE - 96));
        alertBoss("克苏鲁之眼");
    }
}

bool Game::nearStation(int tileId, int rangeTiles) const
{
    const int cx = (int)(m_player.pos.x / TILE);
    const int cy = (int)(m_player.pos.y / TILE);
    const int r = rangeTiles;
    for (int y = cy - r; y <= cy + r; y++)
        for (int x = cx - r; x <= cx + r; x++) {
            const uint16_t t = m_world.tileAt(x, y);
            if (t == tileId) return true;
            if (tileId == T_FURNACE && t == T_FURNACE_LIT) return true;
        }
    return false;
}

bool Game::canPlaceAt(int tx, int ty) const
{
    if (ty < 0 || ty >= WORLD_H) return false;
    const uint16_t t = m_world.tileAt(tx, ty);
    if (t != T_AIR && !isLiquid(t)) return false;
    const Rectangle p = m_player.rect();
    const float bx = (float)(tx * TILE), by = (float)(ty * TILE);
    if (aabb(p.x, p.y, p.width, p.height, bx, by, TILE, TILE)) return false;
    for (const Entity& e : m_ents.all()) {
        if (!e.active || e.friendly) continue;
        if (aabb(e.pos.x, e.pos.y, e.size.x, e.size.y, bx, by, TILE, TILE)) return false;
    }
    return true;
}

// ---------------------------------------------------------------- 时间与摄像机
float Game::daylight() const
{
    // 0=深夜 1=正午
    const float t = m_time;
    if (t < 0.20f) return 0.26f;
    if (t < 0.30f) return lerpf(0.26f, 1.0f, (t - 0.20f) / 0.10f);
    if (t < 0.72f) return 1.0f;
    if (t < 0.82f) return lerpf(1.0f, 0.26f, (t - 0.72f) / 0.10f);
    return 0.26f;
}

Color Game::ambient() const
{
    const float d = daylight();
    return { (unsigned char)(lerpf(136, 255, d)), (unsigned char)(lerpf(146, 255, d)),
             (unsigned char)(lerpf(202, 255, d)), 255 };
}

Camera2D Game::camera() const
{
    Camera2D c;
    c.offset = { (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.5f };
    c.target = m_camTarget;
    c.zoom = ZOOM;
    c.rotation = 0;
    return c;
}

void Game::updateTime(float dt)
{
    const float DAY = 900.0f;    // 一个完整昼夜 15 分钟
    m_time += dt / DAY;
    if (m_time >= 1.0f) m_time -= 1.0f;

    // 天空光等级(整数级, 变化时重算光照)
    const float d = daylight();
    const int lvl = 2 + (int)roundf(d * 13.0f);
    if (lvl != m_world.skyLightLevel()) m_world.setSkyLight((uint8_t)clampi(lvl, 0, LIGHT_MAX));
}

void Game::updateCamera()
{
    const Vector2 pc = m_player.center();
    m_camTarget.x = pc.x;
    m_camTarget.y = pc.y;
    const float halfH = VIEW_H * 0.5f;
    m_camTarget.y = clampf(m_camTarget.y, halfH, (float)(WORLD_H * TILE) - halfH);
    if (m_shake > 0.01f) {
        m_shake = approach(m_shake, 0.0f, m_shake * 5.0f * GetFrameTime() + 0.4f * GetFrameTime());
        m_camTarget.x += (rand01(hash2(m_frame, 1, 31)) - 0.5f) * m_shake * 6.0f;
        m_camTarget.y += (rand01(hash2(m_frame, 2, 31)) - 0.5f) * m_shake * 6.0f;
    } else m_shake = 0.0f;
}

// ---------------------------------------------------------------- 更新
void Game::update(float dt)
{
    m_frame++;

    // ---- 全局按键 ----
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (shopOpen) { shopOpen = false; }
        else if (invOpen) { audio::play(SFX_CLOSE, 0.5f); invOpen = false; craftingPanel = false; heldMouse = 0; dragFrom = -1; m_openChest = -1; }
        else if (state == GS_PLAYING) { audio::play(SFX_MENU, 0.5f); state = GS_PAUSED; }
        else if (state == GS_PAUSED) { audio::play(SFX_MENU, 0.5f); state = GS_PLAYING; }
    }
    if (IsKeyPressed(KEY_E) && state != GS_PAUSED && !m_player.dead) {
        audio::play(invOpen ? SFX_CLOSE : SFX_OPEN, 0.5f);
        invOpen = !invOpen;
        heldMouse = 0;
        dragFrom = -1;
        if (!invOpen) { shopOpen = false; craftingPanel = false; m_openChest = -1; }
    }
    if (IsKeyPressed(KEY_F5) && state == GS_PAUSED) { saveWorld(nullptr); addMessage(loc::get(loc::S_SAVED), rgb(160, 240, 160)); }
    if (IsKeyPressed(KEY_F9) && state == GS_PAUSED) { if (loadWorld(nullptr)) { state = GS_PLAYING; addMessage(loc::get(loc::S_LOADED), rgb(160, 240, 160)); } else addMessage(loc::get(loc::S_LOAD_FAIL), rgb(240, 140, 140)); }

    if (state == GS_PAUSED) {
        m_ui->update(*this, dt);
        return;
    }

    updateTime(dt);
    updateCamera();
    m_world.stream(m_camTarget.x, m_camTarget.y, dt);
    m_world.updateFluids();

    uiBlocking = invOpen || shopOpen || m_player.dead;
    m_player.update(m_world, *this, dt);
    m_ents.update(m_world, m_player, *this, dt);
    m_ents.trySpawnAmbient(m_world, m_player, *this, dt);
    m_parts.update(dt, m_world);
    updateMessages(dt);

    // 熔炉
    if (furnaceIn && furnaceFuel) {
        const int res = smeltResult(furnaceIn);
        if (res && (!furnaceOut || (furnaceOut == res && countFurnaceOut() < 99))) {
            furnaceTimer += dt;
            const float need = 1.2f;
            if (furnaceTimer >= need) {
                furnaceTimer = 0.0f;
                furnaceIn = 0;
                if (!furnaceOut) furnaceOut = res; else furnaceOutCount++;
                m_furnaceFuelTime = fuelValue(furnaceFuel);
            }
        }
    }
    if (m_furnaceFuelTime > 0.0f) {
        m_furnaceFuelTime -= dt;
        if (m_furnaceFuelTime <= 0.0f) furnaceFuel = 0;
    }

    if (m_player.dead && state != GS_DEAD) state = GS_DEAD;
    if (state == GS_DEAD && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE))) {
        m_player.respawn(m_world);
        state = GS_PLAYING;
    }

    if (m_bossAlertTime > 0.0f) m_bossAlertTime -= dt;
    if (--m_npcCheckTimer <= 0) { m_npcCheckTimer = 300; spawnNPCs(); }

    m_ui->update(*this, dt);
    updateDamageNumbers(dt);
}

void Game::updateDamageNumbers(float dt)
{
    for (DamageText& d : m_dmg) {
        d.life -= dt;
        d.vel.y += 240.0f * dt;
        d.pos.x += d.vel.x * dt;
        d.pos.y += d.vel.y * dt;
    }
    m_dmg.erase(std::remove_if(m_dmg.begin(), m_dmg.end(), [](const DamageText& d) { return d.life <= 0; }),
                m_dmg.end());
}

int Game::countFurnaceOut() const { return furnaceOutCount; }

// ---------------------------------------------------------------- 绘制
void Game::draw()
{
    const Camera2D cam = camera();
    BeginDrawing();
    ClearBackground(rgb(5, 5, 9));
    BeginMode2D(cam);

    const float vw = VIEW_W, vh = VIEW_H;
    const float wx0 = m_camTarget.x - vw * 0.5f, wy0 = m_camTarget.y - vh * 0.5f;
    const float wx1 = wx0 + vw, wy1 = wy0 + vh;

    drawSky(wx0, wy0, wx1, wy1);
    drawWorldTiles(wx0, wy0, wx1, wy1);
    m_ents.draw(m_world, wx0 - 64.0f, wy0 - 64.0f, wx1 + 64.0f, wy1 + 64.0f);
    m_player.draw(m_world, *this);
    m_parts.draw();
    drawDamageNumbers();

    EndMode2D();
    m_ui->draw(*this);
    EndDrawing();
}

void Game::drawSky(float wx0, float wy0, float wx1, float wy1)
{
    const int cx = (int)(m_camTarget.x / TILE);
    const float horizon = (float)(m_world.surfaceY(cx) * TILE);
    const float d = daylight();

    float warm = 1.0f - fabsf(m_time - 0.25f) / 0.085f;
    warm = fmaxf(warm, 1.0f - fabsf(m_time - 0.75f) / 0.085f);
    warm = clampf(warm, 0.0f, 1.0f);

    const Color nightTop = rgb(8, 10, 30), nightMid = rgb(30, 38, 82);
    const Color dayTop = rgb(86, 158, 232), dayMid = rgb(172, 214, 246);
    const Color duskTop = rgb(72, 80, 162), duskMid = rgb(248, 162, 102);
    Color top = mixColor(nightTop, dayTop, d);
    Color mid = mixColor(nightMid, dayMid, d);
    top = mixColor(top, duskTop, warm * 0.72f);
    mid = mixColor(mid, duskMid, warm * 0.9f);
    const Color under = rgb(11, 9, 16);

    const float skyTopY = (float)(SKY_TOP * TILE);
    if (wy0 < horizon) {
        const float ya = fmaxf(wy0, skyTopY);
        const float yb = fminf(wy1, horizon);
        if (yb > ya) DrawRectangleGradientV((int)wx0, (int)ya, (int)(wx1 - wx0), (int)(yb - ya), top, mid);
        if (wy0 < skyTopY) DrawRectangle((int)wx0, (int)wy0, (int)(wx1 - wx0), (int)(skyTopY - wy0), nightTop);
    }
    if (wy1 > horizon) {
        const float ya = fmaxf(wy0, horizon);
        if (wy1 > ya) DrawRectangleGradientV((int)wx0, (int)ya, (int)(wx1 - wx0), (int)(wy1 - ya), mid, under);
    }

    // 星星
    const float starA = clampf(1.0f - d * 1.9f, 0.0f, 1.0f);
    if (starA > 0.02f && wy0 < horizon) {
        const int x0i = (int)floorf(wx0 / 40.0f), x1i = (int)ceilf(wx1 / 40.0f);
        const int y0i = (int)floorf(wy0 / 40.0f), y1i = (int)ceilf(fminf(wy1, horizon) / 40.0f);
        for (int y = y0i; y < y1i; y++)
            for (int x = x0i; x < x1i; x++) {
                const uint32_t h = hash2(x, y, 4242u);
                if ((h & 7) != 0) continue;
                const float px = (float)(x * 40 + (int)(h >> 8) % 34);
                const float py = (float)(y * 40 + (int)(h >> 16) % 30);
                const float tw = 0.55f + 0.45f * sinf((float)GetTime() * 2.0f + (float)(h & 255) * 0.13f);
                DrawRectangle((int)px, (int)py, 2, 2, withAlpha(rgb(240, 244, 255), (int)(200 * starA * tw)));
            }
    }

    // 太阳 / 月亮
    {
        const float arcA = (m_time - 0.25f) * 4.0f * PI;
        const float sx = m_camTarget.x - cosf(arcA) * 640.0f;
        const float sy = horizon - 60.0f - sinf(arcA) * 560.0f;
        if (sy < horizon + 40.0f && sy > wy0 - 260.0f) {
            if (m_time > 0.25f && m_time < 0.75f) drawSpriteC("sun", sx, sy, false, WHITE);
            else drawSpriteC("moon_full", sx, sy, false, WHITE);
        }
    }

    // 云
    if (wy0 < horizon) {
        const float t = (float)GetTime() * 14.0f;
        const Frame* cf = Assets::Find("cloud_day");
        if (cf) {
            for (int i = 0; i < 7; i++) {
                const float base = m_camTarget.x - 900.0f + (float)i * 300.0f;
                const float cx2 = base + fmodf(t + (float)i * 173.0f, 1800.0f);
                const float cy = horizon - 300.0f - (float)((i * 61) % 140);
                drawFrameC(*cf, cx2, cy, false, withAlpha(mixColor(WHITE, top, 0.35f), (int)(230 * d)));
            }
        }
    }
}



void Game::drawWorldTiles(float wx0, float wy0, float wx1, float wy1)
{
    static const Frame* s_tileFrame[T_COUNT] = { nullptr };
    static const Frame* s_wallFrame[W_COUNT] = { nullptr };
    if (!s_tileFrame[1]) {
        for (int i = 1; i < T_COUNT; i++) s_tileFrame[i] = Assets::FindTile(tileInfo((uint16_t)i).sprite);
        for (int i = 1; i < W_COUNT; i++) s_wallFrame[i] = Assets::FindTile(wallInfo((uint16_t)i).sprite);
    }
    const Color amb = ambient();
    const int x0 = (int)floorf(wx0 / TILE) - 1, x1 = (int)floorf(wx1 / TILE) + 1;
    const int y0 = (int)floorf(wy0 / TILE) - 1, y1 = (int)floorf(wy1 / TILE) + 1;
    const float wave = (float)GetTime() * 2.0f;

    for (int y = y0; y <= y1; y++) {
        if (y < 0 || y >= WORLD_H) continue;
        for (int x = x0; x <= x1; x++) {
            const float px = (float)(x * TILE), py = (float)(y * TILE);
            float l = m_world.lightAtPx(px + 8.0f, py + 8.0f);
            if (l < 0.06f) l = 0.06f;
            const Color tint = { (unsigned char)(amb.r * l), (unsigned char)(amb.g * l),
                                 (unsigned char)(amb.b * l), 255 };

            const uint16_t wl = m_world.wallAt(x, y);
            if (wl != W_NONE && s_wallFrame[wl]) {
                Color wt = tint;
                wt.r = (unsigned char)(wt.r * 0.60f);
                wt.g = (unsigned char)(wt.g * 0.60f);
                wt.b = (unsigned char)(wt.b * 0.60f);
                drawFrame(*s_wallFrame[wl], px, py, false, wt);
            }

            const uint16_t t = m_world.tileAt(x, y);
            if (t == T_AIR) continue;
            const Frame* f = (t < T_COUNT) ? s_tileFrame[t] : nullptr;
            if (!f) continue;

            if (t == T_WATER || t == T_LAVA) {
                const uint16_t up = (y > 0) ? m_world.tileAt(x, y - 1) : T_AIR;
                const Color wt = withAlpha(tint, t == T_LAVA ? 225 : 178);
                if (up == t) {
                    drawFrame(*f, px, py, false, wt);   // 水体内部也画水贴图, 不能用纯色
                } else {
                    // 自由表面: 按水量截出高度并加波浪
                    const int lvl = m_world.fluidAt(x, y);
                    int h = t == T_LAVA ? TILE : (int)floorf(clampf((float)lvl / 255.0f, 0.12f, 1.0f) * TILE + 0.5f);
                    if (h < 2) h = 2;
                    if (h >= TILE) {
                        drawFrame(*f, px, py, false, wt);
                    } else {
                        const float off = sinf(wave + (float)x * 0.7f) * 1.2f;
                        const Rectangle src{ f->rect.x, f->rect.y + (float)(TILE - h), f->rect.width, (float)h };
                        const Rectangle dst{ px, py + (float)(TILE - h) + off, (float)TILE, (float)h };
                        DrawTexturePro(f->tex, src, dst, { 0, 0 }, 0.0f, wt);
                    }
                }
                continue;
            }
            drawFrame(*f, px, py, false, tint);
        }
    }
}

void Game::drawDamageNumbers()
{
    for (const DamageText& d : m_dmg) {
        const float a = clampf(d.life * 2.2f, 0.0f, 1.0f);
        const Color c = d.crit ? withAlpha(rgb(255, 226, 120), (int)(255 * a))
                               : withAlpha(rgb(250, 250, 250), (int)(230 * a));
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", d.value);
        text::DrawCentered(buf, (int)d.pos.x, (int)d.pos.y, d.crit ? 18 : 14, c);
    }
}

}  // namespace tc
