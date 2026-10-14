// Items.h —— 物品定义与合成配方
#pragma once

#include "Common.h"
#include "Tiles.h"

#include <string>
#include <vector>

namespace tc {

enum ItemId {
    IT_NONE = 0,
    // ---- 方块 / 建材 ----
    IT_DIRT, IT_STONE, IT_SAND, IT_SANDSTONE, IT_SNOW, IT_ICE, IT_MUD, IT_STONE_JUNGLE,
    IT_EBONSTONE, IT_OBSIDIAN, IT_ASH, IT_HELLSTONE_BLOCK, IT_GLASS, IT_BRICKS, IT_BRICKS_STONE,
    IT_MUD_BRICK, IT_SNOW_BRICK, IT_WOOD_PANEL, IT_CLAY, IT_CLAY_BRICK, IT_GLOWSTONE,
    IT_WORKBENCH, IT_FURNACE, IT_ANVIL, IT_CHEST, IT_CAMPFIRE, IT_FENCE, IT_VINE, IT_PLATFORM,
    IT_WOOD, IT_TORCH, IT_DOOR,
    // ---- 矿石 / 宝石 ----
    IT_ORE_COPPER, IT_ORE_IRON, IT_ORE_SILVER, IT_ORE_GOLD, IT_ORE_DEMONITE, IT_ORE_HELLSTONE,
    IT_TOPAZ, IT_AMETHYST, IT_EMERALD, IT_DIAMOND,
    // ---- 金属锭 ----
    IT_COPPER_BAR, IT_IRON_BAR, IT_SILVER_BAR, IT_GOLD_BAR,
    // ---- 镐 / 斧 ----
    IT_PICK_COPPER, IT_PICK_IRON, IT_PICK_SILVER, IT_PICK_GOLD,
    IT_AXE_COPPER, IT_AXE_IRON, IT_AXE_SILVER, IT_AXE_GOLD,
    // ---- 武器 ----
    IT_SWORD_COPPER, IT_SWORD_IRON, IT_SWORD_SILVER, IT_SWORD_GOLD,
    IT_BOW, IT_ARROW, IT_MAGIC_DART,
    // ---- 材料 ----
    IT_GEL, IT_BONE, IT_LENS, IT_STRING,
    // ---- 消耗品 ----
    IT_POTION_HEALTH, IT_POTION_MANA,
    // ---- Boss 召唤物 ----
    IT_SLIME_CROWN, IT_SUSPICIOUS_EYE,
    // ---- 饰品 ----
    IT_AMULET_LIFE, IT_AMULET_MANA, IT_BOOTS_SPEED, IT_BRACER,
    // ---- 护甲 ----
    IT_HELM_COPPER, IT_HELM_IRON, IT_HELM_SILVER, IT_HELM_GOLD,
    IT_ARMOR_COPPER, IT_ARMOR_IRON, IT_ARMOR_SILVER, IT_ARMOR_GOLD,
    IT_COUNT
};

enum ItemKind {
    IK_NONE, IK_BLOCK, IK_PICK, IK_AXE, IK_SWORD, IK_BOW, IK_AMMO, IK_MAGIC,
    IK_MATERIAL, IK_PLACE, IK_CONSUMABLE, IK_ACCESSORY, IK_ARMOR, IK_SUMMON
};

enum EquipSlot { EQ_NONE, EQ_HELM, EQ_ARMOR, EQ_ACC1, EQ_ACC2 };

struct ItemDef {
    const char* sprite;
    const char* name;
    const char* desc;
    ItemKind kind;
    int stack;
    uint16_t placeTile;
    uint16_t placeWall;
    int pickPower;
    float pickSpeed;      // 挖掘速度倍率
    int damage;
    int defense;
    float useTime;        // 攻击/使用间隔(秒)
    int manaCost;
    float crit;
    float speedMult;      // 移速加成
    int lifeBonus, manaBonus;
    int rarity;           // 0 白 1 绿 2 蓝 3 橙
    EquipSlot equip;
    int value;            // 售价
    bool canSell;
};

const ItemDef& itemDef(int id);
const char* itemName(int id);
const char* itemDesc(int id);
int itemPickPower(int id);
float itemPickSpeed(int id);
int itemDamage(int id);
int itemDefense(int id);
int itemStackMax(int id);
Color rarityColor(int rarity);
const char* rarityName(int rarity);

// ---------------------------------------------------------------- 合成
enum Station { ST_INVENTORY = 0, ST_BENCH = 1, ST_ANVIL = 2, ST_FURNACE = 3 };

struct Recipe {
    int result;
    int count;
    int w, h;
    int ing[9];
    Station station;
};

// 熔炼: 输入 -> 产物
int smeltResult(int id);
// 燃料能烧多久(秒), 0 表示不能烧
int fuelValue(int id);

const std::vector<Recipe>& allRecipes();
const Recipe* findRecipe(const int grid[9], int w, int h, Station st);

}  // namespace tc
