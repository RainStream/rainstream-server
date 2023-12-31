#define MSC_CLASS "Transport"

#include "common.h"
#include "Transport.h"
#include "utils.h"
#include "ortc.h"
#include "errors.h"
#include "Logger.h"
#include "Channel.h"
#include "PayloadChannel.h"
#include "Producer.h"
#include "Consumer.h"
#include "DataProducer.h"
#include "DataConsumer.h"
//#include "SctpParameters.h"

namespace mediasoup {

Transport::Transport(
	const json& internal,
	const json& data,
	Channel* channel,
	PayloadChannel* payloadChannel,
	const json& appData,
	GetRouterRtpCapabilities getRouterRtpCapabilities,
	GetProducerById getProducerById,
	GetDataProducerById getDataProducerById)
	: _observer(new EnhancedEventEmitter())
	, _internal(internal)
	, _data(data)
	, _channel(channel)
	, _payloadChannel(payloadChannel)
	, _appData(appData)
	, _getRouterRtpCapabilities(getRouterRtpCapabilities)
	, _getProducerById(getProducerById)
	, _getDataProducerById(getDataProducerById)
{
	MSC_DEBUG("constructor()");
}

Transport::~Transport()
{
	delete _observer;
	_observer = nullptr;
}

std::string Transport::id()
{
	return this->_internal["transportId"];
}

bool Transport::closed()
{
	return this->_closed;
}

json Transport::appData()
{
	return this->_appData;
}

void Transport::appData(json appData) // eslint-disable-line no-unused-vars
{
	MSC_THROW_ERROR("cannot override appData object");
}

EnhancedEventEmitter* Transport::observer()
{
	return this->_observer;
}

Channel* Transport::channelForTesting()
{
	return this->_channel;
}

void Transport::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["transportId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["transportId"]);

	try
	{
		json reqData = { {"transportId", this->_internal["transportId"]}};
		this->_channel->request("router.closeTransport", this->_internal["routerId"], reqData);
	}
	catch (const std::exception&)
	{
	}

	// Close every Producer.
	for (auto [key, producer] : this->_producers)
	{
		producer->transportClosed();
		// Must tell the Router.
		this->emit("@producerclose", producer);

		delete producer;
	}
	this->_producers.clear();

	// Close every Consumer.
	for (auto [key, consumer] : this->_consumers)
	{
		consumer->transportClosed();

		delete consumer;
	}
	this->_consumers.clear();

	// Close every DataProducer.
	for (auto [key, dataProducer] : this->_dataProducers)
	{
		dataProducer->transportClosed();

		// Must tell the Router.
		this->emit("@dataproducerclose", dataProducer);

		delete dataProducer;
	}
	this->_dataProducers.clear();

	// Close every DataConsumer.
	for (auto [key, dataConsumer] : this->_dataConsumers)
	{
		dataConsumer->transportClosed();

		delete dataConsumer;
	}
	this->_dataConsumers.clear();

	this->emit("@close");
	// Emit observer event.
	this->_observer->safeEmit("close");
}

void Transport::routerClosed()
{
	if (this->_closed)
		return;

	MSC_DEBUG("routerClosed()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["transportId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["transportId"]);

	// Close every Producer.
	for (auto [key, producer] : this->_producers)
	{
		producer->transportClosed();

		delete producer;
	}
	this->_producers.clear();

	// Close every Consumer.
	for (auto [key, consumer] : this->_consumers)
	{
		consumer->transportClosed();

		delete consumer;
	}
	this->_consumers.clear();

	// Close every DataProducer.
	for (auto [key, dataProducer] : this->_dataProducers)
	{
		dataProducer->transportClosed();

		delete dataProducer;
	}
	this->_dataProducers.clear();

	// Close every DataConsumer.
	for (auto [key, dataConsumer] : this->_dataConsumers)
	{
		dataConsumer->transportClosed();

		delete dataConsumer;
	}
	this->_dataConsumers.clear();

	this->safeEmit("routerclose");
	// Emit observer event.
	this->_observer->safeEmit("close");
}

void Transport::listenServerClosed()
{
	if (this->_closed)
		return;

	MSC_DEBUG("routerClosed()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["transportId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["transportId"]);

	// Close every Producer.
	for (auto [key, producer] : this->_producers)
	{
		producer->transportClosed();
		// NOTE: No need to tell the Router since it already knows (it has
		// been closed in fact).

		delete producer;
	}
	this->_producers.clear();

	// Close every Consumer.
	for (auto& [key, consumer] : this->_consumers)
	{
		consumer->transportClosed();

		delete consumer;
	}
	this->_consumers.clear();
	
	// Close every DataProducer.
	for (auto [key, dataProducer] : this->_dataProducers)
	{
		dataProducer->transportClosed();

		delete dataProducer;
	}
	this->_dataProducers.clear();

	// Close every DataConsumer.
	for (auto [key, dataConsumer] : this->_dataConsumers)
	{
		dataConsumer->transportClosed();

		delete dataConsumer;
	}
	this->_dataConsumers.clear();

	// Need to emit this event to let the parent Router know since
	// transport.listenServerClosed() is called by the listen server.
	// NOTE: Currently there is just WebRtcServer for WebRtcTransports.
	this->emit("@listenserverclose");
	this->safeEmit("listenserverclose");
	// Emit observer event.
	this->_observer->safeEmit("close");
}

async_simple::coro::Lazy<json> Transport::dump()
{
	MSC_DEBUG("dump()");

	json ret = co_await this->_channel->request("transport.dump", this->_internal["transportId"]);

	co_return ret;
}

async_simple::coro::Lazy<json> Transport::getStats()
{
	// Should not happen.
	MSC_THROW_ERROR("method not implemented in the subclass");
}

async_simple::coro::Lazy<void> Transport::connect(json& params)
{
	// Should not happen.
	MSC_THROW_ERROR("method not implemented in the subclass");
}

async_simple::coro::Lazy<void> Transport::setMaxIncomingBitrate(uint32_t bitrate)
{
	MSC_DEBUG("setMaxIncomingBitrate() [bitrate:%d]", bitrate);

	json reqData = { {"bitrate", bitrate} };

	co_await this->_channel->request(
		"transport.setMaxIncomingBitrate", this->_internal["transportId"], reqData);

	co_return;
}

async_simple::coro::Lazy<void> Transport::setMaxOutgoingBitrate(uint32_t bitrate) {
	MSC_DEBUG("setMaxOutgoingBitrate() [bitrate:%d]", bitrate);

	json reqData = { {"bitrate", bitrate} };

	co_await this->_channel->request("transport.setMaxOutgoingBitrate", this->_internal["transportId"], reqData);

	co_return;
}

async_simple::coro::Lazy<Producer*> Transport::produce(
	std::string id,
	std::string kind,
	json rtpParameters,
	bool paused/* = false*/,
	bool keyFrameRequestDelay/* = 0*/,
	json appData/* = json()*/
)
{
	static std::set<std::string> kinds = { "audio","video" };

	MSC_DEBUG("produce()");

	if (!id.empty() && this->_producers.count(id))
		MSC_THROW_TYPE_ERROR("a Producer with same id \"%s\" already exists", id.c_str());
	else if (!kinds.count(kind))
		MSC_THROW_TYPE_ERROR("invalid kind \"%s\"", kind.c_str());
	else if (!appData.is_null() && !appData.is_object())
		MSC_THROW_TYPE_ERROR("if given, appData must be an object");

	// This may throw.
	ortc::validateRtpParameters(rtpParameters);

	// If missing or empty encodings, add one.
	if (
		!rtpParameters.count("encodings") ||
		!rtpParameters["encodings"].is_array() ||
		rtpParameters["encodings"].size() == 0
		)
	{
		rtpParameters["encodings"] = json::array();
	}

	// Don"t do this in PipeTransports since there we must keep CNAME value in
	// each Producer.
	if (this->typeName() != "PipeTransport")
	{
		// If CNAME is given and we don"t have yet a CNAME for Producers in this
		// Transport, take it.
		if (this->_cnameForProducers.empty() && rtpParameters.count("rtcp") && rtpParameters["rtcp"].count("cname"))
		{
			this->_cnameForProducers = rtpParameters["rtcp"].value("cname", uuidv4().substr(0, 8));
		}
		// Otherwise if we don"t have yet a CNAME for Producers and the RTP parameters
		// do not include CNAME, create a random one.
		else if (this->_cnameForProducers.empty())
		{
			this->_cnameForProducers = uuidv4().substr(0, 8);
		}

		// Override Producer"s CNAME.
		rtpParameters["rtcp"] = rtpParameters.value("rtcp", json::object());
		rtpParameters["rtcp"]["cname"] = this->_cnameForProducers;
	}

	json routerRtpCapabilities = this->_getRouterRtpCapabilities();

	// This may throw.
	json rtpMapping = ortc::getProducerRtpParametersMapping(
		rtpParameters, routerRtpCapabilities);

	// This may throw.
	json consumableRtpParameters = ortc::getConsumableRtpParameters(
		kind, rtpParameters, routerRtpCapabilities, rtpMapping);


	json reqData = {
		{ "producerId",  id.empty() ? uuidv4() : id },
		{ "kind", kind },
		{ "rtpParameters", rtpParameters },
		{ "rtpMapping", rtpMapping },
		{ "keyFrameRequestDelay", keyFrameRequestDelay },
		{ "paused", paused },
	};

	json status =
		co_await this->_channel->request("transport.produce", this->_internal["transportId"], reqData);

	json data =
	{
		{ "kind", kind },
		{ "rtpParameters", rtpParameters },
		{ "type", status["type"] },
		{ "consumableRtpParameters", consumableRtpParameters }
	};

	json internal = this->_internal;
	internal["producerId"] = reqData["producerId"];

	Producer* producer = new Producer(
		internal,
		data,
		this->_channel,
		this->_payloadChannel,
		appData,
		paused
	);

	this->_producers.insert(std::make_pair(producer->id(), producer));
	producer->on("@close", [=]()
	{
		this->_producers.erase(producer->id());
		this->emit("@producerclose", producer);
	});

	this->emit("@newproducer", producer);
	// Emit observer event.
	this->_observer->safeEmit("newproducer", producer);

	co_return producer;
}

async_simple::coro::Lazy<Consumer*> Transport::consume(ConsumerOptions& options)
{
	MSC_DEBUG("consume()");

	std::string producerId = options.producerId;
	json& rtpCapabilities = options.rtpCapabilities;
	bool paused = options.paused;
	std::optional<std::string> mid = options.mid;
	bool pipe = options.pipe;
	json& preferredLayers = options.preferredLayers;
	bool ignoreDtx = options.ignoreDtx;
	json& appData = options.appData;

	if(producerId.empty())
		MSC_THROW_TYPE_ERROR("missing producerId");
	else if (!appData.is_null() && !appData.is_object())
		MSC_THROW_ERROR("if given, appData must be an object");
	else if (mid.has_value() && mid->empty())
		MSC_THROW_ERROR("if given, mid must be non empty string");

	// This may throw.
	ortc::validateRtpCapabilities(rtpCapabilities);

	Producer* producer = this->_getProducerById(producerId);

	if (!producer)
		MSC_THROW_ERROR("Producer with id \"%s\" not found", producerId.c_str());

	json consumableRtpParameters = producer->consumableRtpParameters();

	// This may throw.
	json rtpParameters = ortc::getConsumerRtpParameters(
		consumableRtpParameters, rtpCapabilities, pipe);

	// Set MID.
	if (!pipe)
	{
		if (mid)
		{
			rtpParameters["mid"] = mid.value();
		}
		else
		{
			rtpParameters["mid"] = Utils::Printf("%ud", this->_nextMidForConsumers++);

			// We use up to 8 bytes for MID (string).
			if (this->_nextMidForConsumers == 100000000)
			{
				MSC_ERROR("consume() | reaching max MID value \"${%ud}\"", this->_nextMidForConsumers);

				this->_nextMidForConsumers = 0;
			}
		}	
	}	

	json reqData =
	{
		{ "consumerId", uuidv4() },
		{ "producerId", producerId },
		{ "kind", producer->kind() },
		{ "rtpParameters", rtpParameters },
		{ "type", producer->type() },
		{ "consumableRtpEncodings", producer->consumableRtpParameters()["encodings"] },
		{ "paused", paused},
		{ "preferredLayers", preferredLayers },
		{ "ignoreDtx", ignoreDtx }

	};

	json status =
		co_await this->_channel->request("transport.consume", this->_internal["transportId"], reqData);

	json data = {
		{ "producerId", producerId },
		{ "kind", producer->kind() },
		{ "rtpParameters", rtpParameters },
		{ "type", pipe ? "pipe" : producer->type() }
	};

	json internal = this->_internal;
	internal["consumerId"] = reqData["consumerId"];

	Consumer* consumer = new Consumer(
		internal,
		data,
		this->_channel,
		this->_payloadChannel,
		appData,
		status["paused"],
		status["producerPaused"],
		status["score"],
		status["preferredLayers"]
	);

	this->_consumers.insert(std::make_pair(consumer->id(), consumer));
	consumer->on("@close", [=]() { this->_consumers.erase(consumer->id()); });
	consumer->on("@producerclose", [=]() { this->_consumers.erase(consumer->id()); });

	// Emit observer event.
	this->_observer->safeEmit("newconsumer", consumer);

	co_return consumer;
}

async_simple::coro::Lazy<DataProducer*> Transport::produceData(DataProducerOptions& options)
{
	MSC_DEBUG("produceData()");

	const std::string& id = options.id;
	const json& appData = options.appData;
	json sctpStreamParameters = options.sctpStreamParameters;
	const std::string& label = options.label;
	const std::string& protocol = options.protocol;

	if (!id.empty() && this->_dataProducers.contains(id))
		MSC_THROW_ERROR("a DataProducer with same id \"%s\" already exists", id.c_str());
	else if (!appData.is_null() && !appData.is_object())
			MSC_THROW_ERROR("if given, appData must be an object");

	DataProducerType type ;

	// If this is not a DirectTransport, sctpStreamParameters are required.
	if (this->typeName() != "DirectTransport")
	{
		type = "sctp";

		// This may throw.
		ortc::validateSctpStreamParameters(sctpStreamParameters);
	}
	// If this is a DirectTransport, sctpStreamParameters must not be given.
	else
	{
		type = "direct";

		if (!sctpStreamParameters.is_null())
		{
			MSC_WARN(
				"produceData() | sctpStreamParameters are ignored when producing data on a DirectTransport");
		}
	}
	
	json reqData =
	{
		{ "dataProducerId", uuidv4() },
		{ "type", type },
		{ "sctpStreamParameters", sctpStreamParameters },
		{ "label", label },
		{ "protocol", protocol }
	};

	json data =
		co_await this->_channel->request("transport.produceData", this->_internal["transportId"], reqData);

	json internal = this->_internal;
	internal["dataProducerId"] = reqData["dataProducerId"];

	DataProducer* dataProducer = new DataProducer(
			internal,
			data,
			this->_channel,
			this->_payloadChannel,
			appData);

	this->_dataProducers.insert(std::pair(dataProducer->id(), dataProducer));
	dataProducer->on("@close", [=]()
	{
		this->_dataProducers.erase(dataProducer->id());
		this->emit("@dataproducerclose", dataProducer);
	});

	this->emit("@newdataproducer", dataProducer);

	// Emit observer event.
	this->_observer->safeEmit("newdataproducer", dataProducer);

	co_return dataProducer;
}

	
async_simple::coro::Lazy<DataConsumer*> Transport::consumeData(DataConsumerOptions& options)
{
	MSC_DEBUG("consumeData()");

	const std::string& dataProducerId = options.dataProducerId;
	const json& appData = options.appData;

	if (dataProducerId.empty())
		MSC_THROW_ERROR("missing dataProducerId");
	else if (!appData.is_null() && !appData.is_object())
		MSC_THROW_ERROR("if given, appData must be an object");

	DataProducer* dataProducer = this->_getDataProducerById(dataProducerId);

	if (!dataProducer)
		MSC_THROW_ERROR("DataProducer with id \"%s\" not found", dataProducerId.c_str());

	DataConsumerType type;
	json sctpStreamParameters;
	uint32_t sctpStreamId;

	// If this is not a DirectTransport, use sctpStreamParameters from the
	// DataProducer (if type "sctp") unless they are given in method parameters.
	if (this->typeName() != "DirectTransport")
	{
		type = "sctp";
		sctpStreamParameters = Utils::clone(dataProducer->sctpStreamParameters());

		// Override if given.
		if (options.ordered.has_value())
			sctpStreamParameters["ordered"] = options.ordered.value();

		if (options.maxPacketLifeTime.has_value())
			sctpStreamParameters["maxPacketLifeTime"] = options.maxPacketLifeTime.value();;

		if (options.maxRetransmits.has_value())
			sctpStreamParameters["maxRetransmits"] = options.maxRetransmits.value();;

		// This may throw.
		sctpStreamId = this->getNextSctpStreamId();

		this->_sctpStreamIds[sctpStreamId] = 1;
		sctpStreamParameters["streamId"] = sctpStreamId;
	}
	// If this is a DirectTransport, sctpStreamParameters must not be used.
	else
	{
		type = "direct";

		if (
			options.ordered.has_value() ||
			options.maxPacketLifeTime.has_value() ||
			options.maxRetransmits.has_value()
			)
		{
			MSC_WARN(
				"consumeData() | ordered, maxPacketLifeTime and maxRetransmits are ignored when consuming data on a DirectTransport");
		}
	}

	std::string label = dataProducer->label();
	std::string protocol = dataProducer->protocol();

	json reqData =
	{
		{ "dataConsumerId", uuidv4() },
		{ "dataProducerId", dataProducerId },
		{ "type", type },
		{ "sctpStreamParameters", sctpStreamParameters },
		{ "label", label },
		{ "protocol", protocol },
	};

	json data =
		co_await this->_channel->request("transport.consumeData", this->_internal["transportId"], reqData);

	json internal = this->_internal;
	internal["dataConsumerId"] = reqData["dataConsumerId"];

	DataConsumer* dataConsumer = new DataConsumer(
		internal,
		data,
		this->_channel,
		this->_payloadChannel,
		appData
	);

	this->_dataConsumers.insert(std::pair(dataConsumer->id(), dataConsumer));
	dataConsumer->on("@close", [=]()
	{
		this->_dataConsumers.erase(dataConsumer->id());

		if (this->_sctpStreamIds.size())
			this->_sctpStreamIds[sctpStreamId] = 0;
	});
	dataConsumer->on("@dataproducerclose", [=]()
	{
		this->_dataConsumers.erase(dataConsumer->id());

		if (this->_sctpStreamIds.size())
			this->_sctpStreamIds[sctpStreamId] = 0;
	});

	// Emit observer event.
	this->_observer->safeEmit("newdataconsumer", dataConsumer);

	co_return dataConsumer;
}

async_simple::coro::Lazy<void> Transport::enableTraceEvent(std::vector<TransportTraceEventType> types)
{
	MSC_DEBUG("pause()");

	json reqData = { { "types", types } };

	co_await this->_channel->request(
		"transport.enableTraceEvent", this->_internal["transportId"], reqData);
}

uint32_t Transport::getNextSctpStreamId()
{
	if (
		!this->_data.count("sctpParameters") ||
		!this->_data["sctpParameters"]["MIS"].is_number()
		)
	{
		MSC_THROW_ERROR("missing data.sctpParameters.MIS");
	}

	uint32_t numStreams = this->_data["sctpParameters"].value("MIS", 0);

	if (!this->_sctpStreamIds.size())
		this->_sctpStreamIds.resize(numStreams, 0);

	uint32_t sctpStreamId;

	for (uint32_t idx = 0; idx < this->_sctpStreamIds.size(); ++idx)
	{
		sctpStreamId = (this->_nextSctpStreamId + idx) % this->_sctpStreamIds.size();

		if (!this->_sctpStreamIds[sctpStreamId])
		{
			this->_nextSctpStreamId = sctpStreamId + 1;

			return sctpStreamId;
		}
	}

	MSC_THROW_ERROR("no sctpStreamId available");
}

}
