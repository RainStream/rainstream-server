#include "pch.h"
#include <stdio.h>
#include <uv.h>

#include <iostream>


using namespace std;

int repeat = 0;
static int repeatCount = 10;

static void callback(uv_timer_t* handle) {
	
	repeat = repeat + 1;

	printf("callback %d\n", repeat);

	if (repeatCount == repeat) {
		uv_timer_stop(handle);
		//用完一定要调用uv_close,不然会内存泄露
		uv_close((uv_handle_t*)handle, NULL);
	}
}


int main() {
	uv_loop_t* loop = uv_default_loop();

	uv_timer_t timer_req;

	uv_timer_init(loop, &timer_req);

	uv_timer_start(&timer_req, callback, 1000, 1000);

	return uv_run(loop, UV_RUN_DEFAULT);
}