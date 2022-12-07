#ifndef PROTOO_MESSAGE_HPP
#define PROTOO_MESSAGE_HPP

#include "common.h"

namespace protoo
{
	class Message
	{
	public:
		static json createRequest(std::string method, const json& data);
		static json createSuccessResponse(const json& request, const json& data);
		static json createErrorResponse(const json& request, int errorCode, std::string errorReason);
		static json createNotification(std::string method, const json& data);
	};
}

#endif

