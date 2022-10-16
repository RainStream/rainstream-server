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
#include "WebRtcServer.hpp"
#include "WebRtcTransport.hpp"
#include "PlainTransport.hpp"
#include "PipeTransport.hpp"
#include "SctpParameters.hpp"
#include "AudioLevelObserver.hpp"
#include "ActiveSpeakerObserver.hpp"

//#include "PayloadChannel.hpp"
//#include "DirectTransport.hpp"

Router::Router(
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
}

std::string Router::id()
{
	return this->_internal["routerId"];
}

bool Router::closed()
{
	return this->_closed;
}

json Router::rtpCapabilities()
{
	return this->_data["rtpCapabilities"];;
}

json Router::appData()
{
	return this->_appData;
}

void Router::appData(json appData) // eslint-disable-line no-unused-vars
{
	MSC_THROW_ERROR("cannot override appData object");
}

EnhancedEventEmitter* Router::observer()
{
	return this->_observer;
}

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
	/*for (RtpObserver* rtpObserver : this->_rtpObservers)
	{
		rtpObserver->routerClosed();
	}
	this->_rtpObservers.clear();*/

	// Clear the DataProducers map.
	this->_dataProducers.clear();

	this->emit("@close");
	// Emit observer event.
	this->_observer->safeEmit("close");
}

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
	/*for (auto& [key, rtpObserver] : this->_rtpObservers)
	{
		rtpObserver->routerClosed();
	}*/
	this->_rtpObservers.clear();
	// Clear the DataProducers map.
	this->_dataProducers.clear();

	this->safeEmit("workerclose");
	// Emit observer event.
	this->_observer->safeEmit("close");
}

task_t<json> Router::dump()
{
	MSC_DEBUG("dump()");

	json ret = co_await this->_channel->request("router.dump", this->_internal["routerId"]);

	co_return ret;
}

task_t<WebRtcTransport*> Router::createWebRtcTransport(WebRtcTransportOptions& options)
{
	MSC_DEBUG("createWebRtcTransport()");
	auto webRtcServer = options.webRtcServer;
	bool enableUdp = options.enableUdp;
	bool enableTcp = options.enableTcp;
	bool preferUdp = options.preferUdp;
	bool preferTcp = options.preferTcp;
	uint32_t initialAvailableOutgoingBitrate = options.initialAvailableOutgoingBitrate;
	bool enableSctp = options.enableSctp;
	json numSctpStreams = options.numSctpStreams;
	uint32_t maxSctpMessageSize = options.maxSctpMessageSize;
	uint32_t sctpSendBufferSize = options.sctpSendBufferSize;
	json appData = options.appData;

	if (!webRtcServer && !options.listenIps.is_array())
		MSC_THROW_ERROR("missing webRtcServer and listenIps (one of them is mandatory)");
	else if (!appData.is_null() && !appData.is_object())
		MSC_THROW_ERROR("if given, appData must be an object");

	json listenIps = json::array();
	for (json& listenIp : options.listenIps)
	{
		if (listenIp.is_string() && !listenIp.get<std::string>().empty())
		{
			listenIps.push_back( { { "ip", listenIp } } );
		}
		else if (listenIp.is_object())
		{
			listenIps.push_back(
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

	json reqData = {
		{ "transportId" ,  uuidv4()},
		{ "listenIps" , listenIps },
		{ "enableUdp", enableUdp },
		{ "enableTcp", enableTcp },
		{ "preferUdp", preferUdp },
		{ "preferTcp", preferTcp },
		{ "initialAvailableOutgoingBitrate", initialAvailableOutgoingBitrate },
		{ "enableSctp", enableSctp },
		{ "numSctpStreams", numSctpStreams },
		{ "maxSctpMessageSize", maxSctpMessageSize },
		{ "sctpSendBufferSize", sctpSendBufferSize },
		{ "isDataChannel", true }
	};

	if (options.port.has_value())
	{
		reqData["port"] = options.port.value();
	}

	json data;

	if (webRtcServer)
	{
		reqData["webRtcServerId"] = webRtcServer->id();

		data = co_await this->_channel->request("router.createWebRtcTransportWithServer", this->_internal["routerId"], reqData);
	}
	else
	{
		data = co_await this->_channel->request("router.createWebRtcTransport", this->_internal["routerId"], reqData);
	}

	json internal = this->_internal;
	internal["transportId"] = reqData["transportId"];

	WebRtcTransport* transport = new WebRtcTransport(
		internal,
		data,
		this->_channel,
		this->_payloadChannel,
		appData,
		[=]() { return this->_data["rtpCapabilities"]; },
		[=](std::string producerId) ->Producer* {
			if (this->_producers.contains(producerId))
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

	this->_transports.insert(std::pair(transport->id(), transport));
	transport->on("@close", [=]() { this->_transports.erase(transport->id()); });
	transport->on("@listenserverclose", [=]() { this->_transports.erase(transport->id()); });
	transport->on("@newproducer", [=](Producer* producer) { this->_producers.insert(std::pair(producer->id(), producer)); });
	transport->on("@producerclose", [=](Producer* producer) { this->_producers.erase(producer->id()); });
	// 		transport->on("@newdataproducer", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.insert(std::pair(dataProducer->id(), dataProducer));
	// 		});
	// 		transport->on("@dataproducerclose", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.erase(dataProducer->id());
	// 		});

	if (webRtcServer)
		webRtcServer->handleWebRtcTransport(transport);

	// Emit observer event.
	this->_observer->safeEmit("newtransport", transport);

	co_return transport;
}

task_t<PlainTransport*> Router::createPlainTransport(
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

	this->_transports.insert(std::pair(transport->id(), transport));
	transport->on("@close", [=]() { this->_transports.erase(transport->id()); });
	transport->on("@newproducer", [=](Producer* producer) { this->_producers.insert(std::pair(producer->id(), producer)); });
	transport->on("@producerclose", [=](Producer* producer) { this->_producers.erase(producer->id()); });
	// 		transport->on("@newdataproducer", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.insert(std::pair(dataProducer->id(), dataProducer));
	// 		});
	// 		transport->on("@dataproducerclose", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.erase(dataProducer->id());
	// 		});

			// Emit observer event.
	this->_observer->safeEmit("newtransport", transport);

	co_return transport;
}

task_t<PipeTransport*> Router::createPipeTransport(
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

	this->_transports.insert(std::pair(transport->id(), transport));
	transport->on("@close", [=]() { this->_transports.erase(transport->id()); });
	transport->on("@newproducer", [=](Producer* producer) { this->_producers.insert(std::pair(producer->id(), producer)); });
	transport->on("@producerclose", [=](Producer* producer) { this->_producers.erase(producer->id()); });
	// 		transport->on("@newdataproducer", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.insert(std::pair(dataProducer->id(), dataProducer));
	// 		});
	// 		transport->on("@dataproducerclose", [=](DataProducer* dataProducer) {
	// 			this->_dataProducers.erase(dataProducer->id());
	// 		});
			// Emit observer event.
	this->_observer->safeEmit("newtransport", transport);

	co_return transport;
}
//
//async createDirectTransport({ maxMessageSize = 262144, appData } = {
//	   maxMessageSize: 262144
//	}) {
//	MSC_DEBUG("createDirectTransport()");
//	const reqData = {
//		transportId: (0, uuid_1.v4)(),
//		direct : true,
//		maxMessageSize
//	};
//	const data = co_await this->_channel->request("router.createDirectTransport", this->_internal.routerId, reqData);
//	const transport = new DirectTransport_1.DirectTransport({
//		internal: {
//			...this->_internal,
//			transportId: reqData.transportId
//		},
//		data,
//		channel : this->_channel,
//		payloadChannel: this->_payloadChannel,
//		appData,
//		getRouterRtpCapabilities: () = > this->_data.rtpCapabilities,
//		getProducerById: (producerId) = > (this->_producers.get(producerId)),
//		getDataProducerById: (dataProducerId) = > (this->_dataProducers.get(dataProducerId))
//		});
//	this->_transports.set(transport.id, transport);
//	transport.on("@close", () = > this->_transports.delete(transport.id));
//	transport.on("@listenserverclose", () = > this->_transports.delete(transport.id));
//	transport.on("@newproducer", (producer) = > this->_producers.set(producer.id, producer));
//	transport.on("@producerclose", (producer) = > this->_producers.delete(producer.id));
//	transport.on("@newdataproducer", (dataProducer) = > (this->_dataProducers.set(dataProducer.id, dataProducer)));
//	transport.on("@dataproducerclose", (dataProducer) = > (this->_dataProducers.delete(dataProducer.id)));
//	// Emit observer event.
//	this->_observer->safeEmit("newtransport", transport);
//	return transport;
//}
///**
// * Pipes the given Producer or DataProducer into another Router in same host.
// */
//async pipeToRouter({ producerId, dataProducerId, router, listenIp = "127.0.0.1", enableSctp = true, numSctpStreams = { OS: 1024, MIS : 1024 }, enableRtx = false, enableSrtp = false }) {
//	MSC_DEBUG("pipeToRouter()");
//	if (!producerId && !dataProducerId)
//		throw new TypeError("missing producerId or dataProducerId");
//	else if (producerId && dataProducerId)
//		throw new TypeError("just producerId or dataProducerId can be given");
//	else if (!router)
//		throw new TypeError("Router not found");
//	else if (router == = this)
//		throw new TypeError("cannot use this Router as destination");
//	let producer;
//	let dataProducer;
//	if (producerId) {
//		producer = this->_producers.get(producerId);
//		if (!producer)
//			throw new TypeError("Producer not found");
//	}
//	else if (dataProducerId) {
//		dataProducer = this->_dataProducers.get(dataProducerId);
//		if (!dataProducer)
//			throw new TypeError("DataProducer not found");
//	}
//	const pipeTransportPairKey = router.id;
//	let pipeTransportPairPromise = this->_mapRouterPairPipeTransportPairPromise.get(pipeTransportPairKey);
//	let pipeTransportPair;
//	let localPipeTransport;
//	let remotePipeTransport;
//	if (pipeTransportPairPromise) {
//		pipeTransportPair = co_await pipeTransportPairPromise;
//		localPipeTransport = pipeTransportPair[this.id];
//		remotePipeTransport = pipeTransportPair[router.id];
//	}
//	else {
//		pipeTransportPairPromise = new Promise((resolve, reject) = > {
//			Promise.all([
//				this.createPipeTransport({ listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp }),
//					router.createPipeTransport({ listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp })
//			])
//				.then((pipeTransports) = > {
//					localPipeTransport = pipeTransports[0];
//					remotePipeTransport = pipeTransports[1];
//				})
//					.then(() = > {
//					return Promise.all([
//						localPipeTransport.connect({
//							ip: remotePipeTransport.tuple.localIp,
//							port : remotePipeTransport.tuple.localPort,
//							srtpParameters : remotePipeTransport.srtpParameters
//							}),
//							remotePipeTransport.connect({
//								ip: localPipeTransport.tuple.localIp,
//								port : localPipeTransport.tuple.localPort,
//								srtpParameters : localPipeTransport.srtpParameters
//								})
//					]);
//				})
//					.then(() = > {
//					localPipeTransport.observer.on("close", () = > {
//						remotePipeTransport.close();
//						this->_mapRouterPairPipeTransportPairPromise.delete(pipeTransportPairKey);
//					});
//					remotePipeTransport.observer.on("close", () = > {
//						localPipeTransport.close();
//						this->_mapRouterPairPipeTransportPairPromise.delete(pipeTransportPairKey);
//					});
//					resolve({
//						[this.id] : localPipeTransport,
//						[router.id] : remotePipeTransport
//						});
//				})
//					.catch ((error) = > {
//					logger.error("pipeToRouter() | error creating PipeTransport pair:%o", error);
//					if (localPipeTransport)
//						localPipeTransport.close();
//					if (remotePipeTransport)
//						remotePipeTransport.close();
//					reject(error);
//				});
//		});
//		this->_mapRouterPairPipeTransportPairPromise.set(pipeTransportPairKey, pipeTransportPairPromise);
//		router.addPipeTransportPair(this.id, pipeTransportPairPromise);
//		co_await pipeTransportPairPromise;
//	}
//	if (producer) {
//		let pipeConsumer;
//		let pipeProducer;
//		try {
//			pipeConsumer = co_await localPipeTransport.consume({
//				producerId: producerId
//				});
//			pipeProducer = co_await remotePipeTransport.produce({
//				id: producer.id,
//				kind : pipeConsumer.kind,
//				rtpParameters : pipeConsumer.rtpParameters,
//				paused : pipeConsumer.producerPaused,
//				appData : producer.appData
//				});
//			// Ensure that the producer has not been closed in the meanwhile.
//			if (producer.closed)
//				throw new errors_1.InvalidStateError("original Producer closed");
//			// Ensure that producer.paused has not changed in the meanwhile and, if
//			// so, sych the pipeProducer.
//			if (pipeProducer.paused != = producer.paused) {
//				if (producer.paused)
//					co_await pipeProducer.pause();
//				else
//					co_await pipeProducer.resume();
//			}
//			// Pipe events from the pipe Consumer to the pipe Producer.
//			pipeConsumer.observer.on("close", () = > pipeProducer.close());
//			pipeConsumer.observer.on("pause", () = > pipeProducer.pause());
//			pipeConsumer.observer.on("resume", () = > pipeProducer.resume());
//			// Pipe events from the pipe Producer to the pipe Consumer.
//			pipeProducer.observer.on("close", () = > pipeConsumer.close());
//			return { pipeConsumer, pipeProducer };
//		}
//		catch (error) {
//			logger.error("pipeToRouter() | error creating pipe Consumer/Producer pair:%o", error);
//			if (pipeConsumer)
//				pipeConsumer.close();
//			if (pipeProducer)
//				pipeProducer.close();
//			throw error;
//		}
//	}
//	else if (dataProducer) {
//		let pipeDataConsumer;
//		let pipeDataProducer;
//		try {
//			pipeDataConsumer = co_await localPipeTransport.consumeData({
//				dataProducerId: dataProducerId
//				});
//			pipeDataProducer = co_await remotePipeTransport.produceData({
//				id: dataProducer.id,
//				sctpStreamParameters : pipeDataConsumer.sctpStreamParameters,
//				label : pipeDataConsumer.label,
//				protocol : pipeDataConsumer.protocol,
//				appData : dataProducer.appData
//				});
//			// Ensure that the dataProducer has not been closed in the meanwhile.
//			if (dataProducer.closed)
//				throw new errors_1.InvalidStateError("original DataProducer closed");
//			// Pipe events from the pipe DataConsumer to the pipe DataProducer.
//			pipeDataConsumer.observer.on("close", () = > pipeDataProducer.close());
//			// Pipe events from the pipe DataProducer to the pipe DataConsumer.
//			pipeDataProducer.observer.on("close", () = > pipeDataConsumer.close());
//			return { pipeDataConsumer, pipeDataProducer };
//		}
//		catch (error) {
//			logger.error("pipeToRouter() | error creating pipe DataConsumer/DataProducer pair:%o", error);
//			if (pipeDataConsumer)
//				pipeDataConsumer.close();
//			if (pipeDataProducer)
//				pipeDataProducer.close();
//			throw error;
//		}
//	}
//	else {
//		throw new Error("internal error");
//	}
//}
//
//addPipeTransportPair(pipeTransportPairKey, pipeTransportPairPromise) {
//	if (this->_mapRouterPairPipeTransportPairPromise.has(pipeTransportPairKey)) {
//		throw new Error("given pipeTransportPairKey already exists in this Router");
//	}
//	this->_mapRouterPairPipeTransportPairPromise.set(pipeTransportPairKey, pipeTransportPairPromise);
//	pipeTransportPairPromise
//		.then((pipeTransportPair) = > {
//		const localPipeTransport = pipeTransportPair[this.id];
//		// NOTE: No need to do any other cleanup here since that is done by the
//		// Router calling this method on us.
//		localPipeTransport.observer.on("close", () = > {
//			this->_mapRouterPairPipeTransportPairPromise.delete(pipeTransportPairKey);
//		});
//	})
//		.catch (() = > {
//		this->_mapRouterPairPipeTransportPairPromise.delete(pipeTransportPairKey);
//	});
//}

task_t<ActiveSpeakerObserver*> Router::createActiveSpeakerObserver(const ActiveSpeakerObserverOptions& options)
{
	MSC_DEBUG("createActiveSpeakerObserver()");
	if (!options.appData.is_null() && !options.appData.is_object())
		MSC_THROW_TYPE_ERROR("if given, appData must be an object");
	json reqData = {
		{ "rtpObserverId", uuidv4() },
		{ "interval", options.interval }
	};
	co_await this->_channel->request("router.createActiveSpeakerObserver", this->_internal["routerId"], reqData);

	json internal = this->_internal;
	internal["rtpObserverId"] = reqData["rtpObserverId"];

	ActiveSpeakerObserver* activeSpeakerObserver = new ActiveSpeakerObserver(
		internal,
		this->_channel,
		this->_payloadChannel,
		options.appData,
		[=](std::string producerId)->Producer* {
			if (this->_producers.count(producerId))
				return this->_producers.at(producerId);
			else
				return nullptr;
		});
	this->_rtpObservers.insert(std::pair(activeSpeakerObserver->id(), activeSpeakerObserver));
	activeSpeakerObserver->on("@close", [=]() {
		this->_rtpObservers.erase(activeSpeakerObserver->id());
		});
	// Emit observer event.
	this->_observer->safeEmit("newrtpobserver", activeSpeakerObserver);
	co_return activeSpeakerObserver;
}

task_t<AudioLevelObserver*> Router::createAudioLevelObserver(const AudioLevelObserverOptions& options) {
	MSC_DEBUG("createAudioLevelObserver()");
	if (!options.appData.is_null() && !options.appData.is_object())
		MSC_THROW_TYPE_ERROR("if given, appData must be an object");
	json reqData = {
		{ "rtpObserverId", uuidv4() },
		{ "maxEntries", options.maxEntries },
		{ "threshold", options.threshold },
		{ "interval", options.interval }
	};

	co_await this->_channel->request("router.createAudioLevelObserver", this->_internal["routerId"], reqData);

	json internal = this->_internal;
	internal["rtpObserverId"] = reqData["rtpObserverId"];

	AudioLevelObserver* audioLevelObserver = new AudioLevelObserver(
		internal,
		this->_channel,
		this->_payloadChannel,
		options.appData,
		[=](std::string producerId)->Producer* {
			if (this->_producers.count(producerId))
				return this->_producers.at(producerId);
			else
				return nullptr; 
		});
	this->_rtpObservers.insert(std::pair(audioLevelObserver->id(), audioLevelObserver));
	audioLevelObserver->on("@close", [=](){
		this->_rtpObservers.erase(audioLevelObserver->id());
	});
	// Emit observer event.
	this->_observer->safeEmit("newrtpobserver", audioLevelObserver);
	co_return audioLevelObserver;
}

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

