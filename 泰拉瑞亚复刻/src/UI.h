// UI.h —— 界面: 状态栏 / 快捷栏 / 小地图 / 背包 / 合成 / 商店 / 菜单
#pragma once

#include "Common.h"
#include "Items.h"
#include "Player.h"

namespace tc {

class Game;

class UI {
public:
    bool update(Game& g, float dt);
    void draw(Game& g);

private:
    void drawHUD(Game& g);
    void drawHotbar(Game& g);
    void drawMinimap(Game& g);
    void drawBossBar(Game& g);
    void drawMessages(Game& g);
    void drawInventoryPanel(Game& g);
    void drawShopPanel(Game& g);
    void drawDeathScreen(Game& g);
    void drawPauseMenu(Game& g);
    void drawHeldCursor(Game& g);

    // 槽位交互
    void slotClick(Game& g, Rectangle r, ItemStack& stack, bool rightClick, bool shift);
    void drawSlot(Game& g, Rectangle r, const ItemStack& s, bool selected, int label = -1);
    void drawItemIcon(int id, Rectangle r);
    void drawTooltip(Game& g, int itemId, int mx, int my);
    bool inRect(Rectangle r, Vector2 p) const { return CheckCollisionPointRec(p, r); }
    bool clicked(Rectangle r) const { return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inRect(r, GetMousePosition()); }
    bool rightClicked(Rectangle r) const { return IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && inRect(r, GetMousePosition()); }
    void panelFrame(Rectangle r, const char* title);

    int m_tooltipItem = 0;
    int m_tooltipX = 0, m_tooltipY = 0;
    float m_slotAnim = 0.0f;
};

}  // namespace tc
