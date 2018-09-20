#include "RainStream.hpp"
#include "Server.hpp"
#include "Worker.hpp"
#include "ortc.hpp"
#include "utils.hpp"
#include "errors.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"

namespace rs
{
	int getCpuCount();

	const uint32_t DEFAULT_NUM_WORKERS = getCpuCount();
	const std::vector<std::string> VALID_WORKER_PARAMETERS =
	{
		"logLevel",
		"logTags",
		"rtcIPv4",
		"rtcIPv6",
		"rtcAnnouncedIPv4",
		"rtcAnnouncedIPv6",
		"rtcMinPort",
		"rtcMaxPort",
		"dtlsCertificateFile",
		"dtlsPrivateKeyFile"
	};


	Server::Server(Json options, Listener* listener)
		: listener(listener)
	{
		DLOG(INFO) << "constructor() [options:" << options.dump() << "]";

		std::string serverId = utils::randomString();
		AStringVector parameters;
		uint32_t numWorkers = DEFAULT_NUM_WORKERS;

		// Clone options.
		options = utils::cloneObject(options);

		// Update numWorkers (if given).
		if (options.count("numWorkers") && options["numWorkers"].is_number_integer() && options["numWorkers"] > 0)
			numWorkers = options["numWorkers"];

		// Remove numWorkers.
		options.erase("numWorkers");

		// Normalize some options.

		if (!options["logTags"].is_array())
			options["logTags"] = Json::array();

		if (!options.count("rtcIPv4") || options["rtcIPv4"].is_null())
			options.erase("rtcIPv4");

		if (!options.count("rtcIPv6") || options["rtcIPv6"].is_null())
			options.erase("rtcIPv6");

		if (options["rtcAnnouncedIPv4"].is_null() || options.value("rtcIPv4",false) == false)
			options.erase("rtcAnnouncedIPv4");

		if (options["rtcAnnouncedIPv6"].is_null() || options.value("rtcIPv6", false) == false)
			options.erase("rtcAnnouncedIPv6");

		if (options["rtcMinPort"].is_null() || options["rtcMinPort"].get<uint32_t>() < 1024)
			options["rtcMinPort"] = 10000;

		if (options["rtcMaxPort"].is_null() || options["rtcMaxPort"].get<uint32_t>() > 65535)
			options["rtcMaxPort"] = 59999;

		// Remove rtcMinPort/rtcMaxPort (will be added per worker).
		uint32_t totalRtcMinPort = options["rtcMinPort"];
		uint32_t totalRtcMaxPort = options["rtcMaxPort"];

		options.erase("rtcMinPort");
		options.erase("rtcMaxPort");

		for (Json::iterator it = options.begin(); it != options.end(); ++it)
		{
			std::string key = it.key();

			auto itSub = std::find(VALID_WORKER_PARAMETERS.begin(), VALID_WORKER_PARAMETERS.end(), key);
			if (itSub == VALID_WORKER_PARAMETERS.end())
			{
				LOG(WARNING) << "ignoring unknown option :" << key;

				continue;
			}

			if (key == "logTags")
			{
				const Json& logTags = options["logTags"];
				for (uint32_t m = 0; m < logTags.size(); ++m)
				{
					const Json& logTag = logTags[m];
					std::string parameter = utils::Printf("--logTag=%s", logTag.get<std::string>().c_str());
					parameters.push_back(parameter);
				}

			}
			else
			{
				std::string value = options[key].is_boolean() ? (options.value(key,false)? "true" : "false") 
					: options[key].get<std::string>();
				std::string parameter = utils::Printf("--%s=%s", key.c_str(), value.c_str());
				parameters.push_back(parameter);
			}
		}

		// Create Worker instances.
		for (uint32_t i = 1; i <= numWorkers; i++)
		{
			std::string workerId = serverId + "#" + utils::Printf("%d", i);
			AStringVector workerParameters = parameters;
			// Distribute RTC ports for each worker.

			uint16_t rtcMinPort = totalRtcMinPort;
			uint16_t rtcMaxPort = totalRtcMaxPort;
			uint16_t numPorts = std::floor((rtcMaxPort - rtcMinPort) / numWorkers);

			rtcMinPort = rtcMinPort + (numPorts * (i - 1));
			rtcMaxPort = rtcMinPort + numPorts;

			if (rtcMinPort % 2 != 0)
				rtcMinPort++;

			if (rtcMaxPort % 2 == 0)
				rtcMaxPort--;

			workerParameters.push_back(utils::Printf("--rtcMinPort=%d", rtcMinPort));
			workerParameters.push_back(utils::Printf("--rtcMaxPort=%d", rtcMaxPort));


			// Create a Worker instance (do it in a separate method to avoid creating
			// a callback function within a loop).
			this->_addWorker(new Worker(workerId, workerParameters));
		}
	}

	bool Server::closed()
	{
		return this->_closed;
	}

	/**
	* Close the Server.
	*/
	void Server::close()
	{
		DLOG(INFO) << "close()";

		if (this->_closed)
			return;

		this->_closed = true;

		this->listener->OnServerClose();

		// Close every Worker.
		auto workers = this->_workers;
		for (auto worker : workers)
		{
			worker->close();
		}
	}

	/**
	* Dump the Server.
	*
	* @private
	*
	* @return {Promise}
	*/
	Defer Server::dump()
	{
		DLOG(INFO) << "dump()";

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Server closed"));

		std::vector<Defer> promises;

		for (auto worker : this->_workers)
		{
			promises.push_back(worker->dump());
		}

		return promise::all(promises)
		.then([=](Json datas)
		{
			Json json =
			{
				"workers", datas
			};

			return json;
		});
	}

	/**
	* Update Server settings.
	*
	* @param {Object} options - Object with modified settings.
	*
	* @return {Promise}
	*/
	Defer Server::updateSettings(Json options)
	{
		DLOG(INFO) << "updateSettings() [options:%s" << options.dump() << "]";

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Server closed"));

		std::vector<Defer> promises;

 		for (auto worker : this->_workers)
 		{
 			promises.push_back(worker->updateSettings(options));
 		}

		return promise::all(promises);
	}

	/**
	* Create a Room instance.
	*
	* @param {array<RoomMediaCodec>} mediaCodecs
	*
	* @return {Room}
	*/
	Room* Server::Room(Json mediaCodecs)
	{
		DLOG(INFO) << "Room()";

		if (this->_closed)
			throw errors::InvalidStateError("Server closed");

		// This may throw.
		Json rtpCapabilities = ortc::generateRoomRtpCapabilities(mediaCodecs);
		auto worker = this->_getRandomWorker();
		Json data = 
		{
			{"rtpCapabilities",rtpCapabilities }
		};
		auto room = worker->CreateRoom(data);

		this->listener->OnNewRoom(room);

		return room;
	}

	void Server::_addWorker(Worker* worker)
	{
		// Store the Worker instance and remove it when closed.
		// Also, if it is the latest Worker then close the Server.
		this->_workers.push_back(worker);
		worker->addEventListener("@close", [=](Json)
		{
			for (auto it = this->_workers.begin(); it != this->_workers.end(); ++it)
			{
				if (*it == worker)
				{
					this->_workers.erase(it);
					break;
				}
			}

			if (this->_workers.size() == 0 && !this->_closed)
			{
				DLOG(INFO) << "latest Worker closed, closing Server";

				this->close();
			}
		});
	}

	Worker* Server::_getRandomWorker()
	{
		int size = _workers.size();
		if (size)
		{
			uint32_t target = utils::randomNumber(0, size - 1);
			return _workers[target];
		}
		else
		{
			LOG(ERROR) << "_getRandomWorker() failed: workers.size() == 0";
			return nullptr;
		}
	}


	int getCpuCount()
	{
#ifndef _DEBUG
		uv_cpu_info_t *info;
		int cpu_count;
		uv_cpu_info(&info, &cpu_count);
		uv_free_cpu_info(info, cpu_count);

		return cpu_count;
#else
		return 1;
#endif
	}
}
