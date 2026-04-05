#pragma once

#include <array>
#include <random>
#include <string>
#include <vector>

enum class Command
{
    None,
    MoveLeft,
    MoveRight,
    RotateCW,
    SoftDrop,
    HardDrop,
    Hold
};

struct PieceState
{
    int type = 0;      // [0,6]
    int rotation = 0;  // [0,3]
    int x = 3;
    int y = 0;
};

class Board
{
public:
    Board(int width = 14, int height = 20);

    int Width() const;
    int Height() const;

    bool IsInside(int x, int y) const;
    int Cell(int x, int y) const;
    void SetCell(int x, int y, int value);
    int ClearFullLines();

    const std::vector<std::vector<int>>& Data() const;

private:
    int m_width;
    int m_height;
    std::vector<std::vector<int>> m_cells;
};

class Game
{
public:
    Game();

    bool IsGameOver() const;
    int Score() const;
    int Level() const;
    int TotalLines() const;
    int NextType() const;
    int HoldType() const;
    int Combo() const;

    const Board& GetBoard() const;
    const PieceState& CurrentPiece() const;

    void Reset();
    void Step(Command command);
    void Tick();

    std::string RenderText() const;

private:
    void InitPieceTemplates();
    int RotateIndex(int x, int y, int rotation) const;
    bool PieceFilled(int type, int rotation, int x, int y) const;

    bool CanPlace(const PieceState& piece) const;
    int CalcGhostY() const;
    void RefillBagIfNeeded();
    int DrawFromBag();
    void SpawnPiece();
    void LockPiece();
    void UpdateScore(int linesCleared, int softDropCells, int hardDropCells);

private:
    Board m_board;
    std::array<std::string, 7> m_pieceTemplates;
    PieceState m_current;
    bool m_gameOver;
    int m_score;
    int m_level;
    int m_totalLines;
    int m_combo;

    int m_nextType;
    int m_holdType;
    bool m_holdUsedThisTurn;
    std::vector<int> m_bag;
    std::mt19937 m_rng;
};