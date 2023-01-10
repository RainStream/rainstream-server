#define MSC_CLASS "WorkerNative"

#include "common.h"
#include "WorkerNative.h"
#include "Channel/ChannelNative.h"
#include "PayloadChannel/PayloadChannelNative.h"

namespace mediasoup {

WorkerNative::WorkerNative(json settings)
	: Worker(settings)
{
	
}

WorkerNative::~WorkerNative()
{
	if (_channel)
	{
		delete _channel;
		_channel = nullptr;
	}

	if (_payloadChannel)
	{
		delete _payloadChannel;
		_payloadChannel = nullptr;
	}
}

void WorkerNative::init(AStringVector spawnArgs)
{
	_channel = new ChannelNative;
	_payloadChannel = new PayloadChannelNative;

	std::vector<char*> vecArgs;
	for (std::string spawnArg : spawnArgs)
	{
		vecArgs.push_back(strdup(spawnArg.c_str()));
	}

	_work_thread = std::thread([=]() {
		auto statusCode = mediasoup_worker_run(
			vecArgs.size(),
			(char**)vecArgs.data(),
			__MEDIASOUP_VERSION__,
			0,
			0,
			0,
			0,
			ChannelNative::channelReadFn,
			_channel,
			ChannelNative::channelWriteFn,
			_channel,
			PayloadChannelNative::payloadChannelReadFreeFn,
			_payloadChannel,
			PayloadChannelNative::payloadChannelWriteFn,
			_payloadChannel);

		switch (statusCode)
		{
		case 0:
			std::_Exit(EXIT_SUCCESS);
		case 1:
			std::_Exit(EXIT_FAILURE);
		case 42:
			std::_Exit(42);
		}
	});
}

void WorkerNative::subClose()
{

}

}
