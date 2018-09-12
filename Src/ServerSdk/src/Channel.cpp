#include "RainStream.hpp"
#include "Channel.hpp"
#include "Request.hpp"
#include "process/Socket.hpp"
#include "EnhancedEventEmitter.hpp"
#include "Logger.hpp"

namespace rs
{
	// netstring length for a 65536 bytes payload.
	const int NS_MAX_SIZE = 65543;
	// Max time waiting for a response from the worker subprocess.
	const int REQUEST_TIMEOUT = 5000;
	static uint8_t WriteBuffer[NS_MAX_SIZE];

	Channel::Channel(Socket* socket)
		: logger(new Logger("Channel"))
		, workerLogger(new Logger("rainstream-worker"))
	{
		logger->debug("constructor()");

		// Unix Socket instance.
		this->_socket = socket;

		// Read Channel responses/notifications from the worker.
		this->_socket->on("data", [=](std::string nsPayload)
		{
			try
			{
				// We can receive JSON messages (Channel messages) or log strings.
				switch (nsPayload[0])
				{
					// 123 = "{" (a Channel JSON messsage).
				case 123:
					this->_processMessage(Json::parse(nsPayload));
					break;

					// 68 = "D" (a debug log).
				case 68:
					workerLogger->debug(nsPayload.substr(1).c_str());
					break;

					// 87 = "W" (a warning log).
				case 87:
					workerLogger->warn(nsPayload.substr(1).c_str());
					break;

					// 69 = "E" (an error log).
				case 69:
					workerLogger->error(nsPayload.substr(1).c_str());
					break;

				default:
					workerLogger->error(
						"unexpected data: %s", nsPayload.c_str());
				}
			}
			catch (std::exception error)
			{
				logger->error("received invalid message: %s", error.what());
			}

		});

		this->_socket->on("end", [=]()
		{
			logger->debug("channel ended by the other side");
		});

		this->_socket->on("error", [=](std::string error)
		{
			logger->error("channel error: %s", error.c_str());
		});

		_socket->Start();
	}


	void Channel::close()
	{
		logger->debug("close()");

		// Close every pending sent.
		for (auto request : this->_pendingRequest)
		{
			//request.second->clear();
		}

		// Remove event listeners but leave a fake "error" hander
		// to avoid propagation.
		this->_socket->off("end");
		this->_socket->off("error");
		this->_socket->on("error", []() {});

		// Destroy the socket after a while to allow pending incoming
		// messages.
	// 	setTimeout(() = >
	// 	{
	// 		try
	// 		{
	// 			this->_socket.destroy();
	// 		}
	// 		catch (error)
	// 		{
	// 		}
	// 	}, 250);
	}

	void Channel::request(Request* request)
	{
		logger->debug("request() [method:%s, id:%d]", request->method.c_str(), request->id);

		Json data = request->toJson();

		std::string nsPayload = _makePayload(data);

		if (nsPayload.length() > NS_MAX_SIZE)
			throw Error("request too big");

		// This may raise if closed or remote side ended.
		try
		{
			this->_pendingRequest.insert(std::make_pair(request->id, request));
			this->_socket->Write(nsPayload);
		}
		catch (std::exception error)
		{
			throw Error(error.what());
		}
	}

	Defer Channel::request(std::string method, const Json& internal, const Json& data)
	{
// 		uint32_t id = utils::randomNumber();
// 
// 		logger->debug("request() [method:%s, id:%d]", method.c_str(), id);
// 
// 		Json request = {
// 			{ "id",id },
// 			{ "method", method },
// 			{ "internal", internal },
// 			{ "data", data }
// 		};
// 
// 		std::string nsPayload = _makePayload(request);
// 
// 		if (nsPayload.length() > NS_MAX_SIZE)
// 			return promise::reject(Error("request too big"));
// 
// 		// This may raise if closed or remote side ended.
// 		try
// 		{
// 			this->_socket->Write(nsPayload);
// 		}
// 		catch (std::exception error)
// 		{
// 			return promise::reject(Error(error.what()));
// 		}
// 
		return newPromise([=](Defer d) {
			//this->_pendingSent.insert(std::make_pair(id, d));
		});
	}

	void Channel::addEventListener(uint32_t id, EventListener* listener)
	{
		_eventListeners.insert(std::make_pair(id, listener));
	}

	void Channel::off(uint32_t id)
	{
		_eventListeners.erase(id);
	}

	void Channel::_processMessage(const Json& msg)
	{
		// If a Response, retrieve its associated Request.
		if (msg.count("id"))
		{
			uint32_t id = msg["id"].get<uint32_t>();

			if (!this->_pendingRequest.count(id))
			{
				logger->error("received Response does not match any sent Request");

				return;
			}

			Request* request = this->_pendingRequest[id];

			if (msg.value("accepted", false))
			{
				Json data = msg["data"];
				request->Accept(data);

				logger->debug("request succeeded [id:%d]", id);
			}
			else if (msg.value("rejected", false))
			{
				std::string reason = msg.value("reason", std::string());
				request->Reject(reason);

				logger->error("request failed [id:%d, reason:\"%s\"]", id, reason.c_str());
			}
				
		}
		// If a Notification emit it to the corresponding entity.
		else if (msg.count("targetId") && msg.count("event"))
		{
			uint32_t targetId = msg["targetId"].get<uint32_t>();

			if (_eventListeners.count(targetId))
			{
				auto listener = _eventListeners[targetId];
				listener->onEvent(msg["event"].get<std::string>(), msg.value("data", Json::object()));
			}
		}
		// Otherwise unexpected message.
		else
		{
			logger->error("received message is not a Response nor a Notification");
		}
	}

	std::string Channel::_makePayload(const Json& msg)
	{
		std::string nsPayload;
		size_t nsPayloadLen;
		size_t nsNumLen;
		size_t nsLen;

		nsPayload = msg.dump();
		nsPayloadLen = nsPayload.length();

		if (nsPayloadLen == 0)
		{
			nsNumLen = 1;
			WriteBuffer[0] = '0';
			WriteBuffer[1] = ':';
			WriteBuffer[2] = ',';
		}
		else
		{
			nsNumLen = static_cast<size_t>(std::ceil(std::log10(static_cast<double>(nsPayloadLen) + 1)));
			std::sprintf(reinterpret_cast<char*>(WriteBuffer), "%zu:", nsPayloadLen);
			std::memcpy(WriteBuffer + nsNumLen + 1, nsPayload.c_str(), nsPayloadLen);
			WriteBuffer[nsNumLen + nsPayloadLen + 1] = ',';
		}

		nsLen = nsNumLen + nsPayloadLen + 2;

		return std::string((char*)WriteBuffer, nsLen);
	}

}

