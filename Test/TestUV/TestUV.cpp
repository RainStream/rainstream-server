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


const int FIB_UNTIL = 10;
uv_loop_t* loop = nullptr;

std::promise<int> result[FIB_UNTIL];
int data[FIB_UNTIL];


void fib(uv_work_t *req) {
	int n = *(int*)req->data;
	uv_thread_t id = uv_thread_self();
	printf("work thread id:%d.\n", id);
	Sleep(300);
	result[n].set_value(n);
	fprintf(stderr, "%d fibonacci is %d\n", n, n);
}

void after_fib(uv_work_t *req, int status) {
	uv_thread_t id = uv_thread_self();
	printf("after work thread id:%d.\n", id);
	//fprintf(stderr, "Done calculating %d fibonacci\n", *(int *)req->data);
}//

std::future<int> getResults(uv_loop_t* loop)
{
	uv_work_t req[FIB_UNTIL];
	int i;
	for (i = 0; i < FIB_UNTIL; i++) {
		data[i] = i;
		req[i].data = (void *)&data[i];
		uv_queue_work(loop, &req[i], fib, after_fib);
		co_await result[i].get_future();
	}

	co_return 0;
}

int main()
{
	loop = new uv_loop_t;;
	uv_loop_init(loop);

	uv_thread_t id = uv_thread_self();
	printf("main thread id:%d.\n", id);

	getResults(loop);
	

	return uv_run(loop, UV_RUN_DEFAULT);
}

// 
// void thread_set_promise(std::promise<int>& promiseObj) {
// 	std::cout << "In a thread, making data...\n";
// 	std::this_thread::sleep_for(std::chrono::milliseconds(3000));
// 	promiseObj.set_value(35);
// 	std::cout << "Finished\n";
// }

// std::future<int> compute_value1()
// // {
// // 	bool ret = co_await push_new_public_address_async();
// // 
// // 	co_return ret;
// // }
// 
// std::future<int> compute_value()
// {
// 	std::promise<int> promiseObj;
// 	std::future<int> futureObj = promiseObj.get_future();
// 	std::thread t(&thread_set_promise, std::ref(promiseObj));//
// 
// 	int value = co_await futureObj;
// 	//t.join();
// 
// 	co_return value;
// }
//
// int main() 
// {
// 	std::promise<int> promiseObj;
// // 	std::future<int> futureObj = promiseObj.get_future();
// // 	std::thread t(&thread_set_promise, std::ref(promiseObj));
// // 	//std::cout << futureObj.get() << std::endl;
// 	int cvaluse = compute_value().get();
//	t.join();
// 
// 	system("pause");
// 	return 0;
// }
