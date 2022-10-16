#pragma once 

#include "EnhancedEventEmitter.hpp"
#include "SctpParameters.hpp"

class Channel;
class PayloadChannel;


struct DataConsumerOptions
{
	/**
	 * The id of the DataProducer to consume.
	 */
	std::string dataProducerId;

	/**
	 * Just if consuming over SCTP.
	 * Whether data messages must be received in order. If true the messages will
	 * be sent reliably. Defaults to the value in the DataProducer if it has type
	 * "sctp" or to true if it has type "direct".
	 */
	std::optional<bool> ordered;

	/**
	 * Just if consuming over SCTP.
	 * When ordered is false indicates the time (in milliseconds) after which a
	 * SCTP packet will stop being retransmitted. Defaults to the value in the
	 * DataProducer if it has type "sctp" or unset if it has type "direct".
	 */
	std::optional<uint32_t> maxPacketLifeTime;

	/**
	 * Just if consuming over SCTP.
	 * When ordered is false indicates the maximum uint32_t of times a packet will
	 * be retransmitted. Defaults to the value in the DataProducer if it has type
	 * "sctp" or unset if it has type "direct".
	 */
	std::optional<uint32_t> maxRetransmits;

	/**
	 * Custom application data.
	 */
	json appData;
};

struct DataConsumerStat
{
	DataConsumerStat()
	{

	}
	DataConsumerStat(const json& data)
	{
		if (data.is_object())
		{
			type = data.value("port", type);
			timestamp = data.value("port", timestamp);
			label = data.value("port", label);
			protocol = data.value("port", protocol);
			messagesSent = data.value("port", messagesSent);
			bytesSent = data.value("port", bytesSent);
		}
	}

	std::string type;
	uint32_t timestamp;
	std::string label;
	std::string protocol;
	uint32_t messagesSent;
	uint32_t bytesSent;
};

/**
 * DataConsumer type.
 */
using DataConsumerType = std::string; // "sctp" | "direct";


class DataConsumer : public EnhancedEventEmitter
{
public:
	/**
	 * @private
	 */
	DataConsumer(
		json internal,
		json data,
		Channel* channel,
		PayloadChannel* payloadChannel,
		json appData
	);

	/**
	 * DataConsumer id.
	 */
	std::string id();

	/**
	 * Associated DataProducer id.
	 */
	std::string dataProducerId();

	/**
	 * Whether the DataConsumer is closed.
	 */
	bool closed();

	/**
	 * DataConsumer type.
	 */
	DataConsumerType type();

	/**
	 * SCTP stream parameters.
	 */
	json sctpStreamParameters();

	/**
	 * DataChannel label.
	 */
	std::string label();

	/**
	 * DataChannel protocol.
	 */
	std::string protocol();

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
	 */
	EnhancedEventEmitter* observer();

	/**
	 * Close the DataConsumer.
	 */
	void close();

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	void transportClosed();

	/**
	 * Dump DataConsumer.
	 */
	task_t<json> dump();

	/**
	 * Get DataConsumer stats.
	 */
	task_t<DataConsumerStat> getStats();

	/**
	 * Set buffered amount low threshold.
	 */
	task_t<void> setBufferedAmountLowThreshold(uint32_t threshold);

	/**
	 * Send data.
	 */
	//task_t<void> send(message: string | Buffer, ppid ? : number)
	
		/**
	 * Get buffered amount size.
	 */
	task_t<size_t> getBufferedAmount();

private:
	void _handleWorkerNotifications();

private:
	// Internal data.
	json _internal;
	
	// DataConsumer data.
	json _data;
	
	// Channel instance.
	Channel* _channel;

	// PayloadChannel instance.
	PayloadChannel* _payloadChannel;

	// Closed flag.
	bool _closed = false;

	// Custom app data.
	json _appData;

	// Observer instance.
	EnhancedEventEmitter* _observer{ nullptr };
};

