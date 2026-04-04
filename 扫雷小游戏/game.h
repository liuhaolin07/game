#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define EASY_COUNT 10
#define MID_COUNT 15
#define HARD_COUNT 20

#define ROW 9
#define COL 9

#define ROWS (ROW + 2)
#define COLS (COL + 2)
//≥ı ºªØ∆Â≈Ã
void InitBoard(char board[ROWS][COLS], int rows, int cols, char set);
//¥Ú”°∆Â≈Ã
void DisplayBoard(char board[ROWS][COLS], int row, int col);
//≤º÷√¿◊
void SetMine(char board[ROWS][COLS], int row, int col, int mine_count);
//≈≈≤È¿◊
void FindMine(char mine[ROWS][COLS], char show[ROWS][COLS], int row, int col, int mine_count);
