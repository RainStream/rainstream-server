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

#include "common.h"
#include "Worker.h"
#include "Timer.hpp"

using namespace mediasoup;

// #include "uwebsockets/loop.h"
// #include "uwebsockets/App.h"

/* ws->getUserData returns one of these */
struct UserData {
	/* Fill with user data */
};

int main(int argc, char* argv[])
{
	Logger::SetLogLevel(Logger::LogLevel::LOG_DEBUG);
	Logger::SetDefaultHandler();

	uv_loop_t* loop = uv_default_loop();

	uv_thread_t id = uv_thread_self();
	printf("main thread id:%d.\n", id);

	json settings = {
		{"logLevel","warn"},
		{"logTags",{"info", "ice", "dtls","rtp","srtp","rtcp", "rtx","bwe",	"score", "simulcast","svc",	"sctp"}},
		{"rtcMinPort",40000},
		{"rtcMaxPort",49999}
	};

	Worker* worker = new Worker(settings);

	class Listener : public Timer::Listener
	{
	public:
		Listener(Worker* worker) {
			_worker = worker;
		}

		std::future<void> doDump()
		{
			json data = co_await _worker->dump();

			printf("worker dump results %s.\n", data.dump().c_str());

			co_return;
		}

		virtual void OnTimer(Timer* timer)
		{
			doDump();
		}

		Worker* _worker;
	};

	Listener listener(worker);
	Timer* timer = new Timer(&listener);
	timer->Start(5000, 5000);
	uv_run(loop, UV_RUN_DEFAULT);


// 	uWS::Loop* loop = uWS::Loop::get(loop);
//  
//   	uWS::SSLApp().get("/*", [=](auto *res, auto *req) {
// 
// 		json data =  worker->dump().get();
//   		res->end(data.dump());
//   	}).listen(9001, [](auto *listenSocket) {
//   		if (listenSocket) {
//   			std::cout << "Listening for connections..." << std::endl;
// 
//   		}
//   	}).run();
//   
//   	std::cout << "Shoot! We failed to listen and the App fell through, exiting now!" << std::endl;
 	

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
