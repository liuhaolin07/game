#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

static constexpr int VW = 1280;
static constexpr int VH = 720;
static constexpr float GROUND = 590.0f;
static constexpr float PLAYER_H = 116.0f;

struct Obstacle {
    int type;
    float x, y, w, h;
};

struct Coin {
    float x, y;
    float radius;
    bool taken;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    Color color;
};

static Texture2D playerTex{};
static Texture2D slideTex{};
static std::vector<Texture2D> obstacleTex;
static std::vector<Obstacle> obstacles;
static std::vector<Coin> coins;
static std::vector<Particle> particles;

static bool started = false;
static bool gameOver = false;
static bool paused = false;
static bool sliding = false;
static int jumps = 0;
static int score = 0;
static int coinCount = 0;
static int combo = 0;
static float px = 190.0f;
static float py = GROUND - PLAYER_H;
static float pvy = 0;
static float speed = 360.0f;
static float spawnTimer = 1.8f;
static float coinTimer = 1.0f;
static float safeTimer = 2.5f;
static float worldTime = 0;
static float shake = 0;
static float runCycle = 0;
static float squash = 0;
static float slideSparkTimer = 0;

static Color FadeCol(Color c, float alpha) {
    c.a = (unsigned char)(c.a * std::clamp(alpha, 0.0f, 1.0f));
    return c;
}

static void Emit(float x, float y, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = {x + (float)(rand() % 36 - 18), y + (float)(rand() % 12 - 6)};
        p.vel = {-60.0f - (float)(rand() % 230), -40.0f - (float)(rand() % 190)};
        p.life = p.maxLife = 0.35f + (rand() % 45) / 100.0f;
        p.size = 4.0f + (float)(rand() % 10);
        p.color = color;
        particles.push_back(p);
    }
}

static void Reset() {
    started = true;
    gameOver = false;
    paused = false;
    sliding = false;
    jumps = 0;
    score = 0;
    coinCount = 0;
    combo = 0;
    px = 190.0f;
    py = GROUND - PLAYER_H;
    pvy = 0;
    speed = 360.0f;
    spawnTimer = 1.7f;
    coinTimer = 0.8f;
    safeTimer = 2.6f;
    shake = 0;
    runCycle = 0;
    squash = 0;
    slideSparkTimer = 0;
    obstacles.clear();
    coins.clear();
    particles.clear();
}

static Rectangle PlayerHitbox() {
    return sliding ? Rectangle{px + 18, GROUND - 54, 112, 48}
                   : Rectangle{px + 30, py + 16, 72, 92};
}

static bool Intersects(Rectangle a, Rectangle b) {
    return a.x < b.x + b.width && a.x + a.width > b.x &&
           a.y < b.y + b.height && a.y + a.height > b.y;
}

static void TryJump() {
    if (!started || gameOver || paused || sliding || jumps >= 2) return;
    pvy = jumps == 0 ? -720.0f : -650.0f;
    ++jumps;
    shake = std::max(shake, jumps == 1 ? 2.0f : 4.0f);
    Emit(px + 66, GROUND - 5, jumps == 1 ? 8 : 15, {190, 235, 255, 220});
}

static void Spawn() {
    int type = rand() % (int)obstacleTex.size();
    float w[] = {108, 104, 112, 98, 120, 105, 190, 145};
    float h[] = {88, 110, 91, 128, 58, 105, 33, 53};
    Obstacle o{type, VW + 70.0f, GROUND - h[type], w[type], h[type]};
    if (type == 4 || type == 5) o.y -= 105.0f;
    if (type == 6) o.y -= 48.0f;
    obstacles.push_back(o);
}

static void SpawnCoins() {
    float baseYOptions[] = {GROUND - 78.0f, GROUND - 145.0f, GROUND - 220.0f};
    float y = baseYOptions[rand() % 3];
    float startX = VW + 85.0f;
    int count = 4 + rand() % 4;
    for (int i = 0; i < count; ++i) {
        float wave = sinf((float)i * 0.9f) * 22.0f;
        coins.push_back({startX + i * 52.0f, y + wave, 17.0f, false});
    }
}

static void Update(float dt) {
    worldTime += dt;
    shake = std::max(0.0f, shake - 12.0f * dt);

    for (auto& p : particles) {
        p.life -= dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.y += 450.0f * dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.life <= 0; }), particles.end());

    if (!started || gameOver || paused) return;

    bool grounded = py >= GROUND - PLAYER_H - 0.5f;
    sliding = (IsKeyDown('S') || IsKeyDown(KEY_DOWN)) && grounded;
    runCycle += dt * (grounded && !sliding ? 8.0f + speed / 80.0f : 3.2f);
    squash = std::max(0.0f, squash - dt * 5.8f);
    slideSparkTimer -= dt;
    if (sliding && slideSparkTimer <= 0.0f) {
        slideSparkTimer = 0.07f;
        Emit(px + 20, GROUND - 10, 3, {130, 245, 255, 190});
    }

    float move = 0;
    if (IsKeyDown('A') || IsKeyDown(KEY_LEFT)) move -= 1;
    if (IsKeyDown('D') || IsKeyDown(KEY_RIGHT)) move += 1;
    px = std::clamp(px + move * 315.0f * dt, 90.0f, 470.0f);

    pvy += 1750.0f * dt;
    py += pvy * dt;
    if (py >= GROUND - PLAYER_H) {
        if (!grounded && pvy > 240.0f) {
            shake = 7.0f;
            squash = 1.0f;
            Emit(px + 66, GROUND - 4, 20, {205, 235, 255, 220});
        }
        py = GROUND - PLAYER_H;
        pvy = 0;
        jumps = 0;
    }

    safeTimer = std::max(0.0f, safeTimer - dt);
    float speedGain = 10.0f + std::min(18.0f, worldTime * 0.08f);
    speed = std::min(860.0f, speed + speedGain * dt);
    score += (int)(dt * (110.0f + speed * 0.08f));
    spawnTimer -= dt;
    if (spawnTimer <= 0) {
        Spawn();
        spawnTimer = std::max(0.68f, 1.62f - (speed - 360.0f) / 520.0f) + (rand() % 42) / 100.0f;
    }
    coinTimer -= dt;
    if (coinTimer <= 0) {
        SpawnCoins();
        coinTimer = std::max(0.72f, 1.18f - (speed - 360.0f) / 900.0f) + (rand() % 70) / 100.0f;
    }
    for (auto& o : obstacles) o.x -= speed * dt;
    obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
        [](const Obstacle& o) { return o.x + o.w < -80; }), obstacles.end());
    for (auto& c : coins) c.x -= speed * dt;
    coins.erase(std::remove_if(coins.begin(), coins.end(),
        [](const Coin& c) {
            if (!c.taken && c.x + c.radius < -80) combo = 0;
            return c.taken || c.x + c.radius < -80;
        }), coins.end());

    Rectangle ph = PlayerHitbox();
    for (auto& c : coins) {
        Rectangle coinBox{c.x - c.radius, c.y - c.radius, c.radius * 2.0f, c.radius * 2.0f};
        if (!c.taken && Intersects(ph, coinBox)) {
            c.taken = true;
            ++coinCount;
            combo = std::min(99, combo + 1);
            score += 120 + combo * 8;
            shake = std::max(shake, 2.5f);
            Emit(c.x, c.y, 18, {255, 222, 74, 245});
        }
    }
    for (const auto& o : obstacles) {
        Rectangle oh{o.x + o.w * .16f, o.y + o.h * .15f, o.w * .68f, o.h * .76f};
        if (safeTimer <= 0 && Intersects(ph, oh)) {
            gameOver = true;
            combo = 0;
            shake = 13;
            Emit(px + 70, py + 70, 32, {255, 165, 80, 240});
            break;
        }
    }
}

static void DrawCloud(float x, float y, float scale, Color color) {
    DrawEllipse((int)x, (int)(y + 25 * scale), 48 * scale, 28 * scale, color);
    DrawCircle((int)(x + 42 * scale), (int)y, 43 * scale, color);
    DrawEllipse((int)(x + 85 * scale), (int)(y + 25 * scale), 46 * scale, 27 * scale, color);
}

static void CenterText(const std::string& text, int y, int size, Color color) {
    DrawText(text.c_str(), (VW - MeasureText(text.c_str(), size)) / 2 + 3, y + 4, size, FadeCol(BLACK, .55f));
    DrawText(text.c_str(), (VW - MeasureText(text.c_str(), size)) / 2, y, size, color);
}

static void DrawWorld() {
    DrawRectangleGradientV(0, 0, VW, VH, {20, 86, 184, 255}, {105, 204, 239, 255});
    DrawCircleGradient(170, 130, 96, {255, 248, 175, 255}, {255, 248, 175, 0});
    DrawCircle(170, 130, 58, {255, 247, 185, 255});

    float farOff = fmodf(worldTime * speed * .035f, 360.0f);
    for (int i = -1; i < 5; ++i) {
        float x = i * 360.0f - farOff;
        DrawTriangle({x, GROUND}, {x + 180, 280}, {x + 360, GROUND}, {56, 132, 174, 255});
    }
    float nearOff = fmodf(worldTime * speed * .09f, 500.0f);
    for (int i = -1; i < 4; ++i) {
        float x = i * 500.0f - nearOff;
        DrawTriangle({x, GROUND}, {x + 250, 360}, {x + 500, GROUND}, {30, 93, 116, 255});
    }
    for (int i = 0; i < 6; ++i) {
        float x = fmodf(i * 280.0f - worldTime * (25 + i * 4) + 1600.0f, 1600.0f) - 140;
        DrawCloud(x, 75.0f + (i % 3) * 70.0f, .8f + (i % 2) * .2f, {245, 250, 255, 220});
    }

    DrawRectangleGradientV(0, (int)GROUND, VW, VH - (int)GROUND, {28, 83, 72, 255}, {4, 24, 34, 255});
    DrawRectangle(0, (int)GROUND, VW, 5, {110, 235, 178, 255});
    float roadOff = fmodf(worldTime * speed, 180.0f);
    for (int i = -1; i < 9; ++i) {
        float x = i * 180.0f - roadOff;
        DrawLineEx({640 + (x - 640) * .22f, GROUND + 7}, {x, (float)VH}, 2, {145, 250, 215, 70});
    }
    for (int i = 0; i < 4; ++i) {
        float y = GROUND + 22 + i * i * 12.0f;
        DrawLineEx({0, y}, {(float)VW, y}, 2, {145, 250, 215, 65});
    }
    if (started && !paused && !gameOver && speed > 430) {
        int streakCount = 12 + (int)((speed - 430.0f) / 26.0f);
        for (int i = 0; i < streakCount; ++i) {
            float y = 90.0f + (i * 43 % 410);
            float x = fmodf(i * 137.0f - worldTime * speed * 2.4f + 1800.0f, 1500.0f);
            DrawLineEx({x, y}, {x + 55 + speed * 0.08f, y}, 2, {225, 250, 255, (unsigned char)std::min(120.0f, 45.0f + speed * 0.05f)});
        }
    }

    for (const auto& c : coins) {
        float pulse = 1.0f + sinf(worldTime * 10.0f + c.x * 0.03f) * 0.12f;
        DrawCircle((int)(c.x + 4), (int)(c.y + 5), c.radius * pulse, {140, 85, 0, 90});
        DrawCircleGradient((int)c.x, (int)c.y, c.radius * 1.55f * pulse, {255, 240, 120, 135}, {255, 205, 55, 0});
        DrawCircle((int)c.x, (int)c.y, c.radius * pulse, {255, 205, 45, 255});
        DrawCircle((int)(c.x - 4), (int)(c.y - 5), c.radius * 0.42f * pulse, {255, 252, 190, 235});
        DrawText("$", (int)(c.x - 6), (int)(c.y - 13), 23, {132, 90, 0, 220});
    }

    for (const auto& o : obstacles) {
        DrawEllipse((int)(o.x + o.w / 2), (int)(GROUND - 4), o.w * .4f, 12, {0, 0, 0, 80});
        DrawTexturePro(obstacleTex[o.type], {0, 0, (float)obstacleTex[o.type].width, (float)obstacleTex[o.type].height},
            {o.x, o.y, o.w, o.h}, {0, 0}, 0, WHITE);
    }
    for (const auto& p : particles) {
        DrawCircle((int)p.pos.x, (int)p.pos.y, p.size, FadeCol(p.color, p.life / p.maxLife));
    }

    float air = std::max(0.0f, GROUND - PLAYER_H - py);
    float ss = std::clamp(1.0f - air / 440.0f, .35f, 1.0f);
    DrawEllipse((int)(px + 75), (int)(GROUND - 5), 58 * ss, 12 * ss, {0, 0, 0, (unsigned char)(115 * ss)});

    Texture2D tex = sliding ? slideTex : playerTex;
    float w = sliding ? 154.0f : 132.0f;
    float h = sliding ? 91.0f : 132.0f;
    float y = sliding ? GROUND - h + 5 : py - 12;
    float angle = std::clamp(pvy / 55.0f, -11.0f, 13.0f);
    Rectangle src{0, 0, (float)tex.width, (float)tex.height};
    Rectangle dst{px + w / 2, y + h / 2, w, h};
    Vector2 origin{w / 2, h / 2};
    DrawTexturePro(tex, src, {dst.x + 8, dst.y + 10, w, h}, origin, angle, {35, 65, 90, 105});
    DrawTexturePro(tex, src, dst, origin, angle, WHITE);

    DrawText(("Score: " + std::to_string(score)).c_str(), 32, 25, 38, WHITE);
    DrawText(("FPS " + std::to_string(GetFPS())).c_str(), VW - 105, 25, 22, {180, 255, 205, 255});
    DrawText(("Coins: " + std::to_string(coinCount)).c_str(), 32, 72, 24, {255, 226, 75, 255});
    DrawText(("Speed: " + std::to_string((int)speed)).c_str(), 32, 103, 22, {205, 245, 255, 255});
    DrawRectangle(32, 132, 220, 10, {8, 36, 56, 190});
    DrawRectangle(32, 132, (int)(220.0f * std::clamp((speed - 360.0f) / 500.0f, 0.0f, 1.0f)), 10, {110, 245, 178, 240});
    if (combo > 1 && started && !gameOver) DrawText(("Combo x" + std::to_string(combo)).c_str(), 32, 150, 22, {255, 245, 130, 255});
    if (started && !gameOver) DrawText(("Jumps " + std::to_string(2 - jumps) + "/2").c_str(), 34, combo > 1 ? 178 : 150, 22, {205, 245, 255, 255});

    if (!started) {
        DrawRectangle(0, 0, VW, VH, {2, 18, 42, 150});
        CenterText("PARKOUR DIMENSION", 190, 62, WHITE);
        CenterText("ENTER START     W / UP / SPACE: DOUBLE JUMP     S / DOWN: SLIDE", 285, 24, {205, 245, 255, 255});
        CenterText("COLLECT COINS, BUILD COMBO, SURVIVE AS SPEED RISES", 335, 23, {255, 226, 75, 255});
        CenterText("A / D: MOVE     P: PAUSE     F11: FULLSCREEN", 385, 23, {170, 230, 255, 255});
        CenterText("HARDWARE ACCELERATED / VSYNC", 440, 20, {130, 255, 195, 255});
    }
    if (paused && !gameOver) {
        DrawRectangle(0, 0, VW, VH, {0, 0, 0, 150});
        CenterText("PAUSED", 270, 66, WHITE);
        CenterText("Press P to continue", 365, 25, {205, 245, 255, 255});
    }
    if (gameOver) {
        DrawRectangle(0, 0, VW, VH, {0, 0, 0, 135});
        CenterText("GAME OVER", 235, 72, {255, 72, 92, 255});
        CenterText("Final Score: " + std::to_string(score) + "    Coins: " + std::to_string(coinCount), 345, 32, WHITE);
        CenterText("Press ENTER to Restart", 405, 25, {205, 245, 255, 255});
    }
}

int main() {
    srand((unsigned)time(nullptr));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Parkour Dimension - GPU Smooth Edition");
    SetTargetFPS(60);

    playerTex = LoadTexture("player.png");
    slideTex = LoadTexture("slide.png");
    const char* names[] = {"box.png", "twobox.png", "stonewall.png", "highstonewall.png", "bird.png", "hammer.png", "laser.png", "spine.png"};
    for (auto name : names) obstacleTex.push_back(LoadTexture(name));

    RenderTexture2D target = LoadRenderTexture(VW, VH);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    float accumulator = 0;
    const float fixedDt = 1.0f / 120.0f;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ENTER)) Reset();
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed('W')) TryJump();
        if (IsKeyPressed('P') && started && !gameOver) paused = !paused;
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        accumulator += std::min(GetFrameTime(), 0.05f);
        while (accumulator >= fixedDt) {
            Update(fixedDt);
            accumulator -= fixedDt;
        }

        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawWorld();
        EndTextureMode();

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        float scale = std::min(sw / (float)VW, sh / (float)VH);
        float dw = VW * scale;
        float dh = VH * scale;
        float dx = (sw - dw) / 2;
        float dy = (sh - dh) / 2;
        float sx = shake > 0 ? sinf(worldTime * 90.0f) * shake : 0;
        float sy = shake > 0 ? cosf(worldTime * 76.0f) * shake * .45f : 0;

        BeginDrawing();
        ClearBackground({3, 12, 24, 255});
        DrawTexturePro(target.texture, {0, 0, (float)VW, -(float)VH},
            {dx + sx, dy + sy, dw, dh}, {0, 0}, 0, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    UnloadTexture(playerTex);
    UnloadTexture(slideTex);
    for (auto tex : obstacleTex) UnloadTexture(tex);
    CloseWindow();
    return 0;
}
