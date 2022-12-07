#pragma once

#include "common.h"
#include <regex>

class Url
{
public:
	static std::string Request(const std::string& url, const std::string& request);
};

/* Inline static methods. */

inline std::string Url::Request(const std::string& url, const std::string& request)
{
	std::smatch result;
	if (std::regex_search(url.cbegin(), url.cend(), result, std::regex(request + "=(.*?)&"))) {
		// 匹配具有多个参数的url

		// *? 重复任意次，但尽可能少重复  
		return result[1];
	}
	else if (regex_search(url.cbegin(), url.cend(), result, std::regex(request + "=(.*)"))) {
		// 匹配只有一个参数的url

		return result[1];
	}
	else {
		// 不含参数或制定参数不存在

		return std::string();
	}
}