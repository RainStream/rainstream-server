#ifndef MS_COMMON_HPP
#define MS_COMMON_HPP

#include <string>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <set>
#include <unordered_map>
#include <memory> // std::addressof()
#include <algorithm> // std::transform(), std::find(), std::min(), std::max()
#include <cstddef> // size_t
#include <cstdint> // uint8_t, etc
#include <cinttypes> // PRIu64, etc

#define __STDC__ 1

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#define usleep(x) ::Sleep(x/10000)
#else
#include <sys/socket.h> // struct sockaddr, struct sockaddr_storage, AF_INET, AF_INET6
#include <netinet/in.h> // sockaddr_in, sockaddr_in6
#include <arpa/inet.h> // htonl(), htons(), ntohl(), ntohs()
#endif

// Try to determine endianness:
#if ( \
	defined(__i386__) || defined(__alpha__) || \
	defined(__ia64) || defined(__ia64__) || \
	defined(_M_IX86) || defined(_M_IA64) || \
	defined(_M_ALPHA) || defined(__amd64) || \
	defined(__amd64__) || defined(_M_AMD64) || \
	defined(__x86_64) || defined(__x86_64__) || \
	defined(_M_X64) || defined(__bfin__) || \
	defined(__ARMEL__) || \
	(defined(_WIN32) && defined(__ARM__) && defined(_MSC_VER)) \
)
#define MS_LITTLE_ENDIAN
#elif ( \
	defined (__ARMEB__) || defined(__sparc) || defined(__powerpc__) || defined(__POWERPC__) \
)
#define MS_BIG_ENDIAN
#else
#error Cannot determine endianness of this platform
#endif

#define MS_APP_NAME "MediaSoup"
#define MS_PROCESS_NAME "mediasoup-worker"

#ifdef _WIN32
extern int gettimeofday(struct timeval *tp, void *tzp);
#endif

#include <RainStream/RainStreamInc.hpp>

using AcceptFunc = std::function<void(Json json)>;
using RejectFunc = std::function<void(int, std::string)>;

#endif
