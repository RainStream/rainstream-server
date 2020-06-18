
#include "common.hpp"
#include "Worker.hpp"
#include "Channel.hpp"
#include "Logger.hpp"
#include "utils.hpp"
#include "child_process/SubProcess.hpp"

#define __MEDIASOUP_VERSION__ "__MEDIASOUP_VERSION__"

#ifdef _DEBUG
#define WORK_PATH "D:/Develop/mediasoup/worker/out/Debug/mediasoup-worker.exe"
#else
#define WORK_PATH "D:/Develop/mediasoup/worker/out/Release/mediasoup-worker.exe"
#endif

namespace rs
{

	const int CHANNEL_FD = 3;

	std::string workerPath;

	Worker::Worker(json settings)
	{
		//this->setMaxListeners(Infinity);

		DLOG(INFO) << "constructor() [ "<< "workerSettings:" << settings.dump() << "]";

		std::string spawnBin = WORK_PATH;
		AStringVector spawnArgs;

		std::string logLevel = settings.value("logLevel", "");
		json logTags = settings.value("logTags", json::array());
		uint32_t rtcMinPort = settings.value("rtcMinPort", 0);
		uint32_t rtcMaxPort = settings.value("rtcMaxPort", 0);
		std::string dtlsCertificateFile = settings.value("dtlsCertificateFile", "");
		std::string dtlsPrivateKeyFile = settings.value("dtlsPrivateKeyFile", "");

		if (!logLevel.empty())
			spawnArgs.push_back(utils::Printf("--logLevel=%s", logLevel.c_str()));

		for (auto& logTag : logTags)
		{
			if (logTag.is_string() && !std::string(logTag).empty())
				spawnArgs.push_back(utils::Printf("--logTag=%s", std::string(logTag).c_str()));
		}

		if (rtcMinPort > 0)
			spawnArgs.push_back(utils::Printf("--rtcMinPort=%d", rtcMinPort));

		if (rtcMaxPort > 0)
			spawnArgs.push_back(utils::Printf("--rtcMaxPort=%d", rtcMaxPort));

		if (!dtlsCertificateFile.empty())
			spawnArgs.push_back(utils::Printf("--dtlsCertificateFile=%s", dtlsCertificateFile.c_str()));

		if (!dtlsPrivateKeyFile.empty())
			spawnArgs.push_back(utils::Printf("--dtlsPrivateKeyFile=%s", dtlsPrivateKeyFile.c_str()));

		DLOG(INFO) << utils::Printf("spawning worker process: %s %s", 
			spawnBin.c_str(), utils::join(spawnArgs,",").c_str());

		json spawnOptions = {
			{ "env", { {"MEDIASOUP_VERSION", __MEDIASOUP_VERSION__} } },
			{ "detached", false },
			{ "stdio", { "ignore", "pipe", "pipe", "pipe", "pipe", "pipe", "pipe"} },
			{ "windowsHide", true }
		};

		this->_child = SubProcess::spawn(WORK_PATH, spawnArgs, spawnOptions);

		this->_pid = this->_child->pid();

		this->_channel = new Channel(this->_child->stdio()[3],
			this->_child->stdio()[4],
			this->_pid);
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
