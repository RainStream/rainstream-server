#define MSC_CLASS "ChannelOrigin"

#include "common.h"
#include "Logger.h"
#include "utils.h"
#include "errors.h"
#include "ChannelOrigin.h"
#include "child_process/Socket.h"

namespace mediasoup {

// netstring length for a 4194304 bytes payload.
const int  NS_MESSAGE_MAX_LEN = 4194313;
const int  NS_PAYLOAD_MAX_LEN = 4194304;

ChannelOrigin::ChannelOrigin(Socket* producerSocket, Socket* consumerSocket, int pid)
	: Channel(pid)
	, _producerSocket(producerSocket)
	, _consumerSocket(consumerSocket)
{
	// Read Channel responses/notifications from the worker.
	this->_consumerSocket->on("data", [=](const std::string& nsPayload)
		{
			this->_receivePayload(nsPayload);
		});

	this->_consumerSocket->on("end", [=](json data)
		{
			MSC_DEBUG("Consumer channel ended by the other side");
		});

	this->_consumerSocket->on("error", [=](json data)
		{
			MSC_ERROR("Consumer channel error: %s", data.dump().c_str());
		});

	this->_producerSocket->on("end", [=](json data)
		{
			MSC_DEBUG("Producer channel ended by the other side");
		});

	this->_producerSocket->on("error", [=](json data)
		{
			MSC_ERROR("Producer channel error: %s", data.dump().c_str());
		});

	_consumerSocket->Start();
	//_producerSocket->Start();
}

void ChannelOrigin::subClose()
{
	// Remove event listeners but leave a fake "error" hander to avoid
	// propagation.
	this->_consumerSocket->removeAllListeners("end");
	this->_consumerSocket->removeAllListeners("error");
	this->_consumerSocket->on("error", []() {});
	this->_producerSocket->removeAllListeners("end");
	this->_producerSocket->removeAllListeners("error");
	this->_producerSocket->on("error", []() {});

	// Destroy the socket after a while to allow pending incoming messages.
	// 		setTimeout(() = >
	// 		{
	// 			try { this->_producerSocket.destroy(); }
	// 			catch (error) {}
	// 			try { this->_consumerSocket.destroy(); }
	// 			catch (error) {}
	// 		}, 200);
}

async_simple::coro::Lazy<json> ChannelOrigin::request(std::string method, std::optional<std::string> handlerId, const json& data/* = json()*/)
{
	this->_nextId < 4294967295 ? ++this->_nextId : (this->_nextId = 1);

	uint32_t id = this->_nextId;

	MSC_DEBUG("request() [method \"%s\", id: \"%d\"]", method.c_str(), id);

	if (!handlerId.has_value())
	{
		handlerId = "undefined";
	}

	std::string payload = data.is_null() ? "undefined" : data.dump();

	std::string request = Utils::Printf("%u:%s:%s:%s", id, method.c_str(), handlerId.value().c_str(), payload.c_str());

	if (request.length() > NS_MESSAGE_MAX_LEN)
		MSC_THROW_ERROR("Channel request too big");

	int size = request.size();

	// This may raise if closed or remote side ended.
	try
	{
		this->_producerSocket->Write((const uint8_t*)(int*)(&size), sizeof(size));
		this->_producerSocket->Write((const uint8_t*)request.data(), size);
	}
	catch (std::exception error)
	{
		MSC_THROW_ERROR("Channel request too big");
	}

	async_simple::Promise<json> t_promise;

	this->_sents.insert(std::make_pair(id, std::move(t_promise)));

	auto value = co_await std::move(this->_sents[id].getFuture());

	co_return value;
}
}
