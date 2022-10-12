#define MSC_CLASS "Worker"

#include "common.hpp"
#include "Worker.hpp"
#include "Logger.hpp"
#include "Channel.hpp"
#include "Logger.hpp"
#include "errors.hpp"
#include "utils.hpp"
#include "Router.hpp"
#include "ortc.hpp"
#include "PayloadChannel.hpp"
#include "WebRtcServer.hpp"
#include "child_process/SubProcess.hpp"

#define __MEDIASOUP_VERSION__ "__MEDIASOUP_VERSION__"

#ifdef _DEBUG
#define WORK_PATH "mediasoup-worker.exe"
#else
#define WORK_PATH "mediasoup-worker.exe"
#endif

const int CHANNEL_FD = 3;

std::string workerPath;

Worker::Worker(json settings)
	: _observer(new EnhancedEventEmitter())
{
	//this->setMaxListeners(Infinity);

	MSC_DEBUG("constructor()");

	std::string spawnBin = WORK_PATH;
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

	json spawnOptions = {
		{ "env", { {"MEDIASOUP_VERSION", __MEDIASOUP_VERSION__} } },
		{ "detached", false },
		{ "stdio", { "ignore", "pipe", "pipe", "pipe", "pipe", "pipe", "pipe"} },
		{ "windowsHide", false }
	};

	this->_child = SubProcess::spawn(WORK_PATH, spawnArgs, spawnOptions);

	this->_pid = this->_child->pid();

	this->_channel = new Channel(
		this->_child->stdio()[3],
		this->_child->stdio()[4],
		this->_pid);

	this->_payloadChannel = new PayloadChannel(
		this->_child->stdio()[5],
		this->_child->stdio()[6]);

	this->_appData = settings.value("appData", json());

	bool spawnDone = false;

	// Listen for "running" notification.
	this->_channel->once(std::to_string(this->_pid), [&](std::string event, const json& data)
	{
		if (!spawnDone && event == "running")
		{
			spawnDone = true;

			MSC_DEBUG("worker process running [pid:%d]", this->_pid);

			this->emit("@success");
		}
	});

	this->_child->on("exit", [&](int code, int signal)
	{
		this->_child = nullptr;
		this->close();

		if (!spawnDone)
		{
			spawnDone = true;

			if (code == 42)
			{
				MSC_ERROR(
					"worker process failed due to wrong settings [pid:%d]", this->_pid);

				this->emit("@failure", new TypeError("wrong settings"));
			}
			else
			{
				MSC_ERROR(
					"worker process failed unexpectedly [pid:%d, code:%d, signal:%d]",
					this->_pid, code, signal);

				this->emit(
					"@failure",
					new Error("[pid:${ this->_pid }, code : ${ code }, signal : ${ signal }]"));
			}
		}
		else
		{
			MSC_ERROR(
				"worker process died unexpectedly [pid:%d, code:%d, signal:%d]",
				this->_pid, code, signal);

			this->safeEmit(
				"died",
				new Error("[pid:${ this->_pid }, code : ${ code }, signal : ${ signal }]"));
		}
	});

	this->_child->on("error", [=,&spawnDone](Error error)
	{
		this->_child = nullptr;
		this->close();

		if (!spawnDone)
		{
			spawnDone = true;

			MSC_ERROR(
				"worker process failed [pid:%d]: %s", this->_pid, error.ToString().c_str());

			this->emit("@failure", error);
		}
		else
		{
			MSC_ERROR(
				"worker process error [pid:%d]: %s", this->_pid, error.ToString().c_str());

			this->safeEmit("died", error);
		}
	});

	// Be ready for 3rd party worker libraries logging to stdout.
// 	this->_child->stdout!.on("data", (buffer) = >
// 	{
// 		for (const line of buffer.toString("utf8").split("\n"))
// 		{
// 			if (line)
// 				workerLogger.debug(`(stdout) $ { line }`);
// 		}
// 	});
// 
// 	// In case of a worker bug, mediasoup will log to stderr.
// 	this->_child->stderr!.on("data", (buffer) = >
// 	{
// 		for (const line of buffer.toString("utf8").split("\n"))
// 		{
// 			if (line)
// 				workerLogger.error(`(stderr) $ { line }`);
// 		}
// 	});
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

	// Kill the worker process.
	if (this->_child)
	{
		// Remove event listeners but leave a fake "error" hander to avoid
		// propagation.
		this->_child->removeAllListeners("exit");
		this->_child->removeAllListeners("error");
		this->_child->on("error", [](){});
		//this->_child->kill("SIGTERM");
		this->_child = nullptr;
	}

	// Close the Channel instance.
	this->_channel->close();

	// Close the PayloadChannel instance.
	this->_payloadChannel->close();

	// Close every Router.
	for (Router* router : this->_routers)
	{
		router->workerClosed();
	}
	this->_routers.clear();

	// Emit observer event.
	this->_observer->safeEmit("close");

	delete this;
}

task_t<json> Worker::dump()
{
	MSC_DEBUG("dump()");

	json ret = co_await this->_channel->request("worker.dump");

	co_return ret;
}

task_t<json> Worker::getResourceUsage()
{
	MSC_DEBUG("getResourceUsage()");

	json ret = co_await this->_channel->request("worker.getResourceUsage");

	co_return ret;
}

task_t<void> Worker::updateSettings(std::string logLevel, std::vector<std::string> logTags)
{
	MSC_DEBUG("updateSettings()");

	json reqData = {
		{"logLevel", logLevel},
		{"logTags", logTags}
	};

	co_await this->_channel->request("worker.updateSettings", undefined, reqData);
}



task_t<WebRtcServer*> Worker::createWebRtcServer(const WebRtcServerOptions& options) {
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

task_t<Router*> Worker::createRouter(
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
	for (WebRtcServer* webRtcServer : this->_webRtcServers) {
		webRtcServer->workerClosed();
	}
	this->_webRtcServers.clear();

	this->safeEmit("died", error);
	// Emit observer event.
	this->_observer->safeEmit("close");
}
