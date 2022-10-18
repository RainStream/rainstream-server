#define MSC_CLASS "DirectTransport"

#include "common.hpp"
#include "Logger.hpp"
#include "errors.hpp"
#include "Channel.hpp"
#include "PayloadChannel.hpp"
#include "Producer.hpp"
#include "Consumer.hpp"
#include "DirectTransport.hpp"

DirectTransport::DirectTransport(const json& internal,
	const json& data,
	Channel* channel,
	PayloadChannel* payloadChannel,
	const json& appData,
	GetRouterRtpCapabilities getRouterRtpCapabilities,
	GetProducerById getProducerById,
	GetDataProducerById getDataProducerById)
	: Transport(internal, data, channel, payloadChannel,
		appData, getRouterRtpCapabilities,
		getProducerById, getDataProducerById)
{
	MSC_DEBUG("constructor()");

	// TODO
	this->_data = data;

	this->_handleWorkerNotifications();
}

void DirectTransport::close()
{
	if (this->_closed)
		return;

	Transport::close();
}

void DirectTransport::routerClosed()
{
	if (this->_closed)
		return;

	Transport::routerClosed();
}

task_t<json> DirectTransport::getStats()
{
	MSC_DEBUG("getStats()");

	json ret = co_await this->_channel->request("transport.getStats", this->_internal["transportId"]);

	co_return ret;
}

task_t<void> DirectTransport::connect()
{
	MSC_DEBUG("connect()");

	co_return;
}

task_t<void> DirectTransport::setMaxIncomingBitrate(uint32_t bitrate)
{
	MSC_THROW_UNSUPPORTED_ERROR(
		"setMaxIncomingBitrate() not implemented in DirectTransport");
}

task_t<void> DirectTransport::setMaxOutgoingBitrate(uint32_t bitrate)
{
	MSC_THROW_UNSUPPORTED_ERROR(
		"setMaxOutgoingBitrate() not implemented in DirectTransport");
}

std::string DirectTransport::typeName()
{
	return "DirectTransport";
}

void DirectTransport::_handleWorkerNotifications()
{
	this->_channel->on(this->_internal["transportId"], [=](std::string event, const json& data)
		{
			if (event == "trace")
			{
				const json& trace = data;

				this->safeEmit("trace", trace);

				// Emit observer event.
				this->_observer->safeEmit("trace", trace);
			}
			else
			{
				MSC_ERROR("ignoring unknown event \"%s\"", event.c_str());
			}
		});

	this->_payloadChannel->on(this->_internal["transportId"],
		[=](std::string event, json data, const uint8_t* payload, size_t payloadLen)
	{
			if (event == "rtcp")
			{
				if (this->closed())
					return;

				/*const packet = payload;

				this->safeEmit('rtcp', packet);*/
			}
			else
			{
				MSC_ERROR("ignoring unknown event \"%s\"", event.c_str());
			}
	});
}
