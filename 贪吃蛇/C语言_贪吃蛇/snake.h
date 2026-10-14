#pragma once

#include <windows.h>
#include <time.h>
#include <stdio.h>
#define KEY_PRESS(VK) ((GetAsyncKeyState(VK)&0x1) ? 1 : 0)
//⽅向
enum DIRECTION
{
	UP = 1,
	DOWN,
	LEFT,
	RIGHT
};
//游戏状态
enum GAME_STATUS
{
	OK,//正常运⾏
	KILL_BY_WALL,//撞墙
	KILL_BY_SELF,//咬到⾃⼰
	KILL_BY_OBSTACLE,//撞到障碍物
	END_NOMAL//正常结束
};
#define WALL L'□'
#define BODY L'●' //★○●◇◆□■
#define FOOD L'★' //★○●◇◆□■
#define OBSTACLE L'◆' //障碍物

//最高分存档文件
#define HISCORE_FILE "snake_hiscore.txt"
//障碍物数量上限
#define MAX_OBSTACLES 30

//蛇的初始位置
#define POS_X 24
#define POS_Y 5
//蛇⾝节点
typedef struct SnakeNode
{
	int x;
	int y;
	struct SnakeNode* next;
}SnakeNode, * pSnakeNode;
typedef struct Snake
{
	pSnakeNode _pSnake;//维护整条蛇的指针
	pSnakeNode _pFood;//维护⻝物的指针
	enum DIRECTION _Dir;//蛇头的⽅向默认是向右
	enum GAME_STATUS _Status;//游戏状态
	int _Socre;//当前获得分数
	int _foodWeight;//默认每个⻝物10分
	int _SleepTime;//每⾛⼀步休眠时间
	int _obX[MAX_OBSTACLES];//障碍物坐标
	int _obY[MAX_OBSTACLES];
	int _obstacleCount;//障碍物数量
	int _highScore;//历史最高分
}Snake, * pSnake;
//游戏开始前的初始化
void GameStart(pSnake ps);
//游戏运⾏过程
void GameRun(pSnake ps);
//游戏结束
void GameEnd(pSnake ps);
//设置光标的坐标
void SetPos(short x, short y);
//欢迎界⾯（含障碍模式选择）
void WelcomeToGame(pSnake ps);
//打印帮助信息
void PrintHelpInfo();
//创建地图
void CreateMap();
//初始化蛇
void InitSnake(pSnake ps);
//创建⻝物
void CreateFood(pSnake ps);
//创建障碍物
void CreateObstacles(pSnake ps);
//判断 (x,y) 是否是障碍物
int HitObstacle(pSnake ps, int x, int y);
//读取最高分存档
int LoadHighScore(void);
//保存最高分存档
void SaveHighScore(int score);
//游戏结束动画与清理
void GameOverAnimation(pSnake ps);
void FreeSnake(pSnake ps);
void FreeFood(pSnake ps);
void ClearGameArea(void);