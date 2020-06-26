#pragma once 

#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"
#include "Channel.hpp"
#include "PayloadChannel.hpp"
#include "SctpParameters.hpp"

struct DataProducerOptions
{
	/**
	 * DataProducer id (just for Router.pipeToRouter() method).
	 */
	std::string id?;

	/**
	 * SCTP parameters defining how the endpoint is sending the data.
	 * Just if messages are sent over SCTP.
	 */
	sctpStreamParameters?: SctpStreamParameters;

	/**
	 * A label which can be used to distinguish this DataChannel from others.
	 */
	std::string label?;

	/**
	 * Name of the sub-protocol used by this DataChannel.
	 */
	std::string protocol?;

	/**
	 * Custom application data.
	 */
	appData?: any;
};

struct DataProducerStat
{
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
struct DataProducerType = "sctp" | "direct";

const Logger* logger = new Logger("DataProducer");

class DataProducer : public EnhancedEventEmitter
{
	// Internal data.
private:
	json _internal;
// 	{
// 		std::string routerId;
// 		std::string transportId;
// 		std::string dataProducerId;
// 	};

	// DataProducer data.
	private readonly _data:
	{
		type: DataProducerType;
		sctpStreamParameters?: SctpStreamParameters;
		std::string label;
		std::string protocol;
	};

	// Channel instance.
	private readonly _channel: Channel;

	// PayloadChannel instance.
	private readonly _payloadChannel: PayloadChannel;

	// Closed flag.
	private _closed = false;

	// Custom app data.
	private readonly _appData?: any;

	// Observer instance.
	EnhancedEventEmitter* _observer = new EnhancedEventEmitter();

	/**
	 * @private
	 * @emits transportclose
	 * @emits @close
	 */
	DataProducer(
		{
			internal,
			data,
			channel,
			payloadChannel,
			appData
		}:
		{
			internal: any;
			data: any;
			channel: Channel;
			payloadChannel: PayloadChannel;
			json appData;
		}
	)
	{
		super();

		logger->debug("constructor()");

		this->_internal = internal;
		this->_data = data;
		this->_channel = channel;
		this->_payloadChannel = payloadChannel;
		this->_appData = appData;

		this->_handleWorkerNotifications();
	}

	/**
	 * DataProducer id.
	 */
	std::string id()
	{
		return this->_internal.dataProducerId;
	}

	/**
	 * Whether the DataProducer is closed.
	 */
	bool closed()
	{
		return this->_closed;
	}

	/**
	 * DataProducer type.
	 */
	get type(): DataProducerType
	{
		return this->_data.type;
	}

	/**
	 * SCTP stream parameters.
	 */
	get sctpStreamParameters(): SctpStreamParameters | undefined
	{
		return this->_data.sctpStreamParameters;
	}

	/**
	 * DataChannel label.
	 */
		std::string label()
	{
		return this->_data.label;
	}

	/**
	 * DataChannel protocol.
	 */
		std::string protocol()
	{
		return this->_data.protocol;
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
	 */
	EnhancedEventEmitter* observer()
	{
		return this->_observer;
	}

	/**
	 * Close the DataProducer.
	 */
	void close()
	{
		if (this->_closed)
			return;

		logger->debug("close()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal.dataProducerId);

		this->_channel->request("dataProducer.close", this->_internal)
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
	transportClosed(): void
	{
		if (this->_closed)
			return;

		logger->debug("transportClosed()");

		this->_closed = true;

		this->safeEmit("transportclose");

		// Emit observer event.
		this->_observer->safeEmit("close");
	}

	/**
	 * Dump DataProducer.
	 */
	std::future<json> dump()
	{
		logger->debug("dump()");

		co_return this->_channel->request("dataProducer.dump", this->_internal);
	}

	/**
	 * Get DataProducer stats.
	 */
	async getStats(): Promise<DataProducerStat[]>
	{
		logger->debug("getStats()");

		co_return this->_channel->request("dataProducer.getStats", this->_internal);
	}

	/**
	 * Send data (just valid for DataProducers created on a DirectTransport).
	 */
	send(message | Buffer, ppid?): void
	{
		logger->debug("send()");

		if (typeof message != "string" && !Buffer.isBuffer(message))
		{
			throw new TypeError("message must be a string or a Buffer");
		}

		/*
		 * +-------------------------------+----------+
		 * | Value                         | SCTP     |
		 * |                               | PPID     |
		 * +-------------------------------+----------+
		 * | WebRTC String                 | 51       |
		 * | WebRTC Binary Partial         | 52       |
		 * | (Deprecated)                  |          |
		 * | WebRTC Binary                 | 53       |
		 * | WebRTC String Partial         | 54       |
		 * | (Deprecated)                  |          |
		 * | WebRTC String Empty           | 56       |
		 * | WebRTC Binary Empty           | 57       |
		 * +-------------------------------+----------+
		 */

		if (typeof ppid != "uint32_t")
		{
			ppid = (typeof message == "string")
				? message.length > 0 ? 51 : 56
				: message.length > 0 ? 53 : 57;
		}

		// Ensure we honor PPIDs.
		if (ppid == 56)
			message = " ";
		else if (ppid == 57)
			message = Buffer.alloc(1);

		const notifData = { ppid };

		this->_payloadChannel.notify(
			"dataProducer.send", this->_internal, notifData, message);
	}

	private _handleWorkerNotifications(): void
	{
		// No need to subscribe to any event.
	}
};
