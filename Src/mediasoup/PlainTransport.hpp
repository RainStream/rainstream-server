#pragma once 

#include "Transport.hpp"

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
	// PlainTransport data.
	json _data;
};


