
#ifndef MEDIA_BOOT_HPP
#define MEDIA_BOOT_HPP

#include "common.h"
#include <EnhancedEventEmitter.h>

namespace mediasoup{
class Router;
class Transport;
class DataProducer;
class DataConsumer;
}

using namespace mediasoup;

namespace protoo
{
	class Peer;
}

class Bot
{
public:
	static async_simple::coro::Lazy<Bot*> create(Router* mediasoupRouter);

	Bot(Transport* transport, DataProducer* dataProducer);

	DataProducer* dataProducer();

	void close();

	async_simple::coro::Lazy<void> handlePeerDataProducer(std::string dataProducerId, protoo::Peer* peer);

private:
	// mediasoup DirectTransport.
	Transport* _transport{ nullptr };

	// mediasoup DataProducer.
	DataProducer* _dataProducer{ nullptr };
};

#endif

