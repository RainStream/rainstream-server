#pragma once

#include <netstring.h>
#include <unordered_map>
#include "utils.hpp"
#include "errors.hpp"

namespace rs
{
	class Socket;
	class ChannelListener;

	class Channel
	{
	public:
		Channel(Socket* producerSocket, Socket* consumerSocket, int pid);

		void close();

		Defer request(std::string method, const json& internal = json::object(), const json& data = json::object());

		void addEventListener(std::string id, ChannelListener* listener);
		void off(std::string id);

	protected:
		void _processMessage(const json& msg);

		std::string _makePayload(const json& msg);

	private:
		std::unordered_map<uint32_t, Defer> _sents;
		std::unordered_map<std::string, ChannelListener*> _eventListeners;

		// Closed flag.
		bool _closed = false;

		Socket* _producerSocket{ nullptr };
		Socket* _consumerSocket{ nullptr };

		// Next id for messages sent to the worker process.
		int32_t _nextId = 0;

	};
}
