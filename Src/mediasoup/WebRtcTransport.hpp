#pragma once 

#include "common.hpp"
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
	TransportListenIp listenIps;

	/**
	 * Listen in UDP. Default true.
	 */
	bool enableUdp;

	/**
	 * Listen in TCP. Default false.
	 */
	bool enableTcp;

	/**
	 * Prefer UDP. Default false.
	 */
	bool preferUdp;

	/**
	 * Prefer TCP. Default false.
	 */
	bool preferTcp;

	/**
	 * Initial available outgoing bitrate (in bps). Default 600000.
	 */
	uint32_t initialAvailableOutgoingBitrate;

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
	 * Default 262144.
	 */
	uint32_t maxSctpMessageSize;

	/**
	 * Custom application data.
	 */
	json appData;
};

struct IceParameters
{
	std::string usernameFragment;
	std::string password;
	bool iceLite;
};

struct IceCandidate
{
	std::string foundation;
	uint32_t priority;
	std::string ip;
	TransportProtocol protocol;
	uint32_t port;
	std::string type = "host";
	std::string tcpType = "passive" | undefined;
};

struct DtlsParameters
{
	DtlsRole role;
	std::vector<DtlsFingerprint> fingerprints;
};

/**
 * The hash function algorithm (as defined in the "Hash function Textual Names"
 * registry initially specified in RFC 4572 Section 8) and its corresponding
 * certificate fingerprint value (in lowercase hex string as expressed utilizing
 * the syntax of "fingerprint" in RFC 4572 Section 5).
 */
struct DtlsFingerprint
{
	std::string algorithm;
	std::string value;
};

using IceState = std::string;// "new" | "connected" | "completed" | "disconnected" | "closed";

using DtlsRole = std::string;//"auto" | "client" | "server";

using DtlsState = std::string;//"new" | "connecting" | "connected" | "failed" | "closed";

struct WebRtcTransportStat
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
	// WebRtcTransport specific.
	std::string iceRole;
	IceState iceState;
	TransportTuple iceSelectedTuple;
	DtlsState dtlsState;
};


class WebRtcTransport : public Transport
{
	Logger* logger;
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
		std::string dtlsRemoteCert;
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
	WebRtcTransport(const json& internal,
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
		, logger(new Logger("WebRtcTransport"))
	{
		logger->debug("constructor()");

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
	std::string  dtlsRemoteCert() | undefined
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
	EnhancedEventEmitter* observer()
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

		co_return this->_channel->request("transport.getStats", this->_internal);
	}

	/**
	 * Provide the WebRtcTransport remote parameters.
	 *
	 * @override
	 */
	std::future<void> connect({ dtlsParameters }: { dtlsParameters: DtlsParameters })
	{
		logger->debug("connect()");

		json reqData = { dtlsParameters };

		json data =
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

		json data =
			co_await this->_channel->request("transport.restartIce", this->_internal);

		const { iceParameters } = data;

		this->_data.iceParameters = iceParameters;

		return iceParameters;
	}

private:
	void _handleWorkerNotifications()
	{
		this->_channel->on(this->_internal["transportId"], [=](std::string event, json data)
		{
			if (event == "icestatechange")
			{
				const iceState = data.iceState as IceState;

				this->_data.iceState = iceState;

				this->safeEmit("icestatechange", iceState);

				// Emit observer event.
				this->_observer->safeEmit("icestatechange", iceState);
			}
			else if (event == "iceselectedtuplechange")
			{
				const iceSelectedTuple = data.iceSelectedTuple as TransportTuple;

				this->_data.iceSelectedTuple = iceSelectedTuple;

				this->safeEmit("iceselectedtuplechange", iceSelectedTuple);

				// Emit observer event.
				this->_observer->safeEmit("iceselectedtuplechange", iceSelectedTuple);
			}
			else if (event == "dtlsstatechange")
			{
				const dtlsState = data.dtlsState as DtlsState;
				const dtlsRemoteCert = data.dtlsRemoteCert as string;

				this->_data.dtlsState = dtlsState;

				if (dtlsState == "connected")
					this->_data.dtlsRemoteCert = dtlsRemoteCert;

				this->safeEmit("dtlsstatechange", dtlsState);

				// Emit observer event.
				this->_observer->safeEmit("dtlsstatechange", dtlsState);
			}
			else if (event == "sctpstatechange")
			{
				const sctpState = data.sctpState as SctpState;

				this->_data.sctpState = sctpState;

				this->safeEmit("sctpstatechange", sctpState);

				// Emit observer event.
				this->_observer->safeEmit("sctpstatechange", sctpState);

				break;
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
