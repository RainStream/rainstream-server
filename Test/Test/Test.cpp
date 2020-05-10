// Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include <pplx/pplxtasks.h>

#if defined(_RESUMABLE_FUNCTIONS_SUPPORTED) && _RESUMABLE_FUNCTIONS_SUPPORTED || defined(__cpp_coroutines) && __cpp_coroutines >= 201703L

#include <experimental/coroutine>
#include "pplxawait.h"

#endif

#include <cpprest/http_client.h>

using web::uri;
using web::http::methods;
using web::http::http_exception;
using web::http::client::http_client;
using web::http::client::http_client_config;

using namespace web::http::client;

namespace web::http::client
{
	class http_client;
}

concurrency::task<bool> push_new_public_address_async()
{
	try
	{
		web::http::client::http_client public_address_client_(L"www.baidu.com");
		auto response = co_await public_address_client_.request(methods::GET, L"/");

		co_return response.status_code() == web::http::status_codes::OK;
	}
	catch (http_exception&)
	{
		co_return false;
	}
}

#include <future>

std::future<int> compute_value1()
{
	bool ret  = co_await push_new_public_address_async();

	co_return ret;
}

std::future<int> compute_value()
{
	int result = co_await std::async([]
	{
		return 30;
	});

	co_return result;
}

int main()
{

	bool ret = push_new_public_address_async().get();

	compute_value().get();

	return 0;
}
