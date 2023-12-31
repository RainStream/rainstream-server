#pragma once

#include <random>
#include <string>
#include <vector>

using AStringVector = std::vector<std::string>;

namespace utils
{
	inline uint32_t randomNumber(uint32_t min = 10000000, uint32_t max = 99999999)
	{
		//ƽ���ֲ�
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(min, max);

		return dis(gen);
	}

	inline std::string randomString(uint32_t length = 8)
	{
		static char buffer[64];
		static const char chars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
			'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };

		if (length > 64)
			length = 64;

		for (size_t i{ 0 }; i < length; ++i)
			buffer[i] = chars[randomNumber(0, sizeof(chars) - 1)];

		return std::string(buffer, length);
	}

	std::string & AppendPrintf(std::string & dst, const char * format, ...);

	std::string Printf(const char * format, ...);

	std::string ToLowerCase(const std::string& str);

	std::string ToUpperCase(const std::string& str);

	std::string join(const AStringVector& vec, const std::string & delimeter);

	inline Json cloneObject(const Json& obj)
	{
		if (obj.is_object())
			return obj;

		return obj;
	};
}

uint32_t setInterval(std::function<void(void)> func, int interval);
void clearInterval(uint32_t identifier);
