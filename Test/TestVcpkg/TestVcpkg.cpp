// TestVcpkg.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <cppcoro/single_consumer_async_auto_reset_event.hpp>

#include <cppcoro/config.hpp>
#include <cppcoro/task.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/when_all.hpp>
#include <cppcoro/when_all_ready.hpp>
#include <cppcoro/on_scope_exit.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/async_auto_reset_event.hpp>

#include <thread>
#include <cassert>
#include <vector>

#pragma comment(lib, "Synchronization.lib")


int main()
{
	cppcoro::detail::task_promise<int> t_promise;
	t_promise.initial_suspend();
	cppcoro::task<int> ttt = t_promise.get_return_object();

	cppcoro::static_thread_pool tp{ 3 };

	auto run = [&]() -> cppcoro::task<>
	{
		cppcoro::async_auto_reset_event event;

		int value = 5;
		bool stopd = false;

		auto startWaiter = [&]() -> cppcoro::task<>
		{
			//co_await tp.schedule();
			while (value < 1005)
			{
				co_await event;

				if (stopd)
					co_return;

				++value;

			}
			//event.set();
		};

		auto startSignaller = [&]() -> cppcoro::task<>
		{
			//co_await tp.schedule();
			//value = 5;
			event.set();
			co_return;
		};

		std::vector<cppcoro::task<>> tasks;

		
		tasks.emplace_back(startWaiter());

		for (int i = 0; i < 1000; ++i)
		{
			tasks.emplace_back(startSignaller());
		}		

		co_await cppcoro::when_all(std::move(tasks));

		stopd = true;
		event.set();

		// NOTE: Can't use CHECK() here because it's not thread-safe
		assert(value == 1005);
	};

	std::vector<cppcoro::task<>> tasks;

	for (int i = 0; i < 1; ++i)
	{
		tasks.emplace_back(run());
	}

	cppcoro::sync_wait(cppcoro::when_all(std::move(tasks)));
}
