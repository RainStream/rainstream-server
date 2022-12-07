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

	class Peer : public WebSocketClient::Listener , public EnhancedEventEmitter
	{
	public:
		class Listener
		{
		public:
			virtual void OnPeerClose(Peer* peer) = 0;
			virtual void OnPeerRequest(Peer* peer, Request* request) = 0;
		};
	public:
		explicit Peer(std::string peerName, protoo::WebSocketClient* transport, Listener* listener);
		virtual ~Peer();

	public:
		std::string id();
		void close();

		void Send(const json& message);
		void notify(std::string method, const json& data);
		std::future<json> request(std::string method, const json& data);

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
		virtual void onMessage(const std::string& message);
		virtual void onClosed(int code, const std::string& message);

	protected:
		void _handleTransport();
		void _handleRequest(json& jsonRequest);
		void _handleResponse(json& response);

	private:
		std::string _peerName;

		Listener* _listener{ nullptr };
		WebSocketClient* _transport{ nullptr };

		// Closed flag.
		bool _closed = false;

		std::unordered_map<uint32_t, std::promise<json> > _sents;
	};
}

#endif

