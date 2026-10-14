// Game.h —— 游戏主控: 世界 / 玩家 / 实体 / 界面 / 昼夜
#pragma once

#include "Common.h"
#include "Entities.h"
#include "Items.h"
#include "Particles.h"
#include "Player.h"
#include "World.h"

#include <map>
#include <memory>

namespace tc {

struct DamageText {
    Vector2 pos, vel;
    int value;
    bool crit;
    float life;
};

struct FloatMsg {
    std::string text;
    float life;
    Color color;
};

// 箱子内容(按方块坐标存储)
using ChestSlots = std::vector<ItemStack>;   // 20 格

struct Chest {
    ChestSlots slots;
    bool open = false;
};

enum GameState { GS_PLAYING, GS_DEAD, GS_PAUSED, GS_TITLE };

class UI;

class Game {
public:
    Game();
    ~Game();
    bool init();
    void shutdown();

    void update(float dt);
    void draw();

    void newWorld(uint32_t seed);
    bool saveWorld(const char* name);
    bool loadWorld(const char* name);
    bool hasSave() const;

    // ---- 访问器 ----
    World& world() { return m_world; }
    Player& player() { return m_player; }
    Entities& ents() { return m_ents; }
    Particles& parts() { return m_parts; }
    Chest* chestAt(int x, int y);
    Chest* openChest() { return m_chests.count(m_openChest) ? &m_chests[m_openChest] : nullptr; }

    // ---- 供实体/玩家调用 ----
    void spawnProjectile(float x, float y, float vx, float vy, int kind, int damage,
                         int owner, bool gravity, float life, Color color, float size);
    void spawnDrop(int itemId, int count, float x, float y);
    void damageNumber(float x, float y, int value, bool crit);
    void dropLoot(const Entity& e);
    void shake(float amount) { m_shake = std::max(m_shake, amount); }
    void addMessage(const std::string& s, Color c);
    void alertBoss(const char* name);
    void summonBoss(int bossType);          // 0 史莱姆王 1 克苏鲁之眼
    bool nearStation(int tileId, int rangeTiles = 4) const;

    // ---- 时间 / 光照 ----
    float timeOfDay() const { return m_time; }        // 0=午夜 0.25=日出 0.5=正午 0.75=日落
    bool isNight() const { return m_time > 0.78f || m_time < 0.22f; }
    float daylight() const;                            // 0..1
    Color ambient() const;
    Camera2D camera() const;

    // ---- 界面状态 ----
    int craftStation = -1;      // -1 无 0 背包 1 工作台 2 铁砧 3 熔炉
    bool invOpen = false;
    bool shopOpen = false;
    int shopNpc = -1;
    bool craftingPanel = false; // 背包里的制作页
    GameState state = GS_PLAYING;

    // 合成网格(与背包共享)
    int craftGrid[9] = { 0 };
    // 熔炉
    int furnaceIn = 0, furnaceFuel = 0, furnaceOut = 0, furnaceOutCount = 0;
    float furnaceTimer = 0.0f;
    float m_furnaceFuelTime = 0.0f;
    bool uiBlocking = false;      // 界面打开时屏蔽玩家的鼠标操作
    int countFurnaceOut() const;
    void updateDamageNumbers(float dt);

    int64_t m_openChest = -1;    // 当前打开的箱子
    int hoveredSlot = -1;       // 鼠标悬停的背包格
    int heldMouse = 0;          // 鼠标上拿着的物品
    int heldMouseCount = 0;
    int dragFrom = -1;          // 拖动来源
    float m_spawnAcc = 0.0f;    // 刷怪计时

    uint32_t seed() const { return m_seed; }
    const std::string& worldName() const { return m_worldName; }
    int frameCounter() const { return m_frame; }
    // 屏幕坐标 -> 世界坐标
    static Vector2 screenToWorld(Vector2 screen, const Game& g);
    Vector2 mouseWorld() const;

    // 挖矿/放置
    int mineX = -1, mineY = -1;
    bool canPlaceAt(int tx, int ty) const;

private:
    friend class UI;
    friend class Player;

    void updateTime(float dt);
    void updateCamera();
    void drawSky(float wx0, float wy0, float wx1, float wy1);
    void drawWorldTiles(float wx0, float wy0, float wx1, float wy1);
    void drawDamageNumbers();
    void spawnNPCs();
    bool findHouse(float px, float py, float& outX, float& outY) const;
    void updateMessages(float dt);

    World m_world;
    Player m_player;
    Entities m_ents;
    Particles m_parts;
    std::unique_ptr<UI> m_ui;

    std::vector<DamageText> m_dmg;
    std::vector<FloatMsg> m_msgs;
    std::map<int64_t, Chest> m_chests;

    float m_time = 0.28f;          // 起始清晨
    float m_shake = 0.0f;
    Vector2 m_camTarget{ 0, 0 };
    uint32_t m_seed = 0;
    std::string m_worldName = "world";
    Entity* m_bossAlert = nullptr;
    std::string m_bossName;
    float m_bossAlertTime = 0.0f;
    int m_npcCheckTimer = 0;
    int m_frame = 0;
};

}  // namespace tc
