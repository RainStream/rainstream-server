#define MSC_CLASS "PlainTransport"

#include "common.hpp"
#include "PlainTransport.hpp"
#include "Channel.hpp"
#include "Logger.hpp"


/**
 * @private
 * @emits tuple - (tuple: TransportTuple)
 * @emits rtcptuple - (rtcpTuple: TransportTuple)
 * @emits sctpstatechange - (sctpState: SctpState)
 * @emits trace - (trace: TransportTraceEventData)
 */
PlainTransport::PlainTransport(const json& internal,
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

	this->_data =
	{
		{ "rtcpMux"        , data["rtcpMux"] },
		{ "comedia"        , data["comedia"] },
		{ "tuple"          , data["tuple"] },
		{ "rtcpTuple"      , data["rtcpTuple"] },
		{ "sctpParameters" , data["sctpParameters"] },
		{ "sctpState"      , data["sctpState"] },
		{ "srtpParameters" , data["srtpParameters"] }
	};

	this->_handleWorkerNotifications();
}

/**
 * Transport tuple.
 */
TransportTuple PlainTransport::tuple()
{
	return this->_data["tuple"];
}

/**
 * Transport RTCP tuple.
 */
TransportTuple PlainTransport::rtcpTuple()
{
	return this->_data["rtcpTuple"];
}

/**
 * SCTP parameters.
 */
SctpParameters PlainTransport::sctpParameters()
{
	return this->_data["sctpParameters"];
}

/**
 * SCTP state.
 */
SctpState PlainTransport::sctpState()
{
	return this->_data["sctpState"];
}

/**
 * SRTP parameters.
 */
SrtpParameters PlainTransport::srtpParameters()
{
	return this->_data["srtpParameters"];
}

std::string PlainTransport::typeName()
{
	return "PlainTransport";
}

/**
 * Observer.
 *
 * @override
 * @emits close
 * @emits newproducer - (producer: Producer)
 * @emits newconsumer - (producer: Producer)
 * @emits newdataproducer - (dataProducer: DataProducer)
 * @emits newdataconsumer - (dataProducer: DataProducer)
 * @emits tuple - (tuple: TransportTuple)
 * @emits rtcptuple - (rtcpTuple: TransportTuple)
 * @emits sctpstatechange - (sctpState: SctpState)
 * @emits trace - (trace: TransportTraceEventData)
 */
EnhancedEventEmitter* PlainTransport::observer()
{
	return this->_observer;
}

/**
 * Close the PlainTransport.
 *
 * @override
 */
void PlainTransport::close()
{
	if (this->_closed)
		return;

	if (this->_data.contains("sctpState"))
		this->_data["sctpState"] = "closed";

	Transport::close();
}

/**
 * Router was closed.
 *
 * @private
 * @override
 */
void PlainTransport::routerClosed()
{
	if (this->_closed)
		return;

	if (this->_data.contains("sctpState"))
		this->_data["sctpState"] = "closed";

	Transport::routerClosed();
}

/**
 * Get PlainTransport stats.
 *
 * @override
 */
std::future<json> PlainTransport::getStats()
{
	MSC_DEBUG("getStats()");

	json ret = co_await  this->_channel->request("transport.getStats", this->_internal);

	co_return ret;
}

/**
 * Provide the PlainTransport remote parameters.
 *
 * @override
 */
std::future<void> PlainTransport::connect(
	std::string ip,
	uint32_t port,
	uint32_t rtcpPort,
	SrtpParameters srtpParameters
)
{
	MSC_DEBUG("connect()");

	json reqData = {
		{ "ip", ip },
		{ "port", port },
		{ "rtcpPort", rtcpPort },
		{ "srtpParameters", srtpParameters }
	};

	json data =
		co_await this->_channel->request("transport.connect", this->_internal, reqData);

	// Update data.
	if (data.count("tuple"))
		this->_data["tuple"] = data["tuple"];

	if (data.count("rtcpTuple"))
		this->_data["rtcpTuple"] = data["rtcpTuple"];

	this->_data["srtpParameters"] = data["srtpParameters"];
}

void PlainTransport::_handleWorkerNotifications()
{
	this->_channel->on(this->_internal["transportId"], [=](std::string event, const json& data)
	{
		if (event == "tuple")
		{
			const json& tuple = data["tuple"];

			this->_data["tuple"] = tuple;

			this->safeEmit("tuple", tuple);

			// Emit observer event.
			this->_observer->safeEmit("tuple", tuple);
		}
		else if (event == "rtcptuple")
		{
			const json& rtcpTuple = data["rtcpTuple"];

			this->_data["rtcpTuple"] = rtcpTuple;

			this->safeEmit("rtcptuple", rtcpTuple);

			// Emit observer event.
			this->_observer->safeEmit("rtcptuple", rtcpTuple);
		}
		else if (event == "sctpstatechange")
		{
			std::string sctpState = data["sctpState"];

			this->_data["sctpState"] = sctpState;

			this->safeEmit("sctpstatechange", sctpState);

			// Emit observer event.
			this->_observer->safeEmit("sctpstatechange", sctpState);
		}
		else if (event == "trace")
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
}

