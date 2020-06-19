#pragma once 

#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"
import { UnsupportedError } from "./errors";
import { Transport, TransportTraceEventData } from "./Transport";
import { Producer, ProducerOptions } from "./Producer";
import { Consumer, ConsumerOptions } from "./Consumer";

struct DirectTransportOptions =
{
	/**
	 * Maximum allowed size for direct messages sent from DataProducers.
	 * Default 262144.
	 */
	maxMessageSize: uint32_t;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

struct DirectTransportStat =
{
	// Common to all Transports.
	type: string;
	transportId: string;
	timestamp: uint32_t;
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
}

const Logger* logger = new Logger("DirectTransport");

class DirectTransport : public Transport
{
	// DirectTransport data.
	protected readonly _data:
	{
		// TODO
	};

	/**
	 * @private
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	constructor(params: any)
	{
		super(params);

		logger->debug("constructor()");

		const { data } = params;

		// TODO
		this->_data = data;
		// {
		//
		// };

		this->_handleWorkerNotifications();
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
	get observer(): EnhancedEventEmitter
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

		super.routerClosed();
	}

	/**
	 * Get DirectTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<DirectTransportStat[]>
	{
		logger->debug("getStats()");

		return this->_channel->request("transport.getStats", this->_internal);
	}

	/**
	 * NO-OP method in DirectTransport.
	 *
	 * @override
	 */
	async connect(): Promise<void>
	{
		logger->debug("connect()");
	}

	/**
	 * @override
	 */
	// eslint-disable-next-line @typescript-eslint/no-unused-vars
	async setMaxIncomingBitrate(bitrate: uint32_t): Promise<void>
	{
		throw new UnsupportedError(
			"setMaxIncomingBitrate() not implemented in DirectTransport");
	}

	/**
	 * @override
	 */
	// eslint-disable-next-line @typescript-eslint/no-unused-vars
	async produce(options: ProducerOptions): Promise<Producer>
	{
		throw new UnsupportedError("produce() not implemented in DirectTransport");
	}

	/**
	 * @override
	 */
	// eslint-disable-next-line @typescript-eslint/no-unused-vars
	async consume(options: ConsumerOptions): Promise<Consumer>
	{
		throw new UnsupportedError("consume() not implemented in DirectTransport");
	}

	private _handleWorkerNotifications(): void
	{
		this->_channel->on(this->_internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case "trace":
				{
					const trace = data as TransportTraceEventData;

					this->safeEmit("trace", trace);

					// Emit observer event.
					this->_observer.safeEmit("trace", trace);

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
