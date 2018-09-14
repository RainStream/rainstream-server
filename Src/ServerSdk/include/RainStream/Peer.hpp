#pragma once

#include "EnhancedEventEmitter.hpp"
#include "utils.hpp"
#include "errors.hpp"
#include "ortc.hpp"
#include "WebRtcTransport.hpp"
#include "Producer.hpp"
#include "Consumer.hpp"

namespace rs
{

	class Peer : public EventBus
	{
	public:
		Peer(const Json& internal, const Json& data, Channel* channel, SandBox sandbox);

		std::string name();

		bool closed();

		Json appData();

		void appData(Json appData);

		Json rtpCapabilities();

		/**
		 * Get an array with all the WebRtcTransports.
		 *
		 * @return {Array<WebRtcTransport>}
		 */
		WebRtcTransports transports();

		/**
		 * Get an array with all the Producers.
		 *
		 * @return {Array<Producers>}
		 */
		Producers producers();

		/**
		 * Get an array with all the Consumer.
		 *
		 * @return {Array<Consumer>}
		 */
		Consumers consumers();

		/**
		 * Get the Consumer asasociated to the given source (Producer).
		 *
		 * @return {Consumer}
		 */
		Consumer* getConsumerForSource(Producer* source);

		/**
		 * Close the Peer.
		 *
		 * @param {Any} [appData] - App custom data.
		 * @param {bool} [notifyChannel=true] - Private.
		 */
		void close(Json appData = Json(), bool notifyChannel = true);

		/**
		 * The remote Peer left the Room.
		 * Invoked via remote notification.
		 *
		 * @private
		 *
		 * @param {Any} [appData] - App custom data.
		 */
		void remoteClose(Json appData);

		/**
		 * Get WebRtcTransport by id.
		 *
		 * @param {Number} id
		 *
		 * @return {WebRtcTransport}
		 */
		WebRtcTransport* getTransportById(uint32_t id);

		/**
		 * Get Producer by id.
		 *
		 * @param {Number} id
		 *
		 * @return {Producer}
		 */
		Producer* getProducerById(uint32_t id);

		/**
		 * Get Consumer by id.
		 *
		 * @param {Number} id
		 *
		 * @return {Consumer}
		 */
		Consumer* getConsumerById(uint32_t id);

		/**
		 * Dump the Peer.
		 *
		 * @private
		 *
		 * @return {Promise}
		 */
		Defer dump();

		Json toJson();

		Defer receiveRequest(Json request);

		Defer receiveNotification(Json notification);

		/**
		 * @private
		 */
		void handleNewPeer(Peer* peer);

		/**
		 * @private
		 */
		void handlePeerClosed(Peer* peer, Json appData);

		/**
		 * @private
		 */
		void addConsumerForProducer(Producer* producer, bool notifyClient = true);

		void _sendNotification(std::string method, Json data);

		Defer _createWebRtcTransport(uint32_t id, std::string direction, Json options, Json appData);

		Defer _createProducer(uint32_t id, std::string kind, uint32_t transportId, Json rtpParameters, bool remotelyPaused, Json appData);

	private:

		// Closed flag.
		bool _closed = false;

		// Internal data.
		// - .routerId
		// - .peerName
		Json _internal;

		// Peer data.
		// - .rtpCapabilities
		Json _data;

		// Channel instance.
		Channel* _channel{ nullptr };

		// Store internal sandbox stuff.
		// - .getProducerRtpParametersMapping()
		SandBox _sandbox;

		// App data.
		Json _appData = undefined;

		// Map of WebRtcTransports indexed by id.
		// @type {Map<transportId, WebRtcTransport>}
		WebRtcTransports _transports;

		// Map of Producers indexed by id.
		// @type {Map<producerId, Producer>}
		Producers _producers;

		// Map of Consumers indexed by id.
		// @type {Map<consumerId, Consumers>}
		Consumers _consumers;
	};

	typedef std::map<std::string, Peer*> Peers;

}
