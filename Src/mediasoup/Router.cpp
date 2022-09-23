#define MSC_CLASS "Router"

#include "common.hpp"
#include "Router.hpp"
#include "Producer.hpp"
#include "Consumer.hpp"
#include "Logger.hpp"
#include "ortc.hpp"
#include "utils.hpp"
#include "errors.hpp"
#include "Channel.hpp"
#include "Transport.hpp"
#include "WebRtcTransport.hpp"
#include "PlainTransport.hpp"
#include "PipeTransport.hpp"
#include "SctpParameters.hpp"

//#include "PayloadChannel.hpp"
//#include "DirectTransport.hpp"

/**
 * @private
 * @emits workerclose
 * @emits @close
 */
Router::Router(
	json internal,
	json data,
	Channel* channel,
	PayloadChannel* payloadChannel,
	json appData
)
	: EnhancedEventEmitter()
{
	MSC_DEBUG("constructor()");

	this->_internal = internal;
	this->_data = data;
	this->_channel = channel;
	this->_payloadChannel = payloadChannel;
	this->_appData = appData;
}

/**
 * Router id.
 */
std::string Router::id()
{
	return this->_internal["routerId"];
}

/**
 * Whether the Router is closed.
 */
bool Router::closed()
{
	return this->_closed;
}

/**
 * RTC capabilities of the Router.
 */
json Router::rtpCapabilities()
{
	return this->_data["rtpCapabilities"];;
}

/**
 * App custom data.
 */
json Router::appData()
{
	return this->_appData;
}

/**
 * Invalid setter.
 */
void Router::appData(json appData) // eslint-disable-line no-unused-vars
{
	MSC_THROW_ERROR("cannot override appData object");
}

/**
 * Observer.
 *
 * @emits close
 * @emits newtransport - (transport: Transport)
 * @emits newrtpobserver - (rtpObserver: RtpObserver)
 */
EnhancedEventEmitter* Router::observer()
{
	return this->_observer;
}

/**
 * Close the Router.
 */
void Router::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	try
	{
		json reqData = { { "routerId", this->_internal["routerId"] } };

		this->_channel->request("worker.closeRouter", undefined, reqData);
	}
	catch (const std::exception&)
	{

	}

	// Close every Transport.
	for (auto &[key, transport] : this->_transports)
	{
		transport->routerClosed();
	}

	this->_transports.clear();

	// Clear the Producers map.
	this->_producers.clear();

	// Close every RtpObserver.
// 		for (RtpObserver* rtpObserver : this->_rtpObservers)
// 		{
// 			rtpObserver->routerClosed();
// 		}
//		this->_rtpObservers.clear();

		// Clear the DataProducers map.
	this->_dataProducers.clear();

	// Clear map of Router/PipeTransports.
	this->_mapRouterPipeTransports.clear();

	// Close the pipeToRouter AwaitQueue instance.
	//this->_pipeToRouterQueue.close();

	this->emit("@close");

	// Emit observer event.
	this->_observer->safeEmit("close");

	delete this;
}

/**
 * Worker was closed.
 *
 * @private
 */
void Router::workerClosed()
{
	if (this->_closed)
		return;

	MSC_DEBUG("workerClosed()");

	this->_closed = true;

	// Close every Transport.
	for (auto &[key, transport] : this->_transports)
	{
		transport->routerClosed();
	}
	this->_transports.clear();

	// Clear the Producers map.
	this->_producers.clear();

	// Close every RtpObserver.
// 		for (auto& [key, rtpObserver]: this->_rtpObservers)
// 		{
// 			rtpObserver->routerClosed();
// 		}

	this->_rtpObservers.clear();

	// Clear the DataProducers map.
	this->_dataProducers.clear();

	// Clear map of Router/PipeTransports.
	this->_mapRouterPipeTransports.clear();

	this->safeEmit("workerclose");

	// Emit observer event.
	this->_observer->safeEmit("close");

	delete this;
}

/**
 * Dump Router.
 */
std::future<json> Router::dump()
{
	MSC_DEBUG("dump()");

	json ret = co_await this->_channel->request("router.dump", this->_internal["routerId"]);

	co_return ret;
}

/**
 * Create a WebRtcTransport.
 */
std::future<WebRtcTransport*> Router::createWebRtcTransport(WebRtcTransportOptions& options)
{
	MSC_DEBUG("createWebRtcTransport()");

	json& listenIps = options.listenIps;
	bool enableUdp = options.enableUdp;
	bool enableTcp = options.enableTcp;
	bool preferUdp = options.preferUdp;
	bool preferTcp = options.preferTcp;
	uint32_t initialAvailableOutgoingBitrate = options.initialAvailableOutgoingBitrate;
	bool enableSctp = options.enableSctp;
	json numSctpStreams = options.numSctpStreams;
	uint32_t maxSctpMessageSize = options.maxSctpMessageSize;
	json appData = options.appData;

	if (!listenIps.is_array())
		MSC_THROW_ERROR("missing listenIps");
	else if (!appData.is_null() && !appData.is_object())
		MSC_THROW_ERROR("if given, appData must be an object");

	json tmpListenIps = json::array();
	for (json& listenIp : listenIps)
	{
		if (listenIp.is_string() && !listenIp.get<std::string>().empty())
		{
			tmpListenIps.push_back( { { "ip", listenIp } } );
		}
		else if (listenIp.is_object())
		{
			tmpListenIps.push_back(
			{
				{ "ip", listenIp["ip"] },
				{ "announcedIp", listenIp.value("announcedIp", json()) }
			});
		}
		else
		{
			MSC_THROW_ERROR("wrong listenIp");
		}
	}

	listenIps = tmpListenIps;

	json internal = this->_internal;
	internal["transportId"] = uuidv4();

	json reqData = {
		{ "listenIps" , listenIps},
		{ "enableUdp", enableUdp },
		{ "enableTcp", enableTcp },
		{ "preferUdp", preferUdp },
		{ "preferTcp", preferTcp },
		{ "initialAvailableOutgoingBitrate", initialAvailableOutgoingBitrate },
		{ "enableSctp", enableSctp },
		{ "numSctpStreams", numSctpStreams },
		{ "maxSctpMessageSize", maxSctpMessageSize },
		{ "isDataChannel", true }
	};

	json data =
		co_await this->_channel->request("router.createWebRtcTransport", this->_internal["routerId"], reqData);

	WebRtcTransport* transport = new WebRtcTransport(
		internal,
		data,
		this->_channel,
		this->_payloadChannel,
		appData,
		[=]() { return this->_data["rtpCapabilities"]; },
		[=](std::string producerId) ->Producer* {
		if (this->_producers.count(producerId))
			return this->_producers.at(producerId);
		else
			return nullptr;
	},
		[=](std::string dataProducerId) -> DataProducer* {
		if (this->_dataProducers.count(dataProducerId))
			return this->_dataProducers.at(dataProducerId);
		else
			return nullptr;
	}
	);

	this->_transports.insert(std::make_pair(transport->id(), transport));
	transport->on("@close", [=]() { this->_transports.erase(transport->id()); });
	transport->on("@newproducer", [=](Producer* producer) { this->_producers.insert(std::make_pair(producer->id(), producer)); });
	transport->on("@producerclose", [=](Producer* producer) { this->_producers.erase(producer->id()); });
	// 		transport->on("@newdataproducer", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.insert(std::make_pair(dataProducer->id(), dataProducer));
	// 		});
	// 		transport->on("@dataproducerclose", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.erase(dataProducer->id());
	// 		});

			// Emit observer event.
	this->_observer->safeEmit("newtransport", transport);

	co_return transport;
}

/**
 * Create a PlainTransport.
 */
std::future<PlainTransport*> Router::createPlainTransport(
	json listenIp,
	bool rtcpMux/* = true*/,
	bool comedia/* = false*/,
	bool enableSctp/* = false*/,
	json numSctpStreams/* = { { "OS", 1024 }, { "MIS", 1024 } }*/,
	uint32_t maxSctpMessageSize/* = 262144*/,
	bool enableSrtp/* = false*/,
	std::string srtpCryptoSuite/* = "AES_CM_128_HMAC_SHA1_80"*/,
	json appData/* = json::object()*/
)
{
	MSC_DEBUG("createPlainTransport()");

	if (!listenIp)
		MSC_THROW_ERROR("missing listenIp");
	else if (!appData.is_null() && !appData.is_object())
		MSC_THROW_ERROR("if given, appData must be an object");

	if (listenIp.is_string() && !listenIp.get<std::string>().empty())
	{
		listenIp = { { "ip", listenIp } };
	}
	else if (listenIp.is_object())
	{
		listenIp =
		{
			{ "ip", listenIp["ip"] },
			{ "announcedIp", listenIp.value("announcedIp", json()) }
		};
	}
	else
	{
		MSC_THROW_ERROR("wrong listenIp");
	}

	json internal = this->_internal;
	internal["transportId"] = uuidv4();

	json reqData = {
		{ "listenIp" , listenIp},
		{ "rtcpMux", rtcpMux },
		{ "comedia", comedia },
		{ "enableSctp", enableSctp },
		{ "numSctpStreams", numSctpStreams },
		{ "maxSctpMessageSize", maxSctpMessageSize },
		{ "isDataChannel", false},
		{ "enableSrtp", enableSrtp},
		{ "srtpCryptoSuite", srtpCryptoSuite }
	};

	json data =
		co_await this->_channel->request("router.createPlainTransport", this->_internal["routerId"], reqData);

	PlainTransport* transport = new PlainTransport(
		internal,
		data,
		this->_channel,
		this->_payloadChannel,
		appData,
		[=]() { return this->_data["rtpCapabilities"]; },
		[=](std::string producerId)  -> Producer* {
		if (this->_producers.count(producerId))
			return this->_producers.at(producerId);
		else
			return nullptr;
	},
		[=](std::string dataProducerId) -> DataProducer* {
		if (this->_dataProducers.count(dataProducerId))
			return this->_dataProducers.at(dataProducerId);
		else
			return nullptr;
	}
	);

	this->_transports.insert(std::make_pair(transport->id(), transport));
	transport->on("@close", [=]() { this->_transports.erase(transport->id()); });
	transport->on("@newproducer", [=](Producer* producer) { this->_producers.insert(std::make_pair(producer->id(), producer)); });
	transport->on("@producerclose", [=](Producer* producer) { this->_producers.erase(producer->id()); });
	// 		transport->on("@newdataproducer", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.insert(std::make_pair(dataProducer->id(), dataProducer));
	// 		});
	// 		transport->on("@dataproducerclose", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.erase(dataProducer->id());
	// 		});

			// Emit observer event.
	this->_observer->safeEmit("newtransport", transport);

	co_return transport;
}

/**
 * Create a PipeTransport.
 */
std::future<PipeTransport*> Router::createPipeTransport(
	json listenIp,
	bool enableSctp/* = false*/,
	json numSctpStreams/* = { { "OS", 1024 }, { "MIS", 1024 } }*/,
	uint32_t maxSctpMessageSize/* = 1073741823*/,
	bool enableRtx/* = false*/,
	bool enableSrtp/* = false*/,
	json appData/* = json()*/
)
{
	MSC_DEBUG("createPipeTransport()");

	if (!listenIp)
		MSC_THROW_ERROR("missing listenIp");
	else if (!appData.is_null() && !appData.is_object())
		MSC_THROW_ERROR("if given, appData must be an object");

	// 		if (typeof listenIp == "std::string" && listenIp)
	// 		{
	// 			listenIp = { ip: listenIp };
	// 		}
	// 		else if (typeof listenIp == "object")
	// 		{
	// 			listenIp =
	// 			{
	// 				ip          : listenIp.ip,
	// 				announcedIp : listenIp.announcedIp || undefined
	// 			};
	// 		}
	// 		else
	// 		{
	// 			MSC_THROW_ERROR("wrong listenIp");
	// 		}

	json internal = this->_internal;
	internal["transportId"] = uuidv4();

	json reqData = {
		{ "listenIp" , listenIp },
		{ "enableSctp", enableSctp },
		{ "numSctpStreams", numSctpStreams },
		{ "maxSctpMessageSize", maxSctpMessageSize },
		{ "isDataChannel", false},
		{ "enableRtx", enableRtx },
		{ "enableSrtp", enableSrtp}
	};


	json data =
		co_await this->_channel->request("router.createPipeTransport", this->_internal["routerId"], reqData);

	PipeTransport* transport = new PipeTransport(
		internal,
		data,
		this->_channel,
		this->_payloadChannel,
		appData,
		[=]() { return this->_data["rtpCapabilities"]; },
		[=](std::string producerId) -> Producer* {
		if (this->_producers.count(producerId))
			return this->_producers.at(producerId);
		else
			return nullptr;
	},
		[=](std::string dataProducerId) -> DataProducer* {
		if (this->_dataProducers.count(dataProducerId))
			return this->_dataProducers.at(dataProducerId);
		else
			return nullptr;
	}
	);

	this->_transports.insert(std::make_pair(transport->id(), transport));
	transport->on("@close", [=]() { this->_transports.erase(transport->id()); });
	transport->on("@newproducer", [=](Producer* producer) { this->_producers.insert(std::make_pair(producer->id(), producer)); });
	transport->on("@producerclose", [=](Producer* producer) { this->_producers.erase(producer->id()); });
	// 		transport->on("@newdataproducer", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.insert(std::make_pair(dataProducer->id(), dataProducer));
	// 		});
	// 		transport->on("@dataproducerclose", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.erase(dataProducer->id());
	// 		});
			// Emit observer event.
	this->_observer->safeEmit("newtransport", transport);

	co_return transport;
}

/**
 * Create a DirectTransport.
 */
 // 	std::future<DirectTransport*> Router::createDirectTransport(
 // 		uint32_t maxMessageSize = 262144,
 // 		json appData = json::object()
 // 	)
 // 	{
 // 		MSC_DEBUG("createDirectTransport()");
 // 
 // 		json internal = this->_internal;
 // 		internal["transportId"] = uuidv4();
 // 
 // 		json reqData = {
 // 			{ "direct", true },
 // 			{ "maxMessageSize", maxMessageSize },
 // 		};
 // 
 // 		json data =
 // 			co_await this->_channel->request("router.createDirectTransport", internal, reqData);
 // 
 // 		DirectTransport* transport = new DirectTransport(
 // 			{
 // 				internal,
 // 				data,
 // 				this->_channel,
 // 				this->_payloadChannel,
 // 				appData,
 // 				[=]() { return this->_data["rtpCapabilities"]; },
 // 				[=](std::string producerId) -> Producer* {
 // 					if (this->_producers.count(producerId))
 // 						return this->_producers.at(producerId);
 // 					else
 // 						return nullptr;
 // 				},
 // 				[=](std::string dataProducerId) -> DataProducer* {
 // 					if (this->_dataProducers.count(dataProducerId))
 // 						return this->_dataProducers.at(dataProducerId);
 // 					else
 // 						return nullptr;
 // 				}
 // 			});
 // 
 // 		this->_transports.insert(std::make_pair(transport->id(), transport));
 // 		transport->on("@close", [=]() { this->_transports.erase(transport->id()); });
 // 		transport->on("@newproducer", [=](Producer* producer) { this->_producers.insert(std::make_pair(producer->id(), producer)); });
 // 		transport->on("@producerclose", [=](Producer* producer) { this->_producers.erase(producer->id()); });
 // // 		transport->on("@newdataproducer", [=](DataProducer* dataProducer) {
 // // 			this->_dataProducers.insert(std::make_pair(dataProducer->id(), dataProducer));
 // // 		});
 // // 		transport->on("@dataproducerclose", [=](DataProducer* dataProducer) {
 // // 			this->_dataProducers.erase(dataProducer->id());
 // // 		});
 // 
 // 		// Emit observer event.
 // 		this->_observer->safeEmit("newtransport", transport);
 // 
 // 		return transport;
 // 	}

	 /**
	  * Pipes the given Producer or DataProducer into another Router in same host.
	  */
	  // 	std::future<PipeToRouterResult*> Router::pipeToRouter(
	  // 		{
	  // 			producerId,
	  // 			dataProducerId,
	  // 			router,
	  // 			listenIp = "127.0.0.1",
	  // 			enableSctp = true,
	  // 			numSctpStreams = { OS: 1024, MIS: 1024 },
	  // 			enableRtx = false,
	  // 			enableSrtp = false
	  // 		}: PipeToRouterOptions
	  // 	): Promise<PipeToRouterResult>
	  // 	{
	  // 		if (!producerId && !dataProducerId)
	  // 			MSC_THROW_ERROR("missing producerId or dataProducerId");
	  // 		else if (producerId && dataProducerId)
	  // 			MSC_THROW_ERROR("just producerId or dataProducerId can be given");
	  // 		else if (!router)
	  // 			MSC_THROW_ERROR("Router not found");
	  // 		else if (router == this)
	  // 			MSC_THROW_ERROR("cannot use this Router as destination");
	  // 
	  // 		let producer: Producer | undefined;
	  // 		let dataProducer: DataProducer | undefined;
	  // 
	  // 		if (producerId)
	  // 		{
	  // 			producer = this->_producers.get(producerId);
	  // 
	  // 			if (!producer)
	  // 				MSC_THROW_ERROR("Producer not found");
	  // 		}
	  // 		else if (dataProducerId)
	  // 		{
	  // 			dataProducer = this->_dataProducers.get(dataProducerId);
	  // 
	  // 			if (!dataProducer)
	  // 				MSC_THROW_ERROR("DataProducer not found");
	  // 		}
	  // 
	  // 		// Here we may have to create a new PipeTransport pair to connect source and
	  // 		// destination Routers. We just want to keep a PipeTransport pair for each
	  // 		// pair of Routers. Since this operation is async, it may happen that two
	  // 		// simultaneous calls to router1.pipeToRouter({ producerId: xxx, router: router2 })
	  // 		// would end up generating two pairs of PipeTranports. To prevent that, let"s
	  // 		// use an async queue.
	  // 
	  // 		let localPipeTransport: PipeTransport | undefined;
	  // 		let remotePipeTransport: PipeTransport | undefined;
	  // 
	  // 		co_await this->_pipeToRouterQueue.push(async () =>
	  // 		{
	  // 			let pipeTransportPair = this->_mapRouterPipeTransports.get(router);
	  // 
	  // 			if (pipeTransportPair)
	  // 			{
	  // 				localPipeTransport = pipeTransportPair[0];
	  // 				remotePipeTransport = pipeTransportPair[1];
	  // 			}
	  // 			else
	  // 			{
	  // 				try
	  // 				{
	  // 					pipeTransportPair = co_await Promise.all(
	  // 						[
	  // 							this->createPipeTransport(
	  // 								{ listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp }),
	  // 
	  // 							router.createPipeTransport(
	  // 								{ listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp })
	  // 						]);
	  // 
	  // 					localPipeTransport = pipeTransportPair[0];
	  // 					remotePipeTransport = pipeTransportPair[1];
	  // 
	  // 					co_await Promise.all(
	  // 						[
	  // 							localPipeTransport.connect(
	  // 								{
	  // 									ip             : remotePipeTransport.tuple.localIp,
	  // 									port           : remotePipeTransport.tuple.localPort,
	  // 									srtpParameters : remotePipeTransport.srtpParameters
	  // 								}),
	  // 
	  // 							remotePipeTransport.connect(
	  // 								{
	  // 									ip             : localPipeTransport.tuple.localIp,
	  // 									port           : localPipeTransport.tuple.localPort,
	  // 									srtpParameters : localPipeTransport.srtpParameters
	  // 								})
	  // 						]);
	  // 
	  // 					localPipeTransport.observer.on("close", () =>
	  // 					{
	  // 						remotePipeTransport!.close();
	  // 						this->_mapRouterPipeTransports.delete(router);
	  // 					});
	  // 
	  // 					remotePipeTransport.observer.on("close", () =>
	  // 					{
	  // 						localPipeTransport!.close();
	  // 						this->_mapRouterPipeTransports.delete(router);
	  // 					});
	  // 
	  // 					this->_mapRouterPipeTransports.set(
	  // 						router, [ localPipeTransport, remotePipeTransport ]);
	  // 				}
	  // 				catch (error)
	  // 				{
	  // 					MSC_ERROR(
	  // 						"pipeToRouter() | error creating PipeTransport pair:%o",
	  // 						error);
	  // 
	  // 					if (localPipeTransport)
	  // 						localPipeTransport.close();
	  // 
	  // 					if (remotePipeTransport)
	  // 						remotePipeTransport.close();
	  // 
	  // 					throw error;
	  // 				}
	  // 			}
	  // 		});
	  // 
	  // 		if (producer)
	  // 		{
	  // 			let pipeConsumer: Consumer | undefined;
	  // 			let pipeProducer: Producer | undefined;
	  // 
	  // 			try
	  // 			{
	  // 				pipeConsumer = co_await localPipeTransport!.consume(
	  // 					{
	  // 						producerId : producerId!
	  // 					});
	  // 
	  // 				pipeProducer = co_await remotePipeTransport!.produce(
	  // 					{
	  // 						id            : producer->id(),
	  // 						kind          : pipeConsumer!.kind,
	  // 						rtpParameters : pipeConsumer!.rtpParameters,
	  // 						paused        : pipeConsumer!.producerPaused,
	  // 						appData       : producer.appData
	  // 					});
	  // 
	  // 				// Pipe events from the pipe Consumer to the pipe Producer.
	  // 				pipeConsumer!.observer.on("close", () => pipeProducer!.close());
	  // 				pipeConsumer!.observer.on("pause", () => pipeProducer!.pause());
	  // 				pipeConsumer!.observer.on("resume", () => pipeProducer!.resume());
	  // 
	  // 				// Pipe events from the pipe Producer to the pipe Consumer.
	  // 				pipeProducer.observer.on("close", () => pipeConsumer!.close());
	  // 
	  // 				return { pipeConsumer, pipeProducer };
	  // 			}
	  // 			catch (error)
	  // 			{
	  // 				MSC_ERROR(
	  // 					"pipeToRouter() | error creating pipe Consumer/Producer pair:%o",
	  // 					error);
	  // 
	  // 				if (pipeConsumer)
	  // 					pipeConsumer.close();
	  // 
	  // 				if (pipeProducer)
	  // 					pipeProducer.close();
	  // 
	  // 				throw error;
	  // 			}
	  // 		}
	  // 		else if (dataProducer)
	  // 		{
	  // 			let pipeDataConsumer: DataConsumer | undefined;
	  // 			let pipeDataProducer: DataProducer | undefined;
	  // 
	  // 			try
	  // 			{
	  // 				pipeDataConsumer = co_await localPipeTransport!.consumeData(
	  // 					{
	  // 						dataProducerId : dataProducerId!
	  // 					});
	  // 
	  // 				pipeDataProducer = co_await remotePipeTransport!.produceData(
	  // 					{
	  // 						id                   : dataProducer->id(),
	  // 						sctpStreamParameters : pipeDataConsumer!.sctpStreamParameters,
	  // 						label                : pipeDataConsumer!.label,
	  // 						protocol             : pipeDataConsumer!.protocol,
	  // 						appData              : dataProducer.appData
	  // 					});
	  // 
	  // 				// Pipe events from the pipe DataConsumer to the pipe DataProducer.
	  // 				pipeDataConsumer!.observer.on("close", () => pipeDataProducer!.close());
	  // 
	  // 				// Pipe events from the pipe DataProducer to the pipe DataConsumer.
	  // 				pipeDataProducer.observer.on("close", () => pipeDataConsumer!.close());
	  // 
	  // 				return { pipeDataConsumer, pipeDataProducer };
	  // 			}
	  // 			catch (error)
	  // 			{
	  // 				MSC_ERROR(
	  // 					"pipeToRouter() | error creating pipe DataConsumer/DataProducer pair:%o",
	  // 					error);
	  // 
	  // 				if (pipeDataConsumer)
	  // 					pipeDataConsumer.close();
	  // 
	  // 				if (pipeDataProducer)
	  // 					pipeDataProducer.close();
	  // 
	  // 				throw error;
	  // 			}
	  // 		}
	  // 		else
	  // 		{
	  // 			MSC_THROW_ERROR("internal error");
	  // 		}
	  // 	}

		  /**
		   * Create an AudioLevelObserver.
		   */
		   // 	std::future<AudioLevelObserver*> Router::createAudioLevelObserver(
		   // 		{
		   // 			maxEntries = 1,
		   // 			threshold = -80,
		   // 			interval = 1000,
		   // 			appData = {}
		   // 		}: AudioLevelObserverOptions = {}
		   // 	)
		   // 	{
		   // 		MSC_DEBUG("createAudioLevelObserver()");
		   // 
		   // 		if (!appData.is_null() && !appData.is_object())
		   // 			MSC_THROW_ERROR("if given, appData must be an object");
		   // 
		   // 		json internal = { ...this->_internal, rtpObserverId: uuidv4() };
		   // 		json reqData = { maxEntries, threshold, interval };
		   // 
		   // 		co_await this->_channel->request("router.createAudioLevelObserver", internal, reqData);
		   // 
		   // 		AudioLevelObserver* audioLevelObserver = new AudioLevelObserver(
		   // 			{
		   // 				internal,
		   // 				channel         : this->_channel,
		   // 				payloadChannel  : this->_payloadChannel,
		   // 				appData,
		   // 				getProducerById : (std::string producerId): Producer | undefined => (
		   // 					this->_producers.get(producerId)
		   // 				)
		   // 			});
		   // 
		   // 		this->_rtpObservers.insert(audioLevelObserver->id(), audioLevelObserver);
		   // 		audioLevelObserver->on("@close", [this]() 
		   // 		{
		   // 			this->_rtpObservers.remove(audioLevelObserver->id());
		   // 		});
		   // 
		   // 		// Emit observer event.
		   // 		this->_observer->safeEmit("newrtpobserver", audioLevelObserver);
		   // 
		   // 		return audioLevelObserver;
		   // 	}

			   /**
				* Check whether the given RTP capabilities can consume the given Producer.
				*/
bool Router::canConsume(std::string producerId, json& rtpCapabilities)
{
	Producer* producer = GetMapValue(this->_producers, producerId);

	if (!producer)
	{
		MSC_ERROR(
			"canConsume() | Producer with id \"%s\" not found", producerId.c_str());

		return false;
	}

	try
	{
		json consumableRtpParameters = producer->consumableRtpParameters();
		return ortc::canConsume(consumableRtpParameters, rtpCapabilities);
	}
	catch (std::exception& error)
	{
		MSC_ERROR("canConsume() | unexpected error: %s", error.what());

		return false;
	}
}

