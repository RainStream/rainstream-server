#define MS_CLASS "Channel::Request"
// #define MS_LOG_DEV

#include "protoo/Request.hpp"
#include "protoo/Peer.hpp"
#include "Logger.hpp"
#include "RainStreamError.hpp"

namespace protoo
{
	/* Class variables. */

	// clang-format off
	std::unordered_map<std::string, Request::MethodId> Request::string2MethodId =
	{
		{ "mediasoup-request",                Request::MethodId::MEDIASOUP_REQUEST       },
		{ "mediasoup-notification",           Request::MethodId::MEDIASOUP_NOTIFICATION  },
		{ "change-display-name",              Request::MethodId::CHANGE_DISPLAY_NAME     }
	};
	// clang-format on

	/* Instance methods. */

	Request::Request(Peer* peer, Json& json) : peer(peer)
	{
		MS_TRACE();

		static const std::string JsonStringId{ "id" };
		static const std::string JsonStringMethod{ "method" };
		static const std::string JsonStringData{ "data" };

		if (json[JsonStringId].is_number_unsigned())
			this->id = json[JsonStringId].get<uint32_t>();
		else
			RS_THROW_ERROR("json has no numeric id field");

		if (json[JsonStringMethod].is_string())
			this->method = json[JsonStringMethod].get<std::string>();
		else
			RS_THROW_ERROR("json has no string .method field");

		auto it = Request::string2MethodId.find(this->method);

		if (it != Request::string2MethodId.end())
		{
			this->methodId = it->second;
		}
		else
		{
			RS_THROW_ERROR("unknown request.method '%s'", this->method.c_str());
		}

		if (json[JsonStringData].is_object())
			this->data = json[JsonStringData];
		else
			this->data = Json::object();
	}

	Request::~Request()
	{
		MS_TRACE();
	}

	void Request::Accept()
	{
		MS_TRACE();

		static Json emptyData(Json::object());

		Accept(emptyData);
	}

	void Request::Accept(Json& data)
	{
		MS_TRACE();

		static Json emptyData(Json::object());
		static const std::string JsonStringId{ "id" };
		static const std::string JsonStringAccepted{ "accepted" };
		static const std::string JsonStringData{ "data" };

		MS_ASSERT(!this->replied, "Request already replied");

		this->replied = true;

		Json json(Json::object());

		json[JsonStringId]       = this->id;
		json[JsonStringAccepted] = true;

		if (data.is_object() || data.is_array())
			json[JsonStringData] = data;
		else
			json[JsonStringData] = emptyData;

		this->peer->Send(json);
	}

	void Request::Reject(int code, std::string& reason)
	{
		MS_TRACE();

		Reject(code, reason.c_str());
	}

	/**
	 * Reject the Request.
	 * @param reason  Description string.
	 */
	void Request::Reject(int code, const char* reason)
	{
		MS_TRACE();

		static const std::string JsonStringId{ "id" };
		static const std::string JsonStringRejected{ "rejected" };
		static const std::string JsonStringReason{ "reason" };

		MS_ASSERT(!this->replied, "Request already replied");

		this->replied = true;

		Json json(Json::object());

		json[JsonStringId]       =  this->id;
		json[JsonStringRejected] = true;

		if (reason != nullptr)
			json[JsonStringReason] = reason;

		this->peer->Send(json);
	}
} // namespace protoo
