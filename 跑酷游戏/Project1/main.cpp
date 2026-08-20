// ============================================================
// main.cpp — 游戏主循环与入口
// 功能: 窗口管理、游戏状态控制、渲染主循环、地面绘制
// ============================================================
#include <raylib.h>
#include <string>
#include <sstream>
#include <cmath>
#include <fstream>
#include <limits>
#include "Background.h"
#include "Player.h"
#include "ObstacleManager.h"
#include "SoundManager.h"

// 游戏常量：使用 constexpr 定义编译期常量
constexpr int SCREEN_WIDTH = 1920;         // 屏幕宽度
constexpr int SCREEN_HEIGHT = 1080;        // 屏幕高度
constexpr float BASE_GAME_SPEED = 7.0f;    // 基础游戏速度
constexpr float TITLE_SPEED = 3.0f;        // 标题画面滚动速度
constexpr float PLAYER_START_X = 150.0f;   // 玩家初始X坐标
constexpr float PLAYER_START_Y = 200.0f;   // 玩家初始Y坐标
constexpr float INVINCIBLE_SPEED_MULT = 2.5f;  // 无敌状态速度倍率
constexpr int REVIVE_LIMIT = 1;            // 最大复活次数

/**
 * 从文件读取最高分
 * 健壮性处理：
 *   1. 文件不存在时返回0（不崩溃）
 *   2. 文件内容非法（非数字）时返回0
 *   3. 负分时重置为0
 */
static int LoadHighScore() {
    std::ifstream file("highscore.dat");
    int hs = 0;
    if (file.is_open()) {
        file >> hs;
        // 检查读取是否失败（如文件内容为字母）
        if (file.fail()) {
            hs = 0;
        }
        // 负数分数视为无效
        if (hs < 0) {
            hs = 0;
        }
        file.close();
    }
    return hs;
}

/**
 * 保存最高分到文件
 */
static void SaveHighScore(int score) {
    // 只保存非负分数
    if (score < 0) return;
    std::ofstream file("highscore.dat");
    if (file.is_open()) {
        file << score;
        file.close();
    }
}

/**
 * 游戏主函数
 * 功能：初始化窗口、加载资源、运行游戏主循环
 * 使用面向对象方式管理玩家、障碍物、背景等游戏对象
 */
int main() {
    const int screenWidth = SCREEN_WIDTH;
    const int screenHeight = SCREEN_HEIGHT;

    // 初始化图形窗口和音频设备
    InitWindow(screenWidth, screenHeight, "Parkour & Score");
    InitAudioDevice();
    SoundManager::Init();   // 初始化音效管理器（单例模式）
    SetTargetFPS(60);        // 锁定60帧

    // 创建游戏对象——使用自定义类
    GameBackground background(screenWidth, screenHeight);  // 背景系统
    Player player(PLAYER_START_X, PLAYER_START_Y);         // 玩家角色
    ObstacleManager obsManager;                            // 障碍物管理器

    // 游戏状态变量
    int score = 0;
    int highScore = LoadHighScore();  // 读取历史最高分
    bool gameOver = false;
    bool gameStarted = false;
    float baseGameSpeed = BASE_GAME_SPEED;

    float distance = 0.0f;            // 跑步总距离（影响速度）
    float gameOverSoundTimer = 0.0f;
    float titleAnimTimer = 0.0f;      // 标题动画计时器
    int revivesLeft = REVIVE_LIMIT;   // 剩余复活次数

    // ========== 游戏主循环 ==========
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        titleAnimTimer += dt;

        if (gameStarted) {
            // ---------- 游戏进行中 ----------
            if (!gameOver) {
                // 根据距离递增游戏速度
                float currentSpeed = baseGameSpeed + (distance / 150.0f) * 1.0f;
                distance += currentSpeed * dt;
                if (player.IsInvincible()) currentSpeed *= INVINCIBLE_SPEED_MULT;

                // 更新各游戏对象状态（体现多态特性）
                background.Update(currentSpeed);
                player.Update(screenWidth, screenHeight);

                // 检测碰撞（返回true表示游戏结束）
                if (obsManager.Update(currentSpeed, screenWidth, screenHeight, player, score)) {
                    gameOver = true;
                    gameOverSoundTimer = 0.2f;
                    if (score > highScore) {
                        highScore = score;
                        SaveHighScore(highScore);
                    }
                }
            }
            // ---------- 游戏结束状态 ----------
            else {
                // 延迟播放游戏结束音效
                if (gameOverSoundTimer > 0.0f) {
                    gameOverSoundTimer -= dt;
                    if (gameOverSoundTimer <= 0.0f) {
                        SoundManager::PlayGameOver();
                    }
                }

                // 复活功能：按R键可复活一次
                if (revivesLeft > 0 && IsKeyPressed(KEY_R)) {
                    revivesLeft--;
                    gameOver = false;
                    player.SetInvincible(3.0f);  // 复活后3秒无敌
                    player.Reset(PLAYER_START_X, PLAYER_START_Y);
                    obsManager.Reset();
                }

                // 重新开始：按回车键重置所有状态
                if (IsKeyPressed(KEY_ENTER)) {
                    gameOver = false;
                    score = 0;
                    distance = 0.0f;
                    revivesLeft = REVIVE_LIMIT;
                    player.Reset(PLAYER_START_X, PLAYER_START_Y);
                    obsManager.Reset();
                }
            }
        }
        // ---------- 标题画面 ----------
        else {
            background.Update(3.0f);

            if (IsKeyPressed(KEY_ENTER)) {
                gameStarted = true;
            }
        }

        // F2 截图
        if (IsKeyPressed(KEY_F2)) {
            TakeScreenshot("screenshot.png");
        }

        // ========== 开始绘制帧 ==========
        BeginDrawing();

        ClearBackground(SKYBLUE);

        // 绘制背景（含昼夜循环）
        background.Draw();

        // 计算地面滚动速度
        float speedForGround = gameStarted && !gameOver
            ? baseGameSpeed + (distance / 150.0f) * 1.0f
            : TITLE_SPEED;
        if (gameStarted && !gameOver && player.IsInvincible()) speedForGround *= INVINCIBLE_SPEED_MULT;

        float t = GetTime();
        float gt = static_cast<float>(screenHeight) - 50.0f;  // 地面高度
        float grassH = 14.0f, soilH = 36.0f, soilY = gt + grassH;  // 草地/泥土层高度

        // ========== 草地层（滚动平台） ==========
        float grassScroll = fmodf(t * speedForGround * 3.5f, 32.0f);
        for (int x = -32; x < screenWidth + 32; x += 32) {
            DrawRectangle(static_cast<int>(x + grassScroll), static_cast<int>(gt), 16, static_cast<int>(grassH),
                          { 40, 158, 48, 255 });
            DrawRectangle(static_cast<int>(x + grassScroll + 16), static_cast<int>(gt), 16, static_cast<int>(grassH),
                          { 32, 138, 40, 255 });
        }
        // 草地高光线和阴影
        DrawRectangle(0, static_cast<int>(gt), screenWidth, 2, { 55, 190, 65, 255 });
        DrawRectangle(0, static_cast<int>(gt + grassH - 1), screenWidth, 1, Fade(BLACK, 0.25f));

        // 草地-泥土交界斜边效果
        DrawRectangle(0, static_cast<int>(soilY - 2), screenWidth, 2, { 50, 175, 55, 255 });
        DrawRectangle(0, static_cast<int>(soilY), screenWidth, 1, { 55, 38, 22, 255 });

        // ========== 泥土层（视差滚动） ==========
        int layerCount = 6;
        float layerH = soilH / layerCount;
        static float stoneSeeds[6][12];
        static bool seeded = false;
        if (!seeded) {
            for (int l = 0; l < 6; l++)
                for (int s = 0; s < 12; s++)
                    stoneSeeds[l][s] = static_cast<float>(std::rand() % 1000) / 10.0f;
            seeded = true;
        }

        for (int l = 0; l < layerCount; l++) {
            float ly = soilY + l * layerH;                                 // 当前层Y坐标
            float depthFactor = 1.0f - static_cast<float>(l) / layerCount; // 深度因子（越深越小）
            float layerSpeed = speedForGround * (0.8f + 0.4f * depthFactor); // 视差速度
            float dark = 95.0f - l * 10.0f;                                // 颜色逐渐变暗

            // 绘制泥土块（交错纹理）
            float soilScroll = fmodf(t * layerSpeed, 48.0f);
            for (int x = -48; x < screenWidth + 48; x += 48) {
                DrawRectangle(static_cast<int>(x + soilScroll), static_cast<int>(ly), 24, static_cast<int>(layerH + 1),
                              { static_cast<unsigned char>(dark + 15),
                                static_cast<unsigned char>(dark - 8),
                                static_cast<unsigned char>(dark - 20), 255 });
                DrawRectangle(static_cast<int>(x + soilScroll + 24), static_cast<int>(ly), 24, static_cast<int>(layerH + 1),
                              { static_cast<unsigned char>(dark + 5),
                                static_cast<unsigned char>(dark - 18),
                                static_cast<unsigned char>(dark - 30), 255 });
            }

            // 层间分割线
            DrawRectangle(0, static_cast<int>(ly), screenWidth, 1,
                          Fade({ 40, 28, 12, 255 }, 0.25f + l * 0.05f));

            // 土层中的小石子装饰
            for (int s = 0; s < 3 + l; s++) {
                float sx = fmodf(stoneSeeds[l][s] * 10.0f + t * layerSpeed * 1.5f,
                                 static_cast<float>(screenWidth + 40)) - 20.0f;
                float sy = ly + 2 + fmodf(stoneSeeds[l][s], layerH - 4);
                float sr = 1.0f + fmodf(stoneSeeds[l][s] * 3.0f, 2.0f);
                DrawCircle(static_cast<int>(sx), static_cast<int>(sy), sr, { 70, 58, 42, 255 });
                DrawCircle(static_cast<int>(sx - 0.5f), static_cast<int>(sy - 0.5f), sr * 0.4f, { 90, 78, 62, 255 });
            }
        }

        // ========== 尘土粒子系统 ==========
        static float particles[30][3];
        static bool partInit = false;
        if (!partInit) {
            for (int i = 0; i < 30; i++) {
                particles[i][0] = static_cast<float>(std::rand() % screenWidth);
                particles[i][1] = static_cast<float>(std::rand() % 40);
                particles[i][2] = 0.5f + static_cast<float>(std::rand() % 100) / 100.0f;
            }
            partInit = true;
        }
        for (int i = 0; i < 30; i++) {
            particles[i][0] -= speedForGround * particles[i][2] * 0.3f;
            particles[i][1] -= speedForGround * particles[i][2] * 0.08f;
            if (particles[i][0] < -10) particles[i][0] = static_cast<float>(screenWidth) + 10;
            if (particles[i][1] < -20) particles[i][1] = grassH - 5 + fmodf(particles[i][2] * 10.0f, 10.0f);
            Color dustColor = { 80, 70, 50, static_cast<unsigned char>(30 + 40 * particles[i][2]) };
            DrawCircle(static_cast<int>(particles[i][0]),
                       static_cast<int>(gt - particles[i][1]),
                       particles[i][2] * 0.5f, dustColor);
        }

        // ========== 底部阴影 ==========
        DrawRectangle(0, screenHeight - 3, screenWidth, 3, Fade(BLACK, 0.4f));
        for (int i = 0; i < 12; i++) {
            float a = 0.06f * (1.0f - static_cast<float>(i) / 12.0f);
            DrawRectangle(0, screenHeight - 15 + i, screenWidth, 1, Fade(BLACK, a));
        }

        // ========== 透视深度线（3层流动效果） ==========
        float gt2 = t * speedForGround;
        float s1 = fmodf(gt2 * 3.0f, 50.0f);
        for (int i = -1; i <= screenWidth / 50 + 2; i++) {
            float x = i * 50.0f - s1;
            float dx = x * 0.2f + screenWidth * 0.4f;
            DrawLine(static_cast<int>(x), static_cast<int>(soilY),
                     static_cast<int>(dx), screenHeight,
                     Fade({ 48, 28, 12, 255 }, 0.18f));
        }
        float s2 = fmodf(gt2 * 1.8f, 65.0f);
        for (int i = -1; i <= screenWidth / 65 + 2; i++) {
            float x = i * 65.0f - s2 + 20.0f;
            float dx = x * 0.35f + screenWidth * 0.32f;
            DrawLine(static_cast<int>(x), static_cast<int>(soilY + 10),
                     static_cast<int>(dx), screenHeight,
                     Fade({ 55, 35, 18, 255 }, 0.09f));
        }
        float s3 = fmodf(gt2 * 0.6f, 90.0f);
        for (int i = -1; i <= 3; i++) {
            float bx = i * 90.0f - s3;
            float bw = screenWidth - bx * 0.3f;
            float by = screenHeight - 6.0f + fmodf(i * 1.5f, 4.0f);
            DrawLine(static_cast<int>(bx), static_cast<int>(by),
                     static_cast<int>(bx + bw), static_cast<int>(by),
                     Fade({ 35, 22, 10, 255 }, 0.07f));
        }

        // ========== 游戏内HUD绘制 ==========
        if (gameStarted) {
            player.Draw();
            obsManager.Draw();

            // 当前分数显示
            std::string scoreText = "Score: " + std::to_string(score);
            int scoreFontSize = 40;
            int scoreX = 20;
            int scoreY = 20;
            DrawText(scoreText.c_str(), scoreX + 2, scoreY + 2, scoreFontSize, Fade(BLACK, 0.6f));
            DrawText(scoreText.c_str(), scoreX, scoreY, scoreFontSize, RAYWHITE);

            // 最高分显示
            std::string hsText = "Best: " + std::to_string(highScore);
            DrawText(hsText.c_str(), scoreX + 2, scoreY + 48, 22, Fade(BLACK, 0.5f));
            DrawText(hsText.c_str(), scoreX, scoreY + 46, 22, Fade(GOLD, 0.8f));

            // 调试信息（按F1显示）
            if (IsKeyDown(KEY_F1)) {
                std::ostringstream oss;
                oss << obsManager;
                std::string debugStr = oss.str();
                DrawText(debugStr.c_str(), 20, 70, 20, Fade(YELLOW, 0.8f));
            }
        }

        // ========== 标题画面 ==========
        if (!gameStarted) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));

            // 标题动画（呼吸效果）
            float titleScale = 1.0f + 0.05f * std::sin(titleAnimTimer * 2.0f);
            int titleSize = static_cast<int>(80 * titleScale);
            const char* title = "PARKOUR & SCORE";
            DrawText(title, screenWidth / 2 - MeasureText(title, titleSize) / 2, screenHeight / 2 - 150, titleSize, RAYWHITE);

            DrawText("Press ENTER to Start", screenWidth / 2 - MeasureText("Press ENTER to Start", 30) / 2, screenHeight / 2 + 50, 30, Fade(RAYWHITE, 0.6f + 0.4f * std::sin(titleAnimTimer * 3.0f)));

            // 操作说明
            DrawText("W / UP / SPACE  -  Jump (Double Jump)", screenWidth / 2 - MeasureText("W / UP / SPACE  -  Jump (Double Jump)", 22) / 2, screenHeight / 2 + 140, 22, LIGHTGRAY);
            DrawText("S / DOWN        -  Slide", screenWidth / 2 - MeasureText("S / DOWN        -  Slide", 22) / 2, screenHeight / 2 + 175, 22, LIGHTGRAY);

            DrawText("Collect coins for bonus score!", screenWidth / 2 - MeasureText("Collect coins for bonus score!", 20) / 2, screenHeight / 2 + 270, 20, GOLD);
        }

        // ========== 游戏结束画面 ==========
        if (gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.4f));
            DrawText("GAME OVER", screenWidth / 2 - MeasureText("GAME OVER", 60) / 2, screenHeight / 2 - 40, 60, RED);

            int yOff = 30;
            // 复活提示（仅当还有复活次数时）
            if (revivesLeft > 0) {
                float pulse = 0.6f + 0.4f * std::sin(titleAnimTimer * 4.0f);
                DrawText("Press R to Revive", screenWidth / 2 - MeasureText("Press R to Revive", 30) / 2, screenHeight / 2 + yOff, 30, Fade(GOLD, pulse));
                yOff += 55;
            }
            DrawText("Press ENTER to Restart", screenWidth / 2 - MeasureText("Press ENTER to Restart", 30) / 2, screenHeight / 2 + yOff, 30, RAYWHITE);

            // 最终得分
            std::string finalScoreText = "Final Score: " + std::to_string(score);
            DrawText(finalScoreText.c_str(), screenWidth / 2 - MeasureText(finalScoreText.c_str(), 30) / 2, screenHeight / 2 + yOff + 50, 30, RAYWHITE);

            // 历史最高记录
            if (highScore > 0) {
                std::string bestText = "Best: " + std::to_string(highScore);
                DrawText(bestText.c_str(), screenWidth / 2 - MeasureText(bestText.c_str(), 22) / 2, screenHeight / 2 + yOff + 85, 22, Fade(GOLD, 0.7f));
            }
            // 新纪录提示
            if (score > 0 && score >= highScore) {
                DrawText("NEW BEST!", screenWidth / 2 - MeasureText("NEW BEST!", 28) / 2, screenHeight / 2 + yOff + 110, 28, Fade(RED, 0.6f + 0.4f * std::sin(titleAnimTimer * 5.0f)));
            }
        }

        EndDrawing();
    }

    // ========== 清理资源 ==========
    SoundManager::Shutdown();  // 释放音效管理器
    CloseAudioDevice();        // 关闭音频设备
    CloseWindow();             // 关闭窗口
    return 0;
}
