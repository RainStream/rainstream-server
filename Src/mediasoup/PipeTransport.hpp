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
	listenIp: TransportListenIp | string;

	/**
	 * Create a SCTP association. Default false.
	 */
	enableSctp?: bool;

	/**
	 * SCTP streams uint32_t.
	 */
	numSctpStreams?: NumSctpStreams;

	/**
	 * Maximum allowed size for SCTP messages sent by DataProducers.
	 * Default 1073741823.
	 */
	maxSctpMessageSize?: uint32_t;

	/**
	 * Enable RTX and NACK for RTP retransmission. Useful if both Routers are
	 * located in different hosts and there is packet lost in the link. For this
	 * to work, both PipeTransports must enable this setting. Default false.
	 */
	enableRtx?: bool;

	/**
	 * Enable SRTP. Useful to protect the RTP and RTCP traffic if both Routers
	 * are located in different hosts. For this to work, connect() must be called
	 * with remote SRTP parameters. Default false.
	 */
	enableSrtp?: bool;

	/**
	 * Custom application data.
	 */
	appData?: any;
};

struct PipeTransportStat
{
	// Common to all Transports.
	type: string;
	transportId: string;
	timestamp: uint32_t;
	sctpState?: SctpState;
	bytesReceived: uint32_t;
	recvBitrate: uint32_t;
	bytesSent: uint32_t;
	sendBitrate: uint32_t;
	rtpBytesReceived: uint32_t;
	rtpRecvBitrate: uint32_t;
	rtpBytesSent: uint32_t;
	rtpSendBitrate: uint32_t;
	rtxBytesReceived: uint32_t;
	rtxRecvBitrate: uint32_t;
	rtxBytesSent: uint32_t;
	rtxSendBitrate: uint32_t;
	probationBytesReceived: uint32_t;
	probationRecvBitrate: uint32_t;
	probationBytesSent: uint32_t;
	probationSendBitrate: uint32_t;
	availableOutgoingBitrate?: uint32_t;
	availableIncomingBitrate?: uint32_t;
	maxIncomingBitrate?: uint32_t;
	// PipeTransport specific.
	tuple: TransportTuple;
}

const Logger* logger = new Logger("PipeTransport");

class PipeTransport : public Transport
{
	// PipeTransport data.
	protected readonly _data:
	{
		tuple: TransportTuple;
		sctpParameters?: SctpParameters;
		sctpState?: SctpState;
		rtx: bool;
		srtpParameters?: SrtpParameters;
	};

	/**
	 * @private
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	constructor(params: any)
	{
		super(params);

		logger->debug("constructor()");

		const { data } = params;

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
	get tuple(): TransportTuple
	{
		return this->_data.tuple;
	}

	/**
	 * SCTP parameters.
	 */
	get sctpParameters(): SctpParameters | undefined
	{
		return this->_data.sctpParameters;
	}

	/**
	 * SCTP state.
	 */
	get sctpState(): SctpState | undefined
	{
		return this->_data.sctpState;
	}

	/**
	 * SRTP parameters.
	 */
	get srtpParameters(): SrtpParameters | undefined
	{
		return this->_data.srtpParameters;
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

		if (this->_data.sctpState)
			this->_data.sctpState = "closed";

		super.close();
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 * @override
	 */
	routerClosed(): void
	{
		if (this->_closed)
			return;

		if (this->_data.sctpState)
			this->_data.sctpState = "closed";

		super.routerClosed();
	}

	/**
	 * Get PipeTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<PipeTransportStat[]>
	{
		logger->debug("getStats()");

		return this->_channel->request("transport.getStats", this->_internal);
	}

	/**
	 * Provide the PipeTransport remote parameters.
	 *
	 * @override
	 */
	async connect(
		{
			ip,
			port,
			srtpParameters
		}:
		{
			ip: string;
			port: uint32_t;
			srtpParameters?: SrtpParameters;
		}
	): Promise<void>
	{
		logger->debug("connect()");

		const reqData = { ip, port, srtpParameters };

		const data =
			co_await this->_channel->request("transport.connect", this->_internal, reqData);

		// Update data.
		this->_data.tuple = data.tuple;
	}

	/**
	 * Create a pipe Consumer.
	 *
	 * @override
	 */
	async consume({ producerId, appData = {} }: ConsumerOptions): Promise<Consumer>
	{
		logger->debug("consume()");

		if (!producerId || typeof producerId !== "string")
			throw new TypeError("missing producerId");
		else if (appData && typeof appData !== "object")
			throw new TypeError("if given, appData must be an object");

		const producer = this->_getProducerById(producerId);

		if (!producer)
			throw Error(`Producer with id "${producerId}" not found`);

		// This may throw.
		const rtpParameters = ortc::getPipeConsumerRtpParameters(
			producer.consumableRtpParameters, this->_data.rtx);

		const internal = { ...this->_internal, consumerId: uuidv4(), producerId };
		const reqData =
		{
			kind                   : producer.kind,
			rtpParameters,
			type                   : "pipe",
			consumableRtpEncodings : producer.consumableRtpParameters.encodings
		};

		const status =
			co_await this->_channel->request("transport.consume", internal, reqData);

		const data = { kind: producer.kind, rtpParameters, type: "pipe" };

		const consumer = new Consumer(
			{
				internal,
				data,
				channel        : this->_channel,
				appData,
				paused         : status.paused,
				producerPaused : status.producerPaused
			});

		this->_consumers.set(consumer.id, consumer);
		consumer.on("@close", () => this->_consumers.delete(consumer.id));
		consumer.on("@producerclose", () => this->_consumers.delete(consumer.id));

		// Emit observer event.
		this->_observer->safeEmit("newconsumer", consumer);

		return consumer;
	}

	private _handleWorkerNotifications(): void
	{
		this->_channel->on(this->_internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case "sctpstatechange":
				{
					const sctpState = data.sctpState as SctpState;

					this->_data.sctpState = sctpState;

					this->safeEmit("sctpstatechange", sctpState);

					// Emit observer event.
					this->_observer->safeEmit("sctpstatechange", sctpState);

					break;
				}

				case "trace":
				{
					const trace = data as TransportTraceEventData;

					this->safeEmit("trace", trace);

					// Emit observer event.
					this->_observer->safeEmit("trace", trace);

					break;
				}

				default:
				{
					logger->error("ignoring unknown event \"%s\"", event);
				}
			}
		});
	}
}
