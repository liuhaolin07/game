#include "game.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <sstream>

Board::Board(int width, int height)
    : m_width(width),
      m_height(height),
      m_cells(height, std::vector<int>(width, 0))
{
}

int Board::Width() const
{
    return m_width;
}

int Board::Height() const
{
    return m_height;
}

bool Board::IsInside(int x, int y) const
{
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

int Board::Cell(int x, int y) const
{
    return m_cells[y][x];
}

void Board::SetCell(int x, int y, int value)
{
    m_cells[y][x] = value;
}

int Board::ClearFullLines()
{
    int cleared = 0;
    for (int y = m_height - 1; y >= 0; --y)
    {
        bool full = true;
        for (int x = 0; x < m_width; ++x)
        {
            if (m_cells[y][x] == 0)
            {
                full = false;
                break;
            }
        }

        if (full)
        {
            ++cleared;
            for (int row = y; row > 0; --row)
            {
                m_cells[row] = m_cells[row - 1];
            }
            std::fill(m_cells[0].begin(), m_cells[0].end(), 0);
            ++y;
        }
    }
    return cleared;
}

Game::Game()
    : m_board(14, 20),
      m_gameOver(false),
      m_score(0),
      m_level(1),
      m_totalLines(0),
      m_combo(0),
      m_nextType(0),
      m_holdType(-1),
      m_holdUsedThisTurn(false),
      m_rng(static_cast<unsigned int>(
          std::chrono::steady_clock::now().time_since_epoch().count()))
{
    InitPieceTemplates();
    Reset();
}

bool Game::IsGameOver() const
{
    return m_gameOver;
}

int Game::Score() const
{
    return m_score;
}

int Game::Level() const
{
    return m_level;
}

int Game::TotalLines() const
{
    return m_totalLines;
}

int Game::NextType() const
{
    return m_nextType;
}

int Game::HoldType() const
{
    return m_holdType;
}

int Game::Combo() const
{
    return m_combo;
}

void Game::Reset()
{
    m_board = Board(14, 20);
    m_score = 0;
    m_level = 1;
    m_totalLines = 0;
    m_combo = 0;
    m_gameOver = false;
    m_holdType = -1;
    m_holdUsedThisTurn = false;
    m_bag.clear();

    m_nextType = DrawFromBag();
    SpawnPiece();
}

void Game::InitPieceTemplates()
{
    m_pieceTemplates[0] = "..X...X...X...X."; // I
    m_pieceTemplates[1] = "..X..XX...X....."; // T
    m_pieceTemplates[2] = ".....XX..XX....."; // O
    m_pieceTemplates[3] = "..X..XX..X......"; // S
    m_pieceTemplates[4] = ".X...XX...X....."; // Z
    m_pieceTemplates[5] = ".X...X...XX....."; // J
    m_pieceTemplates[6] = "..X...X..XX....."; // L
}

int Game::RotateIndex(int x, int y, int rotation) const
{
    switch (rotation % 4)
    {
    case 0: return y * 4 + x;
    case 1: return 12 + y - x * 4;
    case 2: return 15 - y * 4 - x;
    case 3: return 3 - y + x * 4;
    default: return y * 4 + x;
    }
}

bool Game::PieceFilled(int type, int rotation, int x, int y) const
{
    return m_pieceTemplates[type][RotateIndex(x, y, rotation)] == 'X';
}

bool Game::CanPlace(const PieceState& piece) const
{
    for (int py = 0; py < 4; ++py)
    {
        for (int px = 0; px < 4; ++px)
        {
            if (!PieceFilled(piece.type, piece.rotation, px, py))
            {
                continue;
            }

            const int bx = piece.x + px;
            const int by = piece.y + py;
            if (!m_board.IsInside(bx, by))
            {
                return false;
            }
            if (m_board.Cell(bx, by) != 0)
            {
                return false;
            }
        }
    }
    return true;
}

int Game::CalcGhostY() const
{
    PieceState ghost = m_current;
    while (CanPlace(PieceState{ ghost.type, ghost.rotation, ghost.x, ghost.y + 1 }))
    {
        ++ghost.y;
    }
    return ghost.y;
}

void Game::RefillBagIfNeeded()
{
    if (!m_bag.empty())
    {
        return;
    }

    m_bag.resize(7);
    std::iota(m_bag.begin(), m_bag.end(), 0);
    std::shuffle(m_bag.begin(), m_bag.end(), m_rng);
}

int Game::DrawFromBag()
{
    RefillBagIfNeeded();
    int value = m_bag.back();
    m_bag.pop_back();
    return value;
}

void Game::SpawnPiece()
{
    m_current.type = m_nextType;
    m_current.rotation = 0;
    m_current.x = m_board.Width() / 2 - 2;
    m_current.y = 0;

    m_nextType = DrawFromBag();
    m_holdUsedThisTurn = false;

    if (!CanPlace(m_current))
    {
        m_gameOver = true;
    }
}

void Game::LockPiece()
{
    for (int py = 0; py < 4; ++py)
    {
        for (int px = 0; px < 4; ++px)
        {
            if (!PieceFilled(m_current.type, m_current.rotation, px, py))
            {
                continue;
            }

            const int bx = m_current.x + px;
            const int by = m_current.y + py;
            if (m_board.IsInside(bx, by))
            {
                m_board.SetCell(bx, by, m_current.type + 1);
            }
        }
    }

    const int lines = m_board.ClearFullLines();
    UpdateScore(lines, 0, 0);
    SpawnPiece();
}

void Game::UpdateScore(int linesCleared, int softDropCells, int hardDropCells)
{
    static const int lineScore[5] = { 0, 100, 300, 500, 800 };
    const int clamped = std::max(0, std::min(linesCleared, 4));

    m_score += softDropCells * 1;
    m_score += hardDropCells * 2;
    m_score += lineScore[clamped] * m_level;

    if (clamped > 0)
    {
        ++m_combo;
        m_score += (m_combo - 1) * 50 * m_level;
    }
    else
    {
        m_combo = 0;
    }

    m_totalLines += clamped;
    m_level = 1 + m_totalLines / 10;
}

void Game::Step(Command command)
{
    if (m_gameOver)
    {
        return;
    }

    PieceState next = m_current;

    switch (command)
    {
    case Command::MoveLeft:
        --next.x;
        if (CanPlace(next))
        {
            m_current = next;
        }
        break;

    case Command::MoveRight:
        ++next.x;
        if (CanPlace(next))
        {
            m_current = next;
        }
        break;

    case Command::RotateCW:
    {
        next.rotation = (next.rotation + 1) % 4;
        static const int kickX[] = { 0, -1, 1, -2, 2 };
        bool rotated = false;
        for (int i = 0; i < 5; ++i)
        {
            PieceState test = next;
            test.x += kickX[i];
            if (CanPlace(test))
            {
                m_current = test;
                rotated = true;
                break;
            }
        }
        if (!rotated)
        {
            // do nothing
        }
        break;
    }

    case Command::SoftDrop:
        ++next.y;
        if (CanPlace(next))
        {
            m_current = next;
            UpdateScore(0, 1, 0);
        }
        else
        {
            LockPiece();
        }
        break;

    case Command::HardDrop:
    {
        int dropped = 0;
        while (CanPlace(next))
        {
            m_current = next;
            ++next.y;
            ++dropped;
        }
        if (dropped > 0)
        {
            UpdateScore(0, 0, dropped - 1);
        }
        LockPiece();
        break;
    }

    case Command::Hold:
    {
        if (m_holdUsedThisTurn)
        {
            break;
        }

        const int currentType = m_current.type;
        if (m_holdType < 0)
        {
            m_holdType = currentType;
            SpawnPiece();
        }
        else
        {
            m_current.type = m_holdType;
            m_current.rotation = 0;
            m_current.x = m_board.Width() / 2 - 2;
            m_current.y = 0;
            m_holdType = currentType; 

            if (!CanPlace(m_current))
            {
                m_gameOver = true;
            }
        }

        m_holdUsedThisTurn = true;
        break;
    }

    case Command::None:
    default:
        break;
    }
}

void Game::Tick()
{
    Step(Command::SoftDrop);
}

std::string Game::RenderText() const
{
    std::vector<std::string> canvas(m_board.Height(), std::string(m_board.Width(), ' '));

    for (int y = 0; y < m_board.Height(); ++y)
    {
        for (int x = 0; x < m_board.Width(); ++x)
        {
            if (m_board.Cell(x, y) != 0)
            {
                canvas[y][x] = '#';
            }
        }
    }

    const int ghostY = CalcGhostY();
    for (int py = 0; py < 4; ++py)
    {
        for (int px = 0; px < 4; ++px)
        {
            if (!PieceFilled(m_current.type, m_current.rotation, px, py))
            {
                continue;
            }

            const int gx = m_current.x + px;
            const int gy = ghostY + py;
            if (gx >= 0 && gx < m_board.Width() && gy >= 0 && gy < m_board.Height() && canvas[gy][gx] == ' ')
            {
                canvas[gy][gx] = '.';
            }
        }
    }

    for (int py = 0; py < 4; ++py)
    {
        for (int px = 0; px < 4; ++px)
        {
            if (!PieceFilled(m_current.type, m_current.rotation, px, py))
            {
                continue;
            }

            const int x = m_current.x + px;
            const int y = m_current.y + py;
            if (x >= 0 && x < m_board.Width() && y >= 0 && y < m_board.Height())
            {
                canvas[y][x] = '@';
            }
        }
    }

    const int renderWidth = m_board.Width() * 2;
    const std::string boardTop = "+" + std::string(renderWidth, '-') + "+";
    const std::string boardBottom = boardTop;

    std::vector<std::string> side;
    side.push_back("操作说明");
    side.push_back("----------------");
    side.push_back("A / D  : 左右移动");
    side.push_back("W      : 旋转");
    side.push_back("S      : 软降");
    side.push_back("Space  : 硬降");
    side.push_back("C      : Hold");
    side.push_back("P      : 暂停");
    side.push_back("Q      : 退出");
    side.push_back("");
    side.push_back("Next: " + std::to_string(m_nextType));
    side.push_back("Hold: " + std::to_string(m_holdType));

    const int sideGap = 4;
    const int sideWidth = 24;
    const int totalWidth = static_cast<int>(boardTop.size()) + sideGap + sideWidth;

    const int consoleWidth = 120;
    int leftPad = (consoleWidth - totalWidth) / 2;
    if (leftPad < 0)
    {
        leftPad = 0;
    }

    std::ostringstream oss;
    const std::string pad(leftPad, ' ');

    // 分数在最上方
    oss << pad
        << "Score: " << m_score
        << "  Level: " << m_level
        << "  Lines: " << m_totalLines
        << "  Combo: " << m_combo
        << "\n\n";

    // 顶边 + 右侧第一行
    oss << pad << boardTop;
    if (!side.empty())
    {
        oss << std::string(sideGap, ' ') << side[0];
    }
    oss << "\n";

    // 棋盘行 + 右侧说明
    for (int y = 0; y < m_board.Height(); ++y)
    {
        oss << pad << "|";
        for (int x = 0; x < m_board.Width(); ++x)
        {
            const char c = canvas[y][x];
            if (c == '#')
            {
                oss << "[]";
            }
            else if (c == '.')
            {
                oss << "..";
            }
            else if (c == '@')
            {
                oss << "##";
            }
            else
            {
                oss << "  ";
            }
        }
        oss << "|";

        const int sideIndex = y + 1;
        if (sideIndex < static_cast<int>(side.size()))
        {
            oss << std::string(sideGap, ' ') << side[sideIndex];
        }

        oss << "\n";
    }

    // 底边
    oss << pad << boardBottom << "\n";

    return oss.str();
}

const std::vector<std::vector<int>>& Board::Data() const
{
    return m_cells;
}

const Board& Game::GetBoard() const
{
    return m_board;
}

const PieceState& Game::CurrentPiece() const
{
    return m_current;
}