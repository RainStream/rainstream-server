#pragma once

#include "EventEmitter.hpp"
#include "errors.hpp"
#include "PlainRtpTransport.hpp"

namespace rs
{
	class Peer;
	class Channel;
	class Producer;
	class WebRtcTransport;

	class Consumer : public ChannelListener , public EventEmitter
	{
	public:
		Consumer(Peer* peer, Producer* source, const Json& internal, const Json& data, Channel* channel);

		uint32_t id();

		bool closed();

		Json appData();

		void appData(const Json& appData);

		std::string kind();

		Peer* peer();

		WebRtcTransport* transport();

		Json rtpParameters();

		Producer* source();

		bool enabled();

		/**
		 * Whether the Consumer is locally paused.
		 *
		 * @return {bool}
		 */
		bool locallyPaused();

		/**
		 * Whether the Consumer is remotely paused.
		 *
		 * @return {bool}
		 */
		bool remotelyPaused();

		/**
		 * Whether the source (Producer) is paused.
		 *
		 * @return {bool}
		 */
		bool sourcePaused();

		/**
		 * Whether the Consumer is paused.
		 *
		 * @return {bool}
		 */
		bool paused();

		/**
		 * The preferred profile.
		 *
		 * @type {String}
		 */
		std::string preferredProfile();

		/**
		 * The effective profile.
		 *
		 * @type {String}
		 */
		std::string effectiveProfile();

		/**
		 * Close the Consumer.
		 *
		 * @private
		 *
		 * @param {bool} [notifyChannel=true] - Private.
		 */
		void close(bool notifyChannel = true);

		/**
		 * Dump the Consumer.
		 *
		 * @private
		 *
		 * @return {Promise}
		 */
		Defer dump();

		Json toJson();

		/**
		 * Whether this Consumer belongs to a Peer.
		 *
		 * @private
		 *
		 * @return {bool}
		 */
		bool hasPeer();

		/**
		 * Enable the Consumer for sending media.
		 *
		 * @private
		 *
		 * @param {Transport} transport
		 * @param {bool} remotelyPaused
		 * @param {String} preferredProfile
		 */
		Defer enable(WebRtcTransport* transport, bool remotelyPaused = false, std::string preferredProfile = "");

		/**
		 * Pauses sending media.
		 *
		 * @param {Any} [appData] - App custom data.
		 *
		 * @return {bool} true if paused.
		 */
		bool pause(Json appData);

		/**
		 * The remote Consumer was paused.
		 * Invoked via remote notification.
		 *
		 * @private
		 *
		 * @param {Any} [appData] - App custom data.
		 */
		void remotePause(const Json& appData = Json::object());

		/**
		 * Resumes sending media.
		 *
		 * @param {Any} [appData] - App custom data.
		 *
		 * @return {bool} true if not paused.
		 */
		bool resume(const Json& appData);

		/**
		 * The remote Consumer was resumed.
		 * Invoked via remote notification.
		 *
		 * @private
		 *
		 * @param {Any} [appData] - App custom data.
		 */
		void remoteResume(const Json& appData);

		/**
		 * Sets the preferred RTP profile.
		 *
		 * @param {String} profile
		 */
		void setPreferredProfile(std::string profile);

		/**
		 * Preferred receiving profile was set on my remote Consumer.
		 *
		 * @private
		 *
		 * @param {String} profile
		 */
		void remoteSetPreferredProfile(std::string profile);

		/**
		 * Sets the encoding preferences.
		 * Only for testing.
		 *
		 * @private
		 *
		 * @param {String} profile
		 */
		void setEncodingPreferences(std::string preferences);

		/**
		 * Request a key frame to the source.
		 */
		void requestKeyFrame();

		/**
		 * Get the Consumer stats.
		 *
		 * @return {Promise}
		 */
		Defer getStats();

		/**
		 * Enables periodic stats retrieval.
		 *
		 * @private
		 */
		void enableStats(int interval = DEFAULT_STATS_INTERVAL);

		/**
		 * Disables periodic stats retrieval.
		 *
		 * @private
		 */
		void disableStats();

		void _handleWorkerNotifications();

	protected:
		/* Methods inherited from Channel ChannelListener. */
		void onEvent(std::string event, Json data) override;

	private:
		// Closed flag.
		bool _closed = false;

		// Internal data.
		// - .routerId
		// - .consumerId
		// - .producerId
		// - .transportId
		Json _internal;

		// Consumer data.
		// - .kind
		// - .peer (just if this Consumer belongs to a Peer)
		// - .transport
		// - .rtpParameters
		// - .source
		Json _data;

		// Channel instance.
		Channel* _channel;

		Peer* _peer{ nullptr };

		Producer* _source{ nullptr };

		WebRtcTransport* _transport{ nullptr };

		// App data.
		Json _appData;

		// Locally paused flag.
		// @type {bool}
		bool _locallyPaused;

		// Remotely paused flag.
		// @type {bool}
		bool _remotelyPaused;

		// Source paused flag.
		// @type {bool}
		bool _sourcePaused;

		// Periodic stats flag.
		// @type {bool}
		bool _statsEnabled;

		// Periodic stats interval identifier.
		// @type {bool}
		bool _statsInterval;

		// Preferred profile.
		// @type {String}
		std::string _preferredProfile;

		// Effective profile.
		// @type {String}
		std::string _effectiveProfile;
	};

	typedef std::map<uint32_t, Consumer*> Consumers;

}
