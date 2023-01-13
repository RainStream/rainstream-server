#pragma once

#include "EnhancedEventEmitter.h"

namespace mediasoup{

class Error;
class Router;
class Channel;
class Request;
class SubProcess;
class WebRtcServer;
class PayloadChannel;

struct WebRtcServerOptions;

#define __MEDIASOUP_VERSION__ "__MEDIASOUP_VERSION__"


class MS_EXPORT Worker : public EnhancedEventEmitter
{
public:
	static Worker* Create(json settings, bool native);

	virtual ~Worker();
	/**
	 * Worker process identifier (PID).
	 */
	uint32_t pid();
	/**
	* Whether the Worker is closed.
	*/
	bool closed();
	/**
	 * Whether the Worker died.
	 */
	bool died();
	/**
	 * App custom data.
	 */
	json appData();
	/**
	 * Invalid setter.
	 */
	void appData(json appData);
	/**
	 * Observer.
	 */
	EnhancedEventEmitter* observer();
	/**
	 * @private
	 * Just for testing purposes.
	 */
	std::set<WebRtcServer*> webRtcServersForTesting();
	/**
	 * @private
	 * Just for testing purposes.
	 */
	std::set<Router*> routersForTesting();
	/**
	 * Close the Worker.
	 */
	void close();
	/**
	* Dump Worker.
	*/
	async_simple::coro::Lazy<json> dump();
	/**
	 * Get mediasoup-worker process resource usage.
	 */
	async_simple::coro::Lazy<json> getResourceUsage();
	/**
	 * Update settings.
	 */
	async_simple::coro::Lazy<void> updateSettings(std::string logLevel, std::vector<std::string> logTags);
	/**
	 * Create a WebRtcServer.
	 */
	async_simple::coro::Lazy<WebRtcServer*> createWebRtcServer(const WebRtcServerOptions& options);
	/**
	 * Create a Router.
	 */
	async_simple::coro::Lazy<Router*> createRouter(json& mediaCodecs, const json& appData = json::object());

protected:
	Worker(json settings);
	void workerDied(const Error &error);

protected:
	virtual void init(AStringVector spawnArgs) = 0;
	virtual void subClose() = 0;
protected:
	// Worker process PID.
	uint32_t _pid{ 0 };
	// Channel instance.
	Channel* _channel{ nullptr };
	// PayloadChannel instance.
	PayloadChannel* _payloadChannel{ nullptr };
	// Closed flag.
	bool _closed = false;
	// Died dlag.
	bool _died = false;
	// Custom app data.
	json _appData;
	// WebRtcServers set.
	std::set<WebRtcServer*> _webRtcServers;
	// Routers set.
	std::set<Router*> _routers;
	// Observer instance.
	EnhancedEventEmitter* _observer{ nullptr };
};

}
