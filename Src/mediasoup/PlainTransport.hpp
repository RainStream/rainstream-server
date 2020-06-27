#pragma once 

#include "common.hpp"
#include "Logger.hpp"
#include "Transport.hpp"
#include "SctpParameters.hpp"
#include "SrtpParameters.hpp"

struct PlainTransportOptions
{
	/**
	 * Listening IP address.
	 */
	TransportListenIp listenIp;

	/**
	 * Use RTCP-mux (RTP and RTCP in the same port). Default true.
	 */
	bool rtcpMux;

	/**
	 * Whether remote IP:port should be auto-detected based on first RTP/RTCP
	 * packet received. If enabled, connect() method must not be called unless
	 * SRTP is enabled. If so, it must be called with just remote SRTP parameters.
	 * Default false.
	 */
	bool comedia;

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
	 * Enable SRTP. For this to work, connect() must be called
	 * with remote SRTP parameters. Default false.
	 */
	bool enableSrtp;

	/**
	 * The SRTP crypto suite to be used if enableSrtp is set. Default
	 * "AES_CM_128_HMAC_SHA1_80".
	 */
	SrtpCryptoSuite srtpCryptoSuite;

	/**
	 * Custom application data.
	 */
	json appData;
};

/**
 * DEPRECATED: Use PlainTransportOptions.
 */
using PlainRtpTransportOptions = PlainTransportOptions;

struct PlainTransportStat
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
	// PlainTransport specific.
	bool rtcpMux;
	bool comedia;
	TransportTuple tuple;
	TransportTuple rtcpTuple;
};

/**
 * DEPRECATED: Use PlainTransportStat.
 */
using PlainRtpTransportStat = PlainTransportStat;

class PayloadChannel;


class PlainTransport : public Transport
{
public:
	/**
	 * @private
	 * @emits tuple - (tuple: TransportTuple)
	 * @emits rtcptuple - (rtcpTuple: TransportTuple)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	PlainTransport(const json& internal,
		const json& data,
		Channel* channel,
		PayloadChannel* payloadChannel,
		const json& appData,
		GetRouterRtpCapabilities getRouterRtpCapabilities,
		GetProducerById getProducerById,
		GetDataProducerById getDataProducerById);

	/**
	 * Transport tuple.
	 */
	TransportTuple tuple();

	/**
	 * Transport RTCP tuple.
	 */
	TransportTuple rtcpTuple();

	/**
	 * SCTP parameters.
	 */
	SctpParameters sctpParameters();

	/**
	 * SCTP state.
	 */
	SctpState sctpState();

	/**
	 * SRTP parameters.
	 */
	SrtpParameters srtpParameters();

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
	 * @emits tuple - (tuple: TransportTuple)
	 * @emits rtcptuple - (rtcpTuple: TransportTuple)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	EnhancedEventEmitter* observer();

	/**
	 * Close the PlainTransport.
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
	 * Get PlainTransport stats.
	 *
	 * @override
	 */
	std::future<json> getStats();

	/**
	 * Provide the PlainTransport remote parameters.
	 *
	 * @override
	 */
	std::future<void> connect(
		std::string ip,
		uint32_t port,
		uint32_t rtcpPort,
		SrtpParameters srtpParameters
	);

private:
	void _handleWorkerNotifications();

private:
	Logger* logger;
	// PlainTransport data.
	json _data;
};


