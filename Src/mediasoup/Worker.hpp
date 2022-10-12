#pragma once

#include "EnhancedEventEmitter.hpp"

class Error;
class Router;
class Channel;
class Request;
class SubProcess;
class WebRtcServer;
class PayloadChannel;

struct WebRtcServerOptions;


class MS_EXPORT Worker : public EnhancedEventEmitter
{
public:
	Worker(json settings);
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
	task_t<json> dump();
	/**
	 * Get mediasoup-worker process resource usage.
	 */
	task_t<json> getResourceUsage();
	/**
	 * Update settings.
	 */
	task_t<void> updateSettings(std::string logLevel, std::vector<std::string> logTags);
	/**
	 * Create a WebRtcServer.
	 */
	task_t<WebRtcServer*> createWebRtcServer(const WebRtcServerOptions& options);
	/**
	 * Create a Router.
	 */
	task_t<Router*> createRouter(json& mediaCodecs, const json& appData = json::object());

private:
	void workerDied(const Error &error);

private:
	// mediasoup-worker child process.
	SubProcess* _child{ nullptr };
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
