#define MSC_CLASS "Message"

#include "Message.hpp"
#include <Utils.hpp>

namespace protoo
{

	json Message::createRequest(std::string method, const json& data)
	{
		json request =
		{
			{ "request" , true },
			{ "id" , Utils::generateRandomNumber() },
			{ "method" , method },
			{ "data", data.is_null() ? json::object() : data }
		};

		return request;
	}

	json Message::createSuccessResponse(const json& request, const json& data)
	{
		json response =
		{
			{ "response", true },
			{ "id" , request["id"] },
			{ "ok" , true },
			{ "data" , data }
		};

		return response;
	}

	json Message::createErrorResponse(const json& request, int errorCode, std::string errorReason)
	{
		json response =
		{
			{ "response", true },
			{ "id" , request["id"] },
			{ "errorCode" , errorCode },
			{ "errorReason" , errorReason }
		};

		return response;
	}

	json Message::createNotification(std::string method, const json& data)
	{
		json notification =
		{
			{ "notification" , true },
			{ "method" , method },
			{ "data" , data }
		};

		return notification;
	}
}
