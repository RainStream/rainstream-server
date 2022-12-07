
#ifndef MEDIA_BOOT_HPP
#define MEDIA_BOOT_HPP

#include "common.h"
#include <EnhancedEventEmitter.h>

class Router;
class Transport;
class DataProducer;
class DataConsumer;

namespace protoo
{
	class Peer;
}

class Bot
{
public:
	static std::future<Bot*> create(Router* mediasoupRouter);

	Bot(Transport* transport, DataProducer* dataProducer);

	DataProducer* dataProducer();

	void close();

	std::future<void> handlePeerDataProducer(std::string dataProducerId, protoo::Peer* peer);

private:
	// mediasoup DirectTransport.
	Transport* _transport{ nullptr };

	// mediasoup DataProducer.
	DataProducer* _dataProducer{ nullptr };
};

#endif

