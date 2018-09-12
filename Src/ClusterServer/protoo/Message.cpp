#define MS_CLASS "protoo/Message"

#include "protoo/Message.hpp"

namespace protoo
{

	Json Message::requestFactory(std::string method, const Json& data)
	{
		Json request =
		{
			{ "request" , true },
			{ "id" , utils::randomNumber() },
			{ "method" , method },
			{ "data", data.is_null() ? Json::object() : data }
		};

		return request;
	}

	Json Message::successResponseFactory(const Json& request, const Json& data)
	{
		Json response =
		{
			{ "response", true },
			{ "id" , request["id"] },
			{ "ok" , true },
			{ "data" , data }
		};

		return response;
	}

	Json Message::errorResponseFactory(const Json& request, int errorCode, std::string errorReason)
	{
		Json response =
		{
			{ "response", true },
			{ "id" , request["id"] },
			{ "errorCode" , errorCode },
			{ "errorReason" , errorReason }
		};

		return response;
	}

	Json Message::notificationFactory(std::string method, const Json& data)
	{
		Json notification =
		{
			{ "notification" , true },
			{ "method" , method },
			{ "data" , data }
		};

		return notification;
	}
}
