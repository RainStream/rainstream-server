#define MSC_CLASS "Request"

#include "Request.hpp"
#include "Peer.hpp"
#include "Message.hpp"
#include <Logger.hpp>
#include <errors.hpp>
#include "Utils.hpp"

namespace protoo
{
	/* Instance methods. */

	Request::Request(WebSocketClient* client, json& jsonRequest)
		: client(client)
	{
		MSC_TRACE();

		auto jsonIdIt = jsonRequest.find("id");

		if (jsonIdIt == jsonRequest.end() || !Utils::Json::IsPositiveInteger(*jsonIdIt))
			MSC_THROW_ERROR("missing id");

		this->id = jsonIdIt->get<uint32_t>();

		auto jsonMethodIt = jsonRequest.find("method");

		if (jsonMethodIt == jsonRequest.end() || !jsonMethodIt->is_string())
			MSC_THROW_ERROR("missing method");

		this->method = jsonMethodIt->get<std::string>();


		auto jsonRoomIdIt = jsonRequest.find("roomId");

		if (jsonRoomIdIt == jsonRequest.end() || !jsonRoomIdIt->is_string())
			MSC_THROW_ERROR("missing roomId");

		this->roomId = jsonRoomIdIt->get<std::string>();


		auto jsonPeerIdIt = jsonRequest.find("peerId");

		if (jsonPeerIdIt == jsonRequest.end() || !jsonPeerIdIt->is_string())
			MSC_THROW_ERROR("missing peerId");

		this->peerId = jsonPeerIdIt->get<std::string>();


		auto jsonDataIt = jsonRequest.find("data");

		if (jsonDataIt != jsonRequest.end() && jsonDataIt->is_object())
			this->data = *jsonDataIt;
		else
			this->data = json::object();
	}

	Request::~Request()
	{
		MSC_TRACE();
	}

	void Request::Accept()
	{
		MSC_TRACE();

		MSC_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json jsonResponse = json::object();

		jsonResponse["id"] = this->id;
		jsonResponse["accepted"] = true;

		this->client->Send(jsonResponse);
	}

	void Request::Accept(const json& data)
	{
		MSC_TRACE();

		MSC_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json jsonResponse = {
			{ "response", true },
			{ "id" , id },
			{ "method" , method },
			{ "peerId" , peerId },
			{ "roomId" , roomId },
			{ "ok" , true },
			{ "data" , data }
		};

		this->client->Send(jsonResponse);
	}

	void Request::Reject(int errorCode, std::string errorReason)
	{
		MSC_TRACE();

		MSC_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json jsonResponse = {
			{ "response", true },
			{ "id" , id },
			{ "method" , method },
			{ "peerId" , peerId },
			{ "roomId" , roomId },
			{ "errorCode" , errorCode },
			{ "errorReason" , errorReason }
		};

		this->client->Send(jsonResponse);
	}

	void Request::Ignore()
	{
		MSC_TRACE();

		MSC_ASSERT(!this->replied, "request already replied");

		this->replied = true;
	}
}
