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
		// ƥ����ж��������url

		// *? �ظ�����Σ������������ظ�  
		return result[1];
	}
	else if (regex_search(url.cbegin(), url.cend(), result, std::regex(request + "=(.*)"))) {
		// ƥ��ֻ��һ��������url

		return result[1];
	}
	else {
		// �����������ƶ�����������

		return std::string();
	}
}