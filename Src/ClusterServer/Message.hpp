#ifndef PROTOO_MESSAGE_HPP
#define PROTOO_MESSAGE_HPP

#include "common.hpp"

namespace protoo
{
	class Message
	{
	public:
		static json requestFactory(std::string method, const json& data);
		static json successResponseFactory(const json& request, const json& data);
		static json errorResponseFactory(const json& request, int errorCode, std::string errorReason);
		static json notificationFactory(std::string method, const json& data);
	};
}

#endif

