// TestUV.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <uv.h>
#include <Windows.h>

#pragma comment(lib,"libuv.lib")

const int FIB_UNTIL = 10;
uv_loop_t* loop = nullptr;

void fib(uv_work_t *req) {
	int n = *(int *)req->data;
	uv_thread_t id = uv_thread_self();
	printf("work thread id:%lu.\n", id);
	Sleep(3);
	long fib = n;
	fprintf(stderr, "%dth fibonacci is %lu\n", n, fib);
}

void after_fib(uv_work_t *req, int status) {
	uv_thread_t id = uv_thread_self();
	printf("after work thread id:%lu.\n", id);
	fprintf(stderr, "Done calculating %dth fibonacci\n", *(int *)req->data);
}

int main()
{
	loop = uv_default_loop();

	uv_thread_t id = uv_thread_self();
	printf("main thread id:%lu.\n", id);

	int data[FIB_UNTIL];
	uv_work_t req[FIB_UNTIL];
	int i;
	for (i = 0; i < FIB_UNTIL; i++) {
		data[i] = i;
		req[i].data = (void *)&data[i];
		uv_queue_work(loop, &req[i], fib, after_fib);
	}

	return uv_run(loop, UV_RUN_DEFAULT);
}
