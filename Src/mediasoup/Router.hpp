#pragma once 

// import { AwaitQueue } from "awaitqueue";
#include "EnhancedEventEmitter.hpp"

class Channel;
class Router;
class Producer;
class Consumer;
class DataProducer;
class DataConsumer;
class RtpObserver;
class Transport;
class PayloadChannel;
class DirectTransport;
class WebRtcTransport;
class PlainTransport;
class PipeTransport;
struct WebRtcTransportOptions;

class MS_EXPORT Router : public EnhancedEventEmitter
{
public:
	/**
	 * @private
	 * @emits workerclose
	 * @emits @close
	 */
	Router(
		json internal,
		json data,
		Channel* channel,
		PayloadChannel* payloadChannel,
		json appData
	);

	/**
	 * Router id.
	 */
	std::string id();

	/**
	 * Whether the Router is closed.
	 */
	bool closed();

	/**
	 * RTC capabilities of the Router.
	 */
	json rtpCapabilities();

	/**
	 * App custom data.
	 */
	json appData();

	/**
	 * Invalid setter.
	 */
	void appData(json appData);

	/**
	 * Observer.
	 *
	 * @emits close
	 * @emits newtransport - (transport: Transport)
	 * @emits newrtpobserver - (rtpObserver: RtpObserver)
	 */
	EnhancedEventEmitter* observer();

	/**
	 * Close the Router.
	 */
	void close();

	/**
	 * Worker was closed.
	 *
	 * @private
	 */
	void workerClosed();

	/**
	 * Dump Router.
	 */
	std::future<json> dump();

	/**
	 * Create a WebRtcTransport.
	 */
	std::future<WebRtcTransport*> createWebRtcTransport(WebRtcTransportOptions& options);

	/**
	 * Create a PlainTransport.
	 */
	std::future<PlainTransport*> createPlainTransport(
		json listenIp,
		bool rtcpMux = true,
		bool comedia = false,
		bool enableSctp = false,
		json numSctpStreams = { { "OS", 1024 }, { "MIS", 1024 } },
		uint32_t maxSctpMessageSize = 262144,
		bool enableSrtp = false,
		std::string srtpCryptoSuite = "AES_CM_128_HMAC_SHA1_80",
		json appData = json::object()
	);

	/**
	 * Create a PipeTransport.
	 */
	std::future<PipeTransport*> createPipeTransport(
		json listenIp,
		bool enableSctp = false,
		json numSctpStreams = { { "OS", 1024 }, { "MIS", 1024 } },
		uint32_t maxSctpMessageSize = 1073741823,
		bool enableRtx = false,
		bool enableSrtp = false,
		json appData = json()
	);

	/**
	 * Create a DirectTransport.
	 */
// 	std::future<DirectTransport*> createDirectTransport(
// 		uint32_t maxMessageSize = 262144,
// 		json appData = json::object()
// 	);

	/**
	 * Pipes the given Producer or DataProducer into another Router in same host.
	 */
// 	std::future<PipeToRouterResult*> pipeToRouter(
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
// 	);

	/**
	 * Create an AudioLevelObserver.
	 */
// 	std::future<AudioLevelObserver*> createAudioLevelObserver(
// 		{
// 			maxEntries = 1,
// 			threshold = -80,
// 			interval = 1000,
// 			appData = {}
// 		}: AudioLevelObserverOptions = {}
// 	);

	/**
	 * Check whether the given RTP capabilities can consume the given Producer.
	 */
	bool canConsume(std::string producerId, json& rtpCapabilities);

private:
	// Internal data.
	json _internal;

	// Router data.
	json _data;

	// Channel instance.
	Channel* _channel;

	// PayloadChannel instance.
	PayloadChannel* _payloadChannel;

	// Closed flag.
	bool _closed = false;

	// Custom app data.
	json _appData;

	// Transports map.
	std::map<std::string, Transport*> _transports;

	// Producers map.
	std::map<std::string, Producer*> _producers;

	// RtpObservers map.
	std::map<std::string, RtpObserver*> _rtpObservers;

	// DataProducers map.
	std::map<std::string, DataProducer*> _dataProducers;

	// Router to PipeTransport map.
	std::map<Router*, PipeTransport*> _mapRouterPipeTransports;

	// AwaitQueue instance to make pipeToRouter tasks happen sequentially.
	// 	readonly _pipeToRouterQueue =
	// 		new AwaitQueue({ ClosedErrorClass: InvalidStateError });

	// Observer instance.
	EnhancedEventEmitter* _observer = new EnhancedEventEmitter();
};
