// Tiles.h —— 方块与背景墙定义
#pragma once

#include "Common.h"

#include <string>

namespace tc {

// ---------------------------------------------------------------- 方块
enum Tile : uint16_t {
    T_AIR = 0,
    T_DIRT, T_GRASS, T_STONE, T_WOOD, T_WOOD_PANEL, T_LEAVES, T_LEAVES_JUNGLE,
    T_SAND, T_SANDSTONE, T_SNOW, T_ICE, T_MUD, T_STONE_JUNGLE, T_GRASS_JUNGLE,
    T_EBONSTONE, T_GRASS_CORRUPT, T_OBSIDIAN, T_ASH, T_HELLSTONE, T_GLOWSTONE,
    T_GLASS, T_BRICKS, T_BRICKS_STONE, T_MUD_BRICK, T_SNOW_BRICK, T_CLAY, T_CLAY_BRICK,
    T_TORCH, T_CAMPFIRE, T_WORKBENCH, T_FURNACE, T_FURNACE_LIT, T_ANVIL, T_CHEST,
    T_DOOR, T_DOOR_OPEN, T_PLATFORM, T_FENCE, T_VINE,
    T_WATER, T_LAVA,
    T_ORE_COPPER, T_ORE_IRON, T_ORE_SILVER, T_ORE_GOLD, T_ORE_DEMONITE, T_ORE_HELLSTONE,
    T_GEM_TOPAZ, T_GEM_AMETHYST, T_GEM_EMERALD, T_GEM_DIAMOND,
    T_COUNT
};

// ---------------------------------------------------------------- 背景墙
enum Wall : uint16_t {
    W_NONE = 0,
    W_DIRT, W_STONE, W_WOOD, W_SAND, W_SNOW, W_MUD, W_JUNGLE,
    W_EBONSTONE, W_HELLSTONE, W_BRICK,
    W_COUNT
};

enum SolidKind { SK_NONE, SK_SOLID, SK_PLATFORM, SK_FENCE, SK_DOOR };

struct TileDef {
    const char* sprite;
    const char* name;
    SolidKind solid;
    bool opaque;      // 完全阻挡光线与背景
    bool dim;         // 半透明(树叶/玻璃), 只挡一部分光
    bool emit;        // 自发光
    int lightPower;
    float hardness;   // 挖掘难度系数, 0 表示不能挖
    int pickPower;    // 需要的镐等级
    int itemDrop;     // 掉落物品 id (Items.h), 0 表示不掉落
    int itemMin, itemMax;
    Color mapColor;
    bool liquid;
    bool door;        // 门(可开关)
    bool furniture;   // 需要背景墙
};

struct WallDef {
    const char* sprite;
    const char* name;
    Color mapColor;
};

const TileDef& tileInfo(uint16_t id);
const WallDef& wallInfo(uint16_t id);
const char* tileName(uint16_t id);
const char* wallName(uint16_t id);

inline bool tileSolid(uint16_t id)
{
    return id != T_AIR && tileInfo(id).solid == SK_SOLID;
}
inline bool tileBlocksLight(uint16_t id) { return id != T_AIR && tileInfo(id).opaque; }
inline bool isLiquid(uint16_t id) { return id != T_AIR && tileInfo(id).liquid; }
inline bool isPlatformTile(uint16_t id) { return id != T_AIR && tileInfo(id).solid == SK_PLATFORM; }

}  // namespace tc
