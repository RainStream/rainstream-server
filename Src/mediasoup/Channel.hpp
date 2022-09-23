#pragma once

#include <unordered_map>
#include "EnhancedEventEmitter.hpp"

class Socket;

class Channel : public EnhancedEventEmitter
{
public:
	Channel(Socket* producerSocket, Socket* consumerSocket, int pid);

	void close();

	std::future<json> request(std::string method, std::optional<std::string> handlerId = std::nullopt, const json& data = json());

protected:
	void _processMessage(const json& msg);

private:
	std::unordered_map<uint32_t, std::promise<json> > _sents;

	// Closed flag.
	bool _closed = false;

	Socket* _producerSocket{ nullptr };
	Socket* _consumerSocket{ nullptr };

	// Next id for messages sent to the worker process.
	int32_t _nextId = 0;
};
