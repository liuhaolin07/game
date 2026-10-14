// Assets.cpp —— 资源加载实现
#include "Assets.h"
#include "Text.h"

#include <cstdio>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace tc {

Color rgb(int r, int g, int b, int a) { return Color{ (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a }; }
Color withAlpha(Color c, int a) { c.a = (unsigned char)clampi(a, 0, 255); return c; }
Color mixColor(Color a, Color b, float t) { return ColorLerp(a, b, t); }
Color scaleColor(Color c, float f)
{
    c.r = (unsigned char)clampi((int)(c.r * f), 0, 255);
    c.g = (unsigned char)clampi((int)(c.g * f), 0, 255);
    c.b = (unsigned char)clampi((int)(c.b * f), 0, 255);
    return c;
}

namespace {
std::unordered_map<std::string, Frame> g_frames;
std::unordered_map<std::string, Texture2D> g_textures;
}  // namespace

bool Assets::Load(const char* assetDir)
{
    std::string dir = assetDir;
    if (dir.empty()) dir = "assets";
    if (dir.back() != '/' && dir.back() != '\\') dir += "/";

    const char* files[] = { "tiles.png", "items.png", "entities.png", "gui.png" };
    for (const char* f : files) {
        std::string path = dir + f;
        if (!FileExists(path.c_str())) {
            TraceLog(LOG_ERROR, "缺少素材文件: %s", path.c_str());
            return false;
        }
        g_textures[f] = LoadTexture(path.c_str());
    }

    std::string manifest = dir + "atlas.txt";
    if (!FileExists(manifest.c_str())) {
        TraceLog(LOG_ERROR, "缺少素材清单: %s", manifest.c_str());
        return false;
    }
    std::ifstream fin(manifest);
    if (!fin) {
        TraceLog(LOG_ERROR, "无法打开素材清单 %s", manifest.c_str());
        return false;
    }
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string file, name;
        int x, y, w, h;
        if (!(ss >> file >> name >> x >> y >> w >> h)) continue;
        auto it = g_textures.find(file);
        if (it == g_textures.end()) continue;
        Frame f;
        f.tex = it->second;
        f.rect = { (float)x, (float)y, (float)w, (float)h };
        g_frames[name] = f;
    }

    TraceLog(LOG_INFO, "已载入素材 %d 帧", (int)g_frames.size());
    return !g_frames.empty();
}

void Assets::Unload()
{
    for (auto& kv : g_textures) UnloadTexture(kv.second);
    g_textures.clear();
    g_frames.clear();
}

const Frame* Assets::Find(const char* name)
{
    auto it = g_frames.find(name);
    return it == g_frames.end() ? nullptr : &it->second;
}

const Frame* Assets::FindTile(const char* name)
{
    const std::string k = std::string("tile:") + name;
    auto it = g_frames.find(k);
    return it == g_frames.end() ? nullptr : &it->second;
}

const Frame* Assets::FindItem(const char* name)
{
    const std::string k = std::string("item:") + name;
    auto it = g_frames.find(k);
    return it == g_frames.end() ? nullptr : &it->second;
}

bool Assets::Exists(const char* name) { return g_frames.find(name) != g_frames.end(); }

void Assets::SetPixelFilter()
{
    for (auto& kv : g_textures) SetTextureFilter(kv.second, TEXTURE_FILTER_POINT);
}

void drawFrame(const Frame& f, float x, float y, bool flip, Color tint, float scale)
{
    if (!f.tex.id) return;
    const float w = f.rect.width * scale, h = f.rect.height * scale;
    if (flip) DrawTexturePro(f.tex, f.rect, { x + w, y, -w, h }, { 0, 0 }, 0.0f, tint);
    else DrawTexturePro(f.tex, f.rect, { x, y, w, h }, { 0, 0 }, 0.0f, tint);
}

void drawFrameC(const Frame& f, float cx, float cy, bool flip, Color tint, float scale)
{
    drawFrame(f, cx - f.rect.width * scale * 0.5f, cy - f.rect.height * scale * 0.5f, flip, tint, scale);
}

void drawSprite(const char* name, float x, float y, bool flip, Color tint, float scale)
{
    const Frame* f = Assets::Find(name);
    if (f) drawFrame(*f, x, y, flip, tint, scale);
}

void drawSpriteC(const char* name, float cx, float cy, bool flip, Color tint, float scale)
{
    const Frame* f = Assets::Find(name);
    if (f) drawFrameC(*f, cx, cy, flip, tint, scale);
}

// ---------------------------------------------------------------- 文本
namespace text {
namespace {
Font g_font;
bool g_cjk = false;
float g_lineSpacing = 1.0f;
}  // namespace

void Init()
{
    // 中文系统字体候选(Windows 自带), 找到就用, 找不到退回默认字体
    const char* candidates[] = {
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc",
        "C:/Windows/Fonts/Deng.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    };
    // 需要打进字体的字符: 全部界面文案 + 全部物品/方块名
    std::string chars;
    for (int cp = 32; cp <= 126; cp++) chars += (char)cp;
    chars += AllDisplayText();

    std::vector<int> cps;
    size_t i = 0;
    while (i < chars.size()) {
        unsigned char c = (unsigned char)chars[i];
        int size = 1, cp = c;
        if ((c & 0xE0) == 0xC0 && i + 1 < chars.size()) { size = 2; cp = ((c & 0x1F) << 6) | (((unsigned char)chars[i + 1]) & 0x3F); }
        else if ((c & 0xF0) == 0xE0 && i + 2 < chars.size()) {
            size = 3;
            cp = ((c & 0x0F) << 12) | ((((unsigned char)chars[i + 1]) & 0x3F) << 6) | (((unsigned char)chars[i + 2]) & 0x3F);
        }
        if (cp > 127) cps.push_back(cp);
        i += size;
    }
    std::sort(cps.begin(), cps.end());
    cps.erase(std::unique(cps.begin(), cps.end()), cps.end());

    for (const char* path : candidates) {
        if (!FileExists(path)) continue;
        g_font = LoadFontEx(path, 22, cps.empty() ? nullptr : cps.data(), (int)cps.size());
        if (g_font.texture.id != 0) { g_cjk = true; break; }
    }
    if (!g_cjk) {
        g_font = GetFontDefault();
        TraceLog(LOG_WARNING, "未找到中文字体, 界面将使用英文名称");
    }
    g_lineSpacing = 1.0f;
    SetTextLineSpacing(0);
}

void Shutdown() {}

bool HasCJK() { return g_cjk; }

void DrawEx(const char* s, Vector2 pos, float size, float spacing, Color c)
{
    if (!s) return;
    if (!s[0]) return;
    DrawTextEx(g_font, s, pos, size, spacing, c);
}

void Draw(const char* s, int x, int y, int size, Color c)
{
    DrawEx(s, { (float)x, (float)y }, (float)size, 1.0f, c);
}

int Width(const char* s, int size) { return s ? (int)MeasureTextEx(g_font, s, (float)size, 1.0f).x : 0; }
int Height(int size) { return size; }

void DrawCentered(const char* s, int cx, int y, int size, Color c)
{
    Draw(s, cx - Width(s, size) / 2, y, size, c);
}

void DrawRight(const char* s, int rx, int y, int size, Color c)
{
    Draw(s, rx - Width(s, size), y, size, c);
}

void DrawShadow(const char* s, int x, int y, int size, Color c)
{
    Draw(s, x + 2, y + 2, size, rgb(0, 0, 0, 190));
    Draw(s, x, y, size, c);
}

}  // namespace text

}  // namespace tc
