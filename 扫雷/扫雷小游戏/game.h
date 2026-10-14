#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 三档难度：棋盘尺寸 + 雷数
#define EASY_ROWS 9
#define EASY_COLS 9
#define EASY_MINES 10

#define MID_ROWS 12
#define MID_COLS 12
#define MID_MINES 26

#define HARD_ROWS 16
#define HARD_COLS 16
#define HARD_MINES 50

// 支持的最大棋盘尺寸（含一圈哨兵边框，防止越界）
#define MAX_ROWS (HARD_ROWS + 2)
#define MAX_COLS (HARD_COLS + 2)

// 玩家操作类型
#define OP_REVEAL 1 // 排查坐标
#define OP_FLAG 2   // 插旗 / 取消插旗

// 棋盘上的特殊标记
#define FLAG 'F'       // 玩家插的旗
#define MINE_MARK 'M'  // 结束时标出的雷

// 初始化棋盘
void InitBoard(char board[MAX_ROWS][MAX_COLS], int rows, int cols, char set);
// 打印棋盘（含剩余雷数与用时）
void DisplayBoard(char board[MAX_ROWS][MAX_COLS], int row, int col,
                  int remain_mines, int elapsed_seconds);
// 布雷（避开首次点击位置及其周围一格，保证开局可展开）
void SetMine(char mine[MAX_ROWS][MAX_COLS], int row, int col,
             int mine_count, int safe_x, int safe_y);
// 统计 (x,y) 周围八格的雷数
int GetMineCount(char mine[MAX_ROWS][MAX_COLS], int x, int y);
// 洪水填充：排查 (x,y)，若为空白则递归展开周围格子，累计已排查数
void ExpandReveal(char mine[MAX_ROWS][MAX_COLS], char show[MAX_ROWS][MAX_COLS],
                  int row, int col, int x, int y, int* revealed);
// 游戏结束后在 show 上标出所有未被插旗标记的雷
void MarkAllMines(char mine[MAX_ROWS][MAX_COLS], char show[MAX_ROWS][MAX_COLS],
                  int row, int col);
// 排雷主循环，返回 1 = 排雷成功，0 = 踩雷失败
int FindMine(char mine[MAX_ROWS][MAX_COLS], char show[MAX_ROWS][MAX_COLS],
             int row, int col, int mine_count, time_t start_time);
