// Text.h —— 全部界面文案(中英双语) 与字符串收集
#pragma once

#include <string>

namespace tc {

namespace loc {

enum Str {
    S_TITLE, S_SUBTITLE,
    S_PAUSED, S_RESUME, S_SAVE, S_LOAD, S_QUIT, S_BACK,
    S_SAVED, S_LOADED, S_SAVE_FAIL, S_LOAD_FAIL, S_NEW_WORLD,
    S_YOU_DIED, S_RESPAWN,
    S_INV, S_EQUIP, S_CRAFT, S_THROW, S_CLOSE, S_TRASH,
    S_HEALTH, S_MANA, S_DEFENSE, S_DAMAGE, S_SPEED, S_CRIT,
    S_NEED_PICK, S_NO_SPACE, S_NEED_BENCH, S_FULL, S_NEED_MANA,
    S_APPEARED, S_SLIME, S_DEMON_EYE, S_ZOMBIE, S_CRAFT_FAIL,
    S_TALK, S_SHOP, S_HOUSE, S_GUIDE_1, S_GUIDE_2, S_GUIDE_3, S_GUIDE_4,
    S_MERCH_1, S_MERCH_2, S_MERCH_LEAVE,
    S_BIOME_FOREST, S_BIOME_DESERT, S_BIOME_SNOW, S_BIOME_JUNGLE,
    S_BIOME_CORRUPT, S_BIOME_OCEAN, S_BIOME_CAVE, S_BIOME_HELL,
    S_DEPTH, S_TIP_1, S_TIP_2, S_TIP_3, S_TIP_4, S_TIP_5,
    S_RARITY_0, S_RARITY_1, S_RARITY_2, S_RARITY_3,
    S_SLIME_CROWN_USE, S_EYE_USE, S_WORKBENCH, S_ANVIL, S_FURNACE_UI,
    S_SUMMON_BOSS, S_NIGHT, S_STATS, S_NO_HOUSE, S_HOUSE_OK,
    S_NEED_TORCH, S_SLIME_GEL, S_CRAFT_AT_WORKBENCH, S_CHEST,
    S_COUNT
};

const char* zh(Str s);
const char* en(Str s);

// 界面里真正显示的文本
const char* get(Str s);
bool cjkAvailable();

}  // namespace loc

// 所有会显示给玩家的文本(物品名/方块名/文案), 用于生成字体字符集
const std::string& AllDisplayText();

}  // namespace tc
