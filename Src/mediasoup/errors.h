#pragma once

#include <stdexcept>

namespace mediasoup {

class Error : public std::runtime_error
{
public:
	Error(std::string message)
		: std::runtime_error(message.c_str())
	{
		setName("Error");
	}

	void setName(std::string name)
	{
		this->name = name;
	}

	std::string ToString() const
	{
		return "[" + name + "] " + what();
	}

protected:
	std::string name;
};

class TypeError : public Error
{
public:
	TypeError(std::string message)
		: Error(message)
	{
		setName("TypeError");
	}
};

/**
* Error indicating not support for something.
*/
class UnsupportedError : public Error
{
public:
	UnsupportedError(std::string message)
		: Error(message)
	{
		this->name = "UnsupportedError";

		// 			if (Error.hasOwnProperty("captureStackTrace")) // Just in V8.
		// 				Error.captureStackTrace(this, UnsupportedError);
		// 			else
		// 				this.stack = (new Error(message)).stack;
	}
};

/**
 * Error produced when calling a method in an invalid state.
 */
class InvalidStateError : public Error
{
public:
	InvalidStateError(std::string message)
		: Error(message)
	{
		this->name = "InvalidStateError";

		// 			if (Error.hasOwnProperty("captureStackTrace")) // Just in V8.
		// 				Error.captureStackTrace(this, InvalidStateError);
		// 			else
		// 				this.stack = (new Error(message)).stack;
	}
};


namespace errors
{
	inline Error createErrorClass(std::string message)
	{
		Error error(message);
		error.setName("InvalidStateError");
		return error;
	}

	inline Error InvalidStateError(std::string message)
	{
		return createErrorClass(message);
	}
}

// clang-format off
#define MSC_THROW_ERROR(desc, ...) \
	do \
	{ \
		MSC_ERROR("throwing Error: " desc, ##__VA_ARGS__); \
		\
		static char buffer[2000]; \
		\
		std::snprintf(buffer, 2000, desc, ##__VA_ARGS__); \
		throw Error(buffer); \
	} while (false)

#define MSC_THROW_TYPE_ERROR(desc, ...) \
	do \
	{ \
		MSC_ERROR("throwing TypeError: " desc, ##__VA_ARGS__); \
		\
		static char buffer[2000]; \
		\
		std::snprintf(buffer, 2000, desc, ##__VA_ARGS__); \
		throw TypeError(buffer); \
	} while (false)

#define MSC_THROW_UNSUPPORTED_ERROR(desc, ...) \
	do \
	{ \
		MSC_ERROR("throwing UnsupportedError: " desc, ##__VA_ARGS__); \
		\
		static char buffer[2000]; \
		\
		std::snprintf(buffer, 2000, desc, ##__VA_ARGS__); \
		throw UnsupportedError(buffer); \
	} while (false)

#define MSC_THROW_INVALID_STATE_ERROR(desc, ...) \
	do \
	{ \
		MSC_ERROR("throwing InvalidStateError: " desc, ##__VA_ARGS__); \
		\
		static char buffer[2000]; \
		\
		std::snprintf(buffer, 2000, desc, ##__VA_ARGS__); \
		throw InvalidStateError(buffer); \
	} while (false)

}
