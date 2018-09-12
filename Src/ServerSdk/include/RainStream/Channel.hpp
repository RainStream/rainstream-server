#pragma once

#include <netstring.h>
#include <unordered_map>
#include "utils.hpp"
#include "errors.hpp"

namespace rs
{
	class Logger;
	class Socket;
	class Request;
	class EventListener;

	class Channel
	{
	public:
		Channel(Socket* socket);

		void close();

		void request(Request* request);
		Defer request(std::string method, const Json& internal = Json::object(), const Json& data = Json::object());

		void addEventListener(uint32_t id, EventListener* listener);
		void off(uint32_t id);

	protected:
		void _processMessage(const Json& msg);

		std::string _makePayload(const Json& msg);

	private:
		std::unordered_map<uint32_t, Request*> _pendingRequest;
		std::unordered_map<uint32_t, EventListener*> _eventListeners;

		Socket* _socket{ nullptr };

		std::unique_ptr<Logger> logger;
		std::unique_ptr<Logger> workerLogger;
	};
}
