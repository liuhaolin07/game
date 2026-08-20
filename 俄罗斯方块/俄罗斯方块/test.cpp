#include "game.h"

#include <conio.h>
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct RankEntry
{
    std::string name;
    int score;
};

void SetupConsoleUi()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        return;
    }

    // 恢复原来字体
    CONSOLE_FONT_INFOEX fontInfo = {};
    fontInfo.cbSize = sizeof(fontInfo);
    fontInfo.dwFontSize.X = 12;
    fontInfo.dwFontSize.Y = 24;
    fontInfo.FontFamily = FF_DONTCARE;
    fontInfo.FontWeight = FW_NORMAL;
    wcscpy_s(fontInfo.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);

    // 恢复原来缓冲区和窗口
    COORD bufferSize = { 140, 80 };
    SetConsoleScreenBufferSize(hOut, bufferSize);

    SMALL_RECT windowRect = { 0, 0, 119, 49 };
    SetConsoleWindowInfo(hOut, TRUE, &windowRect);
}

std::vector<RankEntry> LoadLeaderboard()
{
    std::vector<RankEntry> ranks;
    std::ifstream fin("leaderboard.txt");
    RankEntry e;

    while (fin >> e.name >> e.score)
    {
        ranks.push_back(e);
    }

    std::sort(ranks.begin(), ranks.end(), [](const RankEntry& a, const RankEntry& b)
    {
        return a.score > b.score;
    });

    if (ranks.size() > 10)
    {
        ranks.resize(10);
    }

    return ranks;
}

void SaveLeaderboard(const std::vector<RankEntry>& ranks)
{
    std::ofstream fout("leaderboard.txt", std::ios::trunc);
    for (size_t i = 0; i < ranks.size() && i < 10; ++i)
    {
        fout << ranks[i].name << " " << ranks[i].score << "\n";
    }
}

void UpdateLeaderboard(std::vector<RankEntry>& ranks, const std::string& name, int score)
{
    ranks.push_back(RankEntry{ name, score });

    std::sort(ranks.begin(), ranks.end(), [](const RankEntry& a, const RankEntry& b)
    {
        return a.score > b.score;
    });

    if (ranks.size() > 10)
    {
        ranks.resize(10);
    }
}

void PrintLeaderboard(const std::vector<RankEntry>& ranks)
{
    std::cout << "===== 排行榜 Top 10 =====\n";
    if (ranks.empty())
    {
        std::cout << "暂无记录\n";
    }
    else
    {
        for (size_t i = 0; i < ranks.size(); ++i)
        {
            std::cout << (i + 1) << ". " << ranks[i].name << " - " << ranks[i].score << "\n";
        }
    }
    std::cout << "=========================\n";
}

bool WaitForPlayerStart(const std::vector<RankEntry>& ranks)
{
    while (true)
    {
        std::system("cls");
        std::cout << "==============================\n";
        std::cout << "            TETRIS\n";
        std::cout << "==============================\n\n";
        PrintLeaderboard(ranks);
        std::cout << "\n1 / Enter / S : 开始游戏\n";
        std::cout << "Q / Esc       : 退出\n\n";
        std::cout << "请选择：";

        int ch = _getch();
        if (ch == '1' || ch == 's' || ch == 'S' || ch == 13)
        {
            return true;
        }
        if (ch == 'q' || ch == 'Q' || ch == 27)
        {
            return false;
        }
    }
}

int main()
{
    SetupConsoleUi();

    std::vector<RankEntry> ranks = LoadLeaderboard();

    std::system("cls");
    std::string playerName;
    std::cout << "请输入玩家名（无空格）: ";
    std::getline(std::cin, playerName);
    if (playerName.empty())
    {
        playerName = "Player";
    }

    if (!WaitForPlayerStart(ranks))
    {
        return 0;
    }

    Game game;
    bool paused = false;

    auto lastTick = std::chrono::steady_clock::now();

    while (!game.IsGameOver())
    {
        while (_kbhit())
        {
            int ch = _getch();
            if (ch == 0 || ch == 224)
            {
                if (_kbhit())
                {
                    (void)_getch();
                }
                continue;
            }

            if (ch == 'q' || ch == 'Q')
            {
                return 0;
            }
            if (ch == 'p' || ch == 'P')
            {
                paused = !paused;
                continue;
            }
            if (paused)
            {
                continue;
            }

            if (ch == 'a' || ch == 'A') game.Step(Command::MoveLeft);
            else if (ch == 'd' || ch == 'D') game.Step(Command::MoveRight);
            else if (ch == 'w' || ch == 'W') game.Step(Command::RotateCW);
            else if (ch == 's' || ch == 'S') game.Step(Command::SoftDrop);
            else if (ch == 'c' || ch == 'C') game.Step(Command::Hold);
            else if (ch == ' ') game.Step(Command::HardDrop);
        }

        auto now = std::chrono::steady_clock::now();
        const int baseMs = 550;
        const int speedMs = (std::max)(80, baseMs - (game.Level() - 1) * 35);
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count();

        if (!paused && elapsedMs >= speedMs)
        {
            game.Tick();
            lastTick = now;
        }

        std::system("cls");
        if (paused)
        {
            std::cout << "[PAUSED]\n";
        }
        std::cout << game.RenderText();

        Sleep(16);
    }

    UpdateLeaderboard(ranks, playerName, game.Score());
    SaveLeaderboard(ranks);

    std::system("cls");
    std::cout << game.RenderText();
    std::cout << "\nGame Over! Final Score: " << game.Score() << "\n\n";
    PrintLeaderboard(ranks);
    std::cout << "\n按任意键退出...";
    (void)_getch();

    return 0;
}

