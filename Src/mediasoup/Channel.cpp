#include "common.hpp"
#include "Channel.hpp"
#include "child_process/Socket.hpp"
#include "Logger.hpp"

namespace rs
{
	// netstring length for a 65536 bytes payload.
	const int NS_MAX_SIZE = 65543;
	// Max time waiting for a response from the worker subprocess.
	const int REQUEST_TIMEOUT = 5000;
	static uint8_t WriteBuffer[NS_MAX_SIZE];

	Channel::Channel(Socket* producerSocket, Socket* consumerSocket, int pid)
	{
		DLOG(INFO) << "constructor()";

		this->_producerSocket = producerSocket;
		this->_consumerSocket = consumerSocket;
	

		// Read Channel responses/notifications from the worker.
		this->_consumerSocket->addEventListener("data", [=](json data)
		{
			std::string nsPayload = data.get<std::string>();
			try
			{
				// We can receive JSON messages (Channel messages) or log strings.
				switch (nsPayload[0])
				{
					// 123 = "{" (a Channel JSON messsage).
				case 123:
					this->_processMessage(json::parse(nsPayload));
					break;

					// 68 = "D" (a debug log).
				case 68:
					DLOG(INFO) << nsPayload.substr(1);
					break;

					// 87 = "W" (a warning log).
				case 87:
					LOG(WARNING) << nsPayload.substr(1);
					break;

					// 69 = "E" (an error log).
				case 69:
					LOG(ERROR) << nsPayload.substr(1);
					break;
					// 88 = 'X' (a dump log).
				case 88:
					// eslint-disable-next-line no-console
					DLOG(INFO) << nsPayload.substr(1);
					break;

				default:
					LOG(ERROR) << 
						"unexpected data: %s", nsPayload;
				}
			}
			catch (std::exception error)
			{
				LOG(ERROR) << "received invalid message:" << error.what();
			}

		});

		this->_consumerSocket->addEventListener("end", [=](json data)
		{
			DLOG(INFO) << "Consumer channel ended by the other side";
		});

		this->_consumerSocket->addEventListener("error", [=](json data)
		{
			LOG(ERROR) << "Consumer channel error:" << data.dump();
		});

		this->_producerSocket->addEventListener("end", [=](json data)
		{
			DLOG(INFO) << "Producer channel ended by the other side";
		});

		this->_producerSocket->addEventListener("error", [=](json data)
		{
			LOG(ERROR) << "Producer channel error:" << data.dump();
		});

		_consumerSocket->Start();
		_producerSocket->Start();
	}


	void Channel::close()
	{
		if (this->_closed)
			return;

		DLOG(INFO) << "close()";

		this->_closed = true;

		// Close every pending sent.
		for (auto sent : this->_sents)
		{
			sent.second.clear();
		}

// 		// Remove event listeners but leave a fake 'error' hander to avoid
// 		// propagation.
// 		this->_consumerSocket->removeAllListeners("end");
// 		this->_consumerSocket->removeAllListeners("error");
// 		this->_consumerSocket->on("error", () = > {});
// 
// 		this->_producerSocket->removeAllListeners("end");
// 		this->_producerSocket->removeAllListeners("error");
// 		this->_producerSocket->on("error", () = > {});
// 
// 		// Destroy the socket after a while to allow pending incoming messages.
// 		setTimeout(() = >
// 		{
// 			try { this->_producerSocket.destroy(); }
// 			catch (error) {}
// 			try { this->_consumerSocket.destroy(); }
// 			catch (error) {}
// 		}, 200);
	}

	Defer Channel::request(std::string method, const json& internal, const json& data)
	{
		uint32_t id = utils::randomNumber();

		DLOG(INFO) << "request() [method"<< method << ", id:" << id << "]";

		json request = {
			{ "id",id },
			{ "method", method },
			{ "internal", internal },
			{ "data", data }
		};

		std::string nsPayload = _makePayload(request);

		if (nsPayload.length() > NS_MAX_SIZE)
			return promise::reject(Error("request too big"));

		// This may raise if closed or remote side ended.
		try
		{
			this->_producerSocket->Write(nsPayload);
		}
		catch (std::exception error)
		{
			return promise::reject(Error(error.what()));
		}

		return newPromise([=](Defer d) {
			this->_sents.insert(std::make_pair(id, d));
		});
	}

	void Channel::addEventListener(std::string id, ChannelListener* listener)
	{
		_eventListeners.insert(std::make_pair(id, listener));
	}

	void Channel::off(std::string id)
	{
		_eventListeners.erase(id);
	}

	void Channel::_processMessage(const json& msg)
	{
		// If a Response, retrieve its associated Request.
		if (msg.count("id"))
		{
			uint32_t id = msg["id"].get<uint32_t>();

			if (msg.count("accepted") && msg["accepted"].get<bool>())
				DLOG(INFO) << "request succeeded [id:" << id << "]";
			else
				LOG(ERROR) << "request failed [id:" << id <<
				" reason:"<< msg["reason"].get<std::string>() << "]";

			if (!this->_sents.count(id))
			{
				LOG(ERROR) << "received Response does not match any sent Request";

				return;
			}

			auto sent = this->_sents[id];

			if (msg.value("accepted", false))
				sent.resolve(msg["data"]);
			else if (msg.value("rejected", false))
				sent.reject(Error(msg["reason"].get<std::string>()));
		}
		// If a Notification emit it to the corresponding entity.
		else if (msg.count("targetId") && msg.count("event"))
		{
			std::string targetId = msg["targetId"].get<std::string>();

			if (_eventListeners.count(targetId))
			{
				auto listener = _eventListeners[targetId];
				listener->onEvent(msg["event"].get<std::string>(), msg.value("data", json::object()));
			}
		}
		// Otherwise unexpected message.
		else
		{
			LOG(ERROR) << "received message is not a Response nor a Notification";
		}
	}

	std::string Channel::_makePayload(const json& msg)
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

