#include "raylib.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

static const int W = 720;
static const int H = 640;

static const Color COL_BG{ 246, 247, 251, 255 };
static const Color COL_INK{ 40, 42, 54, 255 };
static const Color COL_GRAY{ 128, 133, 150, 255 };
static const Color COL_PRIMARY{ 72, 84, 175, 255 };
static const Color COL_BOX_BORDER{ 205, 209, 220, 255 };
static const Color COL_GREEN{ 67, 160, 71, 255 };
static const Color COL_RED{ 229, 57, 53, 255 };
static const Color COL_ORANGE{ 245, 145, 32, 255 };
static const Color COL_BLUE{ 30, 136, 229, 255 };

static const char* kRewards[] = {
    "奖励：神秘小礼物一份",
    "奖励：20 元以内的小礼物一份",
    "奖励：奶茶一杯（中杯）",
    "奖励：口头表扬一次，掌声鼓励！",
    "奖励：下次挑战提示卡一张",
    "奖励：今日运气爆棚，好运连连",
};

static const char* kGlyphSeed[] = {
    "猜 数 字",
    "自定义范围与次数的猜数挑战",
    "范围下限", "范围上限", "猜测次数", "开始游戏",
    "请输入有效的数字", "下限需要小于上限", "次数需要在 1 到 50 之间",
    "第", "次", "剩余", "目标区间", "你的猜想", "猜 !",
    "太大了 ↑", "太小了 ↓", "答对了！", "请先输入一个数字再点猜哦",
    "超出设定范围，不消耗次数",
    "恭喜！你猜中了，答案是", "机会用完啦，正确答案是",
    kRewards[0], kRewards[1], kRewards[2], kRewards[3], kRewards[4], kRewards[5],
    "再来一局", "修改设置", "历史记录", "暂无记录，快输入第一次猜想吧",
    "点击 [再来一局] 继续挑战",
    "回车快速提交 · 二分逼近最快",
    "输入你的第一个猜想吧",
    "0123456789#->[]!·^v(),.:/%",
};

struct InputBox
{
    Rectangle rect{};
    char text[16]{};
    bool focus = false;
    int maxLen = 9;
};

struct Particle
{
    Vector2 pos{};
    Vector2 vel{};
    Color col{};
    float life = 0.0f;
    float radius = 0.0f;
};

enum Screen { SCR_SETUP, SCR_PLAY };

static Screen g_screen = SCR_SETUP;

static InputBox g_boxLow;
static InputBox g_boxHigh;
static InputBox g_boxTries;
static InputBox g_boxGuess;
static char g_setupError[128] = "";

static int g_low = 1;
static int g_high = 100;
static int g_total = 7;
static int g_answer = 0;
static int g_used = 0;
static int g_curLow = 1;
static int g_curHigh = 100;
static bool g_over = false;
static bool g_won = false;
static const char* g_reward = "";
static std::vector<std::string> g_history;
static char g_feedback[128] = "";
static Color g_feedbackCol = COL_GRAY;
static std::vector<Particle> g_particles;

static Font g_fontBig;
static Font g_fontMid;

static void CollectCodepoints(const char* s, std::vector<int>& out)
{
    while (*s)
    {
        int len = 0;
        int cp = GetCodepointNext(s, &len);
        out.push_back(cp);
        s += len;
    }
}

static Font LoadUiFont(int size)
{
    const char* candidates[] = {
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/Deng.ttf",
        "C:/Windows/Fonts/simkai.ttf",
        "C:/Windows/Fonts/msyh.ttc",
    };

    std::vector<int> cps;
    for (int c = 0x20; c <= 0x7E; ++c)
    {
        cps.push_back(c);
    }
    for (const char* s : kGlyphSeed)
    {
        CollectCodepoints(s, cps);
    }

    Font font{};
    for (const char* path : candidates)
    {
        if (FileExists(path))
        {
            font = LoadFontEx(path, size, cps.data(), (int)cps.size());
            if (font.texture.id != 0)
            {
                SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
                return font;
            }
        }
    }
    return GetFontDefault();
}

static void DrawTextCentered(Font font, const char* text, float size, float y, Color col, float spacing = 1.0f)
{
    Vector2 m = MeasureTextEx(font, text, size, spacing);
    DrawTextEx(font, text, { (W - m.x) / 2.0f, y }, size, spacing, col);
}

static void DrawTextRight(Font font, const char* text, float size, float rightX, float y, Color col, float spacing = 1.0f)
{
    Vector2 m = MeasureTextEx(font, text, size, spacing);
    DrawTextEx(font, text, { rightX - m.x, y }, size, spacing, col);
}

static bool Button(Rectangle r, const char* label, Color base, Font font, float textSize)
{
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, r);
    Color fill = hover ? ColorBrightness(base, 0.15f) : base;
    DrawRectangleRounded(r, 0.35f, 8, fill);
    Vector2 m = MeasureTextEx(font, label, textSize, 1.0f);
    DrawTextEx(font, label,
               { r.x + (r.width - m.x) / 2.0f, r.y + (r.height - m.y) / 2.0f },
               textSize, 1.0f, WHITE);
    return hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static void UpdateInputBox(InputBox& box, bool& anyFocused, bool enabled)
{
    if (!enabled)
    {
        box.focus = false;
        return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        box.focus = CheckCollisionPointRec(GetMousePosition(), box.rect);
        if (box.focus)
        {
            anyFocused = true;
        }
    }
    if (!box.focus)
    {
        return;
    }
    int ch = GetCharPressed();
    while (ch > 0)
    {
        if (ch >= '0' && ch <= '9' && (int)strlen(box.text) < box.maxLen)
        {
            int n = (int)strlen(box.text);
            box.text[n] = (char)ch;
            box.text[n + 1] = '\0';
        }
        ch = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && strlen(box.text) > 0)
    {
        box.text[strlen(box.text) - 1] = '\0';
    }
}

static void DrawInputBox(InputBox& box, Font font, bool enabled)
{
    Color bg = enabled ? WHITE : Color{ 233, 235, 240, 255 };
    Color border = box.focus ? COL_PRIMARY : COL_BOX_BORDER;
    float thickness = box.focus ? 2.5f : 1.5f;
    DrawRectangleRounded(box.rect, 0.25f, 6, bg);
    DrawRectangleRoundedLinesEx(box.rect, 0.25f, 6, thickness, border);

    Vector2 m = MeasureTextEx(font, box.text, 22.0f, 1.0f);
    DrawTextEx(font, box.text, { box.rect.x + 14, box.rect.y + (box.rect.height - m.y) / 2.0f }, 22.0f, 1.0f, COL_INK);

    if (box.focus && fmodf(GetTime(), 1.0f) < 0.5f)
    {
        float caretX = box.rect.x + 14 + m.x + 2.0f;
        DrawRectangleRec({ caretX, box.rect.y + 10, 2, box.rect.height - 20 }, COL_PRIMARY);
    }
}

static int ParseBox(const InputBox& box)
{
    return atoi(box.text);
}

static void SpawnConfetti()
{
    Vector2 center{ W / 2.0f, 570.0f };
    Color palette[] = {
        COL_RED, COL_ORANGE, COL_GREEN, COL_BLUE, COL_PRIMARY,
        Color{ 233, 30, 99, 255 }, Color{ 0, 188, 212, 255 },
    };
    for (int i = 0; i < 110; ++i)
    {
        Particle p;
        float ang = (float)GetRandomValue(-180, 180) * DEG2RAD - PI / 2.0f;
        float speed = (float)GetRandomValue(140, 420);
        p.pos = center;
        p.vel = { cosf(ang) * speed, sinf(ang) * speed };
        p.col = palette[GetRandomValue(0, (int)(sizeof(palette) / sizeof(palette[0])) - 1)];
        p.life = (float)GetRandomValue(90, 170) / 100.0f;
        p.radius = (float)GetRandomValue(3, 7);
        g_particles.push_back(p);
    }
}

static void StartGame()
{
    g_answer = g_low + rand() % (g_high - g_low + 1);
    g_used = 0;
    g_curLow = g_low;
    g_curHigh = g_high;
    g_over = false;
    g_won = false;
    g_reward = "";
    g_history.clear();
    g_particles.clear();
    g_feedback[0] = '\0';
    g_feedbackCol = COL_GRAY;
    g_boxGuess.text[0] = '\0';
    g_screen = SCR_PLAY;
}

static void MakeGuess()
{
    if (g_over)
    {
        return;
    }
    if (strlen(g_boxGuess.text) == 0)
    {
        strcpy_s(g_feedback, "请先输入一个数字再点猜哦");
        g_feedbackCol = COL_GRAY;
        return;
    }
    int v = atoi(g_boxGuess.text);
    if (v < g_low || v > g_high)
    {
        strcpy_s(g_feedback, "超出设定范围，不消耗次数");
        g_feedbackCol = COL_ORANGE;
        return;
    }

    g_used++;
    char line[64];
    if (v == g_answer)
    {
        snprintf(line, sizeof(line), "#%d  %d -> ", g_used, v);
        g_history.push_back(std::string(line) + "答对了！");
        g_over = true;
        g_won = true;
        g_reward = kRewards[rand() % (int)(sizeof(kRewards) / sizeof(kRewards[0]))];
        snprintf(g_feedback, sizeof(g_feedback), "恭喜！你猜中了，答案是 %d", g_answer);
        g_feedbackCol = COL_GREEN;
        SpawnConfetti();
        g_boxGuess.text[0] = '\0';
        return;
    }

    if (v > g_answer)
    {
        snprintf(line, sizeof(line), "#%d  %d -> 太大了 ↑", g_used, v);
        g_history.push_back(line);
        strcpy_s(g_feedback, "太大了 ↑");
        g_feedbackCol = COL_RED;
        if (v - 1 < g_curHigh)
        {
            g_curHigh = v - 1;
        }
    }
    else
    {
        snprintf(line, sizeof(line), "#%d  %d -> 太小了 ↓", g_used, v);
        g_history.push_back(line);
        strcpy_s(g_feedback, "太小了 ↓");
        g_feedbackCol = COL_BLUE;
        if (v + 1 > g_curLow)
        {
            g_curLow = v + 1;
        }
    }

    if (g_used >= g_total)
    {
        g_over = true;
        snprintf(g_feedback, sizeof(g_feedback), "机会用完啦，正确答案是 %d", g_answer);
        g_feedbackCol = COL_RED;
    }
    g_boxGuess.text[0] = '\0';
}

static void UpdateSetup()
{
    bool anyFocused = false;
    UpdateInputBox(g_boxLow, anyFocused, true);
    UpdateInputBox(g_boxHigh, anyFocused, true);
    UpdateInputBox(g_boxTries, anyFocused, true);

    bool enter = anyFocused && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER));
    Rectangle startBtn{ W / 2.0f - 88, 400, 176, 54 };
    if (enter || Button(startBtn, "开始游戏", COL_PRIMARY, g_fontMid, 24))
    {
        if (strlen(g_boxLow.text) == 0 || strlen(g_boxHigh.text) == 0 || strlen(g_boxTries.text) == 0)
        {
            strcpy_s(g_setupError, "请输入有效的数字");
        }
        else
        {
            int lo = atoi(g_boxLow.text);
            int hi = atoi(g_boxHigh.text);
            int tr = atoi(g_boxTries.text);
            if (lo >= hi)
            {
                strcpy_s(g_setupError, "下限需要小于上限");
            }
            else if (tr < 1 || tr > 50)
            {
                strcpy_s(g_setupError, "次数需要在 1 到 50 之间");
            }
            else
            {
                g_setupError[0] = '\0';
                g_low = lo;
                g_high = hi;
                g_total = tr;
                StartGame();
            }
        }
    }
}

static void UpdatePlay()
{
    float dt = GetFrameTime();
    for (Particle& p : g_particles)
    {
        p.vel.y += 620.0f * dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.life -= dt;
    }
    g_particles.erase(
        std::remove_if(g_particles.begin(), g_particles.end(),
                       [](const Particle& p) { return p.life <= 0.0f; }),
        g_particles.end());

    bool anyFocused = false;
    UpdateInputBox(g_boxGuess, anyFocused, !g_over);

    bool enter = !g_over && g_boxGuess.focus &&
                 (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER));

    Rectangle goBtn{ 445, 170, 110, 48 };
    Rectangle cfgBtn{ 575, 170, 105, 48 };

    if (g_over)
    {
        if (Button(goBtn, "再来一局", COL_PRIMARY, g_fontMid, 21))
        {
            StartGame();
            return;
        }
        if (Button(cfgBtn, "修改设置", COL_GRAY, g_fontMid, 19))
        {
            g_screen = SCR_SETUP;
            return;
        }
    }
    else
    {
        if (enter || Button(goBtn, "猜 !", COL_PRIMARY, g_fontMid, 22))
        {
            MakeGuess();
        }
    }
}

static void DrawSetup()
{
    DrawTextCentered(g_fontBig, "猜 数 字", 52.0f, 58, COL_PRIMARY, 10.0f);
    DrawTextCentered(g_fontMid, "自定义范围与次数的猜数挑战", 20.0f, 132, COL_GRAY);

    struct Row { const char* label; InputBox& box; float y; };
    Row rows[] = {
        { "范围下限", g_boxLow, 196 },
        { "范围上限", g_boxHigh, 258 },
        { "猜测次数", g_boxTries, 320 },
    };
    for (Row& r : rows)
    {
        DrawTextRight(g_fontMid, r.label, 21.0f, 250, r.y + 11, COL_INK);
        DrawInputBox(r.box, g_fontMid, true);
    }

    Rectangle startBtn{ W / 2.0f - 88, 400, 176, 54 };
    Button(startBtn, "开始游戏", COL_PRIMARY, g_fontMid, 24);

    if (g_setupError[0] != '\0')
    {
        DrawTextCentered(g_fontMid, g_setupError, 19.0f, 486, COL_RED);
    }
    DrawTextCentered(g_fontMid, "回车快速提交 · 二分逼近最快", 15.0f, 596, COL_GRAY);
}

static void DrawPlay()
{
    char buf[128];

    snprintf(buf, sizeof(buf), "第 %d / %d 次", g_used, g_total);
    DrawTextEx(g_fontMid, buf, { 40, 26 }, 22.0f, 1.0f, COL_INK);

    snprintf(buf, sizeof(buf), "剩余 %d 次", g_total - g_used);
    DrawTextRight(g_fontMid, buf, 19.0f, 680, 29, COL_GRAY);

    Rectangle bar{ 40, 66, 640, 12 };
    DrawRectangleRounded(bar, 0.5f, 6, Color{ 228, 230, 238, 255 });
    float remain = g_total > 0 ? (float)(g_total - g_used) / (float)g_total : 0.0f;
    Color barCol = remain > 0.5f ? Color{ 76, 175, 80, 255 }
                 : remain > 0.25f ? Color{ 255, 152, 0, 255 }
                 : Color{ 229, 57, 53, 255 };
    if (remain > 0.0f)
    {
        Rectangle fillBar{ bar.x, bar.y, bar.width * remain, bar.height };
        DrawRectangleRounded(fillBar, 0.5f, 6, barCol);
    }

    snprintf(buf, sizeof(buf), "目标区间 [ %d , %d ]", g_curLow, g_curHigh);
    DrawTextCentered(g_fontMid, buf, 19.0f, 98, COL_GRAY);

    DrawTextCentered(g_fontBig, g_feedback[0] ? g_feedback : "输入你的第一个猜想吧", 33.0f, 126, g_feedbackCol);

    DrawTextRight(g_fontMid, "你的猜想:", 20.0f, 205, 184, COL_INK);
    DrawInputBox(g_boxGuess, g_fontMid, !g_over);

    Rectangle goBtn{ 445, 170, 110, 48 };
    Rectangle cfgBtn{ 575, 170, 105, 48 };
    if (g_over)
    {
        Button(goBtn, "再来一局", COL_PRIMARY, g_fontMid, 21);
        Button(cfgBtn, "修改设置", COL_GRAY, g_fontMid, 19);
    }
    else
    {
        Button(goBtn, "猜 !", COL_PRIMARY, g_fontMid, 22);
    }

    DrawTextEx(g_fontMid, "历史记录", { 40, 244 }, 16.0f, 1.0f, COL_GRAY);
    Rectangle listRect{ 40, 272, 640, 244 };
    DrawRectangleRounded(listRect, 0.06f, 6, WHITE);
    DrawRectangleRoundedLinesEx(listRect, 0.06f, 6, 1.5f, COL_BOX_BORDER);

    BeginScissorMode((int)listRect.x + 6, (int)listRect.y + 6,
                     (int)listRect.width - 12, (int)listRect.height - 12);
    if (g_history.empty())
    {
        DrawTextEx(g_fontMid, "暂无记录，快输入第一次猜想吧",
                   { listRect.x + 16, listRect.y + 16 }, 17.0f, 1.0f, COL_GRAY);
    }
    else
    {
        const float lineH = 27.0f;
        const float viewH = listRect.height - 12;
        float totalH = (float)g_history.size() * lineH;
        float offsetY = totalH > viewH ? totalH - viewH : 0.0f;
        float y = listRect.y + 6 - offsetY;
        for (const std::string& s : g_history)
        {
            Color col = COL_INK;
            if (s.find("答对") != std::string::npos)
            {
                col = COL_GREEN;
            }
            else if (s.find("大") != std::string::npos)
            {
                col = COL_RED;
            }
            else if (s.find("小") != std::string::npos)
            {
                col = COL_BLUE;
            }
            DrawTextEx(g_fontMid, s.c_str(), { listRect.x + 16, y }, 17.0f, 1.0f, col);
            y += lineH;
        }
    }
    EndScissorMode();

    Rectangle banner{ 40, 534, 640, 74 };
    if (g_over)
    {
        Color bg = g_won ? Color{ 232, 245, 233, 255 } : Color{ 253, 235, 236, 255 };
        Color border = g_won ? COL_GREEN : COL_RED;
        DrawRectangleRounded(banner, 0.22f, 8, bg);
        DrawRectangleRoundedLinesEx(banner, 0.22f, 8, 2.0f, border);

        DrawTextEx(g_fontMid, g_feedback, { banner.x + 24, banner.y + 12 }, 22.0f, 1.0f, border);
        const char* sub = g_won ? g_reward : "点击 [再来一局] 继续挑战";
        DrawTextEx(g_fontMid, sub, { banner.x + 24, banner.y + 44 }, 19.0f, 1.0f, COL_INK);
    }

    for (const Particle& p : g_particles)
    {
        Color c = p.col;
        c.a = (unsigned char)(std::clamp(p.life, 0.0f, 1.0f) * 255.0f);
        DrawCircleV(p.pos, p.radius, c);
    }
}

int RunGame()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(W, H, "猜数字 · 图形版");
    srand((unsigned int)time(nullptr));

    g_fontBig = LoadUiFont(40);
    g_fontMid = LoadUiFont(22);

    g_boxLow = InputBox{ Rectangle{ 270, 196, 180, 46 }, "1", false, 9 };
    g_boxHigh = InputBox{ Rectangle{ 270, 258, 180, 46 }, "100", false, 9 };
    g_boxTries = InputBox{ Rectangle{ 270, 320, 180, 46 }, "7", false, 9 };
    g_boxGuess = InputBox{ Rectangle{ 215, 170, 190, 48 }, "", false, 9 };

    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        if (g_screen == SCR_SETUP)
        {
            UpdateSetup();
        }
        else
        {
            UpdatePlay();
        }

        BeginDrawing();
        ClearBackground(COL_BG);
        if (g_screen == SCR_SETUP)
        {
            DrawSetup();
        }
        else
        {
            DrawPlay();
        }
        EndDrawing();
    }

    UnloadFont(g_fontBig);
    UnloadFont(g_fontMid);
    CloseWindow();
    return 0;
}

int main()
{
    return RunGame();
}
