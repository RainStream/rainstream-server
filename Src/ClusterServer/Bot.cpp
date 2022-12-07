#define MSC_CLASS "Bot"

#include "Bot.hpp"
#include "Room.hpp"
#include "config.hpp"
#include "Peer.hpp"
#include <Logger.hpp>
#include <Router.hpp>
#include <Transport.hpp>
#include <DataProducer.hpp>
#include <DataConsumer.hpp>
#include <DirectTransport.hpp>
#include <Utils.hpp>


std::future<Bot*> Bot::create(Router* mediasoupRouter)
{
	// Create a DirectTransport for connecting the bot.
	DirectTransportOptions tOptions{.maxMessageSize = 512 };
	Transport* transport = co_await mediasoupRouter->createDirectTransport(tOptions);

	// Create DataProducer to send messages to peers.
	DataProducerOptions pOptions{ .label = "bot"};
	DataProducer* dataProducer = co_await transport->produceData(pOptions);

	// Create the Bot instance.
	co_return new Bot(transport, dataProducer);
}

Bot::Bot(Transport* transport, DataProducer* dataProducer)
{
	// mediasoup DirectTransport.
	this->_transport = transport;

	// mediasoup DataProducer.
	this->_dataProducer = dataProducer;
}

DataProducer* Bot::dataProducer()
{
	return this->_dataProducer;
}

void Bot::close()
{
	// No need to do anyting.
}

std::future<void> Bot::handlePeerDataProducer(std::string dataProducerId, protoo::Peer* peer)
{
	// Create a DataConsumer on the DirectTransport for each Peer.
	DataConsumerOptions options{ .dataProducerId = dataProducerId };
	DataConsumer* dataConsumer = co_await this->_transport->consumeData(options);

	dataConsumer->on("message", [=](std::string message, uint32_t ppid)
		{
			// Ensure it's a WebRTC DataChannel string.
			if (ppid != 51)
			{
				MSC_WARN("ignoring non string messagee from a Peer");

				return;
			}

			std::string text = message;

			MSC_DEBUG(
				"SCTP message received[peerId:%s, size:%d]", peer->id().c_str(), message.length());

			// Create a message to send it back to all Peers in behalf of the sending
			// Peer.
			std::string messageBack = Utils::Printf("${peer.data.displayName} said me : \"%s\"", text.c_str());

			//this->_dataProducer->send(messageBack);
		});
}


