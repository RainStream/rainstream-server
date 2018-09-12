#ifndef MS_UTILS_HPP
#define MS_UTILS_HPP

#include "common.hpp"
#include <cstring> // std::memcmp(), std::memcpy()
#include <string>
#include <random>
#include <regex>
#ifndef _WIN32
#include <sys/time.h> // gettimeofday
#endif
#include <assert.h>
#include <stdarg.h>

/** Evaluates to the number of elements in an array (compile-time!) */
#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))

namespace Utils
{
	class IP
	{
	public:
		static int GetFamily(const char* ip, size_t ipLen);
		static int GetFamily(const std::string& ip);
		static void GetAddressInfo(const struct sockaddr* addr, int* family, std::string& ip, uint16_t* port);
		static bool CompareAddresses(const struct sockaddr* addr1, const struct sockaddr* addr2);
		static struct sockaddr_storage CopyAddress(const struct sockaddr* addr);
	};

	/* Inline static methods. */

	inline int IP::GetFamily(const std::string& ip)
	{
		return GetFamily(ip.c_str(), ip.size());
	}

	inline bool IP::CompareAddresses(const struct sockaddr* addr1, const struct sockaddr* addr2)
	{
		// Compare family.
		if (addr1->sa_family != addr2->sa_family ||
		    (addr1->sa_family != AF_INET && addr1->sa_family != AF_INET6))
		{
			return false;
		}

		// Compare port.
		if (((struct sockaddr_in*)addr1)->sin_port != ((struct sockaddr_in*)addr2)->sin_port)
			return false;

		// Compare IP.
		switch (addr1->sa_family)
		{
			case AF_INET:
				if (std::memcmp(
				        &((struct sockaddr_in*)addr1)->sin_addr, &((struct sockaddr_in*)addr2)->sin_addr, 4) ==
				    0)
				{
					return true;
				}
				break;
			case AF_INET6:
				if (std::memcmp(
				        &((struct sockaddr_in6*)addr1)->sin6_addr,
				        &((struct sockaddr_in6*)addr2)->sin6_addr,
				        16) == 0)
				{
					return true;
				}
				break;
			default:
				return false;
		}

		return false;
	}

	inline struct sockaddr_storage IP::CopyAddress(const struct sockaddr* addr)
	{
		struct sockaddr_storage copiedAddr;

		switch (addr->sa_family)
		{
			case AF_INET:
				std::memcpy(&copiedAddr, addr, sizeof(struct sockaddr_in));
				break;
			case AF_INET6:
				std::memcpy(&copiedAddr, addr, sizeof(struct sockaddr_in6));
				break;
		}

		return copiedAddr;
	}

	class File
	{
	public:
		static void CheckFile(const char* file);
	};

	class Byte
	{
	public:
		/**
		 * Getters below get value in Host Byte Order.
		 * Setters below set value in Network Byte Order.
		 */
		static uint8_t Get1Byte(const uint8_t* data, size_t i);
		static uint16_t Get2Bytes(const uint8_t* data, size_t i);
		static uint32_t Get3Bytes(const uint8_t* data, size_t i);
		static uint32_t Get4Bytes(const uint8_t* data, size_t i);
		static uint64_t Get8Bytes(const uint8_t* data, size_t i);
		static void Set1Byte(uint8_t* data, size_t i, uint8_t value);
		static void Set2Bytes(uint8_t* data, size_t i, uint16_t value);
		static void Set3Bytes(uint8_t* data, size_t i, uint32_t value);
		static void Set4Bytes(uint8_t* data, size_t i, uint32_t value);
		static void Set8Bytes(uint8_t* data, size_t i, uint64_t value);
		static uint16_t PadTo4Bytes(uint16_t size);
		static uint32_t PadTo4Bytes(uint32_t size);
	};

	/* Inline static methods. */

	inline uint8_t Byte::Get1Byte(const uint8_t* data, size_t i)
	{
		return data[i];
	}

	inline uint16_t Byte::Get2Bytes(const uint8_t* data, size_t i)
	{
		return uint16_t{ data[i + 1] } | uint16_t{ data[i] } << 8;
	}

	inline uint32_t Byte::Get3Bytes(const uint8_t* data, size_t i)
	{
		return uint32_t{ data[i + 2] } | uint32_t{ data[i + 1] } << 8 | uint32_t{ data[i] } << 16;
	}

	inline uint32_t Byte::Get4Bytes(const uint8_t* data, size_t i)
	{
		return uint32_t{ data[i + 3] } | uint32_t{ data[i + 2] } << 8 | uint32_t{ data[i + 1] } << 16 |
		       uint32_t{ data[i] } << 24;
	}

	inline uint64_t Byte::Get8Bytes(const uint8_t* data, size_t i)
	{
		return uint64_t{ Byte::Get4Bytes(data, i) } << 32 | Byte::Get4Bytes(data, i + 4);
	}

	inline void Byte::Set1Byte(uint8_t* data, size_t i, uint8_t value)
	{
		data[i] = value;
	}

	inline void Byte::Set2Bytes(uint8_t* data, size_t i, uint16_t value)
	{
		data[i + 1] = static_cast<uint8_t>(value);
		data[i]     = static_cast<uint8_t>(value >> 8);
	}

	inline void Byte::Set3Bytes(uint8_t* data, size_t i, uint32_t value)
	{
		data[i + 2] = static_cast<uint8_t>(value);
		data[i + 1] = static_cast<uint8_t>(value >> 8);
		data[i]     = static_cast<uint8_t>(value >> 16);
	}

	inline void Byte::Set4Bytes(uint8_t* data, size_t i, uint32_t value)
	{
		data[i + 3] = static_cast<uint8_t>(value);
		data[i + 2] = static_cast<uint8_t>(value >> 8);
		data[i + 1] = static_cast<uint8_t>(value >> 16);
		data[i]     = static_cast<uint8_t>(value >> 24);
	}

	inline void Byte::Set8Bytes(uint8_t* data, size_t i, uint64_t value)
	{
		data[i + 7] = static_cast<uint8_t>(value);
		data[i + 6] = static_cast<uint8_t>(value >> 8);
		data[i + 5] = static_cast<uint8_t>(value >> 16);
		data[i + 4] = static_cast<uint8_t>(value >> 24);
		data[i + 3] = static_cast<uint8_t>(value >> 32);
		data[i + 2] = static_cast<uint8_t>(value >> 40);
		data[i + 1] = static_cast<uint8_t>(value >> 48);
		data[i]     = static_cast<uint8_t>(value >> 56);
	}

	inline uint16_t Byte::PadTo4Bytes(uint16_t size)
	{
		// If size is not multiple of 32 bits then pad it.
		if (size & 0x03)
			return (size & 0xFFFC) + 4;
		else
			return size;
	}

	inline uint32_t Byte::PadTo4Bytes(uint32_t size)
	{
		// If size is not multiple of 32 bits then pad it.
		if (size & 0x03)
			return (size & 0xFFFFFFFC) + 4;
		else
			return size;
	}

	class Crypto
	{
	public:
		static void ClassInit();
		static void ClassDestroy();
		static uint32_t GetRandomUInt(uint32_t min, uint32_t max);
		static const std::string GetRandomString(size_t len = 8);
		static uint32_t GetCRC32(const uint8_t* data, size_t size);

	private:
		static uint32_t seed;
		static uint8_t hmacSha1Buffer[];
		static const uint32_t crc32Table[256];
	};

	/* Inline static methods. */

	inline uint32_t Crypto::GetRandomUInt(uint32_t min, uint32_t max)
	{
		//平均分布
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(min, max);

		return dis(gen);
	}

	inline const std::string Crypto::GetRandomString(size_t len)
	{
		static char buffer[64];
		static const char chars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
			                            'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			                            'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };

		if (len > 64)
			len = 64;

		for (size_t i{ 0 }; i < len; ++i)
			buffer[i] = chars[GetRandomUInt(0, sizeof(chars) - 1)];

		return std::string(buffer, len);
	}

	inline uint32_t Crypto::GetCRC32(const uint8_t* data, size_t size)
	{
		uint32_t crc{ 0xFFFFFFFF };
		const uint8_t* p = data;

		while (size--)
			crc = Crypto::crc32Table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

		return crc ^ ~0U;
	}

	class String
	{
	public:
		static std::string ToLowerCase(const std::string& str);
		static std::string ToUpperCase(const std::string& str);
		static bool IsAllNum(const std::string &str);
		static std::string & AppendVPrintf(std::string & a_Dst, const char * format, va_list args);
		static std::string & Printf(std::string & a_Dst, const char * format, ...);
		static std::string Printf(const char * format, ...);
		static std::string & AppendPrintf(std::string & a_Dst, const char * format, ...);
		static AStringVector StringSplit(const std::string & str, const std::string & regex);
		static AStringVector SplitOneOf(const std::string& str, const std::string& delims, const size_t maxSplits = 0);
		/** Join a list of strings with the given delimiter between entries. */
		static std::string StringJoin(const AStringVector& a_Strings, const std::string& a_Delimiter);
		static std::string TrimString(const std::string & str);
		static void ReplaceString(std::string & iHayStack, const std::string & iNeedle, const std::string & iReplaceWith);
	};

	inline std::string String::ToLowerCase(const std::string& str)
	{
		std::string ret = str;
		std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
		return ret;
	}

	inline std::string String::ToUpperCase(const std::string& str)
	{
		std::string ret = str;
		std::transform(ret.begin(), ret.end(), ret.begin(), ::toupper);
		return ret;
	}

	inline bool String::IsAllNum(const std::string &str)
	{
		std::regex rx("[0-9]+");
		return std::regex_match(str.begin(), str.end(), rx);
	}

	inline std::string & String::AppendVPrintf(std::string & str, const char * format, va_list args)
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
		// MS CRT provides secure printf that doesn't behave like in the C99 standard
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

	inline std::string & String::Printf(std::string & str, const char * format, ...)
	{
		str.clear();
		va_list args;
		va_start(args, format);
		std::string & retval = AppendVPrintf(str, format, args);
		va_end(args);
		return retval;
	}

	inline std::string String::Printf(const char * format, ...)
	{
		std::string res;
		va_list args;
		va_start(args, format);
		AppendVPrintf(res, format, args);
		va_end(args);
		return res;
	}

	inline std::string & String::AppendPrintf(std::string & dst, const char * format, ...)
	{
		va_list args;
		va_start(args, format);
		std::string & retval = AppendVPrintf(dst, format, args);
		va_end(args);
		return retval;
	}

	inline AStringVector String::StringSplit(const std::string & str, const std::string & regex)
	{
		// passing -1 as the submatch index parameter performs splitting
		std::regex re(regex);
		std::sregex_token_iterator
			first{ str.begin(), str.end(), re, -1 },
			last;
		return std::vector<std::string>{ first, last };
	}

	inline AStringVector String::SplitOneOf(const std::string& str,
		const std::string& delims,
		const size_t maxSplits)
	{
		std::string remaining(str);
		AStringVector result;
		size_t splits = 0, pos;

		while (((maxSplits == 0) || (splits < maxSplits)) &&
			((pos = remaining.find_first_of(delims)) != std::string::npos)) {
			result.push_back(remaining.substr(0, pos));
			remaining = remaining.substr(pos + 1);
			splits++;
		}

		if (remaining.length() > 0)
			result.push_back(remaining);

		return result;
	}

	inline std::string String::StringJoin(const AStringVector & a_Strings, const std::string & a_Delimeter)
	{
		if (a_Strings.empty())
		{
			return{};
		}

		// Do a dry run to gather the size
		const auto DelimSize = a_Delimeter.size();
		size_t ResultSize = a_Strings[0].size();
		std::for_each(a_Strings.begin() + 1, a_Strings.end(),
			[&](const std::string & a_String)
		{
			ResultSize += DelimSize;
			ResultSize += a_String.size();
		}
		);

		// Now do the actual join
		std::string Result;
		Result.reserve(ResultSize);
		Result.append(a_Strings[0]);
		std::for_each(a_Strings.begin() + 1, a_Strings.end(),
			[&](const std::string & a_String)
		{
			Result += a_Delimeter;
			Result += a_String;
		}
		);
		return Result;
	}

	inline std::string String::TrimString(const std::string & str)
	{
		size_t len = str.length();
		size_t start = 0;
		while (start < len)
		{
			if (static_cast<unsigned char>(str[start]) > 32)
			{
				break;
			}
			++start;
		}
		if (start == len)
		{
			return "";
		}

		size_t end = len;
		while (end >= start)
		{
			if (static_cast<unsigned char>(str[end]) > 32)
			{
				break;
			}
			--end;
		}

		return str.substr(start, end - start + 1);
	}

	inline void String::ReplaceString(std::string & iHayStack, const std::string & iNeedle, const std::string & iReplaceWith)
	{
		// find always returns the current position for an empty needle; prevent endless loop
		if (iNeedle.empty())
		{
			return;
		}

		size_t pos1 = iHayStack.find(iNeedle);
		while (pos1 != std::string::npos)
		{
			iHayStack.replace(pos1, iNeedle.size(), iReplaceWith);
			pos1 = iHayStack.find(iNeedle, pos1 + iReplaceWith.size());
		}
	}


	class Time
	{
		// Seconds from Jan 1, 1900 to Jan 1, 1970.
		static constexpr uint32_t UnixNtpOffset{ 0x83AA7E80 };
		// NTP fractional unit.
		static constexpr double NtpFractionalUnit{ 1LL << 32 };

	public:
		struct Ntp
		{
			uint32_t seconds;
			uint32_t fractions;
		};

		static void CurrentTimeNtp(Ntp& ntp);
		static bool IsNewerTimestamp(uint32_t timestamp, uint32_t prevTimestamp);
		static uint32_t LatestTimestamp(uint32_t timestamp1, uint32_t timestamp2);
	};

	inline void Time::CurrentTimeNtp(Ntp& ntp)
	{
		struct timeval tv;

		gettimeofday(&tv, nullptr);

		ntp.seconds = tv.tv_sec + UnixNtpOffset;
		ntp.fractions =
		    static_cast<uint32_t>(static_cast<double>(tv.tv_usec) * NtpFractionalUnit * 1.0e-6);
	}

	inline bool Time::IsNewerTimestamp(uint32_t timestamp, uint32_t prevTimestamp)
	{
		// Distinguish between elements that are exactly 0x80000000 apart.
		// If t1>t2 and |t1-t2| = 0x80000000: IsNewer(t1,t2)=true,
		// IsNewer(t2,t1)=false
		// rather than having IsNewer(t1,t2) = IsNewer(t2,t1) = false.
		if (static_cast<uint32_t>(timestamp - prevTimestamp) == 0x80000000)
			return timestamp > prevTimestamp;

		return timestamp != prevTimestamp &&
		       static_cast<uint32_t>(timestamp - prevTimestamp) < 0x80000000;
	}

	inline uint32_t Time::LatestTimestamp(uint32_t timestamp1, uint32_t timestamp2)
	{
		return IsNewerTimestamp(timestamp1, timestamp2) ? timestamp1 : timestamp2;
	}

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
}

#endif
