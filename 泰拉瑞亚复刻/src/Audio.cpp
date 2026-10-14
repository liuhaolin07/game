// Audio.cpp —— 音效系统实现
#include "Audio.h"

#include "Noise.h"

namespace tc {
namespace audio {

namespace {

struct Slot {
    Sound snd{};
    bool ok = false;
    double last = -10.0;
};

Slot g_slots[SFX_COUNT];
bool g_ready = false;
bool g_enabled = true;
float g_master = 0.75f;

const char* const kNames[SFX_COUNT] = {
    "dig_dirt", "dig_wood", "dig_stone", "break", "place",
    "jump", "land", "hurt", "pickup", "craft",
    "open", "close", "hit_enemy", "enemy_die", "coin",
    "boss", "menu", "splash", "step", "swing", "levelup"
};

}  // namespace

bool init()
{
    InitAudioDevice();
    g_ready = IsAudioDeviceReady();
    if (!g_ready) {
        TraceLog(LOG_WARNING, "没有可用的音频设备, 游戏将以静音模式运行");
        return false;
    }
    int loaded = 0;
    for (int i = 0; i < SFX_COUNT; i++) {
        char path[128];
        snprintf(path, sizeof(path), "assets/sfx/%s.wav", kNames[i]);
        if (!FileExists(path)) continue;
        g_slots[i].snd = LoadSound(path);
        g_slots[i].ok = true;
        loaded++;
    }
    SetMasterVolume(g_master);
    TraceLog(LOG_INFO, "已载入音效 %d / %d", loaded, (int)SFX_COUNT);
    return loaded > 0;
}

void shutdown()
{
    if (!g_ready) return;
    for (int i = 0; i < SFX_COUNT; i++) {
        if (!g_slots[i].ok) continue;
        UnloadSound(g_slots[i].snd);
        g_slots[i].ok = false;
    }
    CloseAudioDevice();
    g_ready = false;
}

bool ready() { return g_ready; }
bool enabled() { return g_enabled; }
void setEnabled(bool on) { g_enabled = on; }
void setMasterVolume(float v) { g_master = clampf(v, 0.0f, 1.0f); if (g_ready) SetMasterVolume(g_master); }
float masterVolume() { return g_master; }

void play(Sfx s, float volume, float pitch)
{
    if (!g_ready || !g_enabled || s < 0 || s >= SFX_COUNT) return;
    if (!g_slots[s].ok) return;
    if (pitch < 0.05f) pitch = 0.05f;
    if (pitch > 4.0f) pitch = 4.0f;
    SetSoundVolume(g_slots[s].snd, clampf(volume, 0.0f, 1.0f));
    SetSoundPitch(g_slots[s].snd, pitch);
    PlaySound(g_slots[s].snd, { 0.0f, 0.0f });
}

void playThrottled(Sfx s, float minInterval, float volume, float pitch)
{
    if (!g_ready || !g_enabled) return;
    const double now = (double)GetTime();
    if (now - g_slots[s].last < minInterval) return;
    g_slots[s].last = now;
    play(s, volume, pitch);
}

}  // namespace audio
}  // namespace tc
