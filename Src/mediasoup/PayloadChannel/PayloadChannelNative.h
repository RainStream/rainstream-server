#pragma once

#include "lib.hpp"
#include "PayloadChannel.h"

namespace mediasoup {

class PayloadChannelNative : public PayloadChannel
{
public:
	PayloadChannelNative();

	static PayloadChannelReadFreeFn payloadChannelReadFreeFn(
		uint8_t** message,
		uint32_t* messageLen,
		size_t* messageCtx,
		uint8_t** payload,
		uint32_t* payloadLen,
		size_t* payloadCapacity,
		// This is `uv_async_t` handle that can be called later with `uv_async_send()` when there is more
		// data to read.
		const void* handle,
		PayloadChannelReadCtx ctx)
	{

		//uv_async_send((uv_async_t*)handle);
		//return payloadChannelReadFreeFn;
		return nullptr;
	}

	static void payloadChannelReadFreeFn(uint8_t*, uint32_t, size_t)
	{

	}

	static void payloadChannelWriteFn(
		const uint8_t* message,
		uint32_t messageLen,
		const uint8_t* payload,
		uint32_t payloadLen,
		ChannelWriteCtx ctx)
	{
		std::string strMsg((const char*)message, messageLen);
		std::string strPayload((const char*)payload, payloadLen);

		printf("%s---->%s\n", strMsg.c_str(), strPayload.c_str());
	}

protected:
	virtual void subClose() override;

};

}
