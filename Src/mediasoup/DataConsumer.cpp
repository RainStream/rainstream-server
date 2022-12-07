#define MSC_CLASS "DataConsumer"

#include "common.h"
#include "Logger.h"
#include "errors.h"
#include "Channel.h"
#include "PayloadChannel.h"
#include "DataConsumer.h"


DataConsumer::DataConsumer(
	json internal,
	json data,
	Channel* channel,
	PayloadChannel* payloadChannel,
	json appData
)
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

DataConsumer::~DataConsumer()
{
	delete _observer;
	_observer = nullptr;
}

std::string DataConsumer::id()
{
	return this->_internal["dataConsumerId"];
}

std::string DataConsumer::dataProducerId()
{
	return this->_internal["dataProducerId"];
}

bool DataConsumer::closed()
{
	return this->_closed;
}

DataConsumerType DataConsumer::type()
{
	return this->_data["type"];
}

json DataConsumer::sctpStreamParameters()
{
	return this->_data["sctpStreamParameters"];
}

std::string DataConsumer::label()
{
	return this->_data["label"];
}

std::string DataConsumer::protocol()
{
	return this->_data["protocol"];
}

json DataConsumer::appData()
{
	return this->_appData;
}

void DataConsumer::appData(json appData)
{
	MSC_THROW_ERROR("cannot override appData object");
}

EnhancedEventEmitter* DataConsumer::observer()
{
	return this->_observer;
}

void DataConsumer::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["dataConsumerId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["dataConsumerId"]);

	try
	{
		json reqData = { { "dataConsumerId", this->_internal["dataConsumerId"] } };
		this->_channel->request("transport.closeDataConsumer", this->_internal["transportId"], reqData);
	}
	catch (const std::exception&)
	{

	}

	this->emit("@close");

	// Emit observer event.
	this->_observer->safeEmit("close");
}

void DataConsumer::transportClosed()
{
	if (this->_closed)
		return;

	MSC_DEBUG("transportClosed()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["dataConsumerId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["dataConsumerId"]);

	this->safeEmit("transportclose");

	// Emit observer event.
	this->_observer->safeEmit("close");
}

std::future<json> DataConsumer::dump()
{
	MSC_DEBUG("dump()");

	json ret = co_await this->_channel->request("dataConsumer.dump", this->_internal["dataConsumerId"]);

	co_return ret;
}

std::future<json> DataConsumer::getStats()
{
	MSC_DEBUG("getStats()");

	json ret = co_await this->_channel->request("dataConsumer.getStats", this->_internal["dataConsumerId"]);

	co_return ret;
}

/**
* Set buffered amount low threshold.
*/
std::future<void> DataConsumer::setBufferedAmountLowThreshold(uint32_t threshold)
{
	MSC_DEBUG("setBufferedAmountLowThreshold()[threshold:%d]", threshold);

	json reqData = { { "threshold", threshold } };

	co_await this->_channel->request(
		"dataConsumer.setBufferedAmountLowThreshold", this->_internal["dataConsumerId"], reqData);
}

//std::future<void> DataConsumer::send(message: string | Buffer, ppid ? : number)
//{
//	if (typeof message != = 'string' && !Buffer.isBuffer(message))
//	{
//		throw new TypeError('message must be a string or a Buffer');
//	}
//
///*
// * +-------------------------------+----------+
// * | Value                         | SCTP     |
// * |                               | PPID     |
// * +-------------------------------+----------+
// * | WebRTC String                 | 51       |
// * | WebRTC Binary Partial         | 52       |
// * | (Deprecated)                  |          |
// * | WebRTC Binary                 | 53       |
// * | WebRTC String Partial         | 54       |
// * | (Deprecated)                  |          |
// * | WebRTC String Empty           | 56       |
// * | WebRTC Binary Empty           | 57       |
// * +-------------------------------+----------+
// */
//
//if (typeof ppid != = 'number')
//{
//	ppid = (typeof message == = 'string')
//		? message.length > 0 ? 51 : 56
//		: message.length > 0 ? 53 : 57;
//}
//
//// Ensure we honor PPIDs.
//if (ppid == = 56)
//	message = ' ';
//else if (ppid == = 57)
//	message = Buffer.alloc(1);
//
//const requestData = String(ppid);
//
//await this.#payloadChannel.request(
//	'dataConsumer.send', this.#internal.dataConsumerId, requestData, message);
//}

std::future<size_t> DataConsumer::getBufferedAmount()
{
	MSC_DEBUG("getBufferedAmount()");

	json ret =
		co_await this->_channel->request("dataConsumer.getBufferedAmount", this->_internal["dataConsumerId"]);

	size_t bufferedAmount = ret["bufferedAmount"];

	co_return bufferedAmount;
}


void DataConsumer::_handleWorkerNotifications()
{
	this->_channel->on(this->_internal["dataConsumerId"], [=](std::string event, const json& data)
		{
			if (event == "dataproducerclose")
			{
				if (this->_closed)
					return;

				this->_closed = true;

				// Remove notification subscriptions.
				this->_channel->removeAllListeners(this->_internal["dataConsumerId"]);
				this->_payloadChannel->removeAllListeners(this->_internal["dataConsumerId"]);

				this->emit("@dataproducerclose");
				this->safeEmit("dataproducerclose");

				// Emit observer event.
				this->_observer->safeEmit("close");
			}
			else if (event == "sctpsendbufferfull")
			{
				this->safeEmit("sctpsendbufferfull");
			}
			else if (event == "bufferedamountlow")
			{
				std::string bufferedAmount = data["bufferedAmount"];
				this->safeEmit("bufferedamountlow", bufferedAmount);
			}
			else
			{
				MSC_ERROR("ignoring unknown event \"%s\" in channel listener", event.c_str());
			}
		});

	this->_payloadChannel->on(
		this->_internal["dataConsumerId"],
		[=](std::string event, json data, const uint8_t* payload, size_t payloadLen)
		{
			if (event == "message")
			{
				if (this->_closed)
					return;

				uint32_t ppid = data["ppid"];
				std::string message((const char*)payload, payloadLen);

				this->safeEmit("message", message, ppid);
			}
			else
			{
				MSC_ERROR("ignoring unknown event \"%s\" in payload channel listener", event.c_str());
			}

		});
}
