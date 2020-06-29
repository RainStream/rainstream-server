#pragma once 

#include "EnhancedEventEmitter.hpp"
#include "Logger.hpp"
#include "errors.hpp"
#include "Transport.hpp"
#include "Producer.hpp"
#include "Consumer.hpp"


struct DirectTransportOptions
{
	/**
	 * Maximum allowed size for direct messages sent from DataProducers.
	 * Default 262144.
	 */
	uint32_t maxMessageSize;

	/**
	 * Custom application data.
	 */
	json appData;
};

struct DirectTransportStat
{
	// Common to all Transports.
	std::string type;
	std::string transportId;
	uint32_t timestamp;
	uint32_t bytesReceived;
	uint32_t recvBitrate;
	uint32_t bytesSent;
	uint32_t sendBitrate;
	uint32_t rtpBytesReceived;
	uint32_t rtpRecvBitrate;
	uint32_t rtpBytesSent;
	uint32_t rtpSendBitrate;
	uint32_t rtxBytesReceived;
	uint32_t rtxRecvBitrate;
	uint32_t rtxBytesSent;
	uint32_t rtxSendBitrate;
	uint32_t probationBytesReceived;
	uint32_t probationRecvBitrate;
	uint32_t probationBytesSent;
	uint32_t probationSendBitrate;
	uint32_t availableOutgoingBitrate;
	uint32_t availableIncomingBitrate;
	uint32_t maxIncomingBitrate;
};

class PayloadChannel;

class DirectTransport : public Transport
{
	// DirectTransport data.
protected:
	json _data;
// 	{
// 		// TODO
// 	};

public:
	/**
	 * @private
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	DirectTransport(const json& internal,
		const json& data,
		Channel* channel,
		PayloadChannel* payloadChannel,
		const json& appData,
		GetRouterRtpCapabilities getRouterRtpCapabilities,
		GetProducerById getProducerById,
		GetDataProducerById getDataProducerById)
		: Transport(internal, data, channel, payloadChannel,
			appData, getRouterRtpCapabilities,
			getProducerById, getDataProducerById)
	{
		MSC_DEBUG("constructor()");

		// TODO
		this->_data = data;

		this->_handleWorkerNotifications();
	}

	virtual std::string typeName()
	{
		return "DirectTransport";
	}

	/**
	 * Observer.
	 *
	 * @override
	 * @emits close
	 * @emits newdataproducer - (dataProducer: DataProducer)
	 * @emits newdataconsumer - (dataProducer: DataProducer)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	EnhancedEventEmitter* observer()
	{
		return this->_observer;
	}

	/**
	 * Close the DirectTransport.
	 *
	 * @override
	 */
	void close()
	{
		if (this->_closed)
			return;

		Transport::close();
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 * @override
	 */
	void routerClosed()
	{
		if (this->_closed)
			return;

		Transport::routerClosed();
	}

	/**
	 * Get DirectTransport stats.
	 *
	 * @override
	 */
	std::future<json> getStats()
	{
		MSC_DEBUG("getStats()");

		co_return this->_channel->request("transport.getStats", this->_internal);
	}

	/**
	 * NO-OP method in DirectTransport.
	 *
	 * @override
	 */
	std::future<void> connect()
	{
		MSC_DEBUG("connect()");
	}

	/**
	 * @override
	 */
	// eslint-disable-next-line @typescript-eslint/no-unused-vars
	std::future<void> setMaxIncomingBitrate(uint32_t bitrate)
	{
		throw new UnsupportedError(
			"setMaxIncomingBitrate() not implemented in DirectTransport");
	}

	/**
	 * @override
	 */
	// eslint-disable-next-line @typescript-eslint/no-unused-vars
	std::future<Producer*> produce(ProducerOptions& options)
	{
		throw new UnsupportedError("produce() not implemented in DirectTransport");
	}

	/**
	 * @override
	 */
	// eslint-disable-next-line @typescript-eslint/no-unused-vars
	std::future<Consumer*> consume(ConsumerOptions options)
	{
		throw new UnsupportedError("consume() not implemented in DirectTransport");
	}

private:
	void _handleWorkerNotifications()
	{
		this->_channel->on(this->_internal["transportId"], [=](std::string event, json data)
		{
			if (event == "trace")
			{
// 				const trace = data as TransportTraceEventData;
// 
// 				this->safeEmit("trace", trace);
// 
// 				// Emit observer event.
// 				this->_observer->safeEmit("trace", trace);
			}
			else
			{
				MSC_ERROR("ignoring unknown event \"%s\"", event);
			}
		});
	}
};
