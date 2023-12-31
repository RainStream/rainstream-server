#include "pch.h"
#include <stdio.h>
#include <uv.h>
#include "Utils.hpp"
#include <iostream>


using namespace std;

int repeat = 0;
static int repeatCount = 10;

static void check_callback(uv_check_t* handle) {
	repeat = repeat + 1;
	printf("check_callback %d\n", repeat);
	uv_check_stop(handle);
}

static void timer_callback(uv_timer_t* handle) {	
	repeat = repeat + 1;
	printf("timer_callback %d\n", repeat);
	uv_timer_stop(handle);
}


void idle_callback(uv_idle_t * handle) {
	repeat = repeat + 1;
	printf("idle_callback %d\n", repeat);
	uv_idle_stop(handle);
}

void prepare_callback(uv_prepare_t* handle) {
	repeat = repeat + 1;
	printf("prepare_callback %d\n", repeat);
	uv_prepare_stop(handle);
}

uv_async_t async;
double percentage;

void sync_print(uv_async_t* handle)
{
	printf("print thread id: %ld, value is %ld\n", uv_thread_self(), (long)handle->data);

	setImmediate([]()
		{
			printf("setImmediate %d\n", 1);
		});

	setImmediate([]()
		{
			printf("setImmediate %d\n", 2);
		});

	setTimeout([]()
		{
			printf("setTimeout %d\n", 1);
		}, 0);

	setTimeout([]()
		{
			printf("setTimeout %d\n", 2);
		}, 0);

	setImmediate([]()
		{
			printf("setImmediate %d\n", 3);
		});
}

void run(uv_work_t* req)
{
	long count = (long)req->data;
	for (int index = 0; index < count; index++)
	{
		printf("run thread id: %ld, index: %d\n", uv_thread_self(), index);
		async.data = (void*)(long)index;
		uv_async_send(&async);
		Sleep(1);
	}
}

void after(uv_work_t* req, int status)
{
	printf("done, thread id: %ld\n", uv_thread_self());
	uv_close((uv_handle_t*)&async, NULL);
}


int main() {

	printf("main thread id: %ld\n", uv_thread_self()); 
	uv_loop_t* loop = uv_default_loop();
	loadGlobalCheck();

	uv_work_t req;
	int size = 1;
	req.data = (void*)(long)size;

	uv_async_init(loop, &async, sync_print);
	int index = 1;
	async.data = (void*)(long)index;
	//uv_async_send(&async);
	uv_queue_work(loop, &req, run, after);
	

	return uv_run(loop, UV_RUN_DEFAULT);
}

