#pragma once

#include "PipeStreamSocket.hpp"
#include <EventEmitter.hpp>

class Socket : public PipeStreamSocket, public EventEmitter
{
public:
	class Listener
	{
	public:
		virtual void OnChannelPipeStreamSocketRemotelyClosed(Socket* socket) = 0;
	};

public:
	explicit Socket();

private:
	~Socket() override;

public:
	void SetListener(Listener* listener);

	/* Pure virtual methods inherited from ::PipeStreamSocket. */
public:
	void UserOnPipeStreamRead() override;
	void UserOnPipeStreamSocketClosed(bool isClosedByPeer) override;

protected:
	void processMessage(const std::string& msg);

private:
	// Passed by argument.
	Listener * listener{ nullptr };
	// Others.
	size_t msgStart{ 0 }; // Where the latest message starts.
	bool closed{ false };

	Logger* logger;
};
