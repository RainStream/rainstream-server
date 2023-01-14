#pragma once

#include <uv.h>
#include "lib.hpp"
#include "Channel.h"

namespace mediasoup {

class ChannelNative : public Channel
{
protected:
	struct RequestMessage
	{
		uint32_t id;
		std::string request;
	};

public:
	ChannelNative();

	static ChannelReadFreeFn channelReadFn(uint8_t** message,
		uint32_t* messageLen,
		size_t* messageCtx,
		// This is `uv_async_t` handle that can be called later with `uv_async_send()` when there is more
		// data to read.
		const void* handle,
		ChannelReadCtx ctx);

	static void channelReadFreeFn(uint8_t*, uint32_t, size_t);

	static void channelWriteFn(const uint8_t* message, uint32_t messageLen, ChannelWriteCtx /* ctx */);

	bool ProduceMessage(uint8_t** message, uint32_t* messageLen, size_t* messageCtx, const void* handle);

	void SendRequestMessage(uint32_t id);
	
	void ReceiveMessage(const std::string& message);

	bool CallbackWrite();

	virtual async_simple::coro::Lazy<json> request(std::string method, std::optional<std::string> handlerId = std::nullopt, const json& data = json());

protected:
	virtual void subClose() override;

protected:
	uv_async_t* _uvReadHandle{ nullptr };
	uv_async_t* _uvWriteHandle{ nullptr };

	std::queue<std::string> _receiveMessageQueue;
	std::queue<RequestMessage*> _requestMessageQueue;

};

}
