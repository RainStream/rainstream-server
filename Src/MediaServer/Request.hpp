#pragma once

#include <common.hpp>
#include <unordered_map>

namespace protoo
{
	class WebSocketClient;

	class Request
	{
	public:
		Request(WebSocketClient* client, json& jsonRequest);
		virtual ~Request();

		void Accept();
		void Accept(json& data);
		void Reject(int errorCode, std::string errorReason);

	public:
		// Passed by argument.
		WebSocketClient* client{ nullptr };
		uint32_t id{ 0u };
		std::string method;
		std::string roomId;
		std::string peerId;
		json internal;
		json data;
		// Others.
		bool replied{ false };
	};

}
