#ifndef MS_RAINSTREAM_ERROR_HPP
#define MS_RAINSTREAM_ERROR_HPP

#include "Logger.hpp"
#include <cstdio> // std::snprintf()
#include <stdexcept>

class RainStreamError : public std::runtime_error
{
public:
	explicit RainStreamError(const char* description);
};

/* Inline methods. */

inline RainStreamError::RainStreamError(const char* description) : std::runtime_error(description)
{
}

#define RS_THROW_ERROR(desc, ...)                                                                  \
	do                                                                                               \
	{                                                                                                \
		MS_ERROR("throwing RainStreamError | " desc, ##__VA_ARGS__);                                    \
		static char buffer[2000];                                                                      \
		std::snprintf(buffer, 2000, desc, ##__VA_ARGS__);                                              \
		throw RainStreamError(buffer);                                                                  \
	} while (false)

#define RS_THROW_ERROR_STD(desc, ...)                                                              \
	do                                                                                               \
	{                                                                                                \
		MS_ERROR_STD("throwing RainStreamError | " desc, ##__VA_ARGS__);                                \
		static char buffer[2000];                                                                      \
		std::snprintf(buffer, 2000, desc, ##__VA_ARGS__);                                              \
		throw RainStreamError(buffer);                                                                  \
	} while (false)

#endif
