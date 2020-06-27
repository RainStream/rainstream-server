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


class PlainTransport : public Transport
{
	Logger* logger;
	// PlainTransport data.
protected:
	json _data;
// 	{
// 		rtcpMux?: bool;
// 		comedia?: bool;
// 		tuple: TransportTuple;
// 		rtcpTuple?: TransportTuple;
// 		sctpParameters?: SctpParameters;
// 		sctpState?: SctpState;
// 		srtpParameters?: SrtpParameters;
// 	};

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
		GetDataProducerById getDataProducerById)
		: Transport(internal, data, channel, payloadChannel,
			appData, getRouterRtpCapabilities,
			getProducerById, getDataProducerById)
		, logger(new Logger("PlainTransport"))
	{
		logger->debug("constructor()");

		this->_data =
		{
			{ "rtcpMux"        , data["rtcpMux"] },
			{ "comedia"        , data["comedia"] },
			{ "tuple"          , data["tuple"] },
			{ "rtcpTuple"      , data["rtcpTuple"] },
			{ "sctpParameters" , data["sctpParameters"] },
			{ "sctpState"      , data["sctpState"] },
			{ "srtpParameters" , data["srtpParameters"] }
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
	 * Transport RTCP tuple.
	 */
	TransportTuple rtcpTuple()
	{
		return this->_data["rtcpTuple"];
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
	 * @emits tuple - (tuple: TransportTuple)
	 * @emits rtcptuple - (rtcpTuple: TransportTuple)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	EnhancedEventEmitter* observer()
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
	 * Get PlainTransport stats.
	 *
	 * @override
	 */
	std::future<json> getStats()
	{
		logger->debug("getStats()");

		co_return this->_channel->request("transport.getStats", this->_internal);
	}

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
	)
	{
		logger->debug("connect()");

		json reqData = { 
			{ "ip", ip },
			{ "port", port },
			{ "rtcpPort", rtcpPort },
			{ "srtpParameters", srtpParameters }
		};

		json data =
			co_await this->_channel->request("transport.connect", this->_internal, reqData);

		// Update data.
		if (data.count("tuple"))
			this->_data["tuple"] = data["tuple"];

		if (data.count("rtcpTuple"))
			this->_data["rtcpTuple"] = data["rtcpTuple"];

		this->_data["srtpParameters"] = data["srtpParameters"];
	}

private:
	void _handleWorkerNotifications()
	{
		this->_channel->on(this->_internal["transportId"], [=](std::string event, json data)
		{
			if(event == "tuple")
			{
// 				const tuple = data.tuple as TransportTuple;
// 
// 				this->_data.tuple = tuple;
// 
// 				this->safeEmit("tuple", tuple);
// 
// 				// Emit observer event.
// 				this->_observer->safeEmit("tuple", tuple);
			}
			else if (event == "rtcptuple")
			{
// 				const rtcpTuple = data.rtcpTuple as TransportTuple;
// 
// 				this->_data.rtcpTuple = rtcpTuple;
// 
// 				this->safeEmit("rtcptuple", rtcpTuple);
// 
// 				// Emit observer event.
// 				this->_observer->safeEmit("rtcptuple", rtcpTuple);
			}
			else if (event == "sctpstatechange")
			{
// 				const sctpState = data.sctpState as SctpState;
// 
// 				this->_data.sctpState = sctpState;
// 
// 				this->safeEmit("sctpstatechange", sctpState);
// 
// 				// Emit observer event.
// 				this->_observer->safeEmit("sctpstatechange", sctpState);
			}
			else if (event == "trace")
			{
// 				const trace = data as TransportTraceEventData;
// 
// 				this->safeEmit("trace", trace);
// 
// 				// Emit observer event.
// 				this->_observer->safeEmit("trace", trace);
			}
			else
			{
				logger->error("ignoring unknown event \"%s\"", event);
			}
		});
	}
};

/**
 * DEPRECATED: Use PlainTransport.
 */
class PlainRtpTransport : public PlainTransport
{
public:
	PlainRtpTransport(const json& params)
		: PlainTransport(params)
	{

	}
};
