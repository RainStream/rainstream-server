#pragma once 

#include "EnhancedEventEmitter.h"
#include "SctpParameters.h"
#include "SrtpParameters.h"

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
	TransportTuple()
		: localPort(0)
		, remoteIp()
	{

	}

	TransportTuple(const json& data)
	{
		if (data.is_object())
		{
			localIp = data.value("localIp" , "");
			localPort = data.value("localPort", 0);
			remoteIp = data.value("remoteIp", "");
			remotePort = data.value("remotePort", 0);
			protocol = data.value("protocol", "");
		}
	}

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

class Channel;
class Producer;
class Consumer;
class DataProducer;
class DataConsumer;
class PayloadChannel;
struct ConsumerOptions;
struct DataProducerOptions;
struct DataConsumerOptions;

using GetRouterRtpCapabilities = std::function<json(void)>;
using GetProducerById = std::function<Producer*(std::string)>;
using GetDataProducerById =std::function<DataProducer*(std::string)>;


class MS_EXPORT Transport : public EnhancedEventEmitter
{

public:
	/**
	 * @private
	 * @interface
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
	);

	virtual ~Transport();

	/**
	 * Transport id.
	 */
	std::string id();

	/**
	 * Whether the Transport is closed.
	 */
	bool closed();

	/**
	 * App custom data.
	 */
	json appData();

	/**
	 * Invalid setter.
	 */
	void appData(json appData);

	virtual std::string typeName() = 0;

	/**
	 * Observer.
	 */
	EnhancedEventEmitter* observer();

	/**
	 * @private
	 * Just for testing purposes.
	 */
	Channel* channelForTesting();

	/**
	 * Close the Transport.
	 */
	virtual void close();

	/**
	 * Router was closed.
	 *
	 * @private
	 * @virtual
	 */
	virtual void routerClosed();

	/**
	 * Listen server was closed (this just happens in WebRtcTransports when their
	 * associated WebRtcServer is closed).
	 *
	 * @private
	 */
	void listenServerClosed();

	/**
	 * Dump Transport.
	 */
	std::future<json> dump();

	/**
	 * Get Transport stats.
	 *
	 * @abstract
	 */
	virtual std::future<json> getStats();

	/**
	 * Provide the Transport remote parameters.
	 *
	 * @abstract
	 */
	virtual std::future<void> connect(json& params);

	/**
	 * Set maximum incoming bitrate for receiving media.
	 */

	std::future<void> setMaxIncomingBitrate(uint32_t bitrate);

	/**
	 * Set maximum outgoing bitrate for sending media.
	 */
	std::future<void> setMaxOutgoingBitrate(uint32_t bitrate);

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
	);

	/**
	 * Create a Consumer.
	 *
	 * @virtual
	 */
	virtual std::future<Consumer*> consume(ConsumerOptions& options);

	/**
	 * Create a DataProducer.
	 */
 	std::future<DataProducer*> produceData(DataProducerOptions& options);

	/**
	 * Create a DataConsumer.
	 */
 	std::future<DataConsumer*> consumeData(DataConsumerOptions& options);

	/**
	 * Enable "trace" event.
	 */
	std::future<void> enableTraceEvent(std::vector<TransportTraceEventType> types);

 	uint32_t getNextSctpStreamId();

protected:
	// Internal data.
	json _internal;
	// Transport data. This is set by the subclass.
	json _data;
	// Channel instance.
	Channel* _channel;
	// PayloadChannel instance.
	PayloadChannel* _payloadChannel;
	// Close flag.
	bool _closed = false;
	// Custom app data.
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
	// RTCP CNAME for Producers.
	std::string _cnameForProducers;
	// Next MID for Consumers. It"s converted into string when used.
	uint32_t _nextMidForConsumers = 0;
	// Buffer with available SCTP stream ids.
	std::vector<uint32_t> _sctpStreamIds;
	// Next SCTP stream id.
	uint32_t _nextSctpStreamId = 0;
	// Observer instance.
	EnhancedEventEmitter* _observer;
};
