
#include "common.h"
#include "utils.h"
#include <random>
#include <sstream>
#include <uv.h>

namespace Utils
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

#include <map>
#include <queue>

static std::queue<MessageData*> checkMessageDatas;
static std::map<uint64_t, uv_timer_t*> timeInterval;

static void checkCB(uv_check_t* handle)
{
	while (checkMessageDatas.size())
	{
		MessageData* message_data = checkMessageDatas.front();

		if (message_data)
		{
			message_data->Run();

			delete message_data;
		}

		checkMessageDatas.pop();
	}

}

void newCheckInvoke(MessageData* message_data)
{
	checkMessageDatas.push(message_data);
}

void InvokeCb(uv_timer_t* handle)
{
	MessageData* message_data = static_cast<MessageData*>(handle->data);

	if (message_data)
	{
		message_data->Run();

		if (message_data->once)
		{
			delete message_data;

			uv_timer_stop(handle);
			uv_close((uv_handle_t*)handle, nullptr);
		}
	}
}

uint64_t Invoke(MessageData* message_data,
	uint64_t timeout,
	uint64_t repeat)
{
	uv_timer_t* timer_req = new uv_timer_t;
	timer_req->data = message_data;

	uv_timer_init(uv_default_loop(), timer_req);
	uv_timer_start(timer_req, InvokeCb, timeout, repeat);

	if (!message_data->once)
	{
		timeInterval.insert(std::pair(timer_req->start_id, timer_req));
	}

	return timer_req->start_id;
}

void InvokeOnce(MessageData* message_data,
	uint64_t timeout,
	uint64_t repeat)
{
	message_data->once = true;
	Invoke(message_data, timeout, repeat);
}

void clearInterval(uint64_t identifier)
{
	if (timeInterval.contains(identifier))
	{
		uv_timer_t* timer_req = timeInterval[identifier];
		uv_timer_stop(timer_req);
		uv_close((uv_handle_t*)timer_req, nullptr);

		timeInterval.erase(identifier);
	}
}

void loadGlobalCheck()
{
	uv_check_t* check_req = new uv_check_t;

	uv_check_init(uv_default_loop(), check_req);
	uv_check_start(check_req, checkCB);
}

