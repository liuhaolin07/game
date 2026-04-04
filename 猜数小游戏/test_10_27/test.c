#define _CRT_SECURE_NO_WARNINGS 1
//分支循环练习
/*猜数游戏*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
void game()
{
	int times;
	printf("游戏设置，最多猜几次（输入）：");
	scanf("%d", &times);
	int num = rand()%100+1;//错误分析：不要把生成随机数放在游戏循环内
	do {
		int n;
		printf("请输入一到一百的数字:");
		scanf("%d", &n);
		if (n < num) {
			printf("猜小了，还剩%d次机会。\n",times-1);
		}
		else if (n > num) {
			printf("猜大了，还剩%d次机会。\n",times-1);
		}
		else {
			printf("恭喜你猜对了！\n");
			printf("宇宙！！！超级！！！无敌！！！大聪明！！！\n");
			printf("你将获得以下奖励：");
			int num1 = rand() % 2 + 1;
			switch (num1)
			{
			case 1:printf("大嘴烧一份。\n"); break;
			case 2:printf("20个五下单词限两天背完。\n"); break;
			default:break;
			}
			break;
		}
		times--;
	} while (times);
	printf("机会已用完。");
	printf("正确答案是%d。\n", num);
}
void menu()
{
	printf("****猜数游戏****\n");
	printf("****************\n");
	printf("****开始按1*****\n");
	printf("****结束按0*****\n");
	printf("****************\n");
}
int main()
{
	srand((unsigned int)time(NULL));
	while (1) {
	menu();
	int a;
	scanf("%d", &a);
	switch (a) {
		case 1:
			game();
			break;
		case 0:
			return 0;
		default:
			printf("请重新输入：\n");
			break;
		}
	}
	
	return 0;
}