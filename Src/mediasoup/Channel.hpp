#pragma once

#include <unordered_map>
#include "EnhancedEventEmitter.hpp"

class Socket;

class Channel : public EnhancedEventEmitter
{
public:
	Channel(Socket* producerSocket, Socket* consumerSocket, int pid);

	void close();

	cppcoro::task<json> request(std::string method, std::optional<std::string> handlerId = std::nullopt, const json& data = json());

protected:
	void _processMessage(const json& msg);

	struct SendEvent
	{
		std::string method;
		std::function<void(const json&)> pResolve;
		std::function<void(const std::exception_ptr&)> pReject;
	};

	using SendEventPtr = std::shared_ptr<SendEvent>;

private:
	// Closed flag.
	bool _closed = false;
	// Unix Socket instance for sending messages to the worker process.
	Socket* _producerSocket{ nullptr };
	// Unix Socket instance for receiving messages to the worker process.
	Socket* _consumerSocket{ nullptr };
	// Next id for messages sent to the worker process.
	int32_t _nextId = 0;
	// Map of pending sent requests.
	std::unordered_map<uint32_t, SendEventPtr> _sents;
};
