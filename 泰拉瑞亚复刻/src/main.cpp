// main.cpp —— 入口
#include "Game.h"

using namespace tc;

int main()
{
    Game game;
    if (!game.init()) {
        TraceLog(LOG_FATAL, "初始化失败, 请确认 assets/ 目录存在且包含 tiles.png / items.png / entities.png / gui.png / atlas.txt");
        return 1;
    }

    if (!game.loadWorld(nullptr)) game.newWorld(0);

    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        if (dt > 0.1f) continue;      // 防止卡顿后物理爆炸
        game.update(dt);
        game.draw();
    }

    game.saveWorld(nullptr);
    game.shutdown();
    return 0;
}
