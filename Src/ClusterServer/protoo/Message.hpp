#ifndef PROTOO_MESSAGE_HPP
#define PROTOO_MESSAGE_HPP

#include "common.hpp"

namespace protoo
{
	class Message
	{
	public:
		static Json requestFactory(std::string method, const Json& data);
		static Json successResponseFactory(const Json& request, const Json& data);
		static Json errorResponseFactory(const Json& request, int errorCode, std::string errorReason);
		static Json notificationFactory(std::string method, const Json& data);
	};
}

#endif

