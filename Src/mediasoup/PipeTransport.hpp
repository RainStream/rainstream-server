#pragma once 

#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"
#include "ortc.hpp"
#include "Transport.hpp"
#include "Consumer.hpp"
#include "SctpParameters.hpp"
#include "SrtpParameters.hpp"

struct PipeTransportOptions
{
	/**
	 * Listening IP address.
	 */
	TransportListenIp listenIp;

	/**
	 * Create a SCTP association. Default false.
	 */
	bool enableSctp;

	/**
	 * SCTP streams uint32_t.
	 */
	NumSctpStreams numSctpStreams;

	/**
	 * Maximum allowed size for SCTP messages sent by DataProducers.
	 * Default 1073741823.
	 */
	uint32_t maxSctpMessageSize;

	/**
	 * Enable RTX and NACK for RTP retransmission. Useful if both Routers are
	 * located in different hosts and there is packet lost in the link. For this
	 * to work, both PipeTransports must enable this setting. Default false.
	 */
	bool enableRtx;

	/**
	 * Enable SRTP. Useful to protect the RTP and RTCP traffic if both Routers
	 * are located in different hosts. For this to work, connect() must be called
	 * with remote SRTP parameters. Default false.
	 */
	bool enableSrtp;

	/**
	 * Custom application data.
	 */
	json appData;
};

struct PipeTransportStat
{
	// Common to all Transports.
	std::string type;
	std::string transportId;
	uint32_t timestamp;
	SctpState sctpState;
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
	// PipeTransport specific.
	TransportTuple tuple;
};


class PipeTransport : public Transport
{
	Logger* logger;
	// PipeTransport data.
protected:
	json _data;
// 	{
// 		tuple: TransportTuple;
// 		sctpParameters?: SctpParameters;
// 		sctpState?: SctpState;
// 		rtx: bool;
// 		srtpParameters?: SrtpParameters;
// 	};

public:
	/**
	 * @private
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	PipeTransport(const json& internal,
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
		, logger(new Logger("PipeTransport"))
	{
		logger->debug("constructor()");

		this->_data =
		{
			tuple          : data.tuple,
			sctpParameters : data.sctpParameters,
			sctpState      : data.sctpState,
			rtx            : data.rtx,
			srtpParameters : data.srtpParameters
		};

		this->_handleWorkerNotifications();
	}

	/**
	 * Transport tuple.
	 */
	TransportTuple tuple()
	{
		return this->_data["tuple"];
	}

	/**
	 * SCTP parameters.
	 */
	SctpParameters sctpParameters()
	{
		return this->_data["sctpParameters"];
	}

	/**
	 * SCTP state.
	 */
	SctpState sctpState()
	{
		return this->_data["sctpState"];
	}

	/**
	 * SRTP parameters.
	 */
	SrtpParameters srtpParameters()
	{
		return this->_data["srtpParameters"];
	}

	/**
	 * Observer.
	 *
	 * @override
	 * @emits close
	 * @emits newproducer - (producer: Producer)
	 * @emits newconsumer - (producer: Producer)
	 * @emits newdataproducer - (dataProducer: DataProducer)
	 * @emits newdataconsumer - (dataProducer: DataProducer)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	EnhancedEventEmitter* observer()
	{
		return this->_observer;
	}

	/**
	 * Close the PipeTransport.
	 *
	 * @override
	 */
	void close()
	{
		if (this->_closed)
			return;

		if (this->_data.contains("sctpState"))
			this->_data["sctpState"] = "closed";

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

		if (this->_data.contains("sctpState"))
			this->_data["sctpState"] = "closed";

		Transport::routerClosed();
	}

	/**
	 * Get PipeTransport stats.
	 *
	 * @override
	 */
	std::future<json> getStats()
	{
		logger->debug("getStats()");

		co_return this->_channel->request("transport.getStats", this->_internal);
	}

	/**
	 * Provide the PipeTransport remote parameters.
	 *
	 * @override
	 */
	std::future<void> connect(
		std::string ip,
		uint32_t port,
		SrtpParameters srtpParameters
	)
	{
		logger->debug("connect()");

		json reqData = { ip, port, srtpParameters };

		json data =
			co_await this->_channel->request("transport.connect", this->_internal, reqData);

		// Update data.
		this->_data.tuple = data.tuple;
	}

	/**
	 * Create a pipe Consumer.
	 *
	 * @override
	 */
	std::future<Consumer*> consume({ producerId, appData = {} }: ConsumerOptions)
	{
		logger->debug("consume()");

		if (!producerId || typeof producerId != "string")
			throw new TypeError("missing producerId");
		else if (appData && typeof appData != "object")
			throw new TypeError("if given, appData must be an object");

		const producer = this->_getProducerById(producerId);

		if (!producer)
			throw Error(utils::Printf("Producer with id \"${producerId}\" not found", producerId));

		// This may throw.
		const rtpParameters = ortc::getPipeConsumerRtpParameters(
			producer.consumableRtpParameters, this->_data.rtx);

		json internal = { ...this->_internal, consumerId: uuidv4(), producerId };
		json reqData =
		{
			kind                   : producer.kind,
			rtpParameters,
			type                   : "pipe",
			consumableRtpEncodings : producer.consumableRtpParameters.encodings
		};

		json status =
			co_await this->_channel->request("transport.consume", internal, reqData);

		json data = { kind: producer.kind, rtpParameters, type: "pipe" };

		Consumer* consumer = new Consumer(
			{
				internal,
				data,
				channel        : this->_channel,
				appData,
				paused         : status.paused,
				producerPaused : status.producerPaused
			});

		this->_consumers.set(consumer->id, consumer);
		consumer->on("@close", () => this->_consumers.delete(consumer->id));
		consumer->on("@producerclose", () => this->_consumers.delete(consumer->id));

		// Emit observer event.
		this->_observer->safeEmit("newconsumer", consumer);

		return consumer;
	}

private:
	void _handleWorkerNotifications()
	{
		this->_channel->on(this->_internal["transportId"], [=](std::string event, json data)
		{
			if (event == "sctpstatechange")
			{
				const sctpState = data.sctpState as SctpState;

				this->_data.sctpState = sctpState;

				this->safeEmit("sctpstatechange", sctpState);

				// Emit observer event.
				this->_observer->safeEmit("sctpstatechange", sctpState);
			}
			else if (event == "trace")
			{
				const trace = data as TransportTraceEventData;

				this->safeEmit("trace", trace);

				// Emit observer event.
				this->_observer->safeEmit("trace", trace);
			}
			else
			{
				logger->error("ignoring unknown event \"%s\"", event);
			}
		});
	}
};
