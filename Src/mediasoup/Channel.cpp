#include "common.hpp"
#include "Channel.hpp"
#include "child_process/Socket.hpp"
#include "Logger.hpp"
#include <netstring.h>


// netstring length for a 4194304 bytes payload.
const int  NS_MESSAGE_MAX_LEN = 4194313;
const int  NS_PAYLOAD_MAX_LEN = 4194304;

static uint8_t WriteBuffer[NS_MESSAGE_MAX_LEN];

Channel::Channel(Socket* producerSocket, Socket* consumerSocket, int pid)
	: logger(new Logger("Channel"))
{
	logger->debug("constructor()");

	this->_producerSocket = producerSocket;
	this->_consumerSocket = consumerSocket;


	// Read Channel responses/notifications from the worker.
	this->_consumerSocket->on("data", [=](const std::string& nsPayload)
	{
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
				logger->debug(nsPayload.substr(1).c_str());
				break;

				// 87 = "W" (a warning log).
			case 87:
				logger->warn(nsPayload.substr(1).c_str());
				break;

				// 69 = "E" (an error log).
			case 69:
				logger->error(nsPayload.substr(1).c_str());
				break;
				// 88 = "X" (a dump log).
			case 88:
				// eslint-disable-next-line no-console
				logger->debug(nsPayload.substr(1).c_str());
				break;

			default:
				logger->error("unexpected data: %s", nsPayload.c_str());
			}
		}
		catch (std::exception error)
		{
			logger->error("received invalid message : " , error.what());
		}

	});

	this->_consumerSocket->on("end", [=](json data)
	{
		logger->debug("Consumer channel ended by the other side");
	});

	this->_consumerSocket->on("error", [=](json data)
	{
		logger->error("Consumer channel error:", data.dump().c_str());
	});

	this->_producerSocket->on("end", [=](json data)
	{
		logger->debug("Producer channel ended by the other side");
	});

	this->_producerSocket->on("error", [=](json data)
	{
		logger->error("Producer channel error:", data.dump().c_str());
	});

	_consumerSocket->Start();
	_producerSocket->Start();
}


void Channel::close()
{
	if (this->_closed)
		return;

	logger->debug("close()");

	this->_closed = true;

	// Close every pending sent.
// 		for (auto sent : this->_sents)
// 		{
// 			sent.second.clear();
// 		}

// 		// Remove event listeners but leave a fake "error" hander to avoid
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

std::future<json> Channel::request(std::string method, const json& internal, const json& data)
{
	this->_nextId < 4294967295 ? ++this->_nextId : (this->_nextId = 1);

	uint32_t id = this->_nextId;

	logger->debug("request() [method \"%s\", id: \"%d\"]", method.c_str(), id);

	if (this->_closed)
		throw new InvalidStateError("Channel closed");

	json request = {
		{ "id",id },
		{ "method", method },
		{ "internal", internal },
		{ "data", data }
	};

	std::string nsPayload = _makePayload(request);

	if (nsPayload.length() > NS_MESSAGE_MAX_LEN)
		throw new Error("Channel request too big");

	// This may raise if closed or remote side ended.
	try
	{
		this->_producerSocket->Write(nsPayload);
	}
	catch (std::exception error)
	{
		throw new Error("Channel request too big");
	}

	std::promise<json> promise;
	this->_sents.insert(std::make_pair(id, std::move(promise)));

	return this->_sents[id].get_future();
}

void Channel::_processMessage(const json& msg)
{
	// If a Response, retrieve its associated Request.
	if (msg.count("id"))
	{
		uint32_t id = msg["id"].get<uint32_t>();

		if (msg.count("accepted") && msg["accepted"].get<bool>())
			logger->debug("request succeeded [id:\"%d\"]",id);
		else
			logger->error("request failed [id:\"%d\"] reason:%s]", 
				id, msg["reason"].get<std::string>().c_str());

		if (!this->_sents.count(id))
		{
			logger->error("received Response does not match any sent Request");
			return;
		}

		std::promise<json> sent = std::move(this->_sents[id]);
		this->_sents.erase(id);

		if (msg.value("accepted", false))
			sent.set_value(msg["data"]);
		else if (msg.value("rejected", false))
			sent.set_exception(std::make_exception_ptr(Error(msg["reason"].get<std::string>())));

	}
	// If a Notification emit it to the corresponding entity.
	else if (msg.count("targetId") && msg.count("event"))
	{
		std::string targetId = msg["targetId"].get<std::string>();

		this->emit(targetId, msg["event"].get<std::string>(), msg.value("data", json::object()));
	}
	// Otherwise unexpected message.
	else
	{
		logger->error("received message is not a Response nor a Notification");
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

