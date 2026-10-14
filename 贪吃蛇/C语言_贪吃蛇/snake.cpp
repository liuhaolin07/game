#include "Snake.h"
//设置光标的坐标
void SetPos(short x, short y)
{
	COORD pos = { x, y };
	HANDLE hOutput = NULL;
	//获取标准输出的句柄(⽤来标识不同设备的数值)
	hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	//设置标准输出上光标的位置为pos
	SetConsoleCursorPosition(hOutput, pos);
}
void WelcomeToGame(pSnake ps)
{
	SetPos(40, 13);
	printf("欢迎来到贪吃蛇小游戏");
	SetPos(40, 25);//让按任意键继续的出现的位置好看点
	system("pause");
	system("cls");
	SetPos(25, 10);
	printf("用 ↑ . ↓ . ← . → 分别控制蛇的移动， F3为加速，F4为减速\n");
	SetPos(25, 11);
	printf("加速将能得到更高的分数。\n");

	//选择是否开启障碍模式
	ps->_obstacleCount = 0;
	SetPos(25, 14);
	printf("是否开启障碍模式？(Y 开启 / N 关闭)：");
	while (1)
	{
		if (KEY_PRESS('Y'))
		{
			ps->_obstacleCount = -1;//标记待生成，在 GameStart 中布置
			break;
		}
		if (KEY_PRESS('N'))
		{
			break;
		}
		Sleep(30);
	}

	SetPos(40, 25);
	system("pause");
	system("cls");
}
void CreateMap()
{
	int i = 0;
	//上(0,0)-(56, 0)
	SetPos(0, 0);
	for (i = 0; i < 58; i += 2)
	{
		wprintf(L"%c", WALL);
	}
	//下(0,26)-(56, 26)
	SetPos(0, 26);
	for (i = 0; i < 58; i += 2)
	{
		wprintf(L"%c", WALL);
	}
	//左
	//x是0，y从1开始增⻓
	for (i = 1; i < 26; i++)
	{
		SetPos(0, i);
		wprintf(L"%c", WALL);
	}
	//x是56，y从1开始增⻓
	for (i = 1; i < 26; i++)
	{
		SetPos(56, i);
		wprintf(L"%c", WALL);
	}
}
void InitSnake(pSnake ps)
{
	pSnakeNode cur = NULL;
	int i = 0;
	//创建蛇⾝节点，并初始化坐标
	//头插法
	for (i = 0; i < 5; i++)
	{
		//创建蛇⾝的节点
		cur = (pSnakeNode)malloc(sizeof(SnakeNode));
		if (cur == NULL)
		{
			perror("InitSnake()::malloc()");
			return;
		}
		//设置坐标
		cur->next = NULL;
		cur->x = POS_X + i * 2;
		cur->y = POS_Y;
		//头插法
		if (ps->_pSnake == NULL)
		{
			ps->_pSnake = cur;
		}
		else
		{
			cur->next = ps->_pSnake;
			ps->_pSnake = cur;
		}
	}
	//打印蛇的⾝体
	cur = ps->_pSnake;
	while (cur)
	{
		SetPos(cur->x, cur->y);
		wprintf(L"%c", BODY);
		cur = cur->next;
	}
	ps->_SleepTime = 200;
	ps->_Socre = 0;
	ps->_Status = OK;
	ps->_Dir = RIGHT;
	ps->_foodWeight = 10;
	ps->_highScore = LoadHighScore();
}

//读取最高分存档
int LoadHighScore(void)
{
	int score = 0;
	FILE* pf = NULL;
	if (fopen_s(&pf, HISCORE_FILE, "r") == 0 && pf != NULL)
	{
		if (fscanf_s(pf, "%d", &score) != 1)
		{
			score = 0;
		}
		fclose(pf);
	}
	return score;
}

//保存最高分存档
void SaveHighScore(int score)
{
	FILE* pf = NULL;
	if (fopen_s(&pf, HISCORE_FILE, "w") == 0 && pf != NULL)
	{
		fprintf(pf, "%d\n", score);
		fclose(pf);
	}
}

//判断 (x,y) 是否是障碍物
int HitObstacle(pSnake ps, int x, int y)
{
	for (int i = 0; i < ps->_obstacleCount; i++)
	{
		if (ps->_obX[i] == x && ps->_obY[i] == y)
		{
			return 1;
		}
	}
	return 0;
}

//随机布置障碍物，避开蛇的出生区域和边界
void CreateObstacles(pSnake ps)
{
	const int count = 10;//固定布置 10 个障碍物
	int placed = 0;
	while (placed < count && placed < MAX_OBSTACLES)
	{
		int x = rand() % 53 + 2;//与食物一致：2~54 的偶数
		int y = rand() % 25 + 1;

		if (x % 2 != 0)
		{
			continue;
		}
		//避开蛇出生的一行及其上下
		if (y >= POS_Y - 1 && y <= POS_Y + 1)
		{
			continue;
		}
		//不能与其他障碍物、蛇身重叠
		int overlap = HitObstacle(ps, x, y);
		for (pSnakeNode cur = ps->_pSnake; cur && !overlap; cur = cur->next)
		{
			if (cur->x == x && cur->y == y)
			{
				overlap = 1;
			}
		}
		if (overlap)
		{
			continue;
		}

		ps->_obX[placed] = x;
		ps->_obY[placed] = y;
		SetPos((short)x, (short)y);
		wprintf(L"%c", OBSTACLE);
		placed++;
	}
	ps->_obstacleCount = placed;
}
void CreateFood(pSnake ps)
{
	int x = 0;
	int y = 0;
again:
	//产⽣的x坐标应该是2的倍数，这样才可能和蛇头坐标对⻬。
	do
	{
		x = rand() % 53 + 2;
		y = rand() % 25 + 1;
	} while (x % 2 != 0);
	pSnakeNode cur = ps->_pSnake;//获取指向蛇头的指针
	//⻝物不能和蛇⾝、障碍物冲突
	while (cur)
	{
		if (cur->x == x && cur->y == y)
		{
			goto again;
		}
		cur = cur->next;
	}
	if (HitObstacle(ps, x, y))
	{
		goto again;
	}
	pSnakeNode pFood = (pSnakeNode)malloc(sizeof(SnakeNode)); //创建⻝物
	if (pFood == NULL)
	{
		perror("CreateFood::malloc()");
		return;
	}
	else
	{
		pFood->x = x;
		pFood->y = y;
		SetPos(pFood->x, pFood->y);
		wprintf(L"%c", FOOD);
		ps->_pFood = pFood;
		}
}
void PrintHelpInfo()
{
 //打印提⽰信息
 SetPos(64, 15);
 printf("不能穿墙，不能咬到自己\n");
 SetPos(64, 16);
 printf("用↑.↓.←.→分别控制蛇的移动.");
 SetPos(64, 17);
 printf("F3 为加速，F4 为减速\n");
 SetPos(64, 18);
 printf("ESC ：退出游戏.space：暂停游戏.");
 SetPos(64, 19);
 printf("◆ 为障碍物，撞上即失败");
 SetPos(64, 21);
 printf("SCU_Phoenix出版");
}
void pause()//暂停
{
 while (1)
 {
 Sleep(300);
 if (KEY_PRESS(VK_SPACE))
 {
 break;
 }
 }
}
//pSnakeNode psn 是下⼀个节点的地址
//pSnake ps 维护蛇的指针
int NextIsFood(pSnakeNode psn, pSnake ps)
{
 return (psn->x == ps->_pFood->x) && (psn->y == ps->_pFood->y);
}
//pSnakeNode psn 是下⼀个节点的地址
//pSnake ps 维护蛇的指针
void EatFood(pSnakeNode psn, pSnake ps)
{
 //头插法
 psn->next = ps->_pSnake;
 ps->_pSnake = psn;
 pSnakeNode cur = ps->_pSnake;
 //打印蛇
 while (cur)
 {
	 SetPos(cur->x, cur->y);
	 wprintf(L"%c", BODY);
	 cur = cur->next;
 }
 ps->_Socre += ps->_foodWeight;
 free(ps->_pFood);
 CreateFood(ps);
}
//pSnakeNode psn 是下⼀个节点的地址
//pSnake ps 维护蛇的指针
void NoFood(pSnakeNode psn, pSnake ps)
{
	//头插法
	psn->next = ps->_pSnake;
	ps->_pSnake = psn;
	pSnakeNode cur = ps->_pSnake;
	//打印蛇
	while (cur->next->next)
	{
		SetPos(cur->x, cur->y);
		wprintf(L"%c", BODY);
		cur = cur->next;
	}
	//最后⼀个位置打印空格，然后释放节点
	SetPos(cur->next->x, cur->next->y);
	printf(" ");
	free(cur->next);
	cur->next = NULL;
}
//pSnake ps 维护蛇的指针
int KillByWall(pSnake ps)
{
	if ((ps->_pSnake->x == 0)
		|| (ps->_pSnake->x == 56)
		|| (ps->_pSnake->y == 0)
		|| (ps->_pSnake->y == 26))
	{
		ps->_Status = KILL_BY_WALL;
		return 1;
	}
	return 0;
}
//pSnake ps 维护蛇的指针
int KillBySelf(pSnake ps)
{
	pSnakeNode cur = ps->_pSnake->next;
	while (cur)
	{
		if ((ps->_pSnake->x == cur->x)
			&& (ps->_pSnake->y == cur->y))
		{
			ps->_Status = KILL_BY_SELF;
			return 1;
		}
		cur = cur->next;
	}
	return 0;
}
void SnakeMove(pSnake ps)
{
	//创建下⼀个节点
	pSnakeNode pNextNode = (pSnakeNode)malloc(sizeof(SnakeNode));
	if (pNextNode == NULL)
	{
		perror("SnakeMove()::malloc()");
		return;
	}
	//确定下⼀个节点的坐标，下⼀个节点的坐标根据，蛇头的坐标和⽅向确定
	switch (ps->_Dir)
	{
	case UP:
	{
		pNextNode->x = ps->_pSnake->x;
		pNextNode->y = ps->_pSnake->y - 1;
	}
	break;
	case DOWN:
	{
		pNextNode->x = ps->_pSnake->x;
		pNextNode->y = ps->_pSnake->y + 1;
	}
	break;
	case LEFT:
	{
		pNextNode->x = ps->_pSnake->x - 2;
		pNextNode->y = ps->_pSnake->y;
	}
	break;
	case RIGHT:
	{
		pNextNode->x = ps->_pSnake->x + 2;
		pNextNode->y = ps->_pSnake->y;
	}
	break;
	}
	//撞障碍物检测：在移动前判断
	if (HitObstacle(ps, pNextNode->x, pNextNode->y))
	{
		ps->_Status = KILL_BY_OBSTACLE;
		free(pNextNode);
		return;
	}
	//如果下⼀个位置就是⻝物
	if (NextIsFood(pNextNode, ps))
	{
		EatFood(pNextNode, ps);
	}
	else//如果没有⻝物
	{
		NoFood(pNextNode, ps);
	}
	KillByWall(ps);
	KillBySelf(ps);
}
void GameStart(pSnake ps)
{
	//设置控制台窗⼝的⼤⼩，30⾏，100列
	//mode 为DOS命令
	system("mode con cols=100 lines=30");
	//设置cmd窗⼝名称
	system("title 贪吃蛇");
	//获取标准输出的句柄(⽤来标识不同设备的数值)
	HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	//影藏光标操作
	CONSOLE_CURSOR_INFO CursorInfo;
	GetConsoleCursorInfo(hOutput, &CursorInfo);//获取控制台光标信息
	CursorInfo.bVisible = false; //隐藏控制台光标
	SetConsoleCursorInfo(hOutput, &CursorInfo);//设置控制台光标状态
	//打印欢迎界⾯（含障碍模式选择）
	WelcomeToGame(ps);
	//打印地图
	CreateMap();
	//初始化蛇
	InitSnake(ps);
	//布置障碍物（若玩家选择开启）
	if (ps->_obstacleCount == -1)
	{
		ps->_obstacleCount = 0;
		CreateObstacles(ps);
	}
	//创造第⼀个⻝物
	CreateFood(ps);
}
void GameRun(pSnake ps)
{
	//打印右侧帮助信息
	PrintHelpInfo();
	do
	{
		SetPos(64, 10);
		printf("得分：%d ", ps->_Socre);
		printf("每个食物得分：%d分", ps->_foodWeight);
		SetPos(64, 12);
		printf("最高分：%d      ", ps->_highScore);
		if (KEY_PRESS(VK_UP) && ps->_Dir != DOWN)
		{
			ps->_Dir = UP;
		}
		else if (KEY_PRESS(VK_DOWN) && ps->_Dir != UP)
		{
			ps->_Dir = DOWN;
		}
		else if (KEY_PRESS(VK_LEFT) && ps->_Dir != RIGHT)
		{
			ps->_Dir = LEFT;
		}
		else if (KEY_PRESS(VK_RIGHT) && ps->_Dir != LEFT)
		{
			ps->_Dir = RIGHT;
		}
		else if (KEY_PRESS(VK_SPACE))
		{
			pause();
		}
		else if (KEY_PRESS(VK_ESCAPE))
		{
			ps->_Status = END_NOMAL;
			break;
		}
		else if (KEY_PRESS(VK_F3))
		{
			if (ps->_SleepTime >= 50)
			{
				ps->_SleepTime -= 30;
				ps->_foodWeight += 2;
			}
		}
		else if (KEY_PRESS(VK_F4))
		{
			if (ps->_SleepTime < 350)
			{
				ps->_SleepTime += 30;
				if (ps->_SleepTime > 350)
				{
					ps->_SleepTime = 350;
				}
				//减速降低奖励，但每个食物至少 1 分
				ps->_foodWeight = ps->_foodWeight > 2 ? ps->_foodWeight - 2 : 1;
			}
		}
		//蛇每次⼀定之间要休眠的时间，时间短，蛇移动速度就快
		Sleep(ps->_SleepTime);
		SnakeMove(ps);
	} while (ps->_Status == OK);
}
void GameOverAnimation(pSnake ps)
{
	//简单的死亡闪烁动画
	for (int i = 0; i < 3; i++)
	{
		for (pSnakeNode cur = ps->_pSnake; cur; cur = cur->next)
		{
			SetPos(cur->x, cur->y);
			printf("  ");
		}
		Sleep(120);
		for (pSnakeNode cur = ps->_pSnake; cur; cur = cur->next)
		{
			SetPos(cur->x, cur->y);
			wprintf(L"%c", BODY);
		}
		Sleep(120);
	}
}

void FreeSnake(pSnake ps)
{
	pSnakeNode cur = ps->_pSnake;
	while (cur)
	{
		pSnakeNode del = cur;
		cur = cur->next;
		free(del);
	}
	ps->_pSnake = NULL;
}

void FreeFood(pSnake ps)
{
	if (ps->_pFood)
	{
		free(ps->_pFood);
		ps->_pFood = NULL;
	}
}

void ClearGameArea(void)
{
	//清理整个游戏区域（不含最外层边框），防止本局残留影响下一局
	for (int y = 1; y < 26; y++)
	{
		SetPos(2, y);
		for (int x = 2; x < 56; x += 2)
		{
			printf("  ");
		}
	}
}

void GameEnd(pSnake ps)
{
	//根据死亡原因播放动画或提示
	switch (ps->_Status)
	{
	case END_NOMAL:
		SetPos(24, 12);
		printf("您主动退出游戏\n");
		break;
	case KILL_BY_SELF:
		GameOverAnimation(ps);
		SetPos(24, 12);
		printf("您撞上自己了 ,游戏结束!\n");
		break;
	case KILL_BY_WALL:
		GameOverAnimation(ps);
		SetPos(24, 12);
		printf("您撞墙了,游戏结束!\n");
		break;
	case KILL_BY_OBSTACLE:
		GameOverAnimation(ps);
		SetPos(24, 12);
		printf("您撞上障碍物了,游戏结束!\n");
		break;
	default:
		break;
	}
	//刷新最高分存档
	if (ps->_Socre > ps->_highScore)
	{
		ps->_highScore = ps->_Socre;
		SaveHighScore(ps->_highScore);
		SetPos(24, 14);
		printf("新纪录！最高分 %d 已保存\n", ps->_highScore);
	}
	else
	{
		SetPos(24, 14);
		printf("当前最高分：%d\n", ps->_highScore);
	}
	//释放资源
	FreeSnake(ps);
	FreeFood(ps);
	ClearGameArea();
}