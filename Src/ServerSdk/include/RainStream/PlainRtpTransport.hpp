#pragma once

#include "EnhancedEventEmitter.hpp"
#include "errors.hpp"

namespace rs
{

	class Channel;

	class PlainRtpTransport : public EnhancedEventEmitter, public EventListener
	{
	public:
		PlainRtpTransport(const Json& internal, const Json& data, Channel* channel);

		uint32_t id();

		bool closed();

		Json tuple();

		/**
		 * Close the PlainRtpTransport.
		 *
		 * @param {bool} [notifyChannel=true] - Private.
		 */
		void close(bool notifyChannel = true);

		/**
		 * Dump the PlainRtpTransport.
		 *
		 * @private
		 *
		 * @return {Promise}
		 */
		Defer dump();

		/**
		 * Enables periodic stats retrieval.
		 *
		 * Not implemented.
		 *
		 */
		void enableStats();

		/**
		 * Disables periodic stats retrieval.
		 *
		 * Not implemented.
		 *
		 */
		void disableStats();

		void _handleWorkerNotifications();

	protected:
		/* Methods inherited from Channel EventListener. */
		void onEvent(std::string event, Json data) override;

	private:
		// Closed flag.
		bool _closed = false;

		// Internal data.
		// - .routerId
		// - .transportId
		Json _internal;

		// PlainRtpTransport data provided by the worker.
		// - .tuple
		//   - .local
		//     - .ip
		//     - .port
		//     - .protocol
		//   - .remote
		//     - .ip
		//     - .port
		//     - .protocol
		Json _data;

		// Channel instance.
		Channel* _channel{ nullptr };

		std::unique_ptr<Logger> logger;
	};
}