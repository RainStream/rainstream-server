#pragma once

#include <stdexcept>

namespace rs
{
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

	private:
		std::string name;
	};

	class TypeError :public Error
	{
	public:
		TypeError(std::string message)
			: Error(message)
		{
			setName("TypeError");
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
}
