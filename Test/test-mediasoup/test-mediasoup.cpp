// TestUV.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <iostream>
#include <functional>
#include <future>
#include <thread>
#include <chrono>
#include <cstdlib>

#include <uv.h>
#pragma comment(lib,"libuv.lib")
#include "common.hpp"
#include "Worker.hpp"

int main()
{
	uv_loop_t* loop = new uv_loop_t;;
	uv_loop_init(loop);

	uv_thread_t id = uv_thread_self();
	printf("main thread id:%d.\n", id);


	rs::Worker* worker = new rs::Worker("testId", AStringVector());
	

	return uv_run(loop, UV_RUN_DEFAULT);
}
