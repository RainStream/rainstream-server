#define MS_CLASS "mediasoup-worker"
// #define MS_LOG_DEV_LEVEL 3

#include "MediaSoupErrors.hpp"
#include "lib.hpp"
#include <cstdlib> // std::_Exit(), std::genenv()
#include <string>


#define MEDIASOUP_VERSION "3.10.6"

class ChannelNative
{
public:
	static ChannelReadFreeFn channelReadFn(uint8_t**  message ,
		uint32_t* messageLen,
		size_t* messageCtx,
		// This is `uv_async_t` handle that can be called later with `uv_async_send()` when there is more
		// data to read.
		const void* handle,
		ChannelReadCtx ctx)
	{

		ChannelNative* pThis = (ChannelNative*)ctx;

		//return channelReadFreeFn;
		return nullptr;
	}

	static void channelReadFreeFn(uint8_t*, uint32_t, size_t)
	{

	}

	static void channelWriteFn(const uint8_t* message, uint32_t messageLen, ChannelWriteCtx /* ctx */)
	{
		std::string strMsg((const char*)message, messageLen);

		printf("channelWriteFn:%s\n", strMsg.c_str());
	}
};

class PayloadChannelNative
{
public:
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
};


int main(int argc, char* argv[])
{
	std::string version = MEDIASOUP_VERSION;

	ChannelNative* channelNative = new ChannelNative;
	PayloadChannelNative* payloadChannelNative = new PayloadChannelNative;

	auto statusCode = mediasoup_worker_run(
		argc,
		argv,
		version.c_str(),
		0,
		0,
		0,
		0,
		ChannelNative::channelReadFn,
		channelNative,
		ChannelNative::channelWriteFn,
		channelNative,
		PayloadChannelNative::payloadChannelReadFreeFn,
		payloadChannelNative,
		PayloadChannelNative::payloadChannelWriteFn,
		payloadChannelNative);

	switch (statusCode)
	{
		case 0:
			std::_Exit(EXIT_SUCCESS);
		case 1:
			std::_Exit(EXIT_FAILURE);
		case 42:
			std::_Exit(42);
	}

	while (true)
	{

	}
}
