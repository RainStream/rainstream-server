#pragma once 


#include "EnhancedEventEmitter.hpp"
#include "SctpParameters.hpp"

class Channel;
class PayloadChannel;

struct DataProducerOptions
{
	/*DataProducerOptions(const json& data)
	{
		if (data.is_object())
		{
			id = data.value("id", id);
			sctpStreamParameters = data.value("sctpStreamParameters", sctpStreamParameters);
			label = data.value("label", label);
			protocol = data.value("protocol", protocol);
			appData = data.value("appData", appData);
		}
	}*/
	/**
	 * DataProducer id (just for Router.pipeToRouter() method).
	 */
	std::string id;

	/**
	 * SCTP parameters defining how the endpoint is sending the data.
	 * Just if messages are sent over SCTP.
	 */
	json sctpStreamParameters;

	/**
	 * A label which can be used to distinguish this DataChannel from others.
	 */
	std::string label;

	/**
	 * Name of the sub-protocol used by this DataChannel.
	 */
	std::string protocol;

	/**
	 * Custom application data.
	 */
	json appData;
};

struct DataProducerStat
{
	DataProducerStat()
	{

	}
	DataProducerStat(const json& data)
	{
		if(data.is_object())
		{
			type = data.value("port", type);
			timestamp = data.value("port", timestamp);
			label = data.value("port", label);
			protocol = data.value("port", protocol);
			messagesReceived = data.value("port", messagesReceived);
			bytesReceived = data.value("port", bytesReceived);
		}
	}
	std::string type;
	uint32_t timestamp;
	std::string label;
	std::string protocol;
	uint32_t messagesReceived;
	uint32_t bytesReceived;
};

/**
 * DataProducer type.
 */
using DataProducerType = std::string; // "sctp" | "direct";

class DataProducer : public EnhancedEventEmitter
{
public:
	/**
	 * @private
	 * @emits transportclose
	 * @emits @close
	 */
	DataProducer(
		json internal,
		json data,
		Channel* channel,
		PayloadChannel* payloadChannel,
		json appData);

	virtual ~DataProducer();
	/**
	 * DataProducer id.
	 */
	std::string id();

	/**
	 * Whether the DataProducer is closed.
	 */
	bool closed();

	/**
	 * DataProducer type.
	 */
	DataProducerType type();

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
	 * Close the DataProducer.
	 */
	void close();

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	void transportClosed();

	/**
	 * Dump DataProducer.
	 */
	task_t<json> dump();

	/**
	 * Get DataProducer stats.
	 */
	task_t<json> getStats();

	/**
	 * Send data (just valid for DataProducers created on a DirectTransport).
	 */
	//void send(message | Buffer, ppid ? );

private:
	void _handleWorkerNotifications();

private:
	// Internal data.
	json _internal;
	// DataProducer data.
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
	EnhancedEventEmitter* _observer;
};
