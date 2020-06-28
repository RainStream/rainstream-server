#pragma once

#include <random>
#include <string>
#include <vector>

using AStringVector = std::vector<std::string>;

/** Evaluates to the uint32_t of elements in an array (compile-time!) */
#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))

namespace utils
{
	inline uint32_t generateRandomNumber(uint32_t min = 10000000, uint32_t max = 99999999)
	{
		//平均分布
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
			buffer[i] = chars[generateRandomNumber(0, sizeof(chars) - 1)];

		return std::string(buffer, length);
	}

	std::string & AppendPrintf(std::string & dst, const char * format, ...);

	std::string & AppendVPrintf(std::string & str, const char * format, va_list args);

	std::string Printf(const char * format, ...);

	std::string ToLowerCase(const std::string& str);

	std::string ToUpperCase(const std::string& str);

	std::string join(const AStringVector& vec, const std::string & delimeter);

	json clone(const json& item);
}

uint32_t setInterval(std::function<void(void)> func, int interval);
void clearInterval(uint32_t identifier);

std::string uuidv4();

template<class K, class V>
V GetMapValue(const std::map<K, V>& maps, K key)
{
	auto it = maps.find(key);
	if (it != maps.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}


