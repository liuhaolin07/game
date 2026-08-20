#define _CRT_SECURE_NO_WARNINGS 1
//函数与数组实践：扫雷小游戏

#include "game.h"

void menu()
{
	printf("***********************\n");
	printf("***** 1. play *****\n");
	printf("***** 0. exit *****\n");
	printf("***********************\n");
}

int SelectDifficulty()
{
	int level = 0;
	printf("请选择难度:\n");
	printf("1. 简单(10个雷)\n");
	printf("2. 中等(15个雷)\n");
	printf("3. 困难(20个雷)\n");
	printf("输入:>");
	scanf("%d", &level);

	switch (level)
	{
	case 1:
		return EASY_COUNT;
	case 2:
		return MID_COUNT;
	case 3:
		return HARD_COUNT;
	default:
		printf("输入无效，默认简单难度\n");
		return EASY_COUNT;
	}
}

void game()
{
	char mine[ROWS][COLS];//存放布置好的雷
	char show[ROWS][COLS];//存放排查出的雷的信息
	int mine_count = SelectDifficulty();
	//初始化棋盘
	//1. mine数组最开始是全'0'
	//2. show数组最开始是全'*'
	InitBoard(mine, ROWS, COLS, '0');
	InitBoard(show, ROWS, COLS, '*');
	//打印棋盘
	//DisplayBoard(mine, ROW, COL);
	DisplayBoard(show, ROW, COL);
	//1. 布置雷
	SetMine(mine, ROW, COL, mine_count);
	//DisplayBoard(mine, ROW, COL);
	//2. 排查雷
	FindMine(mine, show, ROW, COL, mine_count);
}
int main()
{
	int input = 0;
	srand((unsigned int)time(NULL));
	do
	{
		menu();
		printf("请选择:>");
		scanf("%d", &input);
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
