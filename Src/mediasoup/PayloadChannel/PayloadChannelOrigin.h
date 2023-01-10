#pragma once

#include "PayloadChannel.h"

namespace mediasoup {

class Socket;

class PayloadChannelOrigin : public PayloadChannel
{
public:
	PayloadChannelOrigin(Socket* producerSocket, Socket* consumerSocket);

protected:
	virtual void subClose() override;

protected:
	// Unix Socket instance for sending messages to the worker process.
	Socket* _producerSocket;

	// Unix Socket instance for receiving messages to the worker process.
	Socket* _consumerSocket;
};

}
