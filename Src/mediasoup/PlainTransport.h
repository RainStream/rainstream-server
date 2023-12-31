#pragma once 

#include "Transport.h"

namespace mediasoup {

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


class MS_EXPORT PlainTransport : public Transport
{
public:
	/**
	 * @private
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
	virtual async_simple::coro::Lazy<json> getStats();

	/**
	 * Provide the PlainTransport remote parameters.
	 *
	 * @override
	 */
	async_simple::coro::Lazy<void> connect(
		std::string ip,
		uint32_t port,
		uint32_t rtcpPort,
		SrtpParameters srtpParameters
	);

private:
	void _handleWorkerNotifications();

private:
	// PlainTransport data.
	json _data;
};

}
