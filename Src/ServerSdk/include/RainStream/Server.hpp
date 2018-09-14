#pragma once


namespace rs
{
	class Room;
	class Worker;

	class Server
	{
	public:
		class Listener
		{
		public:
			virtual void OnServerClose() = 0;
			virtual void OnNewRoom(Room* room) = 0;
		};

	public:

		/**
		* Create a Server instance.
		*
		* @param {object} [options]
		* @param {number} [options.numWorkers=HOST_NUM_CORES] - Number of child workers.
		* @param {string} [options.logLevel='debug'] - Log level. Valid values are
		* 'debug', 'warn', 'error'.
		* @param {array} [options.logTags] - Log tags.
		* @param {string|boolean} [options.rtcIPv4=true] - IPv4 for RTC. Valid
		* values are a IPv4, `true` (auto-detect) and `false` (disabled).
		* @param {string|boolean} [options.rtcIPv6=true] - IPv6 for RTC. Valid
		* values are a IPv6, `true` (auto-detect) and `false` (disabled).
		* @param {string} [options.rtcAnnouncedIPv4] - Announced IPv4 for RTC. Valid
		* value is a IPv4.
		* @param {string} [options.rtcAnnouncedIPv6] - Announced IPv6 for RTC. Valid
		* value is a IPv6.
		* @param {number} [options.rtcMinPort=10000] - Minimun RTC port.
		* @param {number} [options.rtcMaxPort=59999] - Maximum RTC port.
		*
		* @return {Server}
		*/

		Server(Json options, Listener* listener);

		bool closed();

		/**
		 * Close the Server.
		 */
		void close();

		/**
		 * Dump the Server.
		 *
		 * @private
		 *
		 * @return {Promise}
		 */
		Defer dump();

		/**
		 * Update Server settings.
		 *
		 * @param {Object} options - Object with modified settings.
		 *
		 * @return {Promise}
		 */
		Defer updateSettings(Json options);

		/**
		 * Create a Room instance.
		 *
		 * @param {array<RoomMediaCodec>} mediaCodecs
		 *
		 * @return {Room}
		 */
		Room* Room(Json mediaCodecs);

		void _addWorker(Worker* worker);

		Worker* _getRandomWorker();

	private:
		// Closed flag.
		bool _closed = false;

		Listener* listener{ nullptr };

		// Set of Worker instances.
		std::vector<Worker*> _workers;
	};

}
