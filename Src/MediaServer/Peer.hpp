#ifndef PROTOO_PEER_HPP
#define PROTOO_PEER_HPP

#include "common.hpp"
#include "WebSocketClient.hpp"
#include <EnhancedEventEmitter.hpp>

class Transport;
class Producer;
class Consumer;
class DataProducer;
class DataConsumer;

using Accept = std::function<void(json&)>;
using Reject = std::function<void(int, std::string)>;


namespace protoo
{
	class Request;
	class WebSocketClient;

	class Peer : public EnhancedEventEmitter
	{
	public:
		class Listener
		{
		public:
			virtual void OnPeerClose(Peer* peer) = 0;
			virtual void OnPeerRequest(Peer* peer, Request* request) = 0;
			virtual void OnPeerNotify(Peer* peer, json& notification) = 0;
		};
	public:
		explicit Peer(std::string peerId, std::string roomId, protoo::WebSocketClient* transport, Listener* listener);
		virtual ~Peer();

	public:
		std::string id();
		void close();

// 		void Accept(uint32_t id, json& data);
// 		void Reject(uint32_t id, uint32_t code, const std::string& errorReason);

		void Send(const json& message);
// 		Defer send(std::string method, json data);
		void notify(std::string method, json& data);
		std::future<void> request(std::string method, json& data);

		struct Data
		{
			bool consume = false;
			bool joined = false;
			std::string displayName;
			json device = json();
			json rtpCapabilities = json();
			json sctpCapabilities = json();

			// Have mediasoup related maps ready even before the Peer joins since we
			// allow creating Transports before joining.
			std::map<std::string, Transport*> transports;
			std::map<std::string, Producer*> producers;
			std::map<std::string, Consumer*> consumers;
			std::map<std::string, DataProducer*> dataProducers;
			std::map<std::string, DataConsumer*> dataConsumers;
		} data;

	protected:
		virtual void OnClosed(int code, const std::string& message);


	private:
		std::string _peerId;
		std::string _roomId;

		Listener* listener{ nullptr };
		WebSocketClient* _transport{ nullptr };

		// Closed flag.
		bool _closed = false;
	};
}

#endif

