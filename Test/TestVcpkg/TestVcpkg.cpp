// TestVcpkg.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <json/json.h>
#include <thread>

#include <folly/futures/Future.h>
#include <folly/Executor.h>
#include <folly/Memory.h>
#include <folly/Unit.h>
#include <folly/dynamic.h>
#include <folly/synchronization/Baton.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <numeric>
#include <queue>
#include <string>
#include <thread>
#include <chrono>
#include <type_traits>

using namespace folly;
using std::string;
using namespace std;
using std::chrono::milliseconds;

#pragma comment(lib,"ws2_32.lib")

void foo(int x) {
	// do something with x
	cout << "foo(" << x << ")" << endl;
}

Promise<double> ppp;

Future<double> getEnergy() {
	ppp = Promise<double>();
	
	return ppp.getFuture();
}

int main()
{
	std::cout << std::this_thread::get_id() << std::endl;

	getEnergy()
	.then([]() 
	{
		std::cout << "getEnergy!\n" ;
		return "qeqw";
	})
	.then([](std::string res)
	{
		std::cout << "getEnergy!\n" << res;
		return 123;
	})
	.then([](int  res)
	{
		std::cout << "getEnergy!\n" << res;
	})
	.onTimeout(milliseconds(500),[]()
	{
		std::cout << std::this_thread::get_id() << std::endl;
		std::cout << "onTimeout!\n";
	})
	.onError([=](std::exception const& e) 
	{
		std::cout << "Hello World!\n";
	}); 

	std::this_thread::sleep_for(milliseconds(100));

	ppp.setException(std::bad_exception());
	
	std::this_thread::sleep_for(milliseconds(1100));
	Json::Value data;
	


    std::cout << "Hello World!\n"; 



	auto pro = makeFuture(123)
		.then([=](int x)
	{
		//cout << "makeFuture(" << code << ")" << endl;
	})
		.onError([=](std::exception const& e)
	{

	});

	cout << "making Promise" << endl;
	Promise<int> p;
	Future<int> f = p.getFuture();
	f.then(foo);
	cout << "Future chain made" << endl;

	// ... now perhaps in another event callback

	cout << "fulfilling Promise" << endl;
	p.setValue(42);
	cout << "Promise fulfilled" << endl;


	std::vector<Promise<Unit>> ps(100);
	std::vector<Future<Unit>> futures;

	for (auto& p : ps) {
		futures.push_back(p.getFuture());
	}

	bool flag = false;
	size_t n = 90;
	collectN(futures, n)
		.via(&InlineExecutor::instance())
		.then([&](std::vector<std::pair<size_t, Try<Unit>>> v) {
		flag = true;
		//EXPECT_EQ(n, v.size());
		for (auto& tt : v) {
			//EXPECT_TRUE(tt.second.hasValue());
		}
	});
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
