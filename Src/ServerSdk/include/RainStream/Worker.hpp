#pragma once

#include "Room.hpp"
#include "utils.hpp"
#include "StringUtils.hpp"

namespace rs
{
	class Logger;
	class Channel;
	class Request;
	class SubProcess;

	class Worker : public EventEmitter
	{
	public:
		Worker(std::string id, AStringVector parameters);

		void close();

		Defer dump();

		Defer updateSettings(Json& options);

		/**
		 * Create a Room instance.
		 *
		 * @return {Room}
		 */
		Room* CreateRoom(const Json& data);


	private:
		std::string _id;

		Channel* _channel{ nullptr };
		// Set of Room instances.
		Rooms _rooms;

		SubProcess* _child{ nullptr };

		std::unique_ptr<Logger> logger;
	};

}
