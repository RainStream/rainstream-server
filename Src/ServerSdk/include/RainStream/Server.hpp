#pragma once


namespace rs
{
	class Logger;
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

		std::unique_ptr<Logger> logger;
	};

}
