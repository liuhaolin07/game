// Text.cpp —— 界面文案
#include "Text.h"

#include "Assets.h"
#include "Items.h"
#include "Tiles.h"

namespace tc {
namespace loc {

namespace {
struct Entry { const char* zh; const char* en; };
const Entry kStr[S_COUNT] = {
    /* S_TITLE       */ { "泰拉瑞亚复刻", "Terraria Clone" },
    /* S_SUBTITLE    */ { "2D 沙盒 · 挖 · 建 · 战斗", "2D sandbox: dig, build, fight" },
    /* S_PAUSED      */ { "已暂停", "Paused" },
    /* S_RESUME      */ { "继续游戏", "Resume" },
    /* S_SAVE        */ { "保存世界", "Save World" },
    /* S_LOAD        */ { "读取世界", "Load World" },
    /* S_QUIT        */ { "退出游戏", "Quit" },
    /* S_BACK        */ { "返回", "Back" },
    /* S_SAVED       */ { "世界已保存", "World saved" },
    /* S_LOADED      */ { "世界已读取", "World loaded" },
    /* S_SAVE_FAIL   */ { "保存失败", "Save failed" },
    /* S_LOAD_FAIL   */ { "没有找到存档", "No save found" },
    /* S_NEW_WORLD   */ { "新的世界", "New World" },
    /* S_YOU_DIED    */ { "你死了", "You were slain" },
    /* S_RESPAWN     */ { "点击任意位置重生", "Click anywhere to respawn" },
    /* S_INV         */ { "物品", "Items" },
    /* S_EQUIP       */ { "装备", "Equip" },
    /* S_CRAFT       */ { "制作", "Craft" },
    /* S_THROW       */ { "丢弃", "Throw" },
    /* S_CLOSE       */ { "关闭", "Close" },
    /* S_TRASH       */ { "垃圾箱", "Trash" },
    /* S_HEALTH      */ { "生命", "Health" },
    /* S_MANA        */ { "魔力", "Mana" },
    /* S_DEFENSE     */ { "防御", "Defense" },
    /* S_DAMAGE      */ { "伤害", "Damage" },
    /* S_SPEED       */ { "移速", "Speed" },
    /* S_CRIT        */ { "暴击", "Crit" },
    /* S_NEED_PICK   */ { "需要更高级的镐", "Pickaxe power too low" },
    /* S_NO_SPACE    */ { "这里放不下", "No space" },
    /* S_NEED_BENCH  */ { "需要靠近工作台", "Requires a work bench" },
    /* S_FULL        */ { "背包已满", "Inventory full" },
    /* S_NEED_MANA   */ { "魔力不足", "Not enough mana" },
    /* S_APPEARED    */ { "出现了", "has appeared" },
    /* S_SLIME       */ { "史莱姆", "Slime" },
    /* S_DEMON_EYE   */ { "恶魔之眼", "Demon Eye" },
    /* S_ZOMBIE      */ { "僵尸", "Zombie" },
    /* S_CRAFT_FAIL  */ { "缺少材料", "Missing materials" },
    /* S_TALK        */ { "交谈", "Talk" },
    /* S_SHOP        */ { "商店", "Shop" },
    /* S_HOUSE       */ { "我需要一个家...", "I need a house..." },
    /* S_GUIDE_1     */ { "嗨, 你是新来的? 跟着光走, 夜晚要小心。", "Hi! Nights are dangerous. Stay near torches." },
    /* S_GUIDE_2     */ { "先砍树, 造个工作台, 然后往地下挖。", "Chop trees, build a bench, then dig down." },
    /* S_GUIDE_3     */ { "铜镐能挖铜, 铁镐才能挖石头。", "A copper pick mines copper; stone needs iron." },
    /* S_GUIDE_4     */ { "做出铁砧和熔炉, 换成好装备再往深处走。", "Make an anvil and furnace before going deep." },
    /* S_MERCH_1     */ { "欢迎光临, 随便看看。", "Welcome! Take a look." },
    /* S_MERCH_2     */ { "要是没钱, 就去把怪物多宰几只。", "No coin? Go slay a few monsters." },
    /* S_MERCH_LEAVE */ { "下次见。", "See you later." },
    /* S_BIOME_FOREST*/ { "森林", "Forest" },
    /* S_BIOME_DESERT*/ { "沙漠", "Desert" },
    /* S_BIOME_SNOW  */ { "雪原", "Snow" },
    /* S_BIOME_JUNGLE*/ { "丛林", "Jungle" },
    /* S_BIOME_CORRUPT*/ { "腐化之地", "Corruption" },
    /* S_BIOME_OCEAN */ { "海洋", "Ocean" },
    /* S_BIOME_CAVE  */ { "洞穴", "Cave" },
    /* S_BIOME_HELL  */ { "地狱", "Underworld" },
    /* S_DEPTH       */ { "深度", "Depth" },
    /* S_TIP_1       */ { "A / D 移动, 空格跳跃", "A/D move, SPACE jump" },
    /* S_TIP_2       */ { "左键挖掘或攻击, 右键放置或使用", "LMB mine/attack, RMB place/use" },
    /* S_TIP_3       */ { "1-0 与滚轮切换物品, E 打开背包", "1-0 / wheel to switch, E for inventory" },
    /* S_TIP_4       */ { "右键放置火把, 洞穴里会安全很多", "Place torches with RMB - caves get safer" },
    /* S_TIP_5       */ { "天黑前找个地方躲起来", "Find shelter before nightfall" },
    /* S_RARITY_0    */ { "普通", "Common" },
    /* S_RARITY_1    */ { "优秀", "Uncommon" },
    /* S_RARITY_2    */ { "精良", "Rare" },
    /* S_RARITY_3    */ { "传奇", "Legendary" },
    /* S_SLIME_CROWN_USE */ { "召唤史莱姆王", "Summon King Slime" },
    /* S_EYE_USE     */ { "召唤克苏鲁之眼", "Summon Eye of Cthulhu" },
    /* S_WORKBENCH   */ { "工作台", "Work Bench" },
    /* S_ANVIL       */ { "铁砧", "Anvil" },
    /* S_FURNACE_UI  */ { "熔炉", "Furnace" },
    /* S_SUMMON_BOSS */ { "举起物品使用", "Raise item to use" },
    /* S_NIGHT       */ { "夜幕降临", "Night falls" },
    /* S_STATS       */ { "状态", "Stats" },
    /* S_NO_HOUSE    */ { "没有找到合适的房子", "No suitable house found" },
    /* S_HOUSE_OK    */ { "这里不错, 我住下了", "Nice place. I'll live here." },
    /* S_NEED_TORCH  */ { "太暗了, 放个火把", "Too dark - place a torch" },
    /* S_SLIME_GEL   */ { "史莱姆会掉落凝胶", "Slimes drop gel" },
    /* S_CRAFT_AT_WORKBENCH */ { "站到工作台旁再合成", "Stand near a work bench" },
    /* S_CHEST        */ { "箱子", "Chest" },
};
}  // namespace

const char* zh(Str s) { return (s >= 0 && s < S_COUNT) ? kStr[s].zh : ""; }
const char* en(Str s) { return (s >= 0 && s < S_COUNT) ? kStr[s].en : ""; }
bool cjkAvailable() { return text::HasCJK(); }
const char* get(Str s) { return text::HasCJK() ? zh(s) : en(s); }

}  // namespace loc

const std::string& AllDisplayText()
{
    static std::string s;
    if (!s.empty()) return s;
    for (int i = 0; i < loc::S_COUNT; i++) { s += loc::zh((loc::Str)i); s += loc::en((loc::Str)i); }
    for (int i = 1; i < IT_COUNT; i++) { s += itemName(i); s += itemDesc(i); }
    for (int i = 0; i < T_COUNT; i++) s += tileName((uint16_t)i);
    for (int i = 0; i < W_COUNT; i++) s += wallName((uint16_t)i);
    return s;
}

}  // namespace tc
