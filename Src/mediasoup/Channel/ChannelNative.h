#pragma once

#include <uv.h>
#include "lib.hpp"
#include "Channel.h"

namespace mediasoup {

class ChannelNative : public Channel
{
public:
	ChannelNative();

	static ChannelReadFreeFn channelReadFn(uint8_t** message,
		uint32_t* messageLen,
		size_t* messageCtx,
		// This is `uv_async_t` handle that can be called later with `uv_async_send()` when there is more
		// data to read.
		const void* handle,
		ChannelReadCtx ctx)
	{

		ChannelNative* pThis = (ChannelNative*)ctx;

		//return channelReadFreeFn;
		return pThis->ProduceMessage(message, messageLen, messageCtx, handle) ? nullptr : nullptr;
	}

	static void channelReadFreeFn(uint8_t*, uint32_t, size_t)
	{

	}

	static void channelWriteFn(const uint8_t* message, uint32_t messageLen, ChannelWriteCtx /* ctx */)
	{
		std::string strMsg((const char*)message, messageLen);

		printf("channelWriteFn:%s\n", strMsg.c_str());
	}

	bool ProduceMessage(uint8_t** message, uint32_t* messageLen, size_t* messageCtx, const void* handle)
	{

		this->_handle = reinterpret_cast<uv_async_t*>(const_cast<void*>(handle));
		return false;
	}

	void sendMessage(std::string msg)
	{
		if (_handle)
		{
			uv_async_send(_handle);
		}
	}

	virtual std::future<json> request(std::string method, std::optional<std::string> handlerId = std::nullopt, const json& data = json());

protected:
	virtual void subClose() override;

protected:
	uv_async_t* _handle;
};

}
