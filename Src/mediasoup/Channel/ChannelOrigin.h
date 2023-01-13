#pragma once

#include "Channel.h"

namespace mediasoup {

class Socket;

class ChannelOrigin : public Channel
{
public:
	ChannelOrigin(Socket* producerSocket, Socket* consumerSocket, int pid);

	virtual async_simple::coro::Lazy<json> request(std::string method, std::optional<std::string> handlerId = std::nullopt, const json& data = json());

protected:
	virtual void subClose() override;

protected:
	// Unix Socket instance for sending messages to the worker process.
	Socket* _producerSocket{ nullptr };
	// Unix Socket instance for receiving messages to the worker process.
	Socket* _consumerSocket{ nullptr };
};

}
