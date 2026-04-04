#define _CRT_SECURE_NO_WARNINGS 1
#include "game.h"

void InitBoard(char board[ROWS][COLS], int rows, int cols, char set)
{
    int i = 0;
    for (i = 0; i < rows; i++)
    {
        int j = 0;
        for (j = 0; j < cols; j++)
        {
            board[i][j] = set;
        }
            
    }
}

void DisplayBoard(char board[ROWS][COLS], int row, int col)
{
    int i = 0;
    printf("--------扫雷游戏-------\n");
    for (i = 0; i <= col; i++)
    {
        printf("%d ", i);
    }
    printf("\n");
    for (i = 1; i <= row; i++)
    {
        printf("%d ", i);
        int j = 0;
        for (j = 1; j <= col; j++)
        {
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }
}

void SetMine(char board[ROWS][COLS], int row, int col, int mine_count)
{
    int count = mine_count;
    while (count)
    {
        int x = rand() % row + 1;
        int y = rand() % col + 1;

        if (board[x][y] == '0')
        {
            board[x][y] = '1';
            count--;
        }
    }
}

int GetMineCount(char mine[ROWS][COLS], int x, int y)
{
    return (mine[x - 1][y] + mine[x - 1][y - 1] + mine[x][y - 1] + mine[x + 1][y - 1] + mine[x + 1][y] +mine[x + 1][y + 1] + mine[x][y + 1] + mine[x - 1][y + 1] - 8 * '0');
}

void FindMine(char mine[ROWS][COLS], char show[ROWS][COLS], int row, int col, int mine_count)
{
    int x = 0;
    int y = 0;
    int win = 0;

    while (win < row * col - mine_count)
    {
        printf("请输入要排查的坐标x(行) y(列):>");
        scanf("%d %d", &x, &y);

        if (x >= 1 && x <= row && y >= 1 && y <= col)
        {
            if (show[x][y] != '*')
            {
                printf("该坐标已排查，请重新输入\n");
                continue;
            }

            if (mine[x][y] == '1')
            {
                printf("很遗憾，你被炸死了\n");
                DisplayBoard(mine, row, col);
                break;
            }
            else
            {
                int count = GetMineCount(mine, x, y);
                show[x][y] = count + '0';
                DisplayBoard(show, row, col);
                win++;
            }
        }
        else
        {
            printf("坐标非法，重新输入\n");
        }
    }

    if (win == row * col - mine_count)
    {
        printf("恭喜你，排雷成功\n");
        DisplayBoard(mine, row, col);
    }
}