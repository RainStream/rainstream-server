#include "RainStream.hpp"
#include "Worker.hpp"
#include "Channel.hpp"
#include "Logger.hpp"
#include "process/SubProcess.hpp"

namespace rs
{

	const int CHANNEL_FD = 3;
#define WORK_PATH "MediaServer"
	std::string workerPath;

	Worker::Worker(std::string id, AStringVector parameters)
		: _id(id)
		, logger(new Logger("Worker"))
	{
		//this->setMaxListeners(Infinity);

		logger->debug("constructor() [id:%s, parameters:\"%s\"]", id.c_str(), join(parameters, " ").c_str());

		parameters.insert(parameters.begin(), id);

		this->_child = SubProcess::spawn(WORK_PATH, parameters);

		// Channel instance.
		this->_channel = new Channel(_child->getSocket());

	}

	void Worker::close()
	{
		logger->debug("close()");

		this->emit("@close");

		// 	// Kill rainstream-worker process.
		// 	if (this->_child)
		// 	{
		// 		// Remove event listeners but leave a fake "error" hander
		// 		// to avoid propagation.
		// 		this->_child.off("exit");
		// 		this->_child.off("error");
		// 		this->_child.on("error", () = > null);
		// 		this->_child.kill("SIGTERM");
		// 		this->_child = null;
		// 	}

			// Close the Channel instance.
		this->_channel->close();

		// Close every Room.
		for (auto &room : this->_rooms)
		{
			room->close(undefined, false);
		}
	}

	Defer Worker::dump()
	{
		logger->debug("dump()");

		return this->_channel->request("worker.dump")
			.then([=](const Json& data)
		{
			logger->debug("\"worker.dump\" request succeeded");

			return data;
		})
			.fail([=](Error error)
		{
			logger->error("\"worker.dump\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	Defer Worker::updateSettings(Json& spawnOptions)
	{
		logger->debug("updateSettings() [spawnOptions:%s]", spawnOptions.dump().c_str());

		return this->_channel->request("worker.updateSettings", nullptr, spawnOptions)
			.then([=]()
		{
			logger->debug("\"worker.updateSettings\" request succeeded");
		})
			.fail([=](Error error)
		{
			logger->error("\"worker.updateSettings\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Create a Room instance.
	*
	* @return {Room}
	*/
	Room* Worker::CreateRoom(const Json& data)
	{
		logger->debug("Room()");

		Json internal;
		internal["routerId"] = utils::randomNumber();

		Room* room = new Room(internal, data, this->_channel);

		// Store the Room instance and remove it when closed.
		this->_rooms.insert(room);
		room->on("@close", [=]()
		{
			this->_rooms.erase(room);
		});

		this->_channel->request("worker.createRouter", internal)
			.then([=]()
		{
			logger->debug("\"worker.createRouter\" request succeeded");
		})
			.fail([=](Error error)
		{
			logger->error("\"worker.createRouter\" request failed: %s", error.ToString().c_str());

			room->close(undefined, false);
		});

		return room;
	}
}
