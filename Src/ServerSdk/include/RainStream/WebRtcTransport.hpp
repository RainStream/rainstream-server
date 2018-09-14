#pragma once

#include "EventEmitter.hpp"
#include "errors.hpp"

namespace rs
{
	class Channel;

	class WebRtcTransport : public ChannelListener, public EventEmitter
	{
	public:
		WebRtcTransport(const Json& internal, const Json& data, Channel* channel);

		uint32_t id();

		bool closed();

		Json appData();

		void appData(Json appData);

		std::string direction();

		std::string iceRole();

		Json iceLocalParameters();

		Json iceLocalCandidates();

		Json iceState();

		Json iceSelectedTuple();

		Json dtlsLocalParameters();

		std::string dtlsState();

		Json dtlsRemoteCert();

		/**
		 * Close the WebRtcTransport.
		 *
		 * @param {Any} [appData] - App custom data.
		 * @param {bool} [notifyChannel=true] - Private.
		 */
		void close(Json appData = Json::object(), bool notifyChannel = true);

		/**
		 * Remote WebRtcTransport was closed.
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
		 * Dump the WebRtcTransport.
		 *
		 * @private
		 *
		 * @return {Promise}
		 */
		Defer dump();

		/**
		 * Provide the remote DTLS parameters.
		 *
		 * @private
		 *
		 * @param {Object} parameters - Remote DTLS parameters.
		 *
		 * @return {Promise} Resolves to this->
		 */
		Defer setRemoteDtlsParameters(Json parameters);

		/**
		 * Set maximum bitrate (in bps).
		 *
		 * @return {Promise} Resolves to this->
		 */
		Defer setMaxBitrate(int bitrate);

		/**
		 * Tell the WebRtcTransport to generate new uFrag and password values.
		 *
		 * @private
		 *
		 * @return {Promise} Resolves to this->
		 */
		Defer changeUfragPwd();

		/**
		 * Get the WebRtcTransport stats.
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

		/**
		 * Tell the WebRtcTransport to start mirroring incoming data.
		 *
		 * @return {Promise} Resolves to this->
		 */
		Defer startMirroring(Json options);

		/**
		 * Tell the WebRtcTransport to stop mirroring incoming data.
		 *
		 * @return {Promise} Resolves to this->
		 */
		Defer stopMirroring();

		void _handleWorkerNotifications();
	protected:
		/* Methods inherited from Channel ChannelListener. */
		void onEvent(std::string event, Json data) override;

	private:
		// Closed flag.
		bool _closed = false;

		// Internal data.
		// - .routerId
		// - .transportId
		Json _internal;

		// WebRtcTransport data provided by the worker.
		// - .direction
		// - .iceRole
		// - .iceLocalParameters
		//   - .usernameFragment
		//   - .password
		//   - .iceLite
		// - .iceLocalCandidates []
		//   - .foundation
		//   - .priority
		//   - .ip
		//   - .port
		//   - .type
		//   - .protocol
		//   - .tcpType
		// - .iceState
		// - .iceSelectedTuple
		//   - .local
		//     - .ip
		//     - .port
		//     - .protocol
		//   - .remote
		//     - .ip
		//     - .port
		//     - .protocol
		// - .dtlsLocalParameters
		//   - .role
		//   - .fingerprints
		//     - .sha-1
		//     - .sha-224
		//     - .sha-256
		//     - .sha-384
		//     - .sha-512
		// - .dtlsState
		// - .dtlsRemoteCert
		Json _data;

		// Channel instance.
		Channel* _channel{ nullptr };

		// App data.
		Json _appData = undefined;

		// Periodic stats flag.
		// @type {bool}
		bool _statsEnabled = false;

		// Periodic stats interval identifier.
		// @type {bool}
		bool _statsInterval = NULL;
	};

	typedef std::map<uint32_t, WebRtcTransport*> WebRtcTransports;

}
