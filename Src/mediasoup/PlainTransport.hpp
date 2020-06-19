#pragma once 

#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"
#include "Transport.hpp"
#include "SctpParameters.hpp"
#include "SrtpParameters.hpp"

struct PlainTransportOptions
{
	/**
	 * Listening IP address.
	 */
	listenIp: TransportListenIp | string;

	/**
	 * Use RTCP-mux (RTP and RTCP in the same port). Default true.
	 */
	rtcpMux?: bool;

	/**
	 * Whether remote IP:port should be auto-detected based on first RTP/RTCP
	 * packet received. If enabled, connect() method must not be called unless
	 * SRTP is enabled. If so, it must be called with just remote SRTP parameters.
	 * Default false.
	 */
	comedia?: bool;

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
	 * Default 262144.
	 */
	maxSctpMessageSize?: uint32_t;

	/**
	 * Enable SRTP. For this to work, connect() must be called
	 * with remote SRTP parameters. Default false.
	 */
	enableSrtp?: bool;

	/**
	 * The SRTP crypto suite to be used if enableSrtp is set. Default
	 * "AES_CM_128_HMAC_SHA1_80".
	 */
	srtpCryptoSuite?: SrtpCryptoSuite;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

/**
 * DEPRECATED: Use PlainTransportOptions.
 */
struct PlainRtpTransportOptions = PlainTransportOptions;

struct PlainTransportStat =
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
	// PlainTransport specific.
	rtcpMux: bool;
	comedia: bool;
	tuple: TransportTuple;
	rtcpTuple?: TransportTuple;
}

/**
 * DEPRECATED: Use PlainTransportStat.
 */
struct PlainRtpTransportStat = PlainTransportStat;

const Logger* logger = new Logger("PlainTransport");

class PlainTransport : public Transport
{
	// PlainTransport data.
	protected readonly _data:
	{
		rtcpMux?: bool;
		comedia?: bool;
		tuple: TransportTuple;
		rtcpTuple?: TransportTuple;
		sctpParameters?: SctpParameters;
		sctpState?: SctpState;
		srtpParameters?: SrtpParameters;
	};

	/**
	 * @private
	 * @emits tuple - (tuple: TransportTuple)
	 * @emits rtcptuple - (rtcpTuple: TransportTuple)
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
			rtcpMux        : data.rtcpMux,
			comedia        : data.comedia,
			tuple          : data.tuple,
			rtcpTuple      : data.rtcpTuple,
			sctpParameters : data.sctpParameters,
			sctpState      : data.sctpState,
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
	 * Transport RTCP tuple.
	 */
	get rtcpTuple(): TransportTuple | undefined
	{
		return this->_data.rtcpTuple;
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
	 * @emits tuple - (tuple: TransportTuple)
	 * @emits rtcptuple - (rtcpTuple: TransportTuple)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	get observer(): EnhancedEventEmitter
	{
		return this->_observer;
	}

	/**
	 * Close the PlainTransport.
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
	 * Get PlainTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<PlainTransportStat[]>
	{
		logger->debug("getStats()");

		return this->_channel->request("transport.getStats", this->_internal);
	}

	/**
	 * Provide the PlainTransport remote parameters.
	 *
	 * @override
	 */
	async connect(
		{
			ip,
			port,
			rtcpPort,
			srtpParameters
		}:
		{
			ip?: string;
			port?: uint32_t;
			rtcpPort?: uint32_t;
			srtpParameters?: SrtpParameters;
		}
	): Promise<void>
	{
		logger->debug("connect()");

		const reqData = { ip, port, rtcpPort, srtpParameters };

		const data =
			co_await this->_channel->request("transport.connect", this->_internal, reqData);

		// Update data.
		if (data.tuple)
			this->_data.tuple = data.tuple;

		if (data.rtcpTuple)
			this->_data.rtcpTuple = data.rtcpTuple;

		this->_data.srtpParameters = data.srtpParameters;
	}

	private _handleWorkerNotifications(): void
	{
		this->_channel->on(this->_internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case "tuple":
				{
					const tuple = data.tuple as TransportTuple;

					this->_data.tuple = tuple;

					this->safeEmit("tuple", tuple);

					// Emit observer event.
					this->_observer.safeEmit("tuple", tuple);

					break;
				}

				case "rtcptuple":
				{
					const rtcpTuple = data.rtcpTuple as TransportTuple;

					this->_data.rtcpTuple = rtcpTuple;

					this->safeEmit("rtcptuple", rtcpTuple);

					// Emit observer event.
					this->_observer.safeEmit("rtcptuple", rtcpTuple);

					break;
				}

				case "sctpstatechange":
				{
					const sctpState = data.sctpState as SctpState;

					this->_data.sctpState = sctpState;

					this->safeEmit("sctpstatechange", sctpState);

					// Emit observer event.
					this->_observer.safeEmit("sctpstatechange", sctpState);

					break;
				}

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

/**
 * DEPRECATED: Use PlainTransport.
 */
class PlainRtpTransport : public PlainTransport
{
	constructor(params: any)
	{
		super(params);
	}
}
