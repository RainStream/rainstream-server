// TestUV.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
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

#include "uwebsockets/App.h"

/* ws->getUserData returns one of these */
struct UserData {
	/* Fill with user data */
};

int main()
{
	uv_loop_t* loop = uv_default_loop();

	uv_thread_t id = uv_thread_self();
	printf("main thread id:%d.\n", id);

	json settings = {
				{"logLevel","warn"},
				{"logTags",{"info", "ice", "dtls","rtp","srtp","rtcp", "rtx","bwe",	"score", "simulcast","svc",	"sctp"}},
				{"rtcMinPort",40000},
				{"rtcMaxPort",49999},
	};
	rs::Worker* worker = new rs::Worker(settings);


	return uv_run(loop, UV_RUN_DEFAULT);

// 
//  	uWS::SSLApp().get("/*", [](auto *res, auto *req) {
//  		res->end("Hello World!");
//  	}).listen(9001, [](auto *listenSocket) {
//  		if (listenSocket) {
//  			std::cout << "Listening for connections..." << std::endl;
//  		}
//  	}).run();
//  
//  	std::cout << "Shoot! We failed to listen and the App fell through, exiting now!" << std::endl;
// 	

	// WebSockets to https://example.com/ws
// 	uWS::SSLApp({})
// 		.ws<UserData>("/ws", {
// 			.open = [](auto *ws, auto *req) {
// 			// May be similar to previous examples, ie. we need a way to add custom headers to the HTTP request before it"s sent
// 			// ...
// 			},
// 			.message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
// 				// ...
// 			}
// 			}).connect("example.com", 443, [](auto *token) {
// 
// 	}).run();
}
