#define _CRT_SECURE_NO_WARNINGS 1
#include "game.h"

void InitBoard(char board[MAX_ROWS][MAX_COLS], int rows, int cols, char set)
{
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            board[i][j] = set;
        }
    }
}

void DisplayBoard(char board[MAX_ROWS][MAX_COLS], int row, int col,
                  int remain_mines, int elapsed_seconds)
{
    printf("---------- 扫雷游戏 ----------\n");
    printf("剩余雷数: %-3d   用时: %d 秒\n", remain_mines, elapsed_seconds);
    printf("操作: 1=排查  2=插旗/拔旗\n\n");

    // 列号表头（列宽与格子一致，均为 3 字符）
    printf("    ");
    for (int j = 1; j <= col; j++)
    {
        printf("%3d", j);
    }
    printf("\n");

    for (int i = 1; i <= row; i++)
    {
        printf("%3d ", i);
        for (int j = 1; j <= col; j++)
        {
            printf("%3c", board[i][j]);
        }
        printf("\n");
    }
}

void SetMine(char mine[MAX_ROWS][MAX_COLS], int row, int col,
             int mine_count, int safe_x, int safe_y)
{
    int placed = 0;
    while (placed < mine_count)
    {
        int x = rand() % row + 1;
        int y = rand() % col + 1;

        // 首次点击位置及其周围一格不放雷
        if (abs(x - safe_x) <= 1 && abs(y - safe_y) <= 1)
        {
            continue;
        }

        if (mine[x][y] == '0')
        {
            mine[x][y] = '1';
            placed++;
        }
    }
}

int GetMineCount(char mine[MAX_ROWS][MAX_COLS], int x, int y)
{
    return mine[x - 1][y] + mine[x - 1][y - 1] + mine[x][y - 1] +
           mine[x + 1][y - 1] + mine[x + 1][y] + mine[x + 1][y + 1] +
           mine[x][y + 1] + mine[x - 1][y + 1] - 8 * '0';
}

void ExpandReveal(char mine[MAX_ROWS][MAX_COLS], char show[MAX_ROWS][MAX_COLS],
                  int row, int col, int x, int y, int* revealed)
{
    if (x < 1 || x > row || y < 1 || y > col)
    {
        return;
    }
    // 已排查或已插旗的格子不展开
    if (show[x][y] != '*')
    {
        return;
    }

    int count = GetMineCount(mine, x, y);
    show[x][y] = (char)(count + '0');
    (*revealed)++;

    if (count != 0)
    {
        return; // 数字格到此为止
    }

    // 空白格向周围八格扩散
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx != 0 || dy != 0)
            {
                ExpandReveal(mine, show, row, col, x + dx, y + dy, revealed);
            }
        }
    }
}

void MarkAllMines(char mine[MAX_ROWS][MAX_COLS], char show[MAX_ROWS][MAX_COLS],
                  int row, int col)
{
    for (int i = 1; i <= row; i++)
    {
        for (int j = 1; j <= col; j++)
        {
            if (mine[i][j] == '1' && show[i][j] == '*')
            {
                show[i][j] = MINE_MARK;
            }
        }
    }
}

// 清空输入缓冲区，防止非法输入导致死循环
static void ClearInputBuffer(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

int FindMine(char mine[MAX_ROWS][MAX_COLS], char show[MAX_ROWS][MAX_COLS],
             int row, int col, int mine_count, time_t start_time)
{
    const int total_safe = row * col - mine_count;
    int revealed = 0;
    int flags = 0;

    while (revealed < total_safe)
    {
        DisplayBoard(show, row, col, mine_count - flags,
                     (int)difftime(time(NULL), start_time));

        int op = 0, x = 0, y = 0;
        printf("请输入 操作 坐标x(行) y(列)，例如: 1 3 5 :>");
        fflush(stdout);
        int n = scanf("%d %d %d", &op, &x, &y);
        if (n == EOF)
        {
            printf("输入已结束，退出排雷\n");
            return 0;
        }
        if (n != 3)
        {
            printf("输入格式错误，请重新输入\n");
            ClearInputBuffer();
            continue;
        }

        if (x < 1 || x > row || y < 1 || y > col)
        {
            printf("坐标非法，请重新输入\n");
            continue;
        }

        if (op == OP_FLAG)
        {
            if (show[x][y] >= '0' && show[x][y] <= '8')
            {
                printf("已排查的坐标不能插旗\n");
                continue;
            }
            if (show[x][y] == FLAG)
            {
                show[x][y] = '*';
                flags--;
            }
            else
            {
                show[x][y] = FLAG;
                flags++;
            }
            continue; // 插旗后刷新界面，不消耗排查
        }

        if (op != OP_REVEAL)
        {
            printf("未知操作 %d，1=排查 2=插旗\n", op);
            continue;
        }

        if (show[x][y] == FLAG)
        {
            printf("该处已插旗，先取消旗帜再排查\n");
            continue;
        }
        if (show[x][y] != '*')
        {
            printf("该坐标已排查过，请换一个\n");
            continue;
        }

        if (mine[x][y] == '1')
        {
            printf("很遗憾，你被炸死了！\n");
            return 0;
        }

        ExpandReveal(mine, show, row, col, x, y, &revealed);
    }

    printf("恭喜你，排雷成功！\n");
    return 1;
}
