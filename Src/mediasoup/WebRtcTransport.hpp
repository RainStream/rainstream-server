#pragma once 

#include "Logger.hpp"

#include "EnhancedEventEmitter.hpp"
#include "Transport.hpp"
#include "SctpParameters.hpp"


struct WebRtcTransportOptions
{
	/**
	 * Listening IP address or addresses in order of preference (first one is the
	 * preferred one).
	 */
	listenIps: (TransportListenIp | string)[];

	/**
	 * Listen in UDP. Default true.
	 */
	enableUdp?: bool;

	/**
	 * Listen in TCP. Default false.
	 */
	enableTcp?: bool;

	/**
	 * Prefer UDP. Default false.
	 */
	preferUdp?: bool;

	/**
	 * Prefer TCP. Default false.
	 */
	preferTcp?: bool;

	/**
	 * Initial available outgoing bitrate (in bps). Default 600000.
	 */
	initialAvailableOutgoingBitrate?: uint32_t;

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
	 * Custom application data.
	 */
	appData?: any;
};

struct IceParameters
{
	usernameFragment: string;
	password: string;
	iceLite?: bool;
};

struct IceCandidate
{
	foundation: string;
	priority: uint32_t;
	ip: string;
	protocol: TransportProtocol;
	port: uint32_t;
	type: "host";
	tcpType: "passive" | undefined;
};

struct DtlsParameters
{
	role?: DtlsRole;
	fingerprints: DtlsFingerprint[];
};

/**
 * The hash function algorithm (as defined in the "Hash function Textual Names"
 * registry initially specified in RFC 4572 Section 8) and its corresponding
 * certificate fingerprint value (in lowercase hex string as expressed utilizing
 * the syntax of "fingerprint" in RFC 4572 Section 5).
 */
struct DtlsFingerprint
{
	algorithm: string;
	value: string;
};

struct IceState = "new" | "connected" | "completed" | "disconnected" | "closed";

struct DtlsRole = "auto" | "client" | "server";

struct DtlsState = "new" | "connecting" | "connected" | "failed" | "closed";

struct WebRtcTransportStat =
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
	// WebRtcTransport specific.
	iceRole: string;
	iceState: IceState;
	iceSelectedTuple?: TransportTuple;
	dtlsState: DtlsState;
};

const Logger* logger = new Logger("WebRtcTransport");

class WebRtcTransport : public Transport
{
	// WebRtcTransport data.
	protected readonly _data:
	{
		iceRole: "controlled";
		iceParameters: IceParameters;
		iceCandidates: IceCandidate[];
		iceState: IceState;
		iceSelectedTuple?: TransportTuple;
		dtlsParameters: DtlsParameters;
		dtlsState: DtlsState;
		dtlsRemoteCert?: string;
		sctpParameters?: SctpParameters;
		sctpState?: SctpState;
	};

	/**
	 * @private
	 * @emits icestatechange - (iceState: IceState)
	 * @emits iceselectedtuplechange - (iceSelectedTuple: TransportTuple)
	 * @emits dtlsstatechange - (dtlsState: DtlsState)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	WebRtcTransport(const json& params)
	{
		super(params);

		logger->debug("constructor()");

		const { data } = params;

		this->_data =
		{
			iceRole          : data.iceRole,
			iceParameters    : data.iceParameters,
			iceCandidates    : data.iceCandidates,
			iceState         : data.iceState,
			iceSelectedTuple : data.iceSelectedTuple,
			dtlsParameters   : data.dtlsParameters,
			dtlsState        : data.dtlsState,
			dtlsRemoteCert   : data.dtlsRemoteCert,
			sctpParameters   : data.sctpParameters,
			sctpState        : data.sctpState
		};

		this->_handleWorkerNotifications();
	}

	/**
	 * ICE role.
	 */
	get iceRole(): "controlled"
	{
		return this->_data.iceRole;
	}

	/**
	 * ICE parameters.
	 */
	IceParameters iceParameters()
	{
		return this->_data.iceParameters;
	}

	/**
	 * ICE candidates.
	 */
	IceCandidate[] iceCandidates()
	{
		return this->_data.iceCandidates;
	}

	/**
	 * ICE state.
	 */
	IceState iceState()
	{
		return this->_data.iceState;
	}

	/**
	 * ICE selected tuple.
	 */
	get iceSelectedTuple(): TransportTuple | undefined
	{
		return this->_data.iceSelectedTuple;
	}

	/**
	 * DTLS parameters.
	 */
	DtlsParameters dtlsParameters()
	{
		return this->_data.dtlsParameters;
	}

	/**
	 * DTLS state.
	 */
	DtlsState dtlsState()
	{
		return this->_data.dtlsState;
	}

	/**
	 * Remote certificate in PEM format.
	 */
	get dtlsRemoteCert(): string | undefined
	{
		return this->_data.dtlsRemoteCert;
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
	 * Observer.
	 *
	 * @override
	 * @emits close
	 * @emits newproducer - (producer: Producer)
	 * @emits newconsumer - (producer: Producer)
	 * @emits newdataproducer - (dataProducer: DataProducer)
	 * @emits newdataconsumer - (dataProducer: DataProducer)
	 * @emits icestatechange - (iceState: IceState)
	 * @emits iceselectedtuplechange - (iceSelectedTuple: TransportTuple)
	 * @emits dtlsstatechange - (dtlsState: DtlsState)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	get observer(): EnhancedEventEmitter
	{
		return this->_observer;
	}

	/**
	 * Close the WebRtcTransport.
	 *
	 * @override
	 */
	void close()
	{
		if (this->_closed)
			return;

		this->_data.iceState = "closed";
		this->_data.iceSelectedTuple = undefined;
		this->_data.dtlsState = "closed";

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
	void routerClosed()
	{
		if (this->_closed)
			return;

		this->_data.iceState = "closed";
		this->_data.iceSelectedTuple = undefined;
		this->_data.dtlsState = "closed";

		if (this->_data.sctpState)
			this->_data.sctpState = "closed";

		super.routerClosed();
	}

	/**
	 * Get WebRtcTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<WebRtcTransportStat[]>
	{
		logger->debug("getStats()");

		return this->_channel->request("transport.getStats", this->_internal);
	}

	/**
	 * Provide the WebRtcTransport remote parameters.
	 *
	 * @override
	 */
	async connect({ dtlsParameters }: { dtlsParameters: DtlsParameters }): Promise<void>
	{
		logger->debug("connect()");

		const reqData = { dtlsParameters };

		const data =
			co_await this->_channel->request("transport.connect", this->_internal, reqData);

		// Update data.
		this->_data.dtlsParameters.role = data.dtlsLocalRole;
	}

	/**
	 * Restart ICE.
	 */
	async restartIce(): Promise<IceParameters>
	{
		logger->debug("restartIce()");

		const data =
			co_await this->_channel->request("transport.restartIce", this->_internal);

		const { iceParameters } = data;

		this->_data.iceParameters = iceParameters;

		return iceParameters;
	}

	private _handleWorkerNotifications(): void
	{
		this->_channel->on(this->_internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case "icestatechange":
				{
					const iceState = data.iceState as IceState;

					this->_data.iceState = iceState;

					this->safeEmit("icestatechange", iceState);

					// Emit observer event.
					this->_observer.safeEmit("icestatechange", iceState);

					break;
				}

				case "iceselectedtuplechange":
				{
					const iceSelectedTuple = data.iceSelectedTuple as TransportTuple;

					this->_data.iceSelectedTuple = iceSelectedTuple;

					this->safeEmit("iceselectedtuplechange", iceSelectedTuple);

					// Emit observer event.
					this->_observer.safeEmit("iceselectedtuplechange", iceSelectedTuple);

					break;
				}

				case "dtlsstatechange":
				{
					const dtlsState = data.dtlsState as DtlsState;
					const dtlsRemoteCert = data.dtlsRemoteCert as string;

					this->_data.dtlsState = dtlsState;

					if (dtlsState === "connected")
						this->_data.dtlsRemoteCert = dtlsRemoteCert;

					this->safeEmit("dtlsstatechange", dtlsState);

					// Emit observer event.
					this->_observer.safeEmit("dtlsstatechange", dtlsState);

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
};
