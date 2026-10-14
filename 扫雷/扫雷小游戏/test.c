#define _CRT_SECURE_NO_WARNINGS 1
// 二维数组 + 函数实践：扫雷小游戏
// 本次升级：插旗、空白自动展开、计时、三档难度、首点保护

#include "game.h"

void menu()
{
    printf("***********************\n");
    printf("*****  1. play   *****\n");
    printf("*****  0. exit   *****\n");
    printf("***********************\n");
}

// 选择难度，通过指针带回棋盘尺寸与雷数
void SelectDifficulty(int* row, int* col, int* mine_count)
{
    int level = 0;
    printf("请选择难度:\n");
    printf("1. 简单 (9 x 9 , 10 雷)\n");
    printf("2. 中等 (12 x 12 , 26 雷)\n");
    printf("3. 困难 (16 x 16 , 50 雷)\n");
    printf("请输入:>");
    if (scanf("%d", &level) != 1)
    {
        level = 1;
    }
    {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF)
        {
        }
    }

    switch (level)
    {
    case 1:
        *row = EASY_ROWS;
        *col = EASY_COLS;
        *mine_count = EASY_MINES;
        break;
    case 2:
        *row = MID_ROWS;
        *col = MID_COLS;
        *mine_count = MID_MINES;
        break;
    case 3:
        *row = HARD_ROWS;
        *col = HARD_COLS;
        *mine_count = HARD_MINES;
        break;
    default:
        printf("输入无效，默认简单难度\n");
        *row = EASY_ROWS;
        *col = EASY_COLS;
        *mine_count = EASY_MINES;
        break;
    }
}

// 读入首次点击坐标（保证合法），EOF 时返回 0 表示中止
int ReadFirstMove(int row, int col, int* x, int* y)
{
    printf("请输入首次排查坐标 x(行) y(列)（首点必安全）:>");
    for (;;)
    {
        int n = scanf("%d %d", x, y);
        if (n == EOF)
        {
            return 0;
        }
        if (n == 2 && *x >= 1 && *x <= row && *y >= 1 && *y <= col)
        {
            return 1;
        }
        printf("坐标非法，重新输入:>");
        while (getchar() != '\n')
        {
            if (feof(stdin))
            {
                return 0;
            }
        }
    }
}

void game()
{
    char mine[MAX_ROWS][MAX_COLS]; // 布好雷的棋盘
    char show[MAX_ROWS][MAX_COLS]; // 展示给玩家的信息
    int row = 0, col = 0, mine_count = 0;

    SelectDifficulty(&row, &col, &mine_count);

    InitBoard(mine, MAX_ROWS, MAX_COLS, '0');
    InitBoard(show, MAX_ROWS, MAX_COLS, '*');
    DisplayBoard(show, row, col, mine_count, 0);

    // 首次点击安全：先拿坐标再布雷
    int first_x = 0, first_y = 0;
    if (!ReadFirstMove(row, col, &first_x, &first_y))
    {
        printf("输入已结束，退出游戏\n");
        return;
    }
    SetMine(mine, row, col, mine_count, first_x, first_y);

    time_t start_time = time(NULL);
    int win = FindMine(mine, show, row, col, mine_count, start_time);
    int elapsed = (int)difftime(time(NULL), start_time);

    MarkAllMines(mine, show, row, col);
    DisplayBoard(show, row, col, win ? 0 : mine_count, elapsed);
    printf("%s！用时 %d 秒\n", win ? "排雷成功" : "游戏结束", elapsed);
}

int main()
{
    int input = 0;
    srand((unsigned int)time(NULL));
    do
    {
        menu();
        printf("请选择:>");
        if (scanf("%d", &input) != 1)
        {
            input = 0; // EOF 或非法输入时退出
        }
        {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF)
            {
            }
        }
        switch (input)
        {
        case 1:
            game();
            break;
        case 0:
            printf("退出游戏\n");
            break;
        default:
            printf("选择错误，重新选择\n");
            break;
        }
    } while (input);
    return 0;
}
