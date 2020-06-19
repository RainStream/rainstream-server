#pragma once

#include <stdexcept>


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
