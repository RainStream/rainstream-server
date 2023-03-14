#pragma once

#include <random>
#include <string>
#include <vector>

using AStringVector = std::vector<std::string>;

/** Evaluates to the uint32_t of elements in an array (compile-time!) */
#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);                \
    TypeName& operator=(const TypeName&)


namespace Utils
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

	MS_EXPORT std::string & AppendPrintf(std::string & dst, const char * format, ...);

	MS_EXPORT std::string Printf(const char * format, ...);

	MS_EXPORT std::string ToLowerCase(const std::string& str);

	MS_EXPORT std::string ToUpperCase(const std::string& str);

	MS_EXPORT std::string join(const AStringVector& vec, const std::string & delimeter);

	MS_EXPORT json clone(const json& item);

	class Json
	{
	public:
		static bool IsPositiveInteger(const json& value)
		{
			if (value.is_number_unsigned())
				return true;
			else if (value.is_number_integer())
				return value.get<int64_t>() >= 0;
			else
				return false;
		}
	};

	class Byte
	{
	public:
		static uint32_t Get4Bytes(const uint8_t* data, size_t i)
		{
			return uint32_t{ data[i] } | uint32_t{ data[i + 1] } << 8 |
				uint32_t{ data[i + 2] } << 16 | uint32_t{ data[i + 3] } << 24;
		}
	};
}


MS_EXPORT std::string uuidv4();

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

class MS_EXPORT MessageData {
public:
	MessageData() {}
	virtual ~MessageData() {}

	virtual void Run() = 0;

	bool once = false;
};

template <class FunctorT>
class MessageWithFunctor : public MessageData {
public:
	explicit MessageWithFunctor(FunctorT&& functor)
		: functor_(std::forward<FunctorT>(functor)) {}

	void Run() override { functor_(); }

private:
	~MessageWithFunctor() {}

	typename std::remove_reference<FunctorT>::type functor_;

	DISALLOW_COPY_AND_ASSIGN(MessageWithFunctor);
};

void newCheckInvoke(MessageData* message_data);

MS_EXPORT uint64_t Invoke(MessageData* message_data,
	uint64_t timeout = 0,
	uint64_t repeat = 0);

MS_EXPORT void InvokeOnce(MessageData* message_data,
	uint64_t timeout = 0,
	uint64_t repeat = 0);

template <class FunctorT>
void setImmediate(FunctorT&& functor) {
	newCheckInvoke(new MessageWithFunctor<FunctorT>(
		std::forward<FunctorT>(functor)));
}

template <class FunctorT>
void setTimeout(FunctorT&& functor, int timeout) {
	InvokeOnce(new MessageWithFunctor<FunctorT>(
		std::forward<FunctorT>(functor)), timeout);
}

template <class FunctorT>
uint64_t setInterval(FunctorT&& functor, int interval) {
	return Invoke(new MessageWithFunctor<FunctorT>(
		std::forward<FunctorT>(functor)), interval, interval);
}

MS_EXPORT void clearInterval(uint64_t identifier);

MS_EXPORT void loadGlobalCheck();
