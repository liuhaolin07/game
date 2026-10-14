// UI.cpp
#include "UI.h"

#include "Assets.h"
#include "Audio.h"
#include "Entities.h"
#include "Game.h"
#include "Player.h"
#include "Text.h"
#include "Tiles.h"
#include "World.h"

#include <algorithm>
#include <cstdio>

namespace tc {

namespace {
constexpr int SLOT = 46;
constexpr int SLOTS_X = 10;
constexpr int SLOTS_Y = 5;

const Color C_PANEL = rgb(28, 26, 44, 232);
const Color C_PANEL_EDGE = rgb(92, 88, 128);
const Color C_SLOT = rgb(58, 56, 82, 200);
const Color C_SLOT_SEL = rgb(120, 116, 168, 235);
const Color C_TEXT = rgb(232, 232, 240);
const Color C_DIM = rgb(150, 150, 168);
}  // namespace

void UI::panelFrame(Rectangle r, const char* title)
{
    DrawRectangleRec(r, C_PANEL);
    DrawRectangleLinesEx(r, 2.0f, C_PANEL_EDGE);
    DrawRectangleRec({ r.x + 6, r.y + 6, r.width - 12, 26 }, rgb(44, 42, 66, 235));
    if (title) text::DrawCentered(title, (int)(r.x + r.width * 0.5f), (int)(r.y + 10), 16, C_TEXT);
}

void UI::drawItemIcon(int id, Rectangle r)
{
    if (!id) return;
    const ItemDef& d = itemDef(id);
    const Frame* f = Assets::FindItem(d.sprite);
    if (!f) { text::DrawCentered("?", (int)(r.x + r.width / 2), (int)(r.y + r.height / 2 - 8), 16, C_DIM); return; }
    const float scale = fminf((r.width - 12.0f) / f->rect.width, (r.height - 12.0f) / f->rect.height);
    const float w = f->rect.width * scale, h = f->rect.height * scale;
    drawFrameC(*f, r.x + r.width * 0.5f, r.y + r.height * 0.5f, false, rarityColor(d.rarity), scale);
    (void)w; (void)h;
}

void UI::drawSlot(Game& g, Rectangle r, const ItemStack& s, bool selected, int label)
{
    DrawRectangleRec(r, selected ? C_SLOT_SEL : C_SLOT);
    DrawRectangleLinesEx(r, 1.0f, selected ? rgb(200, 200, 230) : rgb(86, 84, 112));
    if (s.id) {
        drawItemIcon(s.id, r);
        if (s.count > 1) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", s.count);
            text::DrawRight(buf, (int)(r.x + r.width - 4), (int)(r.y + r.height - 16), 13, C_TEXT);
        }
    }
    if (label >= 0 && !s.id) text::DrawCentered(label == 0 ? "" : "", 0, 0, 10, C_DIM);
}

void UI::slotClick(Game& g, Rectangle r, ItemStack& stack, bool rightClick, bool shift)
{
    (void)rightClick;
    if (!inRect(r, GetMousePosition())) return;
    const int max = stack.id ? itemStackMax(stack.id) : MAX_STACK;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (g.heldMouse) {
            if (stack.id == 0) {
                stack.id = g.heldMouse;
                stack.count = g.heldMouseCount;
                g.heldMouse = 0; g.heldMouseCount = 0;
            } else if (stack.id == g.heldMouse && stack.count < max) {
                const int add = shift ? (max - stack.count) : 1;
                const int n = std::min(add, std::min(max - stack.count, g.heldMouseCount));
                stack.count += n;
                g.heldMouseCount -= n;
                if (g.heldMouseCount <= 0) { g.heldMouse = 0; g.heldMouseCount = 0; }
            }
        } else if (stack.id) {
            g.heldMouse = stack.id;
            g.heldMouseCount = shift ? (stack.count - (stack.count > 1 ? 1 : 0)) : stack.count;
            stack.count -= g.heldMouseCount;
            if (stack.count <= 0) stack.clear();
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        if (g.heldMouse) {
            if (stack.id == 0) {
                stack.id = g.heldMouse; stack.count = 1;
                g.heldMouseCount--;
                if (g.heldMouseCount <= 0) { g.heldMouse = 0; g.heldMouseCount = 0; }
            } else if (stack.id == g.heldMouse && stack.count < max) {
                stack.count++;
                g.heldMouseCount--;
                if (g.heldMouseCount <= 0) { g.heldMouse = 0; g.heldMouseCount = 0; }
            }
        } else if (stack.id && stack.count > 1) {
            const int half = stack.count / 2;
            g.heldMouse = stack.id;
            g.heldMouseCount = half;
            stack.count -= half;
        }
    }
}

// ---------------------------------------------------------------- 每帧
bool UI::update(Game& g, float)
{
    m_slotAnim += 0.1f;

    if (g.state == GS_PAUSED) return true;
    if (g.player().dead) return true;

    if (g.invOpen) {
        const int px = (SCREEN_W - 880) / 2, py = (SCREEN_H - 330) / 2;

        // 装备格
        for (int i = 0; i < 4; i++) {
            const int col = i % 2, row = i / 2;
            Rectangle r{ (float)(px + 16 + col * (SLOT + 8)), (float)(py + 46 + row * (SLOT + 8)), (float)SLOT, (float)SLOT };
            const bool hov = inRect(r, GetMousePosition());
            if (hov && g.player().equip[i]) m_tooltipItem = g.player().equip[i];
            if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (g.player().equip[i]) {
                    if (g.player().give(g.player().equip[i], 1)) g.player().equip[i] = 0;
                } else if (g.heldMouse && itemDef(g.heldMouse).equip == (EquipSlot)(i + 1)) {
                    g.player().equip[i] = g.heldMouse;
                    audio::play(SFX_PICKUP, 0.55f, 1.15f);
                    g.heldMouseCount--;
                    if (g.heldMouseCount <= 0) { g.heldMouse = 0; g.heldMouseCount = 0; }
                }
            }
        }

        // 背包 5x10
        for (int i = 0; i < INV_SLOTS; i++) {
            const int col = i % SLOTS_X, row = i / SLOTS_X;
            Rectangle r{ (float)(px + 130 + col * SLOT), (float)(py + 46 + row * SLOT), (float)SLOT, (float)SLOT };
            const bool hov = inRect(r, GetMousePosition());
            g.hoveredSlot = hov ? i : -1;
            if (hov && g.player().inv[i].id) m_tooltipItem = g.player().inv[i].id;
            slotClick(g, r, g.player().inv[i], false, IsKeyDown(KEY_LEFT_SHIFT));
            if (g.shopOpen && hov && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && g.player().inv[i].id) {
                const int val = std::max(1, itemDef(g.player().inv[i].id).value / 2);
                g.player().coins += val;
                g.player().inv[i].count--;
                if (g.player().inv[i].count <= 0) g.player().inv[i].clear();
            }
        }
        if (!g.hoveredSlot) m_tooltipItem = 0;

        // 打开的箱子
        if (Chest* chest = g.openChest()) {
            const int cx0 = px + 630, cy0 = py + 70;
            for (int i = 0; i < 20; i++) {
                const int col = i % 5, row = i / 5;
                Rectangle r{ (float)(cx0 + col * SLOT), (float)(cy0 + row * SLOT), (float)SLOT, (float)SLOT };
                const bool hov = inRect(r, GetMousePosition());
                if (hov && chest->slots[i].id) m_tooltipItem = chest->slots[i].id;
                slotClick(g, r, chest->slots[i], false, IsKeyDown(KEY_LEFT_SHIFT));
            }
        }
        // 合成 / 熔炉
        else if (g.craftStation == ST_FURNACE) {
            Rectangle rIn{ (float)(px + 630), (float)(py + 70), (float)SLOT, (float)SLOT };
            Rectangle rFuel{ (float)(px + 630), (float)(py + 130), (float)SLOT, (float)SLOT };
            Rectangle rOut{ (float)(px + 742), (float)(py + 100), (float)SLOT, (float)SLOT };
            static ItemStack sIn, sFuel, sOut;
            if (sIn.id != g.furnaceIn || sFuel.id != g.furnaceFuel) {
                sIn.id = g.furnaceIn; sIn.count = 1;
                sFuel.id = g.furnaceFuel; sFuel.count = 1;
            }
            sOut.id = g.furnaceOut; sOut.count = g.furnaceOutCount;
            slotClick(g, rIn, sIn, false, false);
            slotClick(g, rFuel, sFuel, false, false);
            g.furnaceIn = sIn.id; g.furnaceFuel = sFuel.id;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inRect(rOut, GetMousePosition()) && g.furnaceOut) {
                if (g.player().give(g.furnaceOut, g.furnaceOutCount)) { g.furnaceOut = 0; g.furnaceOutCount = 0; }
            }
            sOut.id = g.furnaceOut; sOut.count = g.furnaceOutCount;
        } else {
            const int gx = px + 630, gy = py + 56;
            for (int i = 0; i < 9; i++) {
                const int col = i % 3, row = i / 3;
                Rectangle r{ (float)(gx + col * SLOT), (float)(gy + row * SLOT), (float)SLOT, (float)SLOT };
                ItemStack tmp{ g.craftGrid[i], 1 };
                const bool hov = inRect(r, GetMousePosition());
                if (hov && g.craftGrid[i]) m_tooltipItem = g.craftGrid[i];
                if (g.heldMouse) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hov) {
                        g.craftGrid[i] = g.heldMouse;
                        g.heldMouseCount--;
                        if (g.heldMouseCount <= 0) { g.heldMouse = 0; g.heldMouseCount = 0; }
                    } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && hov && g.craftGrid[i]) {
                        g.heldMouse = g.craftGrid[i];
                        g.heldMouseCount = 1;
                        g.craftGrid[i] = 0;
                    }
                } else {
                    slotClick(g, r, tmp, false, false);
                    g.craftGrid[i] = tmp.id;
                }
            }
            // 归位剩余材料
            for (int i = 0; i < 9; i++)
                if (g.craftGrid[i] && !g.player().hasItem(g.craftGrid[i])) g.craftGrid[i] = 0;

            // 产物
            int rx = 0, rc = 0;
            int minx = 9, maxx = -1, miny = 9, maxy = -1;
            for (int y = 0; y < 3; y++)
                for (int x = 0; x < 3; x++)
                    if (g.craftGrid[y * 3 + x]) {
                        minx = std::min(minx, x); maxx = std::max(maxx, x);
                        miny = std::min(miny, y); maxy = std::max(maxy, y);
                    }
            if (maxx >= 0) {
                const Station st = (g.craftStation == ST_BENCH) ? ST_BENCH
                                 : (g.craftStation == ST_ANVIL) ? ST_ANVIL : ST_INVENTORY;
                int norm[9] = { 0 };
                for (int y = miny; y <= maxy; y++)
                    for (int x = minx; x <= maxx; x++) norm[(y - miny) * 3 + (x - minx)] = g.craftGrid[y * 3 + x];
                const Recipe* rec = findRecipe(norm, maxx - minx + 1, maxy - miny + 1, st);
                if (rec) { rx = rec->result; rc = rec->count; }
            }
            Rectangle rOut{ (float)(gx + 3 * SLOT + 24), (float)(gy + SLOT), (float)SLOT, (float)SLOT };
            ItemStack res{ rx, rc };
            if (inRect(rOut, GetMousePosition()) && rx) m_tooltipItem = rx;
            if (rx && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inRect(rOut, GetMousePosition())) {
                if (g.player().give(rx, rc)) {
                    for (int i = 0; i < 9; i++) if (g.craftGrid[i]) g.craftGrid[i] = 0;
                    audio::play(SFX_CRAFT, 0.6f);
                }
            }
        }

        // 垃圾箱
        Rectangle rTrash{ (float)(px + 848 - SLOT), (float)(py + 330 - SLOT - 12), (float)SLOT, (float)SLOT };
        if (g.heldMouse && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            && inRect(rTrash, GetMousePosition())) {
            g.player().give(g.heldMouse, g.heldMouseCount);
            g.heldMouse = 0; g.heldMouseCount = 0;
        }
        // 扔到世界里
        if (g.heldMouse && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !inRect({0,0,(float)SCREEN_W,(float)SCREEN_H}, GetMousePosition())) {
            // (不处理)
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------- 绘制
void UI::draw(Game& g)
{
    if (g.state != GS_DEAD) {
        drawHUD(g);
        drawHotbar(g);
        drawMinimap(g);
        drawBossBar(g);
    }
    drawMessages(g);
    if (g.invOpen) drawInventoryPanel(g);
    if (g.shopOpen) drawShopPanel(g);
    if (g.state == GS_DEAD) drawDeathScreen(g);
    if (g.state == GS_PAUSED) drawPauseMenu(g);

    // 光标上的物品
    if (g.heldMouse) {
        const Vector2 m = GetMousePosition();
        drawItemIcon(g.heldMouse, { m.x - 22, m.y - 22, 44, 44 });
        if (g.heldMouseCount > 1) {
            char b[16];
            snprintf(b, sizeof(b), "%d", g.heldMouseCount);
            text::DrawRight(b, (int)m.x + 20, (int)m.y + 8, 13, C_TEXT);
        }
    }
    if (m_tooltipItem && !g.heldMouse) {
        drawTooltip(g, m_tooltipItem, (int)GetMouseX() + 16, (int)GetMouseY() + 16);
        m_tooltipItem = 0;
    }
}

void UI::drawHUD(Game& g)
{
    const Player& p = g.player();
    const int hearts = (p.maxHp + 19) / 20;
    const int stars = (p.maxMana + 19) / 20;
    const Frame* hf = Assets::Find("heart_full");
    const Frame* he = Assets::Find("heart_empty");
    const Frame* sf = Assets::Find("star_full");
    const Frame* se = Assets::Find("star_empty");

    for (int i = 0; i < hearts && i < 40; i++) {
        const int col = i % 10, row = i / 10;
        const float x = 22.0f + col * 24.0f, y = 20.0f + row * 24.0f;
        const bool full = p.hp >= (i + 1) * 20;
        const bool half = !full && p.hp > i * 20;
        const Frame* f = full ? hf : he;
        if (f) drawFrameC(*f, x + 10, y + 10, false, WHITE);
        if (half && he) {
            BeginScissorMode((int)x, (int)y - 2, 11, 24);
            drawFrameC(*he, x + 10, y + 10, false, WHITE);
            if (hf) drawFrameC(*hf, x + 10, y + 10, false, WHITE);
            EndScissorMode();
        }
    }
    const int my = 20.0f + ((hearts + 9) / 10) * 24.0f + 4.0f;
    for (int i = 0; i < stars && i < 20; i++) {
        const int col = i % 10, row = i / 10;
        const float x = 22.0f + col * 24.0f, y = my + row * 24.0f;
        const Frame* f = (p.mana >= (i + 1) * 20) ? sf : se;
        if (f) drawFrameC(*f, x + 10, y + 10, false, WHITE);
    }

    // 金币 / 防御
    const int iy = my + ((stars + 9) / 10) * 24.0f + 4.0f;
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", p.coins);
    text::DrawShadow(buf, 24, iy, 15, rgb(255, 216, 120));
    text::DrawShadow("G", 24 + text::Width(buf, 15) + 4, iy, 13, rgb(210, 180, 90));
    snprintf(buf, sizeof(buf), "%s %d", loc::get(loc::S_DEFENSE), p.totalDefense());
    text::DrawShadow(buf, 24 + text::Width("0000", 15) + 40, iy, 14, C_TEXT);

    // 生物群系 / 深度
    const int tx = (int)(p.pos.x / TILE);
    const int ty = (int)(p.pos.y / TILE);
    int biome = g.world().biomeAt(tx);
    if (ty > UNDERWORLD_Y) biome = BIOME_HELL;
    else if (ty > g.world().surfaceY(tx) + 20) biome = BIOME_CAVE;
    const char* bn = loc::get((loc::Str)(loc::S_BIOME_FOREST + biome));
    snprintf(buf, sizeof(buf), "%s  %s %d", bn, loc::get(loc::S_DEPTH), (ty > g.world().surfaceY(tx)) ? (ty - g.world().surfaceY(tx)) : 0);
    text::DrawShadow(buf, SCREEN_W / 2 - text::Width(buf, 14) / 2, 22, 14, rgb(210, 214, 230));
}

void UI::drawHotbar(Game& g)
{
    const Player& p = g.player();
    const int w = HOTBAR_SLOTS * SLOT + 12;
    const int x0 = (SCREEN_W - w) / 2, y0 = SCREEN_H - SLOT - 12;
    DrawRectangleRec({ (float)(x0 - 6), (float)(y0 - 6), (float)(w + 12), (float)(SLOT + 12) }, rgb(20, 18, 30, 200));
    DrawRectangleLinesEx({ (float)(x0 - 6), (float)(y0 - 6), (float)(w + 12), (float)(SLOT + 12) }, 2.0f, rgb(80, 78, 108));
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        Rectangle r{ (float)(x0 + i * SLOT), (float)y0, (float)SLOT, (float)SLOT };
        drawSlot(g, r, p.inv[i], g.player().selected == i);
    }
    // 当前物品名
    if (p.held() && !g.invOpen) {
        const char* n = itemName(p.held());
        const int tw = text::Width(n, 15);
        DrawRectangleRec({ (float)(SCREEN_W / 2 - tw / 2 - 8), (float)(y0 - 32), (float)(tw + 16), 22 }, rgb(0, 0, 0, 150));
        text::DrawCentered(n, SCREEN_W / 2, y0 - 28, 15, rarityColor(itemDef(p.held()).rarity));
    }
    // 蓄力条
    if (p.bowCharging) {
        DrawRectangleRec({ (float)(SCREEN_W / 2 - 60), (float)(y0 - 44), 120, 8 }, rgb(20, 20, 26, 200));
        DrawRectangleRec({ (float)(SCREEN_W / 2 - 60), (float)(y0 - 44), 120.0f * p.bowCharge, 8 }, rgb(240, 220, 120));
    }
    if (p.useTimer > 0.0f) {
        const float t = 1.0f - p.useTimer / std::max(0.01f, itemDef(p.useItem).useTime);
        DrawRectangleRec({ (float)(SCREEN_W / 2 - 60), (float)(y0 - 44), 120, 8 }, rgb(20, 20, 26, 200));
        DrawRectangleRec({ (float)(SCREEN_W / 2 - 60), (float)(y0 - 44), 120.0f * t, 8 }, rgb(120, 230, 140));
    }
}

void UI::drawMinimap(Game& g)
{
    const int w = 200, h = 132;
    const int x = SCREEN_W - w - 16, y = 16;
    DrawRectangleRec({ (float)x, (float)y, (float)w, (float)h }, rgb(10, 10, 16, 210));
    DrawRectangleLinesEx({ (float)x, (float)y, (float)w, (float)h }, 2.0f, rgb(90, 88, 120));
    BeginScissorMode(x + 1, y + 1, w - 2, h - 2);
    const Player& p = g.player();
    const int px = (int)(p.pos.x / TILE), py = (int)(p.pos.y / TILE);
    const int step = 1;
    const float sc = 1.6f;
    for (int j = 0; j < h / (int)sc; j += step) {
        for (int i = 0; i < w / (int)sc; i += step) {
            const int tx = px + i - (int)(w / (2 * sc)), ty = py + j - (int)(h / (2 * sc));
            if (ty < 0 || ty >= WORLD_H) continue;
            const uint16_t t = g.world().tileAt(tx, ty);
            if (t == T_AIR) continue;
            const Color c = tileInfo(t).mapColor;
            DrawRectangle((int)x + i, (int)y + j, step + 1, step + 1, c);
        }
    }
    // 玩家
    DrawRectangle(x + w / 2 - 2, y + h / 2 - 2, 4, 4, rgb(255, 255, 255));
    EndScissorMode();
}

void UI::drawBossBar(Game& g)
{
    Entity* b = g.ents().findBoss();
    if (!b) return;
    const int w = 620, h = 22;
    const int x = (SCREEN_W - w) / 2, y = 76;
    DrawRectangleRec({ (float)x, (float)y, (float)w, (float)h }, rgb(18, 10, 16, 220));
    DrawRectangleRec({ (float)(x + 2), (float)(y + 2), (float)(w - 4) * clampf((float)b->hp / (float)b->maxHp, 0, 1), (float)(h - 4) }, rgb(196, 54, 54));
    DrawRectangleLinesEx({ (float)x, (float)y, (float)w, (float)h }, 2.0f, rgb(240, 200, 120));
    const char* nm = (b->type == E_KING_SLIME) ? loc::zh(loc::S_SLIME) : "克苏鲁之眼";
    text::DrawCentered(nm, SCREEN_W / 2, y - 20, 16, rgb(255, 220, 160));

    if (g.m_bossAlertTime > 0.0f) {
        const float a = clampf(g.m_bossAlertTime, 0.0f, 1.0f);
        const Color c = withAlpha(rgb(255, 90, 90), (int)(255 * a));
        text::DrawCentered(nm, SCREEN_W / 2, 132, 38, c);
        text::DrawCentered(loc::get(loc::S_APPEARED), SCREEN_W / 2, 174, 20, c);
    }
}

void UI::drawMessages(Game& g)
{
    int y = SCREEN_H - 120;
    for (auto it = g.m_msgs.rbegin(); it != g.m_msgs.rend(); ++it) {
        const float a = clampf(it->life, 0.0f, 1.0f);
        text::DrawRight(it->text.c_str(), SCREEN_W - 24, y, 15, withAlpha(it->color, (int)(255 * a)));
        y -= 20;
    }
}

void UI::drawDeathScreen(Game& g)
{
    DrawRectangleRec({ 0, 0, (float)SCREEN_W, (float)SCREEN_H }, rgb(64, 0, 0, 96));
    text::DrawCentered(loc::get(loc::S_YOU_DIED), SCREEN_W / 2, SCREEN_H / 2 - 60, 42, rgb(255, 210, 210));
    text::DrawCentered(loc::get(loc::S_RESPAWN), SCREEN_W / 2, SCREEN_H / 2 + 10, 20, rgb(240, 240, 240));
    (void)g;
}

void UI::drawPauseMenu(Game& g)
{
    DrawRectangleRec({ 0, 0, (float)SCREEN_W, (float)SCREEN_H }, rgb(8, 8, 14, 190));
    text::DrawCentered(loc::get(loc::S_TITLE), SCREEN_W / 2, 110, 34, rgb(240, 232, 200));
    text::DrawCentered(loc::get(loc::S_SUBTITLE), SCREEN_W / 2, 152, 16, C_DIM);
    text::DrawCentered(loc::get(loc::S_PAUSED), SCREEN_W / 2, 210, 22, C_TEXT);

    const char* lines[4] = { loc::get(loc::S_RESUME), loc::get(loc::S_SAVE), loc::get(loc::S_LOAD), loc::get(loc::S_QUIT) };
    for (int i = 0; i < 4; i++) {
        const int y = 270 + i * 44;
        const Rectangle r{ (float)(SCREEN_W / 2 - 130), (float)y, 260, 34 };
        const bool hov = inRect(r, GetMousePosition());
        DrawRectangleRec(r, hov ? rgb(64, 62, 96, 230) : rgb(40, 38, 60, 200));
        DrawRectangleLinesEx(r, 1.5f, hov ? rgb(220, 220, 240) : rgb(90, 88, 120));
        text::DrawCentered(lines[i], SCREEN_W / 2, y + 8, 17, C_TEXT);
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            audio::play(SFX_MENU, 0.6f);
            switch (i) {
            case 0: g.state = GS_PLAYING; break;
            case 1: {
                if (g.saveWorld(nullptr)) g.addMessage(loc::get(loc::S_SAVED), rgb(160, 240, 160));
                else g.addMessage(loc::get(loc::S_SAVE_FAIL), rgb(240, 140, 140));
                break;
            }
            case 2: {
                if (g.loadWorld(nullptr)) {
                    g.state = GS_PLAYING;
                    g.addMessage(loc::get(loc::S_LOADED), rgb(160, 240, 160));
                } else {
                    g.addMessage(loc::get(loc::S_LOAD_FAIL), rgb(240, 140, 140));
                }
                break;
            }
            case 3: CloseWindow(); break;
            }
        }
    }
    const char* tips[5] = { loc::get(loc::S_TIP_1), loc::get(loc::S_TIP_2), loc::get(loc::S_TIP_3),
                            loc::get(loc::S_TIP_4), loc::get(loc::S_TIP_5) };
    for (int i = 0; i < 5; i++)
        text::DrawCentered(tips[i], SCREEN_W / 2, 480 + i * 22, 14, rgb(150, 150, 172));
    text::DrawCentered("F5 保存 / F9 读取", SCREEN_W / 2, 600, 14, rgb(120, 120, 140));
}



void UI::drawInventoryPanel(Game& g)
{
    const int px = (SCREEN_W - 880) / 2, py = (SCREEN_H - 330) / 2;
    const Rectangle panel{ (float)(px - 10), (float)(py - 10), 880, 330 };
    panelFrame(panel, loc::get(loc::S_INV));

    // ---- 装备 ----
    const char* eqNames[4] = { loc::get(loc::S_EQUIP), "", "", "" };
    for (int i = 0; i < 4; i++) {
        const int col = i % 2, row = i / 2;
        Rectangle r{ (float)(px + 16 + col * (SLOT + 8)), (float)(py + 46 + row * (SLOT + 8)), (float)SLOT, (float)SLOT };
        ItemStack s{ g.player().equip[i], 1 };
        drawSlot(g, r, s, inRect(r, GetMousePosition()));
        static const char* kTag[4] = { "头", "身", "饰1", "饰2" };
        if (!s.id) text::DrawCentered(kTag[i], (int)(r.x + r.width / 2), (int)(r.y + r.height / 2 - 8), 14, rgb(110, 110, 130));
    }
    (void)eqNames;

    // ---- 属性 ----
    const Player& p = g.player();
    char buf[96];
    int ty = py + 196;
    snprintf(buf, sizeof(buf), "%s: %d / %d", loc::get(loc::S_HEALTH), p.hp, p.maxHp);
    text::Draw(buf, px + 12, ty, 13, rgb(240, 130, 130)); ty += 17;
    snprintf(buf, sizeof(buf), "%s: %d / %d", loc::get(loc::S_MANA), p.mana, p.maxMana);
    text::Draw(buf, px + 12, ty, 13, rgb(140, 180, 250)); ty += 17;
    snprintf(buf, sizeof(buf), "%s: %d", loc::get(loc::S_DEFENSE), p.totalDefense());
    text::Draw(buf, px + 12, ty, 13, rgb(200, 200, 210)); ty += 17;
    snprintf(buf, sizeof(buf), "%s: %d G", "金币", p.coins);
    text::Draw(buf, px + 12, ty, 13, rgb(255, 216, 120));

    // ---- 背包 ----
    for (int i = 0; i < INV_SLOTS; i++) {
        const int col = i % SLOTS_X, row = i / SLOTS_X;
        Rectangle r{ (float)(px + 130 + col * SLOT), (float)(py + 46 + row * SLOT), (float)SLOT, (float)SLOT };
        const bool sel = (i == g.player().selected && i < HOTBAR_SLOTS);
        drawSlot(g, r, p.inv[i], sel || inRect(r, GetMousePosition()));
    }

    // ---- 箱子 / 合成 / 熔炉 ----
    if (Chest* chest = g.openChest()) {
        panelFrame({ (float)(px + 614), (float)(py + 34), 254, 222 }, loc::get(loc::S_CHEST));
        for (int i = 0; i < 20; i++) {
            const int col = i % 5, row = i / 5;
            Rectangle r{ (float)(px + 630 + col * SLOT), (float)(py + 70 + row * SLOT), (float)SLOT, (float)SLOT };
            drawSlot(g, r, chest->slots[i], inRect(r, GetMousePosition()));
        }
    } else if (g.craftStation == ST_FURNACE) {
        panelFrame({ (float)(px + 614), (float)(py + 34), 254, 196 }, loc::get(loc::S_FURNACE_UI));
        ItemStack sIn{ g.furnaceIn, 1 }, sFuel{ g.furnaceFuel, 1 }, sOut{ g.furnaceOut, g.furnaceOutCount };
        drawSlot(g, { (float)(px + 630), (float)(py + 56), (float)SLOT, (float)SLOT }, sIn, false);
        drawSlot(g, { (float)(px + 630), (float)(py + 130), (float)SLOT, (float)SLOT }, sFuel, false);
        drawSlot(g, { (float)(px + 742), (float)(py + 100), (float)SLOT, (float)SLOT }, sOut, false);
        text::DrawCentered(">", (int)(px + 700), (int)(py + 70), 20, C_DIM);
        const int prog = clampi((int)(g.furnaceTimer / 1.2f * 46.0f), 0, 46);
        DrawRectangleRec({ (float)(px + 686), (float)(py + 128), 46, 6 }, rgb(20, 20, 26, 200));
        DrawRectangleRec({ (float)(px + 686), (float)(py + 128), (float)prog, 6 }, rgb(240, 150, 60));
    } else {
        const char* title = (g.craftStation == ST_BENCH) ? loc::get(loc::S_WORKBENCH)
                          : (g.craftStation == ST_ANVIL) ? loc::get(loc::S_ANVIL) : loc::get(loc::S_CRAFT);
        panelFrame({ (float)(px + 614), (float)(py + 34), 254, 196 }, title);
        for (int i = 0; i < 9; i++) {
            const int col = i % 3, row = i / 3;
            Rectangle r{ (float)(px + 630 + col * SLOT), (float)(py + 70 + row * SLOT), (float)SLOT, (float)SLOT };
            ItemStack s{ g.craftGrid[i], 1 };
            drawSlot(g, r, s, inRect(r, GetMousePosition()));
        }
        // 产物预览
        int rx = 0, rc = 0;
        int minx = 9, maxx = -1, miny = 9, maxy = -1;
        for (int y = 0; y < 3; y++)
            for (int x = 0; x < 3; x++)
                if (g.craftGrid[y * 3 + x]) {
                    minx = std::min(minx, x); maxx = std::max(maxx, x);
                    miny = std::min(miny, y); maxy = std::max(maxy, y);
                }
        if (maxx >= 0) {
            const Station st = (g.craftStation == ST_BENCH) ? ST_BENCH
                             : (g.craftStation == ST_ANVIL) ? ST_ANVIL : ST_INVENTORY;
            int norm[9] = { 0 };
            for (int y = miny; y <= maxy; y++)
                for (int x = minx; x <= maxx; x++) norm[(y - miny) * 3 + (x - minx)] = g.craftGrid[y * 3 + x];
            const Recipe* rec = findRecipe(norm, maxx - minx + 1, maxy - miny + 1, st);
            if (rec) { rx = rec->result; rc = rec->count; }
        }
        Rectangle rOut{ (float)(px + 630 + 3 * SLOT + 24), (float)(py + 70 + SLOT), (float)SLOT, (float)SLOT };
        ItemStack res{ rx, rc };
        drawSlot(g, rOut, res, rx && inRect(rOut, GetMousePosition()));
    }

    // 垃圾箱
    Rectangle rTrash{ (float)(px + 848 - SLOT), (float)(py + 330 - SLOT - 12), (float)SLOT, (float)SLOT };
    DrawRectangleRec(rTrash, rgb(70, 40, 44, 220));
    DrawRectangleLinesEx(rTrash, 1.0f, rgb(150, 90, 90));
    text::DrawCentered(loc::get(loc::S_TRASH), (int)(rTrash.x + rTrash.width / 2), (int)(rTrash.y + 14), 14, rgb(230, 180, 180));

    text::DrawCentered(loc::get(loc::S_CLOSE), (int)(panel.x + panel.width / 2), (int)(panel.y + panel.height - 24), 14, C_DIM);
}

void UI::drawShopPanel(Game& g)
{
    struct ShopItem { int id; int price; };
    static const ShopItem kShop[] = {
        { IT_POTION_HEALTH, 40 }, { IT_TORCH, 12 }, { IT_PLATFORM, 25 },
        { IT_WOOD_PANEL, 20 },   { IT_STRING, 18 }, { IT_PICK_COPPER, 120 },
        { IT_SWORD_COPPER, 150 },{ IT_MAGIC_DART, 60 }, { IT_SLIME_CROWN, 500 },
    };
    const int n = (int)(sizeof(kShop) / sizeof(kShop[0]));
    const int w = 460, h = 90 + n * 46;
    const int px = SCREEN_W - w - 40, py = 90;
    panelFrame({ (float)px, (float)py, (float)w, (float)h }, loc::get(loc::S_SHOP));

    text::Draw(loc::get(g.shopNpc == 1 ? loc::S_MERCH_1 : loc::S_GUIDE_1), px + 18, py + 44, 14, C_DIM);
    char cb[64];
    snprintf(cb, sizeof(cb), "%d G", g.player().coins);
    text::DrawRight(cb, px + w - 18, py + 44, 15, rgb(255, 216, 120));

    for (int i = 0; i < n; i++) {
        const int y = py + 72 + i * 46;
        Rectangle r{ (float)(px + 18), (float)y, (float)w - 36, 42 };
        const bool hov = inRect(r, GetMousePosition());
        const bool can = g.player().coins >= kShop[i].price;
        DrawRectangleRec(r, hov ? rgb(58, 56, 84, 220) : rgb(38, 36, 56, 180));
        ItemStack s{ kShop[i].id, 1 };
        drawSlot(g, { r.x, r.y, 42, 42 }, s, false);
        text::Draw(itemName(kShop[i].id), (int)(r.x + 52), (int)(r.y + 6), 15, rarityColor(itemDef(kShop[i].id).rarity));
        char pb[32];
        snprintf(pb, sizeof(pb), "%d G", kShop[i].price);
        text::DrawRight(pb, (int)(r.x + r.width - 12), (int)(r.y + 12), 14, can ? rgb(255, 216, 120) : rgb(140, 90, 90));
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && can) {
            audio::play(SFX_COIN, 0.6f);
            g.player().coins -= kShop[i].price;
            g.player().give(kShop[i].id, 1);
        }
    }
    text::DrawCentered(loc::get(g.shopNpc == 1 ? loc::S_MERCH_LEAVE : loc::S_MERCH_2),
                       px + w / 2, py + h - 24, 13, C_DIM);
}

void UI::drawTooltip(Game& g, int itemId, int mx, int my)
{
    if (!itemId) return;
    const ItemDef& d = itemDef(itemId);
    const int w = 220, lh = 18;
    int lines = 2;
    if (d.damage) lines += 2;
    if (d.defense) lines += 1;
    if (d.manaCost) lines += 1;
    if (d.pickPower) lines += 1;
    if (d.lifeBonus || d.manaBonus || d.speedMult != 1.0f) lines += 1;
    if (d.value) lines += 1;
    const int h = 34 + lines * lh;
    int x = mx, y = my;
    if (x + w > SCREEN_W - 8) x = SCREEN_W - 8 - w;
    if (y + h > SCREEN_H - 8) y = SCREEN_H - 8 - h;
    const Rectangle r{ (float)x, (float)y, (float)w, (float)h };
    DrawRectangleRec(r, rgb(24, 22, 38, 242));
    DrawRectangleLinesEx(r, 1.5f, rarityColor(d.rarity));

    ItemStack s{ itemId, 1 };
    drawSlot(g, { r.x + 6, r.y + 6, 36, 36 }, s, false);
    text::Draw(d.name, (int)(r.x + 48), (int)(r.y + 8), 16, rarityColor(d.rarity));
    const char* rn = loc::get((loc::Str)(loc::S_RARITY_0 + d.rarity));
    text::Draw(rn, (int)(r.x + 48), (int)(r.y + 27), 12, rgb(140, 140, 160));

    int ly = y + 48;
    char buf[96];
    if (d.damage) { snprintf(buf, sizeof(buf), "%s: %d", loc::get(loc::S_DAMAGE), d.damage); text::Draw(buf, (int)(r.x + 10), ly, 14, rgb(235, 235, 245)); ly += lh; }
    if (d.kind == IK_SWORD || d.kind == IK_AXE) {
        snprintf(buf, sizeof(buf), "攻速: %.2f", 1.0f / std::max(0.05f, d.useTime));
        text::Draw(buf, (int)(r.x + 10), ly, 14, rgb(200, 200, 215)); ly += lh;
    }
    if (d.defense) { snprintf(buf, sizeof(buf), "%s: %d", loc::get(loc::S_DEFENSE), d.defense); text::Draw(buf, (int)(r.x + 10), ly, 14, rgb(200, 220, 255)); ly += lh; }
    if (d.manaCost) { snprintf(buf, sizeof(buf), "魔力: %d", d.manaCost); text::Draw(buf, (int)(r.x + 10), ly, 14, rgb(150, 180, 255)); ly += lh; }
    if (d.pickPower) { snprintf(buf, sizeof(buf), "镐力: %d", d.pickPower); text::Draw(buf, (int)(r.x + 10), ly, 14, rgb(200, 200, 215)); ly += lh; }
    if (d.lifeBonus) { snprintf(buf, sizeof(buf), "生命上限 +%d", d.lifeBonus); text::Draw(buf, (int)(r.x + 10), ly, 14, rgb(240, 140, 140)); ly += lh; }
    if (d.manaBonus) { snprintf(buf, sizeof(buf), "魔力上限 +%d", d.manaBonus); text::Draw(buf, (int)(r.x + 10), ly, 14, rgb(150, 180, 255)); ly += lh; }
    if (d.speedMult != 1.0f) { snprintf(buf, sizeof(buf), "移速 +%d%%", (int)((d.speedMult - 1.0f) * 100)); text::Draw(buf, (int)(r.x + 10), ly, 14, rgb(200, 240, 200)); ly += lh; }
    if (d.value) { snprintf(buf, sizeof(buf), "售价: %d G", d.value); text::Draw(buf, (int)(r.x + 10), ly, 14, rgb(200, 180, 120)); ly += lh; }
    if (d.desc && d.desc[0]) { text::Draw(d.desc, (int)(r.x + 10), ly, 13, rgb(150, 150, 168)); ly += lh; }
    (void)g;
}

}  // namespace tc
