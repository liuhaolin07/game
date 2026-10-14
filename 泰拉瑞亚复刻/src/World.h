// World.h —— 区块化世界: 地形存取 / 光照 / 液体 / 地形生成
#pragma once

#include "Common.h"
#include "Tiles.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tc {

enum Biome {
    BIOME_FOREST = 0, BIOME_DESERT, BIOME_SNOW, BIOME_JUNGLE,
    BIOME_CORRUPTION, BIOME_OCEAN, BIOME_CAVE, BIOME_HELL
};

const char* biomeName(int b);

struct Chunk {
    int cx = 0, cy = 0;
    std::vector<uint16_t> tile;
    std::vector<uint16_t> wall;
    std::vector<uint8_t> light;      // 0..15
    std::vector<uint8_t> fluid;      // 水的存量 0..255
    bool generated = false;
    bool modified = false;
    bool needLight = false;

    uint16_t t(int i) const { return tile[i]; }
    uint16_t w(int i) const { return wall[i]; }
};

class World {
public:
    void init(uint32_t seed);

    // --- 区块 ---
    Chunk* find(int cx, int cy);
    const Chunk* find(int cx, int cy) const;
    Chunk* ensure(int cx, int cy);          // 不存在则生成
    void stream(float camX, float camY, float dt);   // 按需生成/回收区块

    // --- 瓦片存取 ---
    uint16_t tileAt(int x, int y) const;
    uint16_t wallAt(int x, int y) const;
    uint8_t lightAt(int x, int y) const;
    float lightAtPx(float px, float py) const;   // 双线性插值, 0..1
    int fluidAt(int x, int y) const;            // 水量 0..255
    void setFluid(int x, int y, int v);
    void markWet(int x, int y);                 // 把一格加入"待更新"队列
    bool isWet(int x, int y) const;
    void setTile(int x, int y, uint16_t t);
    void setWall(int x, int y, uint16_t w);
    void setLight(int x, int y, uint8_t v);

    inline bool inWorld(int x, int y) const { return y >= 0 && y < WORLD_H; }

    // --- 碰撞查询 ---
    bool solidAt(int x, int y) const;
    bool solidAtPx(float px, float py) const { return solidAt((int)floorf(px / TILE), (int)floorf(py / TILE)); }
    bool platformAt(int x, int y) const;
    bool liquidAt(int x, int y) const;
    bool lavaAt(int x, int y) const;
    void updateFluids(int budget = 700);        // 每帧模拟若干格
    bool inWater(int x, int y) const { return liquidAt(x, y); }
    // 某一列最高的实心方块
    int surfaceHeight(int x) const;

    // --- 生成相关(纯函数, 与区块是否已生成无关) ---
    int surfaceY(int x) const;
    int biomeAt(int x) const;
    bool isOcean(int x) const;

    // --- 存档 ---
    struct SaveChunk { int cx, cy; std::vector<uint16_t> tile, wall; };
    std::vector<SaveChunk> collectModified() const;
    void restoreChunk(const SaveChunk& sc);
    void clearAll();

    uint32_t seed() const { return m_seed; }
    std::vector<Chunk*> allChunks() const;

    // 天空光等级(0..15), 随昼夜变化
    int skyLightLevel() const { return m_skyLight; }
    void setSkyLight(uint8_t v);

private:
    void genChunk(Chunk* c);
    void buildTreeColumn(int cx0, int cy0, int x, int chunksX0, int chunksY0);
    void relight(Chunk* c);
    void queueLight(Chunk* c);
    void flowStep(int x, int y);

    uint32_t m_seed = 0;
    uint8_t m_skyLight = LIGHT_MAX;
    std::vector<int64_t> m_wetList;             // 待更新的水格
    std::vector<int64_t> m_wetBatch;            // 复用的批处理缓冲
    std::unordered_set<int64_t> m_wet;
    std::unordered_map<int64_t, Chunk*> m_chunks;
    std::vector<Chunk*> m_relightQueue;
};

int64_t chunkKey(int cx, int cy);

}  // namespace tc
