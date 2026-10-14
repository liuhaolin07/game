// Tiles.cpp —— 方块与背景墙定义表
#include "Tiles.h"
#include "Items.h"

namespace tc {

namespace {

// sprite, 中文名, solid, opaque, dim, emit, light, hardness, pickPower, drop, min, max, mapColor, liquid, door, furniture
const TileDef kTiles[T_COUNT] = {
    /* T_AIR            */ { "air", "空气", SK_NONE, false, false, false, 0, 0.0f, 0, IT_NONE, 0, 0, rgb(0, 0, 0, 0), false, false, false },
    /* T_DIRT           */ { "dirt", "泥土", SK_SOLID, true, false, false, 0, 0.55f, 0, IT_DIRT, 1, 1, rgb(134, 96, 67), false, false, false },
    /* T_GRASS          */ { "grass", "草地", SK_SOLID, true, false, false, 0, 0.60f, 0, IT_DIRT, 1, 1, rgb(96, 156, 70), false, false, false },
    /* T_STONE          */ { "stone", "石头", SK_SOLID, true, false, false, 0, 1.40f, 0, IT_STONE, 1, 1, rgb(128, 128, 134), false, false, false },
    /* T_WOOD           */ { "wood", "木头", SK_SOLID, true, false, false, 0, 0.90f, 0, IT_WOOD, 1, 1, rgb(139, 100, 63), false, false, false },
    /* T_WOOD_PANEL     */ { "wood_panel", "木板", SK_SOLID, true, false, false, 0, 0.80f, 0, IT_WOOD_PANEL, 1, 1, rgb(170, 122, 72), false, false, false },
    /* T_LEAVES         */ { "leaves", "树叶", SK_SOLID, true, true, false, 0, 0.15f, 0, IT_NONE, 0, 0, rgb(58, 128, 44), false, false, false },
    /* T_LEAVES_JUNGLE  */ { "leaves_jungle", "丛林树叶", SK_SOLID, true, true, false, 0, 0.15f, 0, IT_NONE, 0, 0, rgb(48, 140, 60), false, false, false },
    /* T_SAND           */ { "sand", "沙子", SK_SOLID, true, false, false, 0, 0.50f, 0, IT_SAND, 1, 1, rgb(219, 199, 133), false, false, false },
    /* T_SANDSTONE      */ { "sandstone", "砂岩", SK_SOLID, true, false, false, 0, 1.00f, 0, IT_SANDSTONE, 1, 1, rgb(190, 162, 110), false, false, false },
    /* T_SNOW           */ { "snow", "雪块", SK_SOLID, true, false, false, 0, 0.22f, 0, IT_SNOW, 1, 1, rgb(233, 240, 248), false, false, false },
    /* T_ICE            */ { "ice", "冰块", SK_SOLID, true, true, false, 0, 0.60f, 0, IT_ICE, 1, 1, rgb(141, 205, 232), false, false, false },
    /* T_MUD            */ { "mud", "泥巴", SK_SOLID, true, false, false, 0, 0.60f, 0, IT_MUD, 1, 1, rgb(104, 72, 52), false, false, false },
    /* T_STONE_JUNGLE   */ { "stone_jungle", "丛林石", SK_SOLID, true, false, false, 0, 1.40f, 0, IT_STONE_JUNGLE, 1, 1, rgb(88, 118, 96), false, false, false },
    /* T_GRASS_JUNGLE   */ { "grass_jungle", "丛林草", SK_SOLID, true, false, false, 0, 0.60f, 0, IT_MUD, 1, 1, rgb(58, 156, 66), false, false, false },
    /* T_EBONSTONE      */ { "ebonstone", "黑石", SK_SOLID, true, false, false, 0, 1.60f, 0, IT_EBONSTONE, 1, 1, rgb(62, 58, 88), false, false, false },
    /* T_GRASS_CORRUPT  */ { "grass_corruption", "腐化草", SK_SOLID, true, false, false, 0, 0.60f, 0, IT_EBONSTONE, 1, 1, rgb(108, 66, 148), false, false, false },
    /* T_OBSIDIAN       */ { "obsidian", "黑曜石", SK_SOLID, true, false, false, 0, 3.20f, 65, IT_OBSIDIAN, 1, 1, rgb(46, 38, 66), false, false, false },
    /* T_ASH            */ { "ash", "灰烬", SK_SOLID, true, false, false, 0, 0.30f, 0, IT_ASH, 1, 1, rgb(74, 70, 70), false, false, false },
    /* T_HELLSTONE      */ { "hellstone", "狱石", SK_SOLID, true, false, false, 0, 2.00f, 55, IT_HELLSTONE_BLOCK, 1, 1, rgb(92, 58, 42), false, false, false },
    /* T_GLOWSTONE      */ { "glowstone", "荧石", SK_SOLID, true, false, true, 14, 0.50f, 0, IT_GLOWSTONE, 1, 1, rgb(214, 186, 118), false, false, false },
    /* T_GLASS          */ { "glass", "玻璃", SK_SOLID, true, true, false, 0, 0.30f, 0, IT_GLASS, 1, 1, rgb(168, 200, 214), false, false, false },
    /* T_BRICKS         */ { "bricks", "红砖", SK_SOLID, true, false, false, 0, 1.20f, 0, IT_BRICKS, 1, 1, rgb(158, 76, 68), false, false, false },
    /* T_BRICKS_STONE   */ { "bricks_stone", "石砖", SK_SOLID, true, false, false, 0, 1.20f, 0, IT_BRICKS_STONE, 1, 1, rgb(122, 122, 130), false, false, false },
    /* T_MUD_BRICK      */ { "mud_brick", "泥砖", SK_SOLID, true, false, false, 0, 1.10f, 0, IT_MUD_BRICK, 1, 1, rgb(112, 80, 58), false, false, false },
    /* T_SNOW_BRICK     */ { "snow_brick", "雪砖", SK_SOLID, true, false, false, 0, 1.10f, 0, IT_SNOW_BRICK, 1, 1, rgb(206, 224, 240), false, false, false },
    /* T_CLAY           */ { "clay", "黏土", SK_SOLID, true, false, false, 0, 0.70f, 0, IT_CLAY, 1, 1, rgb(160, 122, 128), false, false, false },
    /* T_CLAY_BRICK     */ { "clay_brick", "黏土砖", SK_SOLID, true, false, false, 0, 1.20f, 0, IT_CLAY_BRICK, 1, 1, rgb(150, 106, 112), false, false, false },
    /* T_TORCH          */ { "torch", "火把", SK_NONE, false, false, true, 14, 0.05f, 0, IT_TORCH, 1, 1, rgb(255, 180, 60), false, false, true },
    /* T_CAMPFIRE       */ { "campfire", "篝火", SK_NONE, false, false, true, 13, 0.05f, 0, IT_CAMPFIRE, 1, 1, rgb(240, 140, 50), false, false, true },
    /* T_WORKBENCH      */ { "workbench", "工作台", SK_NONE, true, false, false, 0, 0.0f, 0, IT_NONE, 0, 0, rgb(154, 112, 70), false, false, true },
    /* T_FURNACE        */ { "furnace", "熔炉", SK_SOLID, true, false, false, 0, 0.0f, 0, IT_NONE, 0, 0, rgb(122, 122, 128), false, false, false },
    /* T_FURNACE_LIT    */ { "furnace_lit", "熔炉", SK_SOLID, true, false, true, 11, 0.0f, 0, IT_NONE, 0, 0, rgb(200, 120, 50), false, false, false },
    /* T_ANVIL          */ { "anvil", "铁砧", SK_SOLID, true, false, false, 0, 0.0f, 0, IT_NONE, 0, 0, rgb(86, 88, 98), false, false, false },
    /* T_CHEST          */ { "chest", "箱子", SK_SOLID, true, false, false, 0, 0.0f, 0, IT_NONE, 0, 0, rgb(162, 110, 58), false, false, false },
    /* T_DOOR           */ { "door", "木门", SK_DOOR, true, false, false, 0, 0.60f, 0, IT_DOOR, 1, 1, rgb(156, 106, 60), false, true, false },
    /* T_DOOR_OPEN      */ { "door_open", "木门", SK_DOOR, false, false, false, 0, 0.60f, 0, IT_DOOR, 1, 1, rgb(156, 106, 60), false, true, false },
    /* T_PLATFORM       */ { "platform", "平台", SK_PLATFORM, false, false, false, 0, 0.10f, 0, IT_PLATFORM, 1, 1, rgb(150, 104, 60), false, false, true },
    /* T_FENCE          */ { "fence", "木栅栏", SK_PLATFORM, false, false, false, 0, 0.10f, 0, IT_FENCE, 1, 1, rgb(150, 104, 60), false, false, true },
    /* T_VINE           */ { "vine", "藤蔓", SK_PLATFORM, false, false, false, 0, 0.08f, 0, IT_VINE, 1, 1, rgb(46, 118, 44), false, false, true },
    /* T_WATER          */ { "water", "水", SK_NONE, false, true, false, 0, 0.0f, 0, IT_NONE, 0, 0, rgb(58, 110, 205), true, false, false },
    /* T_LAVA           */ { "lava", "岩浆", SK_NONE, true, true, true, 12, 0.0f, 0, IT_NONE, 0, 0, rgb(214, 88, 22), true, false, false },
    /* T_ORE_COPPER     */ { "ore_copper", "铜矿", SK_SOLID, true, false, false, 0, 1.60f, 0, IT_ORE_COPPER, 1, 1, rgb(176, 96, 56), false, false, false },
    /* T_ORE_IRON       */ { "ore_iron", "铁矿", SK_SOLID, true, false, false, 0, 1.80f, 0, IT_ORE_IRON, 1, 1, rgb(152, 110, 84), false, false, false },
    /* T_ORE_SILVER     */ { "ore_silver", "白银矿", SK_SOLID, true, false, false, 0, 2.00f, 25, IT_ORE_SILVER, 1, 1, rgb(198, 202, 214), false, false, false },
    /* T_ORE_GOLD       */ { "ore_gold", "黄金矿", SK_SOLID, true, false, false, 0, 2.20f, 40, IT_ORE_GOLD, 1, 1, rgb(216, 180, 62), false, false, false },
    /* T_ORE_DEMONITE   */ { "ore_demonite", "陨石矿", SK_SOLID, true, false, false, 0, 2.40f, 55, IT_ORE_DEMONITE, 1, 1, rgb(86, 44, 70), false, false, false },
    /* T_ORE_HELLSTONE  */ { "ore_hellstone", "狱石矿", SK_SOLID, true, false, false, 0, 2.40f, 55, IT_ORE_HELLSTONE, 1, 1, rgb(232, 122, 42), false, false, false },
    /* T_GEM_TOPAZ      */ { "gem_topaz", "黄玉矿", SK_SOLID, true, false, false, 0, 2.60f, 55, IT_TOPAZ, 1, 1, rgb(226, 168, 52), false, false, false },
    /* T_GEM_AMETHYST   */ { "gem_amethyst", "紫晶矿", SK_SOLID, true, false, false, 0, 2.60f, 55, IT_AMETHYST, 1, 1, rgb(154, 88, 210), false, false, false },
    /* T_GEM_EMERALD    */ { "gem_emerald", "绿玉矿", SK_SOLID, true, false, false, 0, 2.60f, 55, IT_EMERALD, 1, 1, rgb(56, 190, 122), false, false, false },
    /* T_GEM_DIAMOND    */ { "gem_diamond", "钻石矿", SK_SOLID, true, false, false, 0, 2.60f, 55, IT_DIAMOND, 1, 1, rgb(96, 194, 232), false, false, false },
};

const WallDef kWalls[W_COUNT] = {
    /* W_NONE        */ { "", "", rgb(0, 0, 0, 0) },
    /* W_DIRT        */ { "wall_dirt", "土墙", rgb(86, 66, 50) },
    /* W_STONE       */ { "wall_stone", "石墙", rgb(86, 86, 92) },
    /* W_WOOD        */ { "wall_wood", "木墙", rgb(104, 76, 48) },
    /* W_SAND        */ { "wall_sand", "沙墙", rgb(168, 150, 106) },
    /* W_SNOW        */ { "wall_snow", "雪墙", rgb(176, 194, 212) },
    /* W_MUD         */ { "wall_mud", "泥墙", rgb(78, 56, 40) },
    /* W_JUNGLE      */ { "wall_jungle", "丛林墙", rgb(62, 88, 68) },
    /* W_EBONSTONE   */ { "wall_ebonstone", "黑石墙", rgb(44, 42, 66) },
    /* W_HELLSTONE   */ { "wall_hellstone", "狱石墙", rgb(74, 46, 36) },
    /* W_BRICK       */ { "wall_brick", "砖墙", rgb(122, 60, 56) },
};

}  // namespace

static const TileDef kAirTile = { "air", "空气", SK_NONE, false, false, false, 0, 0.0f, 0, IT_NONE, 0, 0, rgb(0, 0, 0, 0), false, false, false };

const TileDef& tileInfo(uint16_t id) { return id < T_COUNT ? kTiles[id] : kAirTile; }
const WallDef& wallInfo(uint16_t id) { return id < W_COUNT ? kWalls[id] : kWalls[0]; }
const char* tileName(uint16_t id) { return tileInfo(id).name; }
const char* wallName(uint16_t id) { return wallInfo(id).name; }

}  // namespace tc
