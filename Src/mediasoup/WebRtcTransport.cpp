#define MSC_CLASS "WebRtcTransport"

#include "common.hpp"
#include "WebRtcTransport.hpp"
#include "Channel.hpp"
#include "Logger.hpp"
#include "SctpParameters.hpp"

/**
 * @private
 * @emits icestatechange - (iceState: IceState)
 * @emits iceselectedtuplechange - (iceSelectedTuple: TransportTuple)
 * @emits dtlsstatechange - (dtlsState: DtlsState)
 * @emits sctpstatechange - (sctpState: SctpState)
 * @emits trace - (trace: TransportTraceEventData)
 */
WebRtcTransport::WebRtcTransport(const json& internal,
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
{
	MSC_DEBUG("constructor()");

	this->_data =
	{
		{ "iceRole"          , data["iceRole"] },
		{ "iceParameters"    , data["iceParameters"] },
		{ "iceCandidates"    , data["iceCandidates"] },
		{ "iceState"         , data["iceState"] },
		{ "iceSelectedTuple" , data["iceSelectedTuple"] },
		{ "dtlsParameters"   , data["dtlsParameters"] },
		{ "dtlsState"        , data["dtlsState"] },
		{ "dtlsRemoteCert"   , data["dtlsRemoteCert"] },
		{ "sctpParameters"   , data["sctpParameters"] },
		{ "sctpState"        , data["sctpState"] }
	};

	this->_handleWorkerNotifications();
}

/**
 * ICE role.
 */
std::string WebRtcTransport::iceRole()
{
	return this->_data["iceRole"];
}

/**
 * ICE parameters.
 */
IceParameters WebRtcTransport::iceParameters()
{
	return this->_data["iceParameters"];
}

/**
 * ICE candidates.
 */
json WebRtcTransport::iceCandidates()
{
	return this->_data["iceCandidates"];
}

/**
 * ICE state.
 */
IceState WebRtcTransport::iceState()
{
	return this->_data["iceState"];
}

/**
 * ICE selected tuple.
 */
TransportTuple WebRtcTransport::iceSelectedTuple()
{
	return this->_data["iceSelectedTuple"];
}

/**
 * DTLS parameters.
 */
DtlsParameters WebRtcTransport::dtlsParameters()
{
	return this->_data["dtlsParameters"];
}

/**
 * DTLS state.
 */
DtlsState WebRtcTransport::dtlsState()
{
	return this->_data["dtlsState"];
}

/**
 * Remote certificate in PEM format.
 */
std::string WebRtcTransport::dtlsRemoteCert()
{
	return this->_data["dtlsRemoteCert"];
}

/**
 * SCTP parameters.
 */
SctpParameters WebRtcTransport::sctpParameters()
{
	return this->_data["sctpParameters"];
}

/**
 * SCTP state.
 */
SctpState WebRtcTransport::sctpState()
{
	return this->_data["sctpState"];
}

std::string WebRtcTransport::typeName()
{
	return "WebRtcTransport";
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
EnhancedEventEmitter* WebRtcTransport::observer()
{
	return this->_observer;
}

/**
 * Close the WebRtcTransport.
 *
 * @override
 */
void WebRtcTransport::close()
{
	if (this->_closed)
		return;

	this->_data["iceState"] = "closed";
	this->_data["iceSelectedTuple"].clear();
	this->_data["dtlsState"] = "closed";

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
void WebRtcTransport::routerClosed()
{
	if (this->_closed)
		return;

	this->_data["iceState"] = "closed";
	this->_data["iceSelectedTuple"].clear();
	this->_data["dtlsState"] = "closed";

	if (this->_data.contains("sctpState"))
		this->_data["sctpState"] = "closed";

	Transport::routerClosed();
}

/**
 * Get WebRtcTransport stats.
 *
 * @override
 */
std::future<json> WebRtcTransport::getStats()
{
	MSC_DEBUG("getStats()");

	json ret = co_await this->_channel->request("transport.getStats", this->_internal);

	co_return ret;
}

/**
 * Provide the WebRtcTransport remote parameters.
 *
 * @override
 */
std::future<void> WebRtcTransport::connect(DtlsParameters dtlsParameters)
{
	MSC_DEBUG("connect()");

	json reqData = { { "dtlsParameters", dtlsParameters } };

	json data =
		co_await this->_channel->request("transport.connect", this->_internal, reqData);

	// Update data.
	this->_data["dtlsParameters"]["role"] = data["dtlsLocalRole"];
}

/**
 * Restart ICE.
 */
std::future<IceParameters> WebRtcTransport::restartIce()
{
	MSC_DEBUG("restartIce()");

	json data =
		co_await this->_channel->request("transport.restartIce", this->_internal);

	json iceParameters = data["iceParameters"];

	this->_data["iceParameters"] = iceParameters;

	return iceParameters;
}

void WebRtcTransport::_handleWorkerNotifications()
{
	this->_channel->on(this->_internal["transportId"], [=](std::string event, json data)
	{
		if (event == "icestatechange")
		{
			// 				const iceState = data.iceState as IceState;
			// 
			// 				this->_data.iceState = iceState;
			// 
			// 				this->safeEmit("icestatechange", iceState);
			// 
			// 				// Emit observer event.
			// 				this->_observer->safeEmit("icestatechange", iceState);
		}
		else if (event == "iceselectedtuplechange")
		{
			// 				const iceSelectedTuple = data.iceSelectedTuple as TransportTuple;
			// 
			// 				this->_data.iceSelectedTuple = iceSelectedTuple;
			// 
			// 				this->safeEmit("iceselectedtuplechange", iceSelectedTuple);
			// 
			// 				// Emit observer event.
			// 				this->_observer->safeEmit("iceselectedtuplechange", iceSelectedTuple);
		}
		else if (event == "dtlsstatechange")
		{
			// 				const dtlsState = data.dtlsState as DtlsState;
			// 				const dtlsRemoteCert = data.dtlsRemoteCert as string;
			// 
			// 				this->_data.dtlsState = dtlsState;
			// 
			// 				if (dtlsState == "connected")
			// 					this->_data.dtlsRemoteCert = dtlsRemoteCert;
			// 
			// 				this->safeEmit("dtlsstatechange", dtlsState);
			// 
			// 				// Emit observer event.
			// 				this->_observer->safeEmit("dtlsstatechange", dtlsState);
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
			MSC_ERROR("ignoring unknown event \"%s\"", event.c_str());
		}
	});
}

