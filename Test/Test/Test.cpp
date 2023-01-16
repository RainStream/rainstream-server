// Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

struct AAA
{
	int i = 0;
};

int main()
{
	AAA* a = new AAA;
	a->i = 12;

	AAA* b = a;
	b->i = 15;

	printf("A v:%d,  B v:%d\n", a->i, b->i);

	return 0;
}
