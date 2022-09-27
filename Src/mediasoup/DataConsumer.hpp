#pragma once 

#include "EnhancedEventEmitter.hpp"
#include "Logger.hpp"
#include "Channel.hpp"
#include "PayloadChannel.hpp"
#include "SctpParameters.hpp"

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
	bool ordered;

	/**
	 * Just if consuming over SCTP.
	 * When ordered is false indicates the time (in milliseconds) after which a
	 * SCTP packet will stop being retransmitted. Defaults to the value in the
	 * DataProducer if it has type "sctp" or unset if it has type "direct".
	 */
	uint32_t maxPacketLifeTime;

	/**
	 * Just if consuming over SCTP.
	 * When ordered is false indicates the maximum uint32_t of times a packet will
	 * be retransmitted. Defaults to the value in the DataProducer if it has type
	 * "sctp" or unset if it has type "direct".
	 */
	uint32_t maxRetransmits;

	/**
	 * Custom application data.
	 */
	json appData;
};

struct DataConsumerStat
{
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
struct DataConsumerType = "sctp" | "direct";

class DataConsumer : public EnhancedEventEmitter
{
	/**
	 * @private
	 * @emits transportclose
	 * @emits dataproducerclose
	 * @emits message - (message: Buffer, ppid)
	 * @emits @close
	 * @emits @dataproducerclose
	 */
	DataConsumer(
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

		this->_handleWorkerNotifications();
	}

	/**
	 * DataConsumer id.
	 */
	std::string id()
	{
		return this->_internal["dataConsumerId"];
	}

	/**
	 * Associated DataProducer id.
	 */
	std::string dataProducerId()
	{
		return this->_internal.dataProducerId;
	}

	/**
	 * Whether the DataConsumer is closed.
	 */
	bool closed()
	{
		return this->_closed;
	}

	/**
	 * DataConsumer type.
	 */
	DataConsumerType type()
	{
		return this->_data["type"];
	}

	/**
	 * SCTP stream parameters.
	 */
	SctpStreamParameters sctpStreamParameters()
	{
		return this->_data["sctpStreamParameters"];
	}

	/**
	 * DataChannel label.
	 */
	std::string label()
	{
		return this->_data["label"];
	}

	/**
	 * DataChannel protocol.
	 */
	std::string protocol()
	{
		return this->_data["protocol"];
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
		MSC_THROW_ERROR("cannot override appData object");
	}

	/**
	 * Observer.
	 *
	 * @emits close
	 */
	EnhancedEventEmitter* observer()
	{
		return this->_observer;
	}

	/**
	 * Close the DataConsumer.
	 */
	void close()
	{
		if (this->_closed)
			return;

		MSC_DEBUG("close()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal["dataConsumerId"]);

		this->_channel->request("dataConsumer.close", this->_internal)
			.catch(() => {});

		this->emit("@close");

		// Emit observer event.
		this->_observer->safeEmit("close");
	}

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	void transportClosed()
	{
		if (this->_closed)
			return;

		MSC_DEBUG("transportClosed()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal["dataConsumerId"]);

		this->safeEmit("transportclose");

		// Emit observer event.
		this->_observer->safeEmit("close");
	}

	/**
	 * Dump DataConsumer.
	 */
	std::future<json> dump()
	{
		MSC_DEBUG("dump()");

		co_return this->_channel->request("dataConsumer.dump", this->_internal);
	}

	/**
	 * Get DataConsumer stats.
	 */
	async getStats(): Promise<DataConsumerStat[]>
	{
		MSC_DEBUG("getStats()");

		co_return this->_channel->request("dataConsumer.getStats", this->_internal);
	}

	private _handleWorkerNotifications(): void
	{
		this->_channel->on(this->_internal["dataConsumerId"], [=](std::string event, const json& data)
		{	
			if(event == "dataproducerclose")
			{
				if (this->_closed)
					return;

				this->_closed = true;

				// Remove notification subscriptions.
				this->_channel->removeAllListeners(this->_internal["dataConsumerId"]);

				this->emit("@dataproducerclose");
				this->safeEmit("dataproducerclose");

				// Emit observer event.
				this->_observer->safeEmit("close");
			}
			else
			{
				MSC_ERROR("ignoring unknown event \"%s\"", event);
			}			
		});

		this->_payloadChannel->on(
			this->_internal["dataConsumerId"],
			[=](std::string event, data: any | undefined, payload: Buffer)
			{
		
			if (event == "message")
			{
				if (this->_closed)
					return;

				const ppid = data.ppid as uint32_t;
				const message = payload;

				this->safeEmit("message", message, ppid);
			}
			else
			{
				MSC_ERROR("ignoring unknown event \"%s\"", event);
			}
				
			});
	}

	// Internal data.
private:
	json _internal;
	// 	{
	//		std::string routerId;
	// 		std::string transportId;
	// 		std::string dataProducerId;
	// 		std::string dataConsumerId;
	// 	};

		// DataConsumer data.
	json _data;
	// 	{
	// 		type: DataConsumerType;
	// 		sctpStreamParameters?: SctpStreamParameters;
	// 		std::string label;
	// 		std::string protocol;
	// 	};

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

