#pragma once 

#include "EnhancedEventEmitter.h"
#include "SctpParameters.h"

namespace mediasoup {

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

/**
 * DataConsumer type.
 */
using DataConsumerType = std::string; // "sctp" | "direct";


class MS_EXPORT DataConsumer : public EnhancedEventEmitter
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

	virtual ~DataConsumer();

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
	async_simple::coro::Lazy<json> dump();

	/**
	 * Get DataConsumer stats.
	 */
	async_simple::coro::Lazy<json> getStats();

	/**
	 * Set buffered amount low threshold.
	 */
	async_simple::coro::Lazy<void> setBufferedAmountLowThreshold(uint32_t threshold);

	/**
	 * Send data.
	 */
	//async_simple::coro::Lazy<void> send(message: string | Buffer, ppid ? : number)
	
		/**
	 * Get buffered amount size.
	 */
	async_simple::coro::Lazy<size_t> getBufferedAmount();

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

}
