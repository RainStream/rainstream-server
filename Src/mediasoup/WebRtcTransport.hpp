#pragma once 

#include "EnhancedEventEmitter.hpp"
#include "Transport.hpp"


using IceState = std::string;// "new" | "connected" | "completed" | "disconnected" | "closed";

using DtlsRole = std::string;//"auto" | "client" | "server";

using DtlsState = std::string;//"new" | "connecting" | "connected" | "failed" | "closed";


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


struct WebRtcTransportOptions
{
	/**
	 * Listening IP address or addresses in order of preference (first one is the
	 * preferred one).
	 */
	json listenIps;

	/**
	 * Listen in UDP. Default true.
	 */
	bool enableUdp = true;

	/**
	 * Listen in TCP. Default false.
	 */
	bool enableTcp = false;

	/**
	 * Prefer UDP. Default false.
	 */
	bool preferUdp = false;

	/**
	 * Prefer TCP. Default false.
	 */
	bool preferTcp = false;

	/**
	 * Initial available outgoing bitrate (in bps). Default 600000.
	 */
	uint32_t initialAvailableOutgoingBitrate  = 600000;

	/**
	 * Create a SCTP association. Default false.
	 */
	bool enableSctp  = false;

	/**
	 * SCTP streams uint32_t.
	 */
	json numSctpStreams = { { "OS", 1024 }, { "MIS", 1024 } };

	/**
	 * Maximum allowed size for SCTP messages sent by DataProducers.
	 * Default 262144.
	 */
	uint32_t maxSctpMessageSize = 262144;

	/**
	 * Custom application data.
	 */
	json appData = json::object();
};

struct IceParameters
{
	IceParameters()
		: iceLite(true)
	{

	}

	IceParameters(const json& data)
		: iceLite(true)
	{
		if (data.is_object())
		{
			usernameFragment = data.value("usernameFragment","");
			password = data.value("password", "");
			iceLite = data.value("iceLite", true);
		}
	}

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
	std::string tcpType = "passive";
};

struct DtlsParameters
{
	DtlsParameters()
	{

	}

	DtlsParameters(const json& data)
		: DtlsParameters()
	{
		if (data.is_object())
		{
			this->role = data.value("role", "");
			for (auto& fingerprint : data["fingerprints"])
			{
				DtlsFingerprint dtlsFingerprint;
				dtlsFingerprint.algorithm = fingerprint.value("algorithm", "");
				dtlsFingerprint.value = fingerprint.value("value", "");

				fingerprints.push_back(dtlsFingerprint);
			}
		}
	}

	operator json() const
	{
		json jFingerprints = json::array();
		for (auto& fingerprint : fingerprints)
		{
			json jFingerprint = 
			{
				{ "algorithm", fingerprint.algorithm },
				{ "value", fingerprint.value }
			};

			jFingerprints.push_back(jFingerprint);
		}

		json data = {
			{ "role", role },
			{ "fingerprints", jFingerprints }
		};

		return data;
	}

	DtlsRole role;
	std::vector<DtlsFingerprint> fingerprints;
};



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

class PayloadChannel;


class MS_EXPORT WebRtcTransport : public Transport
{
public:
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
		GetDataProducerById getDataProducerById);

	/**
	 * ICE role.
	 */
	std::string iceRole();

	/**
	 * ICE parameters.
	 */
	json iceParameters();

	/**
	 * ICE candidates.
	 */
	json iceCandidates();

	/**
	 * ICE state.
	 */
	IceState iceState();

	/**
	 * ICE selected tuple.
	 */
	TransportTuple iceSelectedTuple();

	/**
	 * DTLS parameters.
	 */
	json dtlsParameters();

	/**
	 * DTLS state.
	 */
	DtlsState dtlsState();

	/**
	 * Remote certificate in PEM format.
	 */
	std::string dtlsRemoteCert();

	/**
	 * SCTP parameters.
	 */
	json sctpParameters();

	/**
	 * SCTP state.
	 */
	SctpState sctpState();

	virtual std::string typeName();

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
	EnhancedEventEmitter* observer();

	/**
	 * Close the WebRtcTransport.
	 *
	 * @override
	 */
	void close();

	/**
	 * Router was closed.
	 *
	 * @private
	 * @override
	 */
	void routerClosed();

	/**
	 * Get WebRtcTransport stats.
	 *
	 * @override
	 */
	virtual cppcoro::task<json> getStats();

	/**
	 * Provide the WebRtcTransport remote parameters.
	 *
	 * @override
	 */
	cppcoro::task<void> connect(json& dtlsParameters);

	/**
	 * Restart ICE.
	 */
	cppcoro::task<json> restartIce();

private:
	void _handleWorkerNotifications();

private:
	// WebRtcTransport data.
	json _data;
};
