#pragma once 

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
class AudioLevelObserver;
class ActiveSpeakerObserver;
struct DirectTransportOptions;
struct WebRtcTransportOptions;
struct AudioLevelObserverOptions;
struct ActiveSpeakerObserverOptions;


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

	virtual ~Router();

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
	 * @private
	 * Just for testing purposes.
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
		uint16_t port,
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
	std::future<DirectTransport*> createDirectTransport(const DirectTransportOptions& options);
	/**
	 * Pipes the given Producer or DataProducer into another Router in same host.
	 */
	//pipeToRouter({ producerId, dataProducerId, router, listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp }: PipeToRouterOptions) : Promise<PipeToRouterResult>;
	/**
	 * @private
	 */
	//addPipeTransportPair(pipeTransportPairKey: string, pipeTransportPairPromise : Promise<PipeTransportPair>) : void;
	/**
	 * Create an ActiveSpeakerObserver
	 */
	std::future<ActiveSpeakerObserver*> createActiveSpeakerObserver(const ActiveSpeakerObserverOptions& options);
	/**
	 * Create an AudioLevelObserver.
	 */
	std::future<AudioLevelObserver*> createAudioLevelObserver(const AudioLevelObserverOptions& options);
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
	// Map of PipeTransport pair Promises indexed by the id of the Router in
	// which pipeToRouter() was called.
	//std::map<std::string, DataProducer*> _mapRouterPairPipeTransportPairPromise;
	// Observer instance.
	EnhancedEventEmitter* _observer;
};
