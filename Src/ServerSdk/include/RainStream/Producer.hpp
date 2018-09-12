#pragma once

#include "EnhancedEventEmitter.hpp"
#include "errors.hpp"

namespace rs
{
	class Peer;
	class Logger;
	class Channel;
	class WebRtcTransport;

	class Producer : public EnhancedEventEmitter, public EventListener
	{
	public:
		Producer(Peer* peer, WebRtcTransport*transport, const Json& internal, const Json& data, Channel* channel, const Json& options);

		uint32_t id();

		bool closed();

		Json appData();

		void appData(Json appData);

		std::string kind();

		Peer* peer();

		WebRtcTransport* transport();

		Json rtpParameters();

		Json consumableRtpParameters();

		/**
		 * Whether the Producer is locally paused.
		 *
		 * @return {bool}
		 */
		bool locallyPaused();

		/**
		 * Whether the Producer is remotely paused.
		 *
		 * @return {bool}
		 */
		bool remotelyPaused();

		/**
		 * Whether the Producer is paused.
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
		 * Close the Producer.
		 *
		 * @param {Any} [appData] - App custom data.
		 * @param {bool} [notifyChannel=true] - Private.
		 */
		void close(Json appData, bool notifyChannel = true);

		/**
		 * The remote Producer was closed.
		 * Invoked via remote notification.
		 *
		 * @private
		 *
		 * @param {Any} [appData] - App custom data.
		 * @param {bool} [notifyChannel=true] - Private.
		 */
		void remoteClose(Json appData = Json::object(), bool notifyChannel = true);

		void _destroy(bool notifyChannel = true);

		/**
		 * Dump the Producer.
		 *
		 * @private
		 *
		 * @return {Promise}
		 */
		Defer dump();

		/**
		 * Pauses receiving media.
		 *
		 * @param {Any} [appData] - App custom data.
		 *
		 * @return {bool} true if paused.
		 */
		bool pause(Json appData);

		/**
		 * The remote Producer was paused.
		 * Invoked via remote notification.
		 *
		 * @private
		 *
		 * @param {Any} [appData] - App custom data.
		 */
		void remotePause(Json appData);

		/**
		 * Resumes receiving media.
		 *
		 * @param {Any} [appData] - App custom data.
		 *
		 * @return {bool} true if not paused.
		 */
		bool resume(Json appData);

		/**
		 * The remote Producer was resumed.
		 * Invoked via remote notification.
		 *
		 * @private
		 *
		 * @param {Any} [appData] - App custom data.
		 */
		void remoteResume(Json appData);

		/**
		 * Sets the preferred RTP profile.
		 *
		 * @param {String} profile
		 */
		void setPreferredProfile(std::string profile);

		/**
		 * Change receiving RTP parameters.
		 *
		 * @private
		 *
		 * @param {RTCRtpParameters} rtpParameters - New RTP parameters.
		 *
		 * @return {Promise}
		 */
		Defer updateRtpParameters(Json rtpParameters);

		/**
		 * Get the Producer stats.
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

		void _handleTransportEvents();

		void _handleWorkerNotifications();

	protected:
		/* Methods inherited from Channel EventListener. */
		void onEvent(std::string event, Json data) override;

	private:
		// Closed flag.
		bool _closed = false;

		// Internal data.
		// - .routerId
		// - .producerId
		// - .transportId
		Json _internal;

		// Producer data.
		// - .kind
		// - .peer
		// - .transport
		// - .rtpParameters
		// - .consumableRtpParameters
		Json _data;

		// Channel instance.
		Channel* _channel{ nullptr };

		Peer* _peer{ nullptr };

		WebRtcTransport* _transport{ nullptr };

		// App data.
		Json _appData = undefined;

		// Locally paused flag.
		// @type {bool}
		bool _locallyPaused = false;

		// Remotely paused flag.
		// @type {bool}
		bool _remotelyPaused = false;

		// Periodic stats flag.
		// @type {bool}
		bool _statsEnabled = false;

		// Periodic stats interval identifier.
		// @type {bool}
		bool _statsInterval = NULL;

		// Preferred profile.
		// @type {String}
		std::string _preferredProfile = "default";

		std::unique_ptr<Logger> logger;
	};


	typedef std::map<uint32_t, Producer*> Producers;

}
