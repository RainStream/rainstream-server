#define MSC_CLASS "DataProducer"

#include "common.h"
#include "Logger.h"
#include "errors.h"
#include "Channel.h"
#include "PayloadChannel.h"
#include "DataProducer.h"

DataProducer::DataProducer(
	json internal,
	json data,
	Channel* channel,
	PayloadChannel* payloadChannel,
	json appData)
	: _observer(new EnhancedEventEmitter())
	, _internal(internal)
	, _data(data)
	, _channel(channel)
	, _payloadChannel(payloadChannel)
	, _appData(appData)
{
	MSC_DEBUG("constructor()");

	this->_handleWorkerNotifications();
}

DataProducer::~DataProducer()
{
	delete _observer;
	_observer = nullptr;
}

std::string DataProducer::id()
{
	return this->_internal["dataProducerId"];
}

bool DataProducer::closed()
{
	return this->_closed;
}

DataProducerType DataProducer::type()
{
	return this->_data["type"];
}

json DataProducer::sctpStreamParameters()
{
	return this->_data["sctpStreamParameters"];
}

std::string DataProducer::label()
{
	return this->_data["label"];
}

std::string DataProducer::protocol()
{
	return this->_data["protocol"];
}

json DataProducer::appData()
{
	return this->_appData;
}

void DataProducer::appData(json appData) // eslint-disable-line no-unused-vars
{
	MSC_THROW_ERROR("cannot override appData object");
}

EnhancedEventEmitter* DataProducer::observer()
{
	return this->_observer;
}

void DataProducer::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["dataProducerId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["dataProducerId"]);

	try
	{
		json reqData = { { "dataProducerId",(this->_internal["dataProducerId"]) } };
		this->_channel->request("transport.closeDataProducer", this->_internal, reqData);
	}
	catch (const std::exception& error)
	{
	}

	this->emit("@close");

	// Emit observer event.
	this->_observer->safeEmit("close");
}

void DataProducer::transportClosed()
{
	if (this->_closed)
		return;

	MSC_DEBUG("transportClosed()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["dataProducerId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["dataProducerId"]);

	this->safeEmit("transportclose");

	// Emit observer event.
	this->_observer->safeEmit("close");
}

std::future<json> DataProducer::dump()
{
	MSC_DEBUG("dump()");

	json ret = co_await this->_channel->request("dataProducer.dump", this->_internal["dataProducerId"]);

	co_return ret;
}

std::future<json> DataProducer::getStats()
{
	MSC_DEBUG("getStats()");

	json ret = co_await this->_channel->request("dataProducer.getStats", this->_internal["dataProducerId"]);

	co_return ret;
}

//void DataProducer::send(message | Buffer, ppid ? )
//{
//	MSC_DEBUG("send()");
//
//	if (typeof message != "string" && !Buffer.isBuffer(message))
//	{
//		MSC_THROW_ERROR("message must be a string or a Buffer");
//	}
//
//	/*
//	 * +-------------------------------+----------+
//	 * | Value                         | SCTP     |
//	 * |                               | PPID     |
//	 * +-------------------------------+----------+
//	 * | WebRTC String                 | 51       |
//	 * | WebRTC Binary Partial         | 52       |
//	 * | (Deprecated)                  |          |
//	 * | WebRTC Binary                 | 53       |
//	 * | WebRTC String Partial         | 54       |
//	 * | (Deprecated)                  |          |
//	 * | WebRTC String Empty           | 56       |
//	 * | WebRTC Binary Empty           | 57       |
//	 * +-------------------------------+----------+
//	 */
//
//	if (typeof ppid != "uint32_t")
//	{
//		ppid = (typeof message == "string")
//			? message.length > 0 ? 51 : 56
//			: message.length > 0 ? 53 : 57;
//	}
//
//	// Ensure we honor PPIDs.
//	if (ppid == 56)
//		message = " ";
//	else if (ppid == 57)
//		message = Buffer.alloc(1);
//
//	const notifData = { ppid };
//
//	this->_payloadChannel.notify(
//		"dataProducer.send", this->_internal, notifData, message);
//}

void DataProducer::_handleWorkerNotifications()
{
	// No need to subscribe to any event.
}
