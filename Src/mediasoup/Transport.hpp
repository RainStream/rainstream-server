
#pragma once 

#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"
#include "utils.hpp"
#include "ortc.hpp"
#include "Channel.hpp"
//#include "PayloadChannel.hpp"
#include "Producer.hpp"
#include "Consumer.hpp"
//#include "DataProducer.hpp"
//#include "DataConsumer.hpp"
#include "RtpParameters.hpp"
//#include "SctpParameters.hpp"

struct TransportListenIp
{
	/**
	 * Listening IPv4 or IPv6.
	 */
	std::string ip;

	/**
	 * Announced IPv4 or IPv6 (useful when running mediasoup behind NAT with
	 * private IP).
	 */
	std::string announcedIp;
};

/**
 * Transport protocol.
 */
using TransportProtocol = std::string;// = "udp" | "tcp";

struct TransportTuple
{
	std::string localIp;
	uint32_t localPort;
	std::string remoteIp;
	uint32_t remotePort;
	TransportProtocol protocol;
};

/**
 * Valid types for "trace" event.
 */
using TransportTraceEventType = std::string;// "probation" | "bwe";

/**
 * "trace" event data.
 */
struct TransportTraceEventData
{
	/**
	 * Trace type.
	 */
	TransportTraceEventType type;

	/**
	 * Event timestamp.
	 */
	uint32_t timestamp;

	/**
	 * Event direction.
	 */
	std::string direction;// "in" | "out";

	/**
	 * Per type information.
	 */
	json info;
};

using SctpState = std::string;//  "new" | "connecting" | "connected" | "failed" | "closed";

class Producer;
class Consumer;
class DataProducer;
class DataConsumer;


using GetRouterRtpCapabilities = std::function<json(void)>;
using GetProducerById = std::function<Producer*(std::string)>;
using GetDataProducerById =std::function<DataProducer*(std::string)>;


class Transport : public EnhancedEventEmitter
{
	Logger* logger;
	// Internal data.
protected:
	json _internal;
	// 	{
	// 		std::string routerId;
	// 		std::string transportId;
	// 	};

		// Transport data. This is set by the subclass.
	json _data;
	// 	{
	// 		sctpParameters?: SctpParameters;
	// 		sctpState?: SctpState;
	// 	};

		// Channel instance.
	Channel* _channel;

	// PayloadChannel instance.
	PayloadChannel* _payloadChannel;

	// Close flag.
	bool _closed = false;

	// Custom app data.
protected:
	json _appData;

	// Method to retrieve Router RTP capabilities.
	GetRouterRtpCapabilities _getRouterRtpCapabilities;

	// Method to retrieve a Producer.
	GetProducerById _getProducerById;

	// Method to retrieve a DataProducer.
	GetDataProducerById _getDataProducerById;

	// Producers map.
	std::map<std::string, Producer*> _producers;

	// Consumers map.
	std::map<std::string, Consumer*> _consumers;

	// DataProducers map.
	std::map<std::string, DataProducer*> _dataProducers;

	// DataConsumers map.
	std::map<std::string, DataConsumer*> _dataConsumers;
private:
	// RTCP CNAME for Producers.
	std::string _cnameForProducers;

	// Next MID for Consumers. It"s converted into string when used.
	uint32_t _nextMidForConsumers = 0;

	// Buffer with available SCTP stream ids.
	//Buffer _sctpStreamIds;

	// Next SCTP stream id.
	uint32_t _nextSctpStreamId = 0;

	// Observer instance.
protected:
	EnhancedEventEmitter* _observer;

public:
	/**
	 * @private
	 * @interface
	 * @emits routerclose
	 * @emits @close
	 * @emits @newproducer - (producer: Producer)
	 * @emits @producerclose - (producer: Producer)
	 * @emits @newdataproducer - (dataProducer: DataProducer)
	 * @emits @dataproducerclose - (dataProducer: DataProducer)
	 */
	Transport(
		const json& internal,
		const json& data,
		Channel* channel,
		PayloadChannel* payloadChannel,
		const json& appData,
		GetRouterRtpCapabilities getRouterRtpCapabilities,
		GetProducerById getProducerById,
		GetDataProducerById getDataProducerById
	)
		: EnhancedEventEmitter()
		, logger( new Logger("Transport"))
	{
		logger->debug("constructor()");

		this->_internal = internal;
		this->_data = data;
		this->_channel = channel;
		this->_payloadChannel = payloadChannel;
		this->_appData = appData;
		this->_getRouterRtpCapabilities = getRouterRtpCapabilities;
		this->_getProducerById = getProducerById;
		this->_getDataProducerById = getDataProducerById;
	}

	/**
	 * Transport id.
	 */
	std::string id()
	{
		return this->_internal["transportId"];
	}

	/**
	 * Whether the Transport is closed.
	 */
	bool closed()
	{
		return this->_closed;
	}

	/**
	 * App custom data.
	 */
	json appData()
	{
		return this->_appData;
	}

	/**
	 * Invalid setter.
	 */
	void appData(json appData) // eslint-disable-line no-unused-vars
	{
		throw new Error("cannot override appData object");
	}

	/**
	 * Observer.
	 *
	 * @emits close
	 * @emits newproducer - (producer: Producer)
	 * @emits newconsumer - (producer: Producer)
	 * @emits newdataproducer - (dataProducer: DataProducer)
	 * @emits newdataconsumer - (dataProducer: DataProducer)
	 */
	EnhancedEventEmitter* observer()
	{
		return this->_observer;
	}

	/**
	 * Close the Transport.
	 */
	void close()
	{
		if (this->_closed)
			return;

		logger->debug("close()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal["transportId"]);

		try
		{
			this->_channel->request("transport.close", this->_internal);
		}
		catch (const std::exception&)
		{

		}

		// Close every Producer.
		for (auto &[key, producer] : this->_producers)
		{
			producer->transportClosed();

			// Must tell the Router.
			this->emit("@producerclose", producer);
		}
		this->_producers.clear();

		// Close every Consumer.
		for (auto &[key, consumer] : this->_consumers)
		{
			consumer->transportClosed();
		}
		this->_consumers.clear();

// 		// Close every DataProducer.
// 		for (DataProducer* dataProducer : this->_dataProducers)
// 		{
// 			dataProducer->transportClosed();
// 
// 			// Must tell the Router.
// 			this->emit("@dataproducerclose", dataProducer);
// 		}
// 		this->_dataProducers.clear();
// 
// 		// Close every DataConsumer.
// 		for (DataConsumer* dataConsumer : this->_dataConsumers)
// 		{
// 			dataConsumer->transportClosed();
// 		}
// 		this->_dataConsumers.clear();

		this->emit("@close");

		// Emit observer event.
		this->_observer->safeEmit("close");
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 * @virtual
	 */
	void routerClosed()
	{
		if (this->_closed)
			return;

		logger->debug("routerClosed()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal["transportId"]);

		// Close every Producer.
		for (auto &[key, producer] : this->_producers)
		{
			producer->transportClosed();

			// NOTE: No need to tell the Router since it already knows (it has
			// been closed in fact).
		}
		this->_producers.clear();

		// Close every Consumer.
		for (auto &[key, consumer] : this->_consumers)
		{
			consumer->transportClosed();
		}
		this->_consumers.clear();

		// Close every DataProducer.
// 		for (DataProducer* dataProducer : this->_dataProducers)
// 		{
// 			dataProducer->transportClosed();
// 
// 			// NOTE: No need to tell the Router since it already knows (it has
// 			// been closed in fact).
// 		}
// 		this->_dataProducers.clear();
// 
// 		// Close every DataConsumer.
// 		for (DataConsumer* dataConsumer : this->_dataConsumers)
// 		{
// 			dataConsumer->transportClosed();
// 		}
// 		this->_dataConsumers.clear();

		this->safeEmit("routerclose");

		// Emit observer event.
		this->_observer->safeEmit("close");
	}

	/**
	 * Dump Transport.
	 */
	std::future<json> dump()
	{
		logger->debug("dump()");

		co_return this->_channel->request("transport.dump", this->_internal);
	}

	/**
	 * Get Transport stats.
	 *
	 * @abstract
	 */
	std::future<json> getStats()
	{
		// Should not happen.
		throw new Error("method not implemented in the subclass");
	}

	/**
	 * Provide the Transport remote parameters.
	 *
	 * @abstract
	 */
	// eslint-disable-next-line @typescript-eslint/no-unused-vars
		std::future<void> connect(json params)
	{
		// Should not happen.
		throw new Error("method not implemented in the subclass");
	}

	/**
	 * Set maximum incoming bitrate for receiving media.
	 */
	std::future<void> setMaxIncomingBitrate(uint32_t bitrate)
	{
		logger->debug("setMaxIncomingBitrate() [bitrate:%s]", bitrate);

		json reqData = { {"bitrate", bitrate} };

		co_await this->_channel->request(
			"transport.setMaxIncomingBitrate", this->_internal, reqData);
	}

	/**
	 * Create a Producer.
	 */
	std::future<Producer*> produce(
		std::string id,
		std::string kind,
		json rtpParameters,
		bool paused = false,
		bool keyFrameRequestDelay = 0,
		json appData = json()
	)
	{
		static std::set<std::string> kinds = {"audio","video"};

		logger->debug("produce()");

		if (!id.empty() && this->_producers.count(id))
			throw new TypeError(utils::Printf("a Producer with same id \"${ %s }\" already exists",id.c_str()));
		else if (!kinds.count(kind))
			throw new TypeError(utils::Printf("invalid kind \"${%s}\"", kind.c_str()));
		else if (!appData.is_null() && !appData.is_object())
			throw new TypeError("if given, appData must be an object");

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
		if (this->constructor.name != "PipeTransport")
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

		json internal = this->_internal;
		internal["producerId"] = id.empty() ? uuidv4() : id;

		json reqData = { 
			{ "kind", kind },
			{ "rtpParameters", rtpParameters },
			{ "rtpMapping", rtpMapping },
			{ "keyFrameRequestDelay", keyFrameRequestDelay },
			{ "paused", paused },
		};

		json status =
			co_await this->_channel->request("transport.produce", internal, reqData);

		json data =
		{
			{ "kind", kind },
			{ "rtpParameters", rtpParameters },
			{ "type", status["type"] },
			{ "consumableRtpParameters", consumableRtpParameters }
		};

		Producer* producer = new Producer(
			internal,
			data,
			this->_channel,
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

		return producer;
	}

	/**
	 * Create a Consumer.
	 *
	 * @virtual
	 */
	std::future<Consumer*> consume(
		std::string producerId,
		json rtpCapabilities,
		bool paused = false,
		ConsumerLayers preferredLayers = ConsumerLayers(),
		json appData = json()
	)
	{
		logger->debug("consume()");

		if (!appData.is_null() && !appData.is_object())
			throw new TypeError("if given, appData must be an object");

		// This may throw.
		ortc::validateRtpCapabilities(rtpCapabilities);

		Producer* producer = this->_getProducerById(producerId);

		if (!producer)
			throw Error(utils::Printf("Producer with id \"${%s}\" not found", producerId.c_str()));

		// This may throw.
		json rtpParameters = ortc::getConsumerRtpParameters(
			producer->consumableRtpParameters(), rtpCapabilities);

		// Set MID.
		rtpParameters["mid"] = utils::Printf("%ud", this->_nextMidForConsumers++);

		// We use up to 8 bytes for MID (string).
		if (this->_nextMidForConsumers == 100000000)
		{
			logger->error("consume() | reaching max MID value \"${%ud}\"", this->_nextMidForConsumers);

			this->_nextMidForConsumers = 0;
		}

		json internal = this->_internal;
		internal["consumerId"] = uuidv4();
		internal["producerId"] = producerId;


		json reqData =
		{
			{ "kind", producer->kind() },
			{ "rtpParameters", rtpParameters },
			{ "type", producer->type() },
			{ "consumableRtpEncodings", producer->consumableRtpParameters()["encodings"] },
			{ "paused", paused},
			{ "preferredLayers", preferredLayers }
		};

		json status =
			co_await this->_channel->request("transport.consume", internal, reqData);

		json data = { 
			{ "kind", producer->kind() },
			{ "rtpParameters", rtpParameters }, 
			{ "type", producer->type() }
		};

		Consumer* consumer = new Consumer(
				internal,
				data,
				this->_channel,
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

		return consumer;
	}

	/**
	 * Create a DataProducer.
	 */
// 	std::future<DataProducer*> produceData(
// 		{
// 			id = undefined,
// 			sctpStreamParameters,
// 			label = "",
// 			protocol = "",
// 			appData = {}
// 		}: DataProducerOptions = {}
// 	)
// 	{
// 		logger->debug("produceData()");
// 
// 		if (id && this->_dataProducers.has(id))
// 			throw new TypeError(`a DataProducer with same id "${id}" already exists`);
// 		else if (!appData.is_null() && !appData.is_object())
// 			throw new TypeError("if given, appData must be an object");
// 
// 		let type: DataProducerType;
// 
// 		// If this is not a DirectTransport, sctpStreamParameters are required.
// 		if (this->constructor.name != "DirectTransport")
// 		{
// 			type = "sctp";
// 
// 			// This may throw.
// 			ortc::validateSctpStreamParameters(sctpStreamParameters!);
// 		}
// 		// If this is a DirectTransport, sctpStreamParameters must not be given.
// 		else
// 		{
// 			type = "direct";
// 
// 			if (sctpStreamParameters)
// 			{
// 				logger->warn(
// 					"produceData() | sctpStreamParameters are ignored when producing data on a DirectTransport");
// 			}
// 		}
// 
// 		json internal = { ...this->_internal, dataProducerId: id || uuidv4() };
// 		json reqData =
// 		{
// 			type,
// 			sctpStreamParameters,
// 			label,
// 			protocol
// 		};
// 
// 		json data =
// 			co_await this->_channel->request("transport.produceData", internal, reqData);
// 
// 		const dataProducer = new DataProducer(
// 			{
// 				internal,
// 				data,
// 				channel        : this->_channel,
// 				payloadChannel : this->_payloadChannel,
// 				appData
// 			});
// 
// 		this->_dataProducers.set(dataproducer->id(), dataProducer);
// 		dataProducer->on("@close", () =>
// 		{
// 			this->_dataProducers.delete(dataproducer->id());
// 			this->emit("@dataproducerclose", dataProducer);
// 		});
// 
// 		this->emit("@newdataproducer", dataProducer);
// 
// 		// Emit observer event.
// 		this->_observer->safeEmit("newdataproducer", dataProducer);
// 
// 		return dataProducer;
// 	}

	/**
	 * Create a DataConsumer.
	 */
// 	std::future<DataConsumer*> consumeData(
// 		{
// 			dataProducerId,
// 			ordered,
// 			maxPacketLifeTime,
// 			maxRetransmits,
// 			appData = {}
// 		}: DataConsumerOptions
// 	)
// 	{
// 		logger->debug("consumeData()");
// 
// 		if (!dataProducerId || typeof dataProducerId != "string")
// 			throw new TypeError("missing dataProducerId");
// 		else if (!appData.is_null() && !appData.is_object())
// 			throw new TypeError("if given, appData must be an object");
// 
// 		const dataProducer = this->_getDataProducerById(dataProducerId);
// 
// 		if (!dataProducer)
// 			throw Error(`DataProducer with id "${dataProducerId}" not found`);
// 
// 		let type: DataConsumerType;
// 		let sctpStreamParameters: SctpStreamParameters | undefined;
// 		let sctpStreamId;
// 
// 		// If this is not a DirectTransport, use sctpStreamParameters from the
// 		// DataProducer (if type "sctp") unless they are given in method parameters.
// 		if (this->constructor.name != "DirectTransport")
// 		{
// 			type = "sctp";
// 			sctpStreamParameters =
// 				utils.clone(dataProducer->sctpStreamParameters) as SctpStreamParameters;
// 
// 			// Override if given.
// 			if (ordered != undefined)
// 				sctpStreamParameters.ordered = ordered;
// 
// 			if (maxPacketLifeTime != undefined)
// 				sctpStreamParameters.maxPacketLifeTime = maxPacketLifeTime;
// 
// 			if (maxRetransmits != undefined)
// 				sctpStreamParameters.maxRetransmits = maxRetransmits;
// 
// 			// This may throw.
// 			sctpStreamId = this->_getNextSctpStreamId();
// 
// 			this->_sctpStreamIds![sctpStreamId] = 1;
// 			sctpStreamParameters.streamId = sctpStreamId;
// 		}
// 		// If this is a DirectTransport, sctpStreamParameters must not be used.
// 		else
// 		{
// 			type = "direct";
// 
// 			if (
// 				ordered != undefined ||
// 				maxPacketLifeTime != undefined ||
// 				maxRetransmits != undefined
// 			)
// 			{
// 				logger->warn(
// 					"consumeData() | ordered, maxPacketLifeTime and maxRetransmits are ignored when consuming data on a DirectTransport");
// 			}
// 		}
// 
// 		const { label, protocol } = dataProducer;
// 
// 		json internal = { ...this->_internal, dataConsumerId: uuidv4(), dataProducerId };
// 		json reqData =
// 		{
// 			type,
// 			sctpStreamParameters,
// 			label,
// 			protocol
// 		};
// 
// 		json data =
// 			co_await this->_channel->request("transport.consumeData", internal, reqData);
// 
// 		DataConsumer* dataConsumer = new DataConsumer(
// 			{
// 				internal,
// 				data,
// 				channel        : this->_channel,
// 				payloadChannel : this->_payloadChannel,
// 				appData
// 			});
// 
// 		this->_dataConsumers.set(dataconsumer->id(), dataConsumer);
// 		dataConsumer->on("@close", () =>
// 		{
// 			this->_dataConsumers.delete(dataconsumer->id());
// 
// 			if (this->_sctpStreamIds)
// 				this->_sctpStreamIds[sctpStreamId] = 0;
// 		});
// 		dataConsumer->on("@dataproducerclose", () =>
// 		{
// 			this->_dataConsumers.delete(dataconsumer->id());
// 
// 			if (this->_sctpStreamIds)
// 				this->_sctpStreamIds[sctpStreamId] = 0;
// 		});
// 
// 		// Emit observer event.
// 		this->_observer->safeEmit("newdataconsumer", dataConsumer);
// 
// 		return dataConsumer;
// 	}

	/**
	 * Enable "trace" event.
	 */
	std::future<void> enableTraceEvent(std::vector<TransportTraceEventType> types)
	{
		logger->debug("pause()");

		json reqData = { types };

		co_await this->_channel->request(
			"transport.enableTraceEvent", this->_internal, reqData);
	}

// 	uint32_t _getNextSctpStreamId()
// 	{
// 		if (
// 			!this->_data.count("sctpParameters") ||
// 			!this->_data["sctpParameters"]["MIS"].is_number()
// 		)
// 		{
// 			throw new TypeError("missing data.sctpParameters.MIS");
// 		}
// 
// 		uint32_t numStreams = this->_data["sctpParameters"].value("MIS", 0);
// 
// 		if (!this->_sctpStreamIds)
// 			this->_sctpStreamIds = Buffer.alloc(numStreams, 0);
// 
// 		uint32_t sctpStreamId;
// 
// 		for (uint32_t idx = 0; idx < this->_sctpStreamIds.length; ++idx)
// 		{
// 			sctpStreamId = (this->_nextSctpStreamId + idx) % this->_sctpStreamIds.length;
// 
// 			if (!this->_sctpStreamIds[sctpStreamId])
// 			{
// 				this->_nextSctpStreamId = sctpStreamId + 1;
// 
// 				return sctpStreamId;
// 			}
// 		}
// 
// 		throw new Error("no sctpStreamId available");
// 	}
};
