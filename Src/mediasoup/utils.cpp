#include "common.hpp"
#include "utils.hpp"

namespace utils
{
	std::string & AppendVPrintf(std::string & str, const char * format, va_list args)
	{
		assert(format != nullptr);

		char buffer[2048];
		int len;
#ifdef va_copy
		va_list argsCopy;
		va_copy(argsCopy, args);
#else
#define argsCopy args
#endif
#ifdef _MSC_VER
		// MS CRT provides secure printf that doesn"t behave like in the C99 standard
		if ((len = _vsnprintf_s(buffer, ARRAYCOUNT(buffer), _TRUNCATE, format, argsCopy)) != -1)
#else  // _MSC_VER
		if ((len = vsnprintf(buffer, ARRAYCOUNT(buffer), format, argsCopy)) < static_cast<int>(ARRAYCOUNT(buffer)))
#endif  // else _MSC_VER
		{
			// The result did fit into the static buffer
#ifdef va_copy
			va_end(argsCopy);
#endif
			str.append(buffer, static_cast<size_t>(len));
			return str;
		}
#ifdef va_copy
		va_end(argsCopy);
#endif

		// The result did not fit into the static buffer, use a dynamic buffer:
#ifdef _MSC_VER
		// for MS CRT, we need to calculate the result length
		len = _vscprintf(format, args);
		if (len == -1)
		{
			return str;
		}
#endif  // _MSC_VER

		// Allocate a buffer and printf into it:
#ifdef va_copy
		va_copy(argsCopy, args);
#endif
		std::vector<char> Buffer(static_cast<size_t>(len) + 1);
#ifdef _MSC_VER
		vsprintf_s(&(Buffer.front()), Buffer.size(), format, argsCopy);
#else  // _MSC_VER
		vsnprintf(&(Buffer.front()), Buffer.size(), format, argsCopy);
#endif  // else _MSC_VER
		str.append(&(Buffer.front()), Buffer.size() - 1);
#ifdef va_copy
		va_end(argsCopy);
#endif
		return str;
	}

	std::string & AppendPrintf(std::string & dst, const char * format, ...)
	{
		va_list args;
		va_start(args, format);
		std::string & retval = AppendVPrintf(dst, format, args);
		va_end(args);
		return retval;
	}

	std::string Printf(const char * format, ...)
	{
		std::string res;
		va_list args;
		va_start(args, format);
		AppendVPrintf(res, format, args);
		va_end(args);
		return res;
	}

	std::string ToLowerCase(const std::string& str)
	{
		std::string ret = str;
		std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
		return ret;
	}

	std::string ToUpperCase(const std::string& str)
	{
		std::string ret = str;
		std::transform(ret.begin(), ret.end(), ret.begin(), ::toupper);
		return ret;
	}

	std::string join(const AStringVector& vec, const std::string & delimeter)
	{
		if (vec.empty())
		{
			return {};
		}

		// Do a dry run to gather the size
		const auto DelimSize = delimeter.size();
		size_t ResultSize = vec.at(0).size();
		std::for_each(vec.begin() + 1, vec.end(),
			[&](const std::string & a_String)
		{
			ResultSize += DelimSize;
			ResultSize += a_String.size();
		}
		);

		// Now do the actual join
		std::string Result;
		Result.reserve(ResultSize);
		Result.append(vec.at(0));
		std::for_each(vec.begin() + 1, vec.end(),
			[&](const std::string & a_String)
		{
			Result += delimeter;
			Result += a_String;
		}
		);
		return Result;
	}

	json clone(const json& item)
	{
		return item;
	}
}


uint32_t setInterval(std::function<void(void)> func, int interval)
{
	return 0;
}

void clearInterval(uint32_t identifier)
{

}

#include <random>
#include <sstream>

std::string uuidv4()
{
	static std::random_device              rd;
	static std::mt19937                    gen(rd());
	static std::uniform_int_distribution<> dis(0, 15);
	static std::uniform_int_distribution<> dis2(8, 11);

	std::stringstream ss;
	int i;
	ss << std::hex;
	for (i = 0; i < 8; i++) {
		ss << dis(gen);
	}
	ss << "-";
	for (i = 0; i < 4; i++) {
		ss << dis(gen);
	}
	ss << "-4";
	for (i = 0; i < 3; i++) {
		ss << dis(gen);
	}
	ss << "-";
	ss << dis2(gen);
	for (i = 0; i < 3; i++) {
		ss << dis(gen);
	}
	ss << "-";
	for (i = 0; i < 12; i++) {
		ss << dis(gen);
	};
	return ss.str();
}
