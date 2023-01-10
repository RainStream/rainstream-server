#define MSC_CLASS "Worker"

#include "common.h"
#include "Worker.h"
#include "Logger.h"
#include "Channel.h"
#include "Logger.h"
#include "errors.h"
#include "utils.h"
#include "Router.h"
#include "ortc.h"
#include "PayloadChannel.h"
#include "WebRtcServer.h"
#include "Utils.h"
#include "Worker/WorkerNative.h"
#include "Worker/WorkerOrigin.h"

namespace mediasoup {

Worker* Worker::Create(json settings, bool native)
{
	Worker* worker;

	if (native)
	{
		worker = new WorkerNative(settings);
	}
	else
	{
		worker = new WorkerOrigin(settings);
	}

	AStringVector spawnArgs;

	std::string logLevel = settings.value("logLevel", "");
	json logTags = settings.value("logTags", json::array());
	uint32_t rtcMinPort = settings.value("rtcMinPort", 0);
	uint32_t rtcMaxPort = settings.value("rtcMaxPort", 0);
	std::string dtlsCertificateFile = settings.value("dtlsCertificateFile", "");
	std::string dtlsPrivateKeyFile = settings.value("dtlsPrivateKeyFile", "");

	if (!logLevel.empty())
		spawnArgs.push_back(Utils::Printf("--logLevel=%s", logLevel.c_str()));

	for (auto& logTag : logTags)
	{
		if (logTag.is_string() && !std::string(logTag).empty())
			spawnArgs.push_back(Utils::Printf("--logTag=%s", std::string(logTag).c_str()));
	}

	if (rtcMinPort > 0)
		spawnArgs.push_back(Utils::Printf("--rtcMinPort=%d", rtcMinPort));

	if (rtcMaxPort > 0)
		spawnArgs.push_back(Utils::Printf("--rtcMaxPort=%d", rtcMaxPort));

	if (!dtlsCertificateFile.empty())
		spawnArgs.push_back(Utils::Printf("--dtlsCertificateFile=%s", dtlsCertificateFile.c_str()));

	if (!dtlsPrivateKeyFile.empty())
		spawnArgs.push_back(Utils::Printf("--dtlsPrivateKeyFile=%s", dtlsPrivateKeyFile.c_str()));

	worker->init(spawnArgs);

	return worker;
}

Worker::Worker(json settings)
	: _observer(new EnhancedEventEmitter())
{
	MSC_DEBUG("constructor()");
}

Worker::~Worker()
{
	delete _observer;
	_observer = nullptr;
}

uint32_t Worker::pid() {
	return this->_pid;
}

bool Worker::closed() {
	return this->_closed;
}

bool Worker::died() {
	return this->_died;
}

json Worker::appData() {
	return this->_appData;
}

void Worker::appData(json appData) {
	MSC_THROW_ERROR("cannot override appData object");
}

EnhancedEventEmitter* Worker::observer() {
	return this->_observer;
}

std::set<WebRtcServer*> Worker::webRtcServersForTesting() {
	return this->_webRtcServers;
}

std::set<Router*> Worker::routersForTesting() {
	return this->_routers;
}

void Worker::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	this->subClose();

	// Close the Channel instance.
	this->_channel->close();

	// Close the PayloadChannel instance.
	this->_payloadChannel->close();

	// Close every Router.
	for (Router* router : this->_routers)
	{
		router->workerClosed();

		delete router;
	}
	this->_routers.clear();

	// Emit observer event.
	this->_observer->safeEmit("close");
}

std::future<json> Worker::dump()
{
	MSC_DEBUG("dump()");

	json ret = co_await this->_channel->request("worker.dump");

	co_return ret;
}

std::future<json> Worker::getResourceUsage()
{
	MSC_DEBUG("getResourceUsage()");

	json ret = co_await this->_channel->request("worker.getResourceUsage");

	co_return ret;
}

std::future<void> Worker::updateSettings(std::string logLevel, std::vector<std::string> logTags)
{
	MSC_DEBUG("updateSettings()");

	json reqData = {
		{"logLevel", logLevel},
		{"logTags", logTags}
	};

	co_await this->_channel->request("worker.updateSettings", undefined, reqData);
}



std::future<WebRtcServer*> Worker::createWebRtcServer(const WebRtcServerOptions& options) {
	MSC_DEBUG("createWebRtcServer()");

	if (!options.appData.is_null() && !options.appData.is_object())
		MSC_THROW_TYPE_ERROR("if given, appData must be an object");

	json reqData = {
		{ "webRtcServerId", uuidv4() },
		{ "listenInfos", options.listenInfos }
	};

	co_await this->_channel->request("worker.createWebRtcServer", undefined, reqData);

	WebRtcServer* webRtcServer = new WebRtcServer(
		{ {"webRtcServerId",  reqData["webRtcServerId"] }},
		 this->_channel, options.appData);

	this->_webRtcServers.insert(webRtcServer);

	webRtcServer->on("@close", [=]() { this->_webRtcServers.erase(webRtcServer); });
	// Emit observer event.
	this->_observer->safeEmit("newwebrtcserver", webRtcServer);

	co_return webRtcServer;
}

std::future<Router*> Worker::createRouter(
	json& mediaCodecs, const json& appData/* = json()*/)
{
	MSC_DEBUG("createRouter()");

	if (!appData.is_null() && !appData.is_object())
		MSC_THROW_ERROR("if given, appData must be an object");

	// This may throw.
	json rtpCapabilities = ortc::generateRouterRtpCapabilities(mediaCodecs);

	json reqData = { { "routerId", uuidv4() } };

	co_await this->_channel->request("worker.createRouter", undefined, reqData);

	json data = { { "rtpCapabilities", rtpCapabilities } };
	Router* router = new Router(
		{ {"routerId", reqData["routerId"]}},
		data,
		this->_channel,
		this->_payloadChannel,
		appData);

	this->_routers.insert(router);
	router->on("@close", [=]() {this->_routers.erase(router); });

	// Emit observer event.
	this->_observer->safeEmit("newrouter", router);

	co_return router;
}

void Worker::workerDied(const Error& error)
{
	if (this->_closed)
		return;

	MSC_DEBUG("died()[error:%s]", error.ToString().c_str());

	this->_closed = true;
	this->_died = true;

	// Close the Channel instance.
	this->_channel->close();
	// Close the PayloadChannel instance.
	this->_payloadChannel->close();

	// Close every Router.
	for (Router* router : this->_routers) {
		router->workerClosed();
	}
	this->_routers.clear();

	// Close every WebRtcServer.
	for (WebRtcServer* webRtcServer : this->_webRtcServers)
	{
		webRtcServer->workerClosed();
		
		delete webRtcServer;
	}
	this->_webRtcServers.clear();

	this->safeEmit("died", error);
	// Emit observer event.
	this->_observer->safeEmit("close");
}

}
