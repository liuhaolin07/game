// World.cpp —— 区块存取 / 地形生成 / 光照传播
#include "World.h"

#include "Items.h"
#include "Noise.h"

#include <algorithm>

namespace tc {

const char* biomeName(int b)
{
    switch (b) {
    case BIOME_FOREST: return "森林";
    case BIOME_DESERT: return "沙漠";
    case BIOME_SNOW: return "雪原";
    case BIOME_JUNGLE: return "丛林";
    case BIOME_CORRUPTION: return "腐化之地";
    case BIOME_OCEAN: return "海洋";
    case BIOME_CAVE: return "洞穴";
    case BIOME_HELL: return "地狱";
    default: return "?";
    }
}

int64_t chunkKey(int cx, int cy) { return ((int64_t)(uint32_t)cx << 32) | (uint32_t)cy; }

// 光照区域边长 = 区块 + 四周扩张
static const int LP = LIGHT_PAD;
static const int LS = CHUNK + LP * 2;

void World::init(uint32_t seed)
{
    clearAll();
    m_seed = seed;
}

void World::clearAll()
{
    for (auto& kv : m_chunks) delete kv.second;
    m_chunks.clear();
    m_relightQueue.clear();
    m_wet.clear();
    m_wetList.clear();
}

void World::setSkyLight(uint8_t v)
{
    if (v == m_skyLight) return;
    m_skyLight = v;
    for (auto& kv : m_chunks) queueLight(kv.second);
}

Chunk* World::find(int cx, int cy)
{
    auto it = m_chunks.find(chunkKey(cx, cy));
    return it == m_chunks.end() ? nullptr : it->second;
}

const Chunk* World::find(int cx, int cy) const
{
    auto it = m_chunks.find(chunkKey(cx, cy));
    return it == m_chunks.end() ? nullptr : it->second;
}

std::vector<Chunk*> World::allChunks() const
{
    std::vector<Chunk*> v;
    v.reserve(m_chunks.size());
    for (auto& kv : m_chunks) v.push_back(kv.second);
    return v;
}

// ---------------------------------------------------------------- 地形函数
int World::surfaceY(int x) const
{
    float cont = fbm1((float)x * 0.0016f, m_seed + 1u, 4);
    int h = SURFACE_Y - 14 - (int)(cont * 46.0f);
    if (x < 140) h = OCEAN_Y + 30;
    else if (x < 240) h = (int)lerpN((float)(OCEAN_Y + 30), (float)(SURFACE_Y - 18), (float)(x - 140) / 100.0f);
    h += (int)(fbm1((float)x * 0.0090f, m_seed + 4u, 3) * 13.0f);   // 中频丘陵
    h += (int)(fbm1((float)x * 0.0230f, m_seed + 3u, 3) * 6.0f);    // 小起伏
    if (h < OCEAN_Y - 14) h = OCEAN_Y - 14;
    return h;
}

bool World::isOcean(int x) const { return surfaceY(x) >= OCEAN_Y; }

int World::biomeAt(int x) const
{
    int sy = surfaceY(x);
    if (sy >= OCEAN_Y) return BIOME_OCEAN;
    float b = fbm1((float)x * 0.00072f, m_seed + 11u, 3);
    float c = fbm1((float)x * 0.0021f, m_seed + 12u, 2);
    float v = b + c * 0.35f;
    if (v > 0.42f) return BIOME_CORRUPTION;
    if (v > 0.20f) return BIOME_JUNGLE;
    if (v < -0.40f) return BIOME_DESERT;
    if (v < -0.14f) return BIOME_SNOW;
    return BIOME_FOREST;
}

// ---------------------------------------------------------------- 存取
uint16_t World::tileAt(int x, int y) const
{
    if (y < 0 || y >= WORLD_H) return T_AIR;
    const Chunk* c = find(x >> 6, y >> 6);
    if (!c || !c->generated) return T_AIR;
    int lx = x & (CHUNK - 1), ly = y & (CHUNK - 1);
    return c->tile[ly * CHUNK + lx];
}

uint16_t World::wallAt(int x, int y) const
{
    if (y < 0 || y >= WORLD_H) return W_NONE;
    const Chunk* c = find(x >> 6, y >> 6);
    if (!c || !c->generated) return W_NONE;
    int lx = x & (CHUNK - 1), ly = y & (CHUNK - 1);
    return c->wall[ly * CHUNK + lx];
}

uint8_t World::lightAt(int x, int y) const
{
    if (y < 0) return LIGHT_MAX;
    if (y >= WORLD_H) return LIGHT_MAX;
    const Chunk* c = find(x >> 6, y >> 6);
    if (!c || !c->generated) return LIGHT_MAX;
    int lx = x & (CHUNK - 1), ly = y & (CHUNK - 1);
    return c->light[ly * CHUNK + lx];
}

float World::lightAtPx(float px, float py) const
{
    const float fx = px / TILE - 0.5f, fy = py / TILE - 0.5f;
    const int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
    const float tx = fx - (float)x0, ty = fy - (float)y0;
    const float a = lightAt(x0, y0), b = lightAt(x0 + 1, y0);
    const float c = lightAt(x0, y0 + 1), d = lightAt(x0 + 1, y0 + 1);
    const float top = a + (b - a) * tx;
    const float bot = c + (d - c) * tx;
    return clampf((top + (bot - top) * ty) / (float)LIGHT_MAX, 0.0f, 1.0f);
}

void World::setTile(int x, int y, uint16_t t){
    if (y < 0 || y >= WORLD_H) return;
    Chunk* c = find(x >> 6, y >> 6);
    if (!c || !c->generated) return;
    int lx = x & (CHUNK - 1), ly = y & (CHUNK - 1);
    const int li = ly * CHUNK + lx;
    if (c->fluid.empty()) c->fluid.assign(CHUNK_AREA, 0);
    const int had = c->fluid[li];
    const bool wasWater = c->tile[li] == T_WATER;
    c->tile[li] = t;
    c->modified = true;

    if (t == T_WATER) {
        c->fluid[li] = 255;
        markWet(x, y);
    } else {
        if (had > 0 || wasWater) {
            c->fluid[li] = 0;
            if (had > 0) {                       // 填方块: 水往上溢
                const int up = fluidAt(x, y - 1) + had;
                setFluid(x, y - 1, up);
                markWet(x, y - 1);
            }
        }
        // 与水相邻时唤醒模拟
        if (fluidAt(x, y - 1) > 0) markWet(x, y - 1);
        if (fluidAt(x, y + 1) > 0) markWet(x, y + 1);
        if (fluidAt(x - 1, y) > 0) markWet(x - 1, y);
        if (fluidAt(x + 1, y) > 0) markWet(x + 1, y);
        if (fluidAt(x, y) > 0) markWet(x, y);
    }
    queueLight(c);
    Chunk* n[4] = { find((x >> 6) - 1, y >> 6), find((x >> 6) + 1, y >> 6),
                    find(x >> 6, (y >> 6) - 1), find(x >> 6, (y >> 6) + 1) };
    for (Chunk* k : n) if (k) queueLight(k);
}

void World::setWall(int x, int y, uint16_t w)
{
    if (y < 0 || y >= WORLD_H) return;
    Chunk* c = find(x >> 6, y >> 6);
    if (!c || !c->generated) return;
    c->wall[(y & (CHUNK - 1)) * CHUNK + (x & (CHUNK - 1))] = w;
    c->modified = true;
}

void World::setLight(int x, int y, uint8_t v)
{
    if (y < 0 || y >= WORLD_H) return;
    Chunk* c = find(x >> 6, y >> 6);
    if (!c || !c->generated) return;
    c->light[(y & (CHUNK - 1)) * CHUNK + (x & (CHUNK - 1))] = v;
}

bool World::solidAt(int x, int y) const
{
    if (y < 0) return false;
    if (y >= WORLD_H) return true;
    return tileSolid(tileAt(x, y));
}

bool World::platformAt(int x, int y) const
{
    if (y < 0 || y >= WORLD_H) return false;
    return isPlatformTile(tileAt(x, y));
}

bool World::liquidAt(int x, int y) const
{
    if (y < 0 || y >= WORLD_H) return false;
    return isLiquid(tileAt(x, y));
}

bool World::lavaAt(int x, int y) const
{
    if (y < 0 || y >= WORLD_H) return false;
    return tileAt(x, y) == T_LAVA;
}

int World::surfaceHeight(int x) const
{
    int top = surfaceY(x) - 34;
    if (top < 0) top = 0;
    for (int y = top; y < WORLD_H; y++) {
        if (tileBlocksLight(tileAt(x, y))) return y;
    }
    return surfaceY(x);
}

// ---------------------------------------------------------------- 生成
namespace {

struct SoilSpec {
    uint16_t top;     // 地表方块
    uint16_t sub;     // 地下土壤
    uint16_t stone;   // 深层石
    uint16_t wall;    // 背景墙
    uint16_t topWall;
    int depth;        // 土壤厚度基准
};

SoilSpec soilFor(int biome)
{
    SoilSpec s{};
    switch (biome) {
    case BIOME_DESERT:  s = { T_SAND, T_SAND, T_STONE, W_STONE, W_SAND, 16 }; break;
    case BIOME_SNOW:    s = { T_SNOW, T_SNOW, T_STONE, W_STONE, W_SNOW, 9 };  break;
    case BIOME_JUNGLE:  s = { T_GRASS_JUNGLE, T_MUD, T_STONE_JUNGLE, W_JUNGLE, W_MUD, 6 }; break;
    case BIOME_CORRUPTION: s = { T_GRASS_CORRUPT, T_EBONSTONE, T_EBONSTONE, W_EBONSTONE, W_EBONSTONE, 8 }; break;
    case BIOME_OCEAN:   s = { T_SAND, T_SAND, T_STONE, W_STONE, W_SAND, 12 }; break;
    default:            s = { T_GRASS, T_DIRT, T_STONE, W_STONE, W_DIRT, 7 }; break;
    }
    return s;
}

}  // namespace

void World::genChunk(Chunk* c)
{
    const int x0 = c->cx * CHUNK, y0 = c->cy * CHUNK;
    c->tile.assign(CHUNK_AREA, T_AIR);
    c->wall.assign(CHUNK_AREA, W_NONE);
    c->light.assign(CHUNK_AREA, LIGHT_MAX);
    c->fluid.assign(CHUNK_AREA, 0);
    c->generated = true;

    for (int ly = 0; ly < CHUNK; ly++) {
        const int y = y0 + ly;
        if (y < 0) continue;
        for (int lx = 0; lx < CHUNK; lx++) {
            const int x = x0 + lx;
            const int i = ly * CHUNK + lx;

            if (y >= WORLD_H) { c->tile[i] = T_AIR; continue; }

            const int sy = surfaceY(x);
            const int biome = biomeAt(x);
            const SoilSpec soil = soilFor(biome);

            // ---- 地狱 ----
            if (y >= UNDERWORLD_Y) {
                int hwTop = UNDERWORLD_Y + (int)(fbm1((float)x * 0.009f, m_seed + 31u, 3) * 24.0f);
                if (y >= hwTop) {
                    c->wall[i] = W_HELLSTONE;
                    if (y > WORLD_H - 30) c->tile[i] = T_LAVA;
                    else {
                        float h = fbm2((float)x * 0.06f, (float)y * 0.06f, m_seed + 41u, 2);
                        c->tile[i] = (h > 0.42f) ? T_HELLSTONE : (h < -0.55f ? T_OBSIDIAN : T_ASH);
                    }
                } else {
                    c->wall[i] = W_STONE;
                }
                continue;
            }

            // ---- 空气 / 海洋水 ----
            if (y < sy) {
                if (biome == BIOME_OCEAN && y >= OCEAN_Y) c->tile[i] = T_WATER;
                continue;
            }

            // ---- 地表以下 ----
            const int d = y - sy;
            int soilDepth = soil.depth + (int)(fbm1((float)x * 0.05f, m_seed + 51u, 2) * 3.0f);
            uint16_t t;
            if (d == 0) t = (biome == BIOME_DESERT) ? T_SANDSTONE : soil.top;
            else if (d < soilDepth) t = soil.sub;
            else t = soil.stone;

            // 黏土层
            if (t == T_STONE && d > 12) {
                float cl = fbm2((float)x * 0.03f, (float)y * 0.05f, m_seed + 61u, 2);
                if (cl > 0.52f) t = T_CLAY;
            }
            // 地下雪/冰带
            if (t == T_STONE && d > 40 && d < 62 && biome == BIOME_SNOW) {
                if (fbm2((float)x * 0.04f, (float)y * 0.04f, m_seed + 62u, 2) > 0.35f) t = T_SNOW;
            }

            c->tile[i] = t;

            // ---- 背景墙 ----
            if (d < 3) c->wall[i] = soil.topWall;
            else c->wall[i] = soil.wall;
        }
    }

    // ---- 洞穴与矿脉(只在地表以下 / 地狱) ----
    for (int ly = 0; ly < CHUNK; ly++) {
        const int y = y0 + ly;
        if (y < 0 || y >= WORLD_H) continue;
        for (int lx = 0; lx < CHUNK; lx++) {
            const int x = x0 + lx;
            const int i = ly * CHUNK + lx;
            if (c->tile[i] == T_AIR || c->tile[i] == T_WATER) continue;

            const int sy = surfaceY(x);
            if (y >= UNDERWORLD_Y + 40) {
                // 地狱矿脉
                float h = fbm2((float)x * 0.08f, (float)y * 0.08f, m_seed + 71u, 2);
                if (h > 0.46f) c->tile[i] = T_ORE_HELLSTONE;
                continue;
            }
            const int d = y - sy;
            if (d < 4) continue;

            // 洞穴
            const float depthT = clampf((float)d / 300.0f, 0.0f, 1.0f);
            const float cn = fbm2((float)x * 0.036f, (float)y * 0.046f, m_seed + 21u, 3);
            const float big = fbm2((float)x * 0.0125f, (float)y * 0.0155f, m_seed + 22u, 2);
            const float th = 0.048f + depthT * 0.020f;
            bool carve = (cn > -th && cn < th) || (big > 0.60f - depthT * 0.10f);
            if (carve) { c->tile[i] = T_AIR; continue; }

            uint16_t t = c->tile[i];
            if (t == T_STONE || t == T_STONE_JUNGLE || t == T_EBONSTONE || t == T_SANDSTONE) {
                // 矿脉
                struct Ore { uint16_t tile; float freq, thr; int dmin, dmax; int salt; };
                static const Ore ores[] = {
                    { T_ORE_COPPER,   0.090f, 0.60f,  8, 320, 3 },
                    { T_ORE_IRON,     0.095f, 0.66f, 40, 420, 5 },
                    { T_ORE_SILVER,   0.100f, 0.74f, 90, 520, 7 },
                    { T_ORE_GOLD,     0.105f, 0.80f, 130, 560, 11 },
                    { T_ORE_DEMONITE, 0.110f, 0.84f, 170, 520, 13 },
                    { T_GEM_TOPAZ,    0.150f, 0.88f, 200, 600, 17 },
                    { T_GEM_AMETHYST, 0.152f, 0.89f, 210, 600, 19 },
                    { T_GEM_EMERALD,  0.154f, 0.90f, 220, 600, 23 },
                    { T_GEM_DIAMOND,  0.156f, 0.91f, 240, 600, 29 },
                };
                for (const Ore& o : ores) {
                    if (d < o.dmin || d > o.dmax) continue;
                    float v = fbm2((float)x * o.freq, (float)y * o.freq, m_seed + 100u + (uint32_t)o.salt, 2);
                    if (v > o.thr) { c->tile[i] = o.tile; break; }
                }
                // 荧石洞
                if (c->tile[i] == t) {
                    float gl = fbm2((float)x * 0.07f, (float)y * 0.07f, m_seed + 81u, 2);
                    if (gl > 0.74f) c->tile[i] = T_GLOWSTONE;
                }
            }
            // 丛林地下长藤蔓
            if (biomeAt(x) == BIOME_JUNGLE && d > 10 && d < 90 && c->tile[i] == T_MUD) {
                if (fbm2((float)x * 0.11f, (float)y * 0.05f, m_seed + 91u, 2) > 0.62f) c->tile[i] = T_VINE;
            }
        }
    }

    // ---- 洞穴里的水潭 ----
    for (int ly = 0; ly < CHUNK; ly++) {
        const int y = y0 + ly;
        if (y < SURFACE_Y || y >= UNDERWORLD_Y) continue;
        for (int lx = 0; lx < CHUNK; lx++) {
            const int x = x0 + lx, i = ly * CHUNK + lx;
            if (c->tile[i] != T_AIR) continue;
            if (ly + 1 < CHUNK && c->tile[(ly + 1) * CHUNK + lx] != T_AIR) continue;
            // 只在地面以下(洞穴/洼地)积水, 不会把地表上方的空气变成水
            float w = fbm2((float)x * 0.018f, (float)y * 0.035f, m_seed + 101u, 2);
            if (w > 0.56f && y >= surfaceY(x) + 5) c->tile[i] = T_WATER;
        }
    }

    // 初始水体一律视为"已稳定", 不进入模拟队列
    for (int i = 0; i < CHUNK_AREA; i++) if (c->tile[i] == T_WATER) c->fluid[i] = 255;

    // ---- 地表植被 / 树(跨区块, 用扩展列范围保证一致) ----
    const int fx0 = x0 - 48, fx1 = x0 + CHUNK + 48;
    for (int x = fx0; x < fx1; x++) buildTreeColumn(c->cx, c->cy, x, x0, y0);

    c->needLight = true;
    queueLight(c);
}

// 在列 x 上生成地表装饰, 只写入落在区块内的部分
void World::buildTreeColumn(int /*cx*/, int /*cy*/, int x, int x0, int y0)
{
    // 落在本区块内才写入
    if (x < x0 - 48 || x >= x0 + CHUNK + 48) return;
    const int sy = surfaceY(x);
    if (sy < 2 || sy >= WORLD_H) return;
    const int biome = biomeAt(x);

    // 找到该列的地表方块(可能在水面上)
    int gy = sy;
    while (gy < WORLD_H && tileAt(x, gy) == T_AIR) gy++;
    if (gy >= WORLD_H || gy - sy > 30) return;

    auto put = [&](int px, int py, uint16_t t, bool overwriteSolid) {
        if (px < x0 || px >= x0 + CHUNK || py < y0 || py >= y0 + CHUNK) return;
        Chunk* c = find(px >> 6, py >> 6);
        if (!c || !c->generated) return;
        const int i = (py & (CHUNK - 1)) * CHUNK + (px & (CHUNK - 1));
        if (t == T_AIR) { if (c->tile[i] != T_AIR) c->tile[i] = T_AIR; return; }
        if (!overwriteSolid && c->tile[i] != T_AIR) return;
        c->tile[i] = t;
    };

    const uint32_t h = hash2(x, 0, m_seed + 301u);
    const float r = rand01(h);

    if (biome == BIOME_DESERT) {
        // 仙人掌
        if (r > 0.955f) {
            int th = 2 + (int)(rand01(h * 7u) * 3.0f);
            for (int k = 0; k < th; k++) put(x, gy - 1 - k, T_WOOD, false);
        }
        return;
    }
    if (biome == BIOME_OCEAN) return;

    if (biome == BIOME_CORRUPTION) {
        // 枯树
        if (r > 0.972f) {
            int th = 4 + (int)(rand01(h * 13u) * 4.0f);
            for (int k = 0; k < th; k++) put(x, gy - 1 - k, T_WOOD, false);
        }
        // 腐化裂隙
        if (r < 0.05f) {
            for (int k = 0; k < 4; k++) put(x, gy + k, T_AIR, false);
        }
        return;
    }

    float chance = (biome == BIOME_JUNGLE) ? 0.30f : 0.22f;
    if (r > chance) return;

    const int isSnow = (biome == BIOME_SNOW);
    const bool tall = (biome == BIOME_JUNGLE) && (r > chance + 0.12f);
    int th = tall ? 13 + (int)(rand01(h * 3u) * 8.0f) : (5 + (int)(rand01(h * 5u) * 5.0f));
    if (isSnow) th = 7 + (int)(rand01(h * 11u) * 4.0f);

    // 检查树基位置是空的
    if (tileAt(x, gy - 1) != T_AIR) return;
    if (tileAt(x + 1, gy) != T_AIR && tileAt(x - 1, gy) != T_AIR) return;

    for (int k = 0; k < th; k++) put(x, gy - 1 - k, T_WOOD, false);

    const uint16_t leaf = (biome == BIOME_JUNGLE) ? T_LEAVES_JUNGLE : T_LEAVES;
    const int topY = gy - th;
    if (tall) {
        // 丛林高树: 上下两团树冠
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -3; dx <= 3; dx++) {
                float dist = sqrtf((float)(dx * dx + dy * dy));
                if (dist > 3.4f) continue;
                put(x + dx, topY + dy, leaf, true);
            }
        for (int dy = 0; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++) {
                if (abs(dx) + abs(dy) > 3) continue;
                put(x + dx, topY + 4 + dy, leaf, true);
            }
    } else {
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -3; dx <= 3; dx++) {
                float dist = sqrtf((float)(dx * dx * 0.7f + dy * dy));
                if (dist > 3.1f) continue;
                put(x + dx, topY + dy, leaf, true);
            }
    }
}

// ---------------------------------------------------------------- 光照
void World::queueLight(Chunk* c)
{
    if (!c || c->needLight) return;
    c->needLight = true;
    m_relightQueue.push_back(c);
}

void World::relight(Chunk* c)
{
    static std::vector<uint8_t> buf(LS * LS);
    static std::vector<int> queue;
    const int x0 = c->cx * CHUNK - LP;
    const int y0 = c->cy * CHUNK - LP;

    auto tileAtLocal = [&](int i, int j) -> uint16_t {
        const int wx = x0 + i, wy = y0 + j;
        if (wy < 0 || wy >= WORLD_H) return T_AIR;
        const Chunk* n = find(wx >> 6, wy >> 6);
        if (!n || !n->generated) return T_AIR;
        return n->tile[(wy & (CHUNK - 1)) * CHUNK + (wx & (CHUNK - 1))];
    };

    buf.assign(LS * LS, 0);

    // 天空光: 从上往下灌
    for (int i = 0; i < LS; i++) {
        for (int j = 0; j < LS; j++) {
            const int wy = y0 + j;
            if (wy >= WORLD_H) break;
            const uint16_t t = tileAtLocal(i, j);
            if (tileBlocksLight(t)) break;
            buf[j * LS + i] = m_skyLight;
        }
    }

    // 自发光
    queue.clear();
    for (int j = 0; j < LS; j++) {
        const int wy = y0 + j;
        if (wy < 0 || wy >= WORLD_H) continue;
        for (int i = 0; i < LS; i++) {
            const uint16_t t = tileAtLocal(i, j);
            const TileDef& def = tileInfo(t);
            const int k = j * LS + i;
            if (def.emit) {
                if (def.lightPower > buf[k]) buf[k] = (uint8_t)def.lightPower;
            }
        }
    }
    for (int k = 0; k < LS * LS; k++) if (buf[k] > 0) queue.push_back(k);

    // 广度优先扩散
    size_t head = 0;
    while (head < queue.size()) {
        const int k = queue[head++];
        const int cur = buf[k];
        if (cur <= 1) continue;
        const int i = k % LS, j = k / LS;
        static const int dx4[4] = { 1, -1, 0, 0 };
        static const int dy4[4] = { 0, 0, 1, -1 };
        for (int d = 0; d < 4; d++) {
            const int ni = i + dx4[d], nj = j + dy4[d];
            if (ni < 0 || nj < 0 || ni >= LS || nj >= LS) continue;
            const int nk = nj * LS + ni;
            const uint16_t nt = tileAtLocal(ni, nj);
            if (tileBlocksLight(nt)) continue;
            int decay = tileInfo(nt).dim ? 2 : 1;
            const int nv = cur - decay;
            if (nv > buf[nk]) { buf[nk] = (uint8_t)nv; queue.push_back(nk); }
        }
    }

    for (int ly = 0; ly < CHUNK; ly++)
        for (int lx = 0; lx < CHUNK; lx++)
            c->light[ly * CHUNK + lx] = buf[(ly + LP) * LS + (lx + LP)];

    c->needLight = false;
}

// ---------------------------------------------------------------- 流式加载
void World::stream(float camX, float camY, float)
{
    const int pcx = (int)floorf(camX / TILE) >> 6;
    const int pcy = (int)floorf(camY / TILE) >> 6;
    const int R = 6;      // 加载半径(区块)
    const int KEEP = 9;

    // 目标区块列表(按距离从近到远), 玩家跨区块时重建
    static std::vector<std::pair<int, int>> pending;   // (距离平方, 打包坐标)
    static int lastCx = 0x7fffffff, lastCy = 0x7fffffff;
    if (lastCx != pcx || lastCy != pcy) {
        lastCx = pcx; lastCy = pcy;
        pending.clear();
        for (int dy = -R; dy <= R; dy++)
            for (int dx = -R; dx <= R; dx++)
                if (dx * dx + dy * dy <= R * R)
                    pending.push_back({ dx * dx + dy * dy, (pcx + dx) * 100003 + (pcy + dy) });
        std::sort(pending.begin(), pending.end());
    }

    int budget = 3;
    for (size_t i = 0; i < pending.size() && budget > 0; i++) {
        const int key = pending[i].second;
        const int cx = key / 100003, cy = key - cx * 100003;
        if (find(cx, cy)) continue;
        Chunk* c = new Chunk();
        c->cx = cx; c->cy = cy;
        m_chunks[chunkKey(cx, cy)] = c;
        genChunk(c);
        // 四邻也要存在, 否则光照等邻居就绪
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                Chunk* n = find(cx + dx, cy + dy);
                if (n) queueLight(n);
            }
        budget--;
    }

    // 光照更新(每帧最多 3 个; 邻居没就绪就整帧等待)
    int lb = 3;
    while (lb-- > 0 && !m_relightQueue.empty()) {
        Chunk* c = m_relightQueue.front();
        m_relightQueue.erase(m_relightQueue.begin());
        if (!c->needLight) continue;
        bool ready = true;
        for (int dy = -1; dy <= 1 && ready; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                Chunk* n = find(c->cx + dx, c->cy + dy);
                if (!n || !n->generated) { ready = false; break; }
            }
        if (!ready) {
            m_relightQueue.insert(m_relightQueue.begin(), c);
            break;
        }
        relight(c);
    }

    // 回收
    if (m_chunks.size() > 0) {
        std::vector<int64_t> drop;
        for (auto& kv : m_chunks) {
            Chunk* c = kv.second;
            int dx = c->cx - pcx, dy = c->cy - pcy;
            if (dx * dx + dy * dy <= KEEP * KEEP) continue;
            if (c->modified) continue;
            drop.push_back(kv.first);
        }
        for (int64_t k : drop) {
            auto it = m_chunks.find(k);
            if (it != m_chunks.end()) { delete it->second; m_chunks.erase(it); }
        }
    }
}


// ---------------------------------------------------------------- 液体模拟
// 规则: 只有"湿"的格子才会被更新。挖开/填上与水相邻的方块时把邻格标记为湿,
// 变化再向外传播。没有被扰动的海洋/水潭初始就是稳定的, 不会消耗模拟预算。
namespace {
inline int64_t cellKey(int x, int y) { return ((int64_t)(uint32_t)x << 32) | (uint32_t)y; }
inline int keyX(int64_t k) { return (int)(uint32_t)(k >> 32); }
inline int keyY(int64_t k) { return (int)(uint32_t)k; }
constexpr int MAX_WET = 40000;
}  // namespace

int World::fluidAt(int x, int y) const
{
    if (y < 0 || y >= WORLD_H) return 0;
    const Chunk* c = find(x >> 6, y >> 6);
    if (!c || !c->generated || c->fluid.empty()) return 0;
    return c->fluid[(y & (CHUNK - 1)) * CHUNK + (x & (CHUNK - 1))];
}

void World::setFluid(int x, int y, int v)
{
    if (y < 0 || y >= WORLD_H) return;
    Chunk* c = find(x >> 6, y >> 6);
    if (!c || !c->generated || c->fluid.empty()) return;
    const int i = (y & (CHUNK - 1)) * CHUNK + (x & (CHUNK - 1));
    c->fluid[i] = (uint8_t)clampi(v, 0, 255);
    // 0 <-> 非 0 变化时同步方块 id
    const bool wasWater = c->tile[i] == T_WATER;
    const bool isWater = c->fluid[i] > 0;
    if (wasWater != isWater) {
        c->tile[i] = isWater ? (uint16_t)T_WATER : (uint16_t)T_AIR;
        c->modified = true;
        queueLight(c);
        Chunk* n[4] = { find((x >> 6) - 1, y >> 6), find((x >> 6) + 1, y >> 6),
                        find(x >> 6, (y >> 6) - 1), find(x >> 6, (y >> 6) + 1) };
        for (Chunk* k : n) if (k) queueLight(k);
    }
}

void World::markWet(int x, int y)
{
    if (y < 0 || y >= WORLD_H) return;
    if ((int)m_wet.size() >= MAX_WET) return;
    const int64_t k = cellKey(x, y);
    if (m_wet.insert(k).second) m_wetList.push_back(k);
}

bool World::isWet(int x, int y) const { return m_wet.count(cellKey(x, y)) != 0; }

// 水能否进入这一格: 空气 / 水 / 平台 / 门 都算
static inline bool waterCanEnter(uint16_t t)
{
    return t == T_AIR || t == T_WATER || t == T_PLATFORM || t == T_FENCE || t == T_VINE ||
           t == T_DOOR || t == T_DOOR_OPEN;
}

void World::flowStep(int x, int y)
{
    int lvl = fluidAt(x, y);
    const uint16_t self = tileAt(x, y);
    if (!waterCanEnter(self)) lvl = 0;          // 被方块堵住
    if (lvl > 0) setFluid(x, y, lvl);

    // 1) 向下流(重力): 下面是空的就直接掉下去, 下面有水就均压
    const uint16_t belowTile = tileAt(x, y + 1);
    if (y + 1 < WORLD_H && waterCanEnter(belowTile)) {
        int b = fluidAt(x, y + 1);
        if (b < 255) {
            const int move = (b == 0) ? lvl : (lvl > b ? std::max(1, (lvl - b) / 2) : 0);
            if (move > 0) {
                setFluid(x, y, lvl - move);
                setFluid(x, y + 1, b + move);
                markWet(x, y);
                markWet(x, y + 1);
                markWet(x, y - 1);
                markWet(x - 1, y + 1);
                markWet(x + 1, y + 1);
                return;
            }
        }
    }

    // 2) 向两侧摊开: 只在"已经掉不下去"(躺在地面上)或几乎装满时才摊,
    //    否则水会在半空中摊成一片薄云, 看起来像悬空
    const bool blocked = !(y + 1 < WORLD_H && waterCanEnter(tileAt(x, y + 1)));
    const bool spread = blocked || lvl > 220;
    if (spread) {
        for (int s = -1; s <= 1; s += 2) {
            const int nx = x + s;
            const uint16_t nt = tileAt(nx, y);
            if (!waterCanEnter(nt)) continue;
            const int n = fluidAt(nx, y);
            if (n >= 255) continue;
            // 必须有足够水位差才摊开, 否则 1 单位的水会永远爬满整个世界
            if (lvl - n < 8) continue;
            const int move = (lvl - n) / 4;
            if (move <= 0 || move >= lvl) continue;
            setFluid(x, y, lvl - move);
            setFluid(nx, y, n + move);
            markWet(x, y);
            markWet(nx, y);
            markWet(nx, y - 1);
            markWet(nx, y + 1);
        }
    }
}

void World::updateFluids(int budget)
{
    if (m_wetList.empty()) return;
    if (budget > (int)m_wetList.size()) budget = (int)m_wetList.size();
    if (budget <= 0) return;

    // 整批摘出来并立刻从集合中移除: 这样处理过程中被 markWet 再次标记的格子
    // 会重新入队, 不会因为"已经在集合里"而被漏掉
    const int n = budget;
    m_wetBatch.assign(m_wetList.begin(), m_wetList.begin() + n);
    m_wetList.erase(m_wetList.begin(), m_wetList.begin() + n);
    for (int64_t k : m_wetBatch) m_wet.erase(k);

    for (int64_t k : m_wetBatch) {
        const int x = keyX(k), y = keyY(k);
        if (!find(x >> 6, y >> 6)) continue;      // 区块没加载就跳过
        flowStep(x, y);
    }

    // 兜底: 队列过长时按集合重建
    if ((int)m_wetList.size() > MAX_WET) {
        m_wetList.clear();
        m_wetList.reserve(m_wet.size());
        for (int64_t k : m_wet) m_wetList.push_back(k);
    }
}

// ---------------------------------------------------------------- 存档
std::vector<World::SaveChunk> World::collectModified() const
{
    std::vector<SaveChunk> v;
    for (auto& kv : m_chunks) {
        const Chunk* c = kv.second;
        if (!c->modified || !c->generated) continue;
        SaveChunk s;
        s.cx = c->cx; s.cy = c->cy;
        s.tile = c->tile; s.wall = c->wall;
        v.push_back(s);
    }
    return v;
}

void World::restoreChunk(const SaveChunk& sc)
{
    Chunk* c = find(sc.cx, sc.cy);
    if (!c) {
        c = new Chunk();
        c->cx = sc.cx; c->cy = sc.cy;
        m_chunks[chunkKey(sc.cx, sc.cy)] = c;
        genChunk(c);
    }
    c->tile = sc.tile;
    c->wall = sc.wall;
    c->fluid.assign(CHUNK_AREA, 0);
    for (int i = 0; i < CHUNK_AREA; i++) if (c->tile[i] == T_WATER) c->fluid[i] = 255;
    c->modified = true;
    c->needLight = true;
    queueLight(c);
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            Chunk* n = find(sc.cx + dx, sc.cy + dy);
            if (n) queueLight(n);
        }
}

}  // namespace tc
