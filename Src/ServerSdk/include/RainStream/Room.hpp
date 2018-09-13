#pragma once

#include "EnhancedEventEmitter.hpp"
#include "utils.hpp"
#include "errors.hpp"
#include "ortc.hpp"
#include "Peer.hpp"
#include "Producer.hpp"
#include <set>

namespace rs
{
	class Room;
	class Logger;
	class Channel;
	class ActiveSpeakerDetector;
	class RtpStreamer;

	typedef std::set<Room*> Rooms;

	class Room : public EnhancedEventEmitter, public ChannelListener
	{
	public:
		Room(const Json& internal, const Json& data, Channel* channel);

		uint32_t id();

		bool closed();

		Json rtpCapabilities();

		/**
		 * Get an array with all the Peers.
		 *
		 * @return {Array<Peer>}
		 */
		Peers peers();

		/**
		 * Close the Room.
		 *
		 * @param {Any} [appData] - App custom data.
		 * @param {bool} [notifyChannel=true] - Private.
		 */
		void close(const Json& appData = Json::object(), bool notifyChannel = true);

		/**
		 * Dump the Room.
		 *
		 * @private
		 *
		 * @return {Promise}
		 */
		Defer dump();

		/**
		 * Get Peer by name.
		 *
		 * @param {String} name
		 *
		 * @return {Peer}
		 */
		Peer* getPeerByName(std::string name);

		Defer receiveRequest(const Json& request);

		ActiveSpeakerDetector* createActiveSpeakerDetector();

		Defer createRtpStreamer(Producer* producer, const Json& options);

		Peer* _createPeer(std::string peerName, const Json& rtpCapabilities, const Json& appData);

		void _handleProducer(Producer* producer);

		Defer _createPlainRtpTransport(const Json& options);

		void _handleWorkerNotifications();

	protected:
		/* Methods inherited from Channel ChannelListener. */
		void onEvent(std::string event, Json data) override;

	private:
		// Closed flag.
		bool _closed = false;
		// Internal data.
		// - .routerId
		Json _internal;
		// Room data.
		// - .rtpCapabilities
		Json _data;
		// Channel instance.
		Channel* _channel{ nullptr };
		// Map of Peer instances indexed by peerName.
		// @type {Map<peerName, Peer>}
		Peers _peers;
		// Map of Producer instances indexed by producerId.
		// @type {Map<producerId, Producer>}
		Producers _producers;

		std::unique_ptr<Logger> logger;
	};
}
