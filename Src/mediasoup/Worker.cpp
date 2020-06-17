
#include "common.hpp"
#include "Worker.hpp"
#include "Channel.hpp"
#include "Logger.hpp"
#include "child_process/SubProcess.hpp"

namespace rs
{

	const int CHANNEL_FD = 3;
#define WORK_PATH "MediaServer"
	std::string workerPath;

	Worker::Worker(std::string id, AStringVector parameters)
		: _id(id)
	{
		//this->setMaxListeners(Infinity);

		DLOG(INFO) << "constructor() [id:" << id << "parameters:" << join(parameters, " ") << "]";

		parameters.insert(parameters.begin(), id);

		this->_child = SubProcess::spawn(id, WORK_PATH, parameters);

		// Channel instance.
		this->_channel = new Channel(_child->getSocket());

	}

	void Worker::close()
	{
		DLOG(INFO) << "close()";

		if (this->_closed)
			return;

		DLOG(INFO) << "close()";

		this->_closed = true;

		// Kill the worker process.
// 		if (this->_child)
// 		{
// 			// Remove event listeners but leave a fake 'error' hander to avoid
// 			// propagation.
// 			this->_child->removeAllListeners('exit');
// 			this->_child->removeAllListeners('error');
// 			this->_child->on('error', () = > {});
// 			this->_child->kill('SIGTERM');
// 			this->_child = undefined;
// 		}

		// Close the Channel instance.
		this->_channel->close();

		// Close the PayloadChannel instance.
		this->_payloadChannel->close();

		// Close every Router.
// 		for (auto router : this->_routers)
// 		{
// 			router->workerClosed();
// 		}
// 		this->_routers.clear();
// 
// 		// Emit observer event.
// 		this->_observer->safeEmit('close');
	}

// 	Defer Worker::dump()
// 	{
// 		DLOG(INFO) << "dump()";
// 
// 		return this->_channel->request("worker.dump")
// 			.then([=](const json& data)
// 		{
// 			DLOG(INFO) << "\"worker.dump\" request succeeded";
// 
// 			return data;
// 		})
// 			.fail([=](Error error)
// 		{
// 			LOG(ERROR) << "\"worker.dump\" request failed:"<< error.ToString();
// 
// 			throw error;
// 		});
// 	}
// 
// 	Defer Worker::updateSettings(json& spawnOptions)
// 	{
// 		DLOG(INFO) << "updateSettings() [spawnOptions:" << spawnOptions.dump() << "]";
// 
// 		return this->_channel->request("worker.updateSettings", nullptr, spawnOptions)
// 			.then([=]()
// 		{
// 			DLOG(INFO) << "\"worker.updateSettings\" request succeeded";
// 		})
// 			.fail([=](Error error)
// 		{
// 			LOG(ERROR) << "\"worker.updateSettings\" request failed:"<< error.ToString();
// 
// 			throw error;
// 		});
// 	}

	/**
	* Create a Room instance.
	*
	* @return {Room}
	*/
// 	Router* Worker::createRouter(const json& data)
// 	{
// 		DLOG(INFO) << "Room()";
// 
// 		json internal;
// 		internal["routerId"] = utils::randomNumber();
// 
// 		Room* room = new Room(internal, data, this->_channel);
// 
// 		// Store the Room instance and remove it when closed.
// 		this->_rooms.insert(room);
// 		room->addEventListener("@close", [=](json)
// 		{
// 			this->_rooms.erase(room);
// 		});
// 
// 		this->_channel->request("worker.createRouter", internal)
// 			.then([=]()
// 		{
// 			DLOG(INFO) << "\"worker.createRouter\" request succeeded";
// 		})
// 			.fail([=](Error error)
// 		{
// 			LOG(ERROR) << "\"worker.createRouter\" request failed: " << error.ToString();
// 
// 			room->close(undefined, false);
// 		});
// 
// 		return room;
// 	}
}
