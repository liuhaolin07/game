// Items.cpp —— 物品表与合成配方
#include "Items.h"

namespace tc {

namespace {

// sprite, 名称, 描述, kind, stack, placeTile, placeWall, pickPower, pickSpeed,
// damage, defense, useTime, manaCost, crit, speedMult, life, mana, rarity, equip, value, canSell
const ItemDef kItems[IT_COUNT] = {
    { "", "空", "", IK_NONE, 1, 0, 0, 0, 1.0f, 0, 0, 0.4f, 0, 0.04f, 1.0f, 0, 0, 0, EQ_NONE, 0, false },

    /* IT_DIRT            */ { "dirt", "泥土", "普普通通的泥土", IK_BLOCK, 999, T_DIRT, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_STONE           */ { "stone", "石头", "地下随处可见", IK_BLOCK, 999, T_STONE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_SAND            */ { "sand", "沙子", "", IK_BLOCK, 999, T_SAND, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_SANDSTONE       */ { "sandstone", "砂岩", "", IK_BLOCK, 999, T_SANDSTONE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_SNOW            */ { "snow", "雪块", "", IK_BLOCK, 999, T_SNOW, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_ICE             */ { "ice", "冰块", "", IK_BLOCK, 999, T_ICE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_MUD             */ { "mud", "泥巴", "", IK_BLOCK, 999, T_MUD, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_STONE_JUNGLE    */ { "stone_jungle", "丛林石", "", IK_BLOCK, 999, T_STONE_JUNGLE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_EBONSTONE       */ { "ebonstone", "黑石", "腐化之地的石头", IK_BLOCK, 999, T_EBONSTONE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_OBSIDIAN        */ { "obsidian", "黑曜石", "极其坚硬的黑色石头", IK_BLOCK, 999, T_OBSIDIAN, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 1, 0, EQ_NONE, 5, true },
    /* IT_ASH             */ { "ash", "灰烬", "", IK_BLOCK, 999, T_ASH, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_HELLSTONE_BLOCK */ { "hellstone", "狱石", "来自地狱的石头", IK_BLOCK, 999, T_HELLSTONE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 1, 0, EQ_NONE, 3, true },
    /* IT_GLASS           */ { "glass", "玻璃", "沙子熔炼而成", IK_BLOCK, 999, T_GLASS, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 2, true },
    /* IT_BRICKS          */ { "bricks", "红砖", "", IK_BLOCK, 999, T_BRICKS, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 2, true },
    /* IT_BRICKS_STONE    */ { "bricks_stone", "石砖", "", IK_BLOCK, 999, T_BRICKS_STONE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_MUD_BRICK       */ { "mud_brick", "泥砖", "", IK_BLOCK, 999, T_MUD_BRICK, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_SNOW_BRICK      */ { "snow_brick", "雪砖", "", IK_BLOCK, 999, T_SNOW_BRICK, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_WOOD_PANEL      */ { "wood_panel", "木板", "", IK_BLOCK, 999, T_WOOD_PANEL, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_CLAY            */ { "clay", "黏土", "", IK_BLOCK, 999, T_CLAY, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 2, true },
    /* IT_CLAY_BRICK      */ { "clay_brick", "黏土砖", "", IK_BLOCK, 999, T_CLAY_BRICK, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 2, true },
    /* IT_GLOWSTONE       */ { "glowstone", "荧石", "会发光的石头", IK_BLOCK, 999, T_GLOWSTONE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 3, true },
    /* IT_WORKBENCH       */ { "workbench", "工作台", "解锁 3x3 合成", IK_PLACE, 99, T_WORKBENCH, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 5, true },
    /* IT_FURNACE         */ { "furnace", "熔炉", "把矿石熔炼成锭", IK_PLACE, 99, T_FURNACE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 10, true },
    /* IT_ANVIL           */ { "anvil", "铁砧", "制作金属装备", IK_PLACE, 99, T_ANVIL, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 10, true },
    /* IT_CHEST           */ { "chest", "箱子", "存放物品", IK_PLACE, 99, T_CHEST, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 10, true },
    /* IT_CAMPFIRE        */ { "campfire", "篝火", "温暖的火光", IK_PLACE, 99, T_CAMPFIRE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 5, true },
    /* IT_FENCE           */ { "fence", "木栅栏", "可以穿过的矮墙", IK_BLOCK, 999, T_FENCE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_VINE            */ { "vine", "藤蔓", "", IK_BLOCK, 999, T_VINE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_PLATFORM        */ { "platform", "平台", "可以踩上去的木板", IK_BLOCK, 999, T_PLATFORM, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_WOOD            */ { "wood", "木头", "最基本的材料", IK_MATERIAL, 999, T_WOOD, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 2, true },
    /* IT_TORCH           */ { "torch", "火把", "照亮黑暗", IK_BLOCK, 999, T_TORCH, 0, 0, 1, 0, 0, 0.15f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_DOOR            */ { "door", "木门", "可以开关的门", IK_BLOCK, 99, T_DOOR, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 3, true },

    /* IT_ORE_COPPER      */ { "ore_copper", "铜矿", "", IK_MATERIAL, 999, T_ORE_COPPER, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 4, true },
    /* IT_ORE_IRON        */ { "ore_iron", "铁矿", "", IK_MATERIAL, 999, T_ORE_IRON, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 5, true },
    /* IT_ORE_SILVER      */ { "ore_silver", "白银矿", "", IK_MATERIAL, 999, T_ORE_SILVER, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 12, true },
    /* IT_ORE_GOLD        */ { "ore_gold", "黄金矿", "", IK_MATERIAL, 999, T_ORE_GOLD, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 15, true },
    /* IT_ORE_DEMONITE    */ { "ore_demonite", "陨石矿", "带着邪恶的气息", IK_MATERIAL, 999, T_ORE_DEMONITE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 2, 0, EQ_NONE, 20, true },
    /* IT_ORE_HELLSTONE   */ { "ore_hellstone", "狱石矿", "", IK_MATERIAL, 999, T_ORE_HELLSTONE, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 12, true },
    /* IT_TOPAZ           */ { "gem_topaz", "黄玉", "", IK_MATERIAL, 999, T_GEM_TOPAZ, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 2, 0, EQ_NONE, 18, true },
    /* IT_AMETHYST        */ { "gem_amethyst", "紫晶", "", IK_MATERIAL, 999, T_GEM_AMETHYST, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 18, true },
    /* IT_EMERALD         */ { "gem_emerald", "绿玉", "", IK_MATERIAL, 999, T_GEM_EMERALD, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 18, true },
    /* IT_DIAMOND         */ { "gem_diamond", "钻石", "", IK_MATERIAL, 999, T_GEM_DIAMOND, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 3, 0, EQ_NONE, 30, true },

    /* IT_COPPER_BAR      */ { "bar_copper", "铜锭", "", IK_MATERIAL, 999, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 6, true },
    /* IT_IRON_BAR        */ { "bar_iron", "铁锭", "", IK_MATERIAL, 999, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 8, true },
    /* IT_SILVER_BAR      */ { "bar_silver", "银锭", "", IK_MATERIAL, 999, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 20, true },
    /* IT_GOLD_BAR        */ { "bar_gold", "金锭", "", IK_MATERIAL, 999, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 30, true },

    /* IT_PICK_COPPER     */ { "pickaxe_copper", "铜镐", "挖掘的基础工具", IK_PICK, 1, 0, 0, 10, 1.0f, 4, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 15, true },
    /* IT_PICK_IRON       */ { "pickaxe_iron", "铁镐", "更快的挖掘速度", IK_PICK, 1, 0, 0, 25, 1.5f, 5, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 25, true },
    /* IT_PICK_SILVER     */ { "pickaxe_silver", "银镐", "", IK_PICK, 1, 0, 0, 40, 2.0f, 6, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 45, true },
    /* IT_PICK_GOLD       */ { "pickaxe_gold", "金镐", "坚不可摧的镐", IK_PICK, 1, 0, 0, 55, 2.6f, 7, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 80, true },
    /* IT_AXE_COPPER      */ { "axe_copper", "铜斧", "砍树更快", IK_AXE, 1, 0, 0, 10, 1.0f, 7, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 15, true },
    /* IT_AXE_IRON        */ { "axe_iron", "铁斧", "", IK_AXE, 1, 0, 0, 25, 1.5f, 9, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 25, true },
    /* IT_AXE_SILVER      */ { "axe_silver", "银斧", "", IK_AXE, 1, 0, 0, 40, 2.0f, 11, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 45, true },
    /* IT_AXE_GOLD        */ { "axe_gold", "金斧", "", IK_AXE, 1, 0, 0, 55, 2.6f, 13, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 80, true },

    /* IT_SWORD_COPPER    */ { "sword_copper", "铜剑", "", IK_SWORD, 1, 0, 0, 10, 1.0f, 8, 0, 0.42f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 18, true },
    /* IT_SWORD_IRON      */ { "sword_iron", "铁剑", "", IK_SWORD, 1, 0, 0, 25, 1.5f, 12, 0, 0.40f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 30, true },
    /* IT_SWORD_SILVER    */ { "sword_silver", "银剑", "", IK_SWORD, 1, 0, 0, 40, 2.0f, 16, 0, 0.38f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 55, true },
    /* IT_SWORD_GOLD      */ { "sword_gold", "金剑", "", IK_SWORD, 1, 0, 0, 55, 2.6f, 21, 0, 0.36f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 95, true },
    /* IT_BOW             */ { "bow", "短弓", "按住左键蓄力", IK_BOW, 1, 0, 0, 10, 1.0f, 10, 0, 1.0f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 25, true },
    /* IT_ARROW           */ { "arrow", "箭", "", IK_AMMO, 999, 0, 0, 0, 1, 6, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 1, true },
    /* IT_MAGIC_DART      */ { "magic_dart", "魔法珠", "消耗魔力", IK_MAGIC, 999, 0, 0, 0, 1, 14, 0, 0.55f, 15, 0.04f, 1, 0, 1, 0, EQ_NONE, 20, true },

    /* IT_GEL             */ { "gel", "史莱姆凝胶", "黏糊糊的", IK_MATERIAL, 999, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 2, true },
    /* IT_BONE            */ { "bone", "骨头", "能做成装备", IK_MATERIAL, 999, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 3, true },
    /* IT_LENS            */ { "lens", "透镜", "恶魔之眼的残骸", IK_MATERIAL, 999, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 1, 0, EQ_NONE, 8, true },
    /* IT_STRING          */ { "string", "绳索", "", IK_MATERIAL, 999, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 2, true },

    /* IT_POTION_HEALTH   */ { "potion_health", "治疗药水", "恢复 60 点生命", IK_CONSUMABLE, 30, 0, 0, 0, 1, 0, 0, 0.8f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 20, true },
    /* IT_POTION_MANA     */ { "potion_mana", "魔力药水", "恢复 60 点魔力", IK_CONSUMABLE, 30, 0, 0, 0, 1, 0, 0, 0.8f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 20, true },

    /* IT_SLIME_CROWN     */ { "crown_slime", "史莱姆王冠", "在地狱之外使用", IK_SUMMON, 1, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 3, 0, EQ_NONE, 0, false },
    /* IT_SUSPICIOUS_EYE  */ { "eye_suspicious", "可疑眼球", "太阳下山后使用", IK_SUMMON, 1, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 3, 0, EQ_NONE, 0, false },

    /* IT_AMULET_LIFE     */ { "amulet_life", "生命护符", "生命上限 +20", IK_ACCESSORY, 1, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 20, 0, 1, EQ_ACC1, 60, true },
    /* IT_AMULET_MANA     */ { "amulet_mana", "魔力护符", "魔力上限 +20", IK_ACCESSORY, 1, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 20, 1, EQ_ACC2, 60, true },
    /* IT_BOOTS_SPEED     */ { "boots_speed", "疾行靴", "移动速度 +12%", IK_ACCESSORY, 1, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1.12f, 0, 0, 1, EQ_ACC1, 60, true },
    /* IT_BRACER          */ { "bracer_defense", "硬木护腕", "防御 +4", IK_ACCESSORY, 1, 0, 0, 0, 1, 0, 4, 0.4f, 0, 0.04f, 1, 0, 0, 1, EQ_ACC2, 60, true },

    /* IT_HELM_COPPER     */ { "helm_copper", "铜头盔", "防御 +2", IK_ARMOR, 1, 0, 0, 0, 1, 0, 2, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_HELM, 20, true },
    /* IT_HELM_IRON       */ { "helm_iron", "铁头盔", "防御 +3", IK_ARMOR, 1, 0, 0, 0, 1, 0, 3, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_HELM, 35, true },
    /* IT_HELM_SILVER     */ { "helm_silver", "银头盔", "防御 +4", IK_ARMOR, 1, 0, 0, 0, 1, 0, 4, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_HELM, 60, true },
    /* IT_HELM_GOLD       */ { "helm_gold", "金头盔", "防御 +5", IK_ARMOR, 1, 0, 0, 0, 1, 0, 5, 0.4f, 0, 0.04f, 1, 0, 1, 0, EQ_HELM, 100, true },
    /* IT_ARMOR_COPPER    */ { "armor_copper", "铜护甲", "防御 +5", IK_ARMOR, 1, 0, 0, 0, 1, 0, 5, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_ARMOR, 30, true },
    /* IT_ARMOR_IRON      */ { "armor_iron", "铁护甲", "防御 +7", IK_ARMOR, 1, 0, 0, 0, 1, 0, 7, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_ARMOR, 50, true },
    /* IT_ARMOR_SILVER    */ { "armor_silver", "银护甲", "防御 +9", IK_ARMOR, 1, 0, 0, 0, 1, 0, 9, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_ARMOR, 85, true },
    /* IT_ARMOR_GOLD      */ { "armor_gold", "金护甲", "防御 +11", IK_ARMOR, 1, 0, 0, 0, 1, 0, 11, 0.4f, 0, 0.04f, 1, 0, 1, 0, EQ_ARMOR, 140, true },
};

const ItemDef kNone = { "", "空", "", IK_NONE, 1, 0, 0, 0, 1, 0, 0, 0.4f, 0, 0.04f, 1, 0, 0, 0, EQ_NONE, 0, false };

// ---- 配方 ---------------------------------------------------------------
// 0 表示空格
Recipe R(int res, int cnt, Station st,
         int a, int b, int c,
         int d, int e, int f,
         int g, int h, int i)
{
    Recipe r;
    r.result = res; r.count = cnt; r.station = st;
    r.ing[0] = a; r.ing[1] = b; r.ing[2] = c;
    r.ing[3] = d; r.ing[4] = e; r.ing[5] = f;
    r.ing[6] = g; r.ing[7] = h; r.ing[8] = i;
    int w = 0, hh = 0;
    for (int y = 0; y < 3; y++) {
        int rowW = 0;
        for (int x = 0; x < 3; x++) if (r.ing[y * 3 + x]) { if (x + 1 > rowW) rowW = x + 1; }
        if (rowW) { hh = y + 1; if (rowW > w) w = rowW; }
    }
    r.w = w ? w : 1; r.h = hh ? hh : 1;
    return r;
}

// 常用简写
#define IV ST_INVENTORY
#define BN ST_BENCH

std::vector<Recipe> buildRecipes()
{
    std::vector<Recipe> v;
    // ---- 基础建材 ----
    v.push_back(R(IT_WOOD_PANEL, 1, IV, IT_WOOD, 0, 0, 0, 0, 0, 0, 0, 0));
    v.push_back(R(IT_TORCH, 3, IV, IT_WOOD, IT_GEL, 0, 0, 0, 0, 0, 0, 0));
    v.push_back(R(IT_PLATFORM, 2, IV, IT_WOOD_PANEL, IT_WOOD_PANEL, 0, 0, 0, 0, 0, 0, 0));
    v.push_back(R(IT_FENCE, 3, IV, IT_WOOD_PANEL, IT_WOOD_PANEL, IT_WOOD_PANEL, 0, 0, 0, 0, 0, 0));
    v.push_back(R(IT_CAMPFIRE, 1, IV, IT_WOOD, IT_WOOD, 0, IT_WOOD, IT_WOOD, 0, IT_GEL, IT_GEL, 0));
    v.push_back(R(IT_WORKBENCH, 1, BN,
                  IT_WOOD, IT_WOOD, IT_WOOD,
                  IT_WOOD, IT_WOOD, IT_WOOD,
                  IT_WOOD, IT_WOOD, IT_WOOD));
    v.push_back(R(IT_FURNACE, 1, BN,
                  IT_STONE, IT_STONE, IT_STONE,
                  IT_STONE, IT_STONE, IT_STONE,
                  IT_WOOD, IT_WOOD, IT_TORCH));
    v.push_back(R(IT_ANVIL, 1, BN,
                  IT_IRON_BAR, IT_IRON_BAR, IT_IRON_BAR,
                  IT_IRON_BAR, IT_IRON_BAR, 0,
                  0, 0, 0));
    v.push_back(R(IT_CHEST, 1, BN,
                  IT_WOOD_PANEL, IT_WOOD_PANEL, IT_WOOD_PANEL,
                  IT_WOOD_PANEL, IT_WOOD_PANEL, IT_WOOD_PANEL,
                  IT_WOOD_PANEL, IT_WOOD_PANEL, IT_GOLD_BAR));
    v.push_back(R(IT_DOOR, 1, BN,
                  IT_WOOD_PANEL, IT_WOOD_PANEL, IT_WOOD_PANEL,
                  IT_WOOD_PANEL, IT_WOOD_PANEL, IT_WOOD_PANEL, 0, 0, 0));
    v.push_back(R(IT_BRICKS_STONE, 4, IV, IT_STONE, 0, 0, 0, 0, 0, 0, 0, 0));
    v.push_back(R(IT_CLAY_BRICK, 4, IV, IT_CLAY, IT_CLAY, 0, IT_CLAY, IT_CLAY, 0, 0, 0, 0));
    v.push_back(R(IT_SNOW_BRICK, 4, IV, IT_SNOW, IT_SNOW, 0, IT_SNOW, IT_SNOW, 0, 0, 0, 0));
    v.push_back(R(IT_MUD_BRICK, 4, IV, IT_MUD, IT_MUD, 0, IT_MUD, IT_MUD, 0, 0, 0, 0));

    // ---- 武器 / 工具 ----
    v.push_back(R(IT_SWORD_COPPER, 1, BN,
                  IT_COPPER_BAR, IT_COPPER_BAR, 0,
                  0, IT_COPPER_BAR, 0,
                  0, IT_COPPER_BAR, 0));
    v.push_back(R(IT_SWORD_IRON, 1, BN,
                  IT_IRON_BAR, IT_IRON_BAR, 0,
                  0, IT_IRON_BAR, 0,
                  0, IT_IRON_BAR, 0));
    v.push_back(R(IT_SWORD_SILVER, 1, BN,
                  IT_SILVER_BAR, IT_SILVER_BAR, 0,
                  0, IT_SILVER_BAR, 0,
                  0, IT_SILVER_BAR, 0));
    v.push_back(R(IT_SWORD_GOLD, 1, BN,
                  IT_GOLD_BAR, IT_GOLD_BAR, 0,
                  0, IT_GOLD_BAR, 0,
                  0, IT_GOLD_BAR, 0));
    v.push_back(R(IT_PICK_COPPER, 1, BN,
                  IT_COPPER_BAR, IT_COPPER_BAR, IT_COPPER_BAR,
                  0, IT_COPPER_BAR, 0,
                  IT_COPPER_BAR, 0, IT_COPPER_BAR));
    v.push_back(R(IT_PICK_IRON, 1, BN,
                  IT_IRON_BAR, IT_IRON_BAR, IT_IRON_BAR,
                  0, IT_IRON_BAR, 0,
                  IT_IRON_BAR, 0, IT_IRON_BAR));
    v.push_back(R(IT_PICK_SILVER, 1, BN,
                  IT_SILVER_BAR, IT_SILVER_BAR, IT_SILVER_BAR,
                  0, IT_SILVER_BAR, 0,
                  IT_SILVER_BAR, 0, IT_SILVER_BAR));
    v.push_back(R(IT_PICK_GOLD, 1, BN,
                  IT_GOLD_BAR, IT_GOLD_BAR, IT_GOLD_BAR,
                  0, IT_GOLD_BAR, 0,
                  IT_GOLD_BAR, 0, IT_GOLD_BAR));
    v.push_back(R(IT_AXE_COPPER, 1, BN,
                  IT_COPPER_BAR, IT_COPPER_BAR, IT_COPPER_BAR,
                  0, IT_COPPER_BAR, 0,
                  0, IT_COPPER_BAR, 0));
    v.push_back(R(IT_AXE_IRON, 1, BN,
                  IT_IRON_BAR, IT_IRON_BAR, IT_IRON_BAR,
                  0, IT_IRON_BAR, 0,
                  0, IT_IRON_BAR, 0));
    v.push_back(R(IT_AXE_SILVER, 1, BN,
                  IT_SILVER_BAR, IT_SILVER_BAR, IT_SILVER_BAR,
                  0, IT_SILVER_BAR, 0,
                  0, IT_SILVER_BAR, 0));
    v.push_back(R(IT_AXE_GOLD, 1, BN,
                  IT_GOLD_BAR, IT_GOLD_BAR, IT_GOLD_BAR,
                  0, IT_GOLD_BAR, 0,
                  0, IT_GOLD_BAR, 0));
    v.push_back(R(IT_BOW, 1, BN,
                  IT_WOOD, IT_WOOD, IT_WOOD,
                  IT_WOOD, 0, IT_WOOD,
                  IT_WOOD, 0, IT_GEL));
    v.push_back(R(IT_ARROW, 5, IV, IT_WOOD, IT_GEL, 0, 0, 0, 0, 0, 0, 0));
    v.push_back(R(IT_MAGIC_DART, 10, BN, IT_AMETHYST, IT_GEL, 0, IT_GEL, IT_BONE, 0, 0, 0, 0));

    // ---- 消耗品 / Boss ----
    v.push_back(R(IT_POTION_HEALTH, 1, IV, IT_BONE, IT_GEL, 0, IT_GEL, 0, 0, 0, 0, 0));
    v.push_back(R(IT_POTION_MANA, 1, IV, IT_AMETHYST, IT_GEL, 0, 0, 0, 0, 0, 0, 0));
    v.push_back(R(IT_SLIME_CROWN, 1, BN,
                  IT_GEL, IT_GEL, IT_GEL,
                  IT_GEL, IT_GEL, IT_GEL,
                  IT_GOLD_BAR, IT_GOLD_BAR, IT_GOLD_BAR));
    v.push_back(R(IT_SUSPICIOUS_EYE, 1, BN, IT_LENS, IT_LENS, IT_LENS, IT_LENS, IT_LENS, IT_LENS, 0, 0, 0));

    // ---- 饰品 ----
    v.push_back(R(IT_AMULET_LIFE, 1, BN, IT_BONE, IT_GEL, 0, IT_BONE, IT_GEL, 0, 0, 0, 0));
    v.push_back(R(IT_AMULET_MANA, 1, BN, IT_AMETHYST, IT_BONE, 0, IT_BONE, 0, 0, 0, 0, 0));
    v.push_back(R(IT_BOOTS_SPEED, 1, BN, IT_BONE, IT_STRING, 0, IT_BONE, IT_STRING, 0, 0, 0, 0));
    v.push_back(R(IT_BRACER, 1, BN, IT_STRING, IT_STRING, 0, IT_WOOD, IT_WOOD, 0, IT_WOOD, IT_WOOD, 0));

    // ---- 护甲 ----
    struct Tier { int bar, helm, armor; };
    const Tier tiers[4] = { { IT_COPPER_BAR, IT_HELM_COPPER, IT_ARMOR_COPPER },
                            { IT_IRON_BAR, IT_HELM_IRON, IT_ARMOR_IRON },
                            { IT_SILVER_BAR, IT_HELM_SILVER, IT_ARMOR_SILVER },
                            { IT_GOLD_BAR, IT_HELM_GOLD, IT_ARMOR_GOLD } };
    for (const Tier& t : tiers) {
        v.push_back(R(t.helm, 1, BN,
                      t.bar, t.bar, t.bar,
                      t.bar, 0, t.bar, 0, 0, 0));
        v.push_back(R(t.armor, 1, BN,
                      t.bar, t.bar, t.bar,
                      t.bar, t.bar, t.bar,
                      t.bar, t.bar, t.bar));
    }
    return v;
}

}  // namespace

const ItemDef& itemDef(int id) { return (id > 0 && id < IT_COUNT) ? kItems[id] : kNone; }
const char* itemName(int id) { return itemDef(id).name; }
const char* itemDesc(int id) { return itemDef(id).desc; }
int itemPickPower(int id) { return itemDef(id).pickPower; }
float itemPickSpeed(int id) { return itemDef(id).pickSpeed; }
int itemDamage(int id) { return itemDef(id).damage; }
int itemDefense(int id) { return itemDef(id).defense; }
int itemStackMax(int id) { return id > 0 ? kItems[id].stack : 1; }

Color rarityColor(int r)
{
    switch (r) {
    case 1: return rgb(122, 204, 122);
    case 2: return rgb(110, 160, 240);
    case 3: return rgb(244, 186, 92);
    default: return rgb(232, 232, 232);
    }
}

int smeltResult(int id)
{
    switch (id) {
    case IT_SAND: return IT_GLASS;
    case IT_CLAY: return IT_BRICKS;
    case IT_ORE_COPPER: return IT_COPPER_BAR;
    case IT_ORE_IRON: return IT_IRON_BAR;
    case IT_ORE_SILVER: return IT_SILVER_BAR;
    case IT_ORE_GOLD: return IT_GOLD_BAR;
    case IT_ORE_HELLSTONE: return IT_HELLSTONE_BLOCK;
    default: return 0;
    }
}

int fuelValue(int id)
{
    switch (id) {
    case IT_WOOD: return 15;
    case IT_WOOD_PANEL: return 15;
    case IT_CAMPFIRE: return 30;
    case IT_TORCH: return 5;
    default: return 0;
    }
}

const std::vector<Recipe>& allRecipes()
{
    static const std::vector<Recipe> recipes = buildRecipes();
    return recipes;
}

// 在合成格中查找匹配配方(支持平移对齐)
const Recipe* findRecipe(const int grid[9], int w, int h, Station st)
{
    const std::vector<Recipe>& rs = allRecipes();
    for (const Recipe& r : rs) {
        if (r.station != st) continue;
        if (r.w != w || r.h != h) continue;
        bool ok = true;
        for (int y = 0; y < 3 && ok; y++)
            for (int x = 0; x < 3; x++) {
                int want = r.ing[y * 3 + x];
                int have = grid[y * 3 + x];
                if (want != have) { ok = false; break; }
            }
        if (ok) return &r;
    }
    return nullptr;
}

}  // namespace tc
