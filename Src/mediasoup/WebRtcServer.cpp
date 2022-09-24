#define MSC_CLASS "WebRtcServer"

#include "common.hpp"
#include "WebRtcServer.hpp"
#include "WebRtcTransport.hpp"
#include "Channel.hpp"
#include "errors.hpp"
#include "Logger.hpp"


WebRtcServer::WebRtcServer(const json& internal, Channel* channel, const json& appData)
	: EnhancedEventEmitter()
	, _internal(internal)
	, _channel(channel)
	, _appData(appData)
{
	MSC_DEBUG("constructor()");
}

std::string WebRtcServer::id()
{
	return this->_internal["webRtcServerId"];
}

bool WebRtcServer::closed()
{
	return this->_closed;
}

json WebRtcServer::appData()
{
	MSC_THROW_ERROR("cannot override appData object");
}

void WebRtcServer::appData(json appData)
{
	this->_appData = appData;
}

std::map<std::string, WebRtcTransport*> WebRtcServer::webRtcTransportsForTesting()
{
	return this->_webRtcTransports;
}

void WebRtcServer::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	json reqData = { { "webRtcServerId", this->_internal["webRtcServerId"]}};

	try
	{
		this->_channel->request("worker.closeWebRtcServer", undefined, reqData);
	}	
	catch (const std::exception&)
	{

	}

	// Close every WebRtcTransport.
	//for (const webRtcTransport of this->_webRtcTransports.values())
	//{
	//	webRtcTransport.listenServerClosed();

	//	// Emit observer event.
	//	this->_observer.safeEmit("webrtctransportunhandled", webRtcTransport);
	//}
	this->_webRtcTransports.clear();

	this->emit("@close");

	// Emit observer event.
	//this->_observer.safeEmit("close");
}

void WebRtcServer::workerClosed()
{
	if (this->_closed)
		return;

	MSC_DEBUG("workerClosed()");

	this->_closed = true;

	// NOTE: No need to close WebRtcTransports since they are closed by their
	// respective Router parents.
	this->_webRtcTransports.clear();

	this->safeEmit("workerclose");

	// Emit observer event.
	//this->_observer.safeEmit("close");
}

std::future<json> WebRtcServer::dump()
{
	MSC_DEBUG("dump()");

	json ret = co_await this->_channel->request("webRtcServer.dump", this->_internal["webRtcServerId"]);

	co_return ret;
}

void WebRtcServer::handleWebRtcTransport(WebRtcTransport* webRtcTransport)
{
	this->_webRtcTransports.insert(std::pair(webRtcTransport->id(), webRtcTransport));

	// Emit observer event.
	//this->_observer.safeEmit("webrtctransporthandled", webRtcTransport);

	webRtcTransport->on("@close", [=]()
	{
		this->_webRtcTransports.erase(webRtcTransport->id());

		// Emit observer event.
		//this->_observer.safeEmit("webrtctransportunhandled", webRtcTransport);
	});
}
