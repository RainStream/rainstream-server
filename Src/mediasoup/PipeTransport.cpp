#define MSC_CLASS "PipeTransport"

#include "common.hpp"
#include "PipeTransport.hpp"
#include "ortc.hpp"
#include "utils.hpp"
#include "errors.hpp"
#include "Logger.hpp"
#include "Producer.hpp"
#include "Consumer.hpp"
#include "Channel.hpp"



/**
 * @private
 * @emits sctpstatechange - (sctpState: SctpState)
 * @emits trace - (trace: TransportTraceEventData)
 */
PipeTransport::PipeTransport(const json& internal,
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
		{ "tuple"          , data["tuple"] },
		{ "sctpParameters" , data["sctpParameters"] },
		{ "sctpState"      , data["sctpState"] },
		{ "rtx"            , data["rtx"] },
		{ "srtpParameters" , data["srtpParameters"] }
	};

	this->_handleWorkerNotifications();
}

/**
 * Transport tuple.
 */
TransportTuple PipeTransport::tuple()
{
	return this->_data["tuple"];
}

/**
 * SCTP parameters.
 */
SctpParameters PipeTransport::sctpParameters()
{
	return this->_data["sctpParameters"];
}

/**
 * SCTP state.
 */
SctpState PipeTransport::sctpState()
{
	return this->_data["sctpState"];
}

/**
 * SRTP parameters.
 */
SrtpParameters PipeTransport::srtpParameters()
{
	return this->_data["srtpParameters"];
}

std::string PipeTransport::typeName()
{
	return "PipeTransport";
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
 * @emits sctpstatechange - (sctpState: SctpState)
 * @emits trace - (trace: TransportTraceEventData)
 */
EnhancedEventEmitter* PipeTransport::observer()
{
	return this->_observer;
}

/**
 * Close the PipeTransport.
 *
 * @override
 */
void PipeTransport::close()
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
void PipeTransport::routerClosed()
{
	if (this->_closed)
		return;

	if (this->_data.contains("sctpState"))
		this->_data["sctpState"] = "closed";

	Transport::routerClosed();
}

/**
 * Get PipeTransport stats.
 *
 * @override
 */
std::future<json> PipeTransport::getStats()
{
	MSC_DEBUG("getStats()");

	json ret = co_await this->_channel->request("transport.getStats", this->_internal["transportId"]);

	co_return ret;
}

/**
 * Provide the PipeTransport remote parameters.
 *
 * @override
 */
std::future<void> PipeTransport::connect(
	std::string ip,
	uint32_t port,
	SrtpParameters& srtpParameters
)
{
	MSC_DEBUG("connect()");

	json reqData = {
		{ "ip", ip },
		{ "port", port },
		{ "srtpParameters", srtpParameters }
	};

	json data =
		co_await this->_channel->request("transport.connect", this->_internal["transportId"], reqData);

	// Update data.
	this->_data["tuple"] = data["tuple"];
}

/**
 * Create a pipe Consumer.
 *
 * @override
 */
std::future<Consumer*> PipeTransport::consume(ConsumerOptions& options)
{
	MSC_DEBUG("consume()");

	std::string producerId = options.producerId;
	json& appData = options.appData;

	if (producerId.empty())
		MSC_THROW_ERROR("missing producerId");
	else if (!appData.is_null() && !appData.is_object())
		MSC_THROW_ERROR("if given, appData must be an object");

	Producer* producer = this->_getProducerById(producerId);

	if (!producer)
		MSC_THROW_ERROR("Producer with id \"%s\" not found", producerId.c_str());

	// This may throw.
	json rtpParameters = ortc::getPipeConsumerRtpParameters(
		producer->consumableRtpParameters(), this->_data["rtx"]);

	json internal = this->_internal;
	internal["consumerId"] = uuidv4();
	internal["producerId"] = producerId;

	json reqData =
	{
		{ "consumerId", uuidv4() },
		{ "producerId", producerId },
		{ "kind"                   , producer->kind() },
		{ "rtpParameters"          , rtpParameters },
		{ "type"                   , "pipe" },
		{ "consumableRtpEncodings" , producer->consumableRtpParameters()["encodings"] }
	};

	json status =
		co_await this->_channel->request("transport.consume", this->_internal["transportId"], reqData);

	json data = {
		{ "kind", producer->kind() },
		{ "rtpParameters", rtpParameters },
		{ "type" , "pipe" }
	};

	Consumer* consumer = new Consumer(
		internal,
		data,
		this->_channel,
		this->_payloadChannel,
		appData,
		status["paused"],
		status["producerPaused"]
	);

	this->_consumers.insert(std::make_pair(consumer->id(), consumer));
	consumer->on("@close", [=]() { this->_consumers.erase(consumer->id()); });
	consumer->on("@producerclose", [=]() { this->_consumers.erase(consumer->id()); });

	// Emit observer event.
	this->_observer->safeEmit("newconsumer", consumer);

	co_return consumer;
}

void PipeTransport::_handleWorkerNotifications()
{
	this->_channel->on(this->_internal["transportId"], [=](std::string event, const json& data)
	{
		if (event == "sctpstatechange")
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
