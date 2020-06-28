#ifndef MS_COMMON_HPP
#define MS_COMMON_HPP

#include <algorithm>  // std::transform(), std::find(), std::min(), std::max()
#include <cinttypes>  // PRIu64, etc
#include <cstddef>    // size_t
#include <cstdint>    // uint8_t, etc
#include <functional> // std::function
#include <memory>     // std::addressof()
#ifdef _WIN32
#include <winsock2.h>
// https://stackoverflow.com/a/27443191/2085408
#undef max
#undef min
// avoid uv/win.h: error C2628 "intptr_t" followed by "int" is illegal.
#if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#define SSIZE_MAX INTPTR_MAX
#define _SSIZE_T_
#define _SSIZE_T_DEFINED
#endif
#else
#include <arpa/inet.h>  // htonl(), htons(), ntohl(), ntohs()
#include <netinet/in.h> // sockaddr_in, sockaddr_in6
#include <sys/socket.h> // struct sockaddr, struct sockaddr_storage, AF_INET, AF_INET6
#endif

// STL stuff:
#include <array>
#include <chrono>
#include <vector>
#include <list>
#include <deque>
#include <string>
#include <map>
#include <algorithm>
#include <memory>
#include <set>
#include <queue>
#include <limits>
#include <random>
#include <type_traits>
#include <atomic>
#include <mutex>
#include <thread>
#include <any>
#include <optional>
#include <variant>
#include <future>
#include <condition_variable>

#include <json.hpp>
using json = nlohmann::json;


#ifdef _MSC_VER
#  define MS_DECL_EXPORT __declspec(dllexport)
#endif

#if defined(MS_SHARED) || !defined(MS_STATIC)
#  ifdef MS_STATIC
#    error "Both MS_SHARED and MS_STATIC defined, please make up your mind"
#  endif
#  ifndef MS_SHARED
#    define MS_SHARED
#  endif
#  if defined(MS_BUILD_LIB)
#    define MS_EXPORT MS_DECL_EXPORT
#  endif
#else
#  define MS_EXPORT
#endif

#endif
