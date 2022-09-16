#pragma once

#include "EnhancedEventEmitter.hpp"


class Router;
class Channel;
class Request;
class SubProcess;
class PayloadChannel;

class MS_EXPORT Worker : public EnhancedEventEmitter
{
public:
	Worker(json settings);

	void close();

	uint32_t pid();

	std::future<json> dump();

	/**
	 * Get mediasoup-worker process resource usage.
	 */
	std::future<json> getResourceUsage();


	/**
	 * Create a Router.
	 */
	std::future<Router*> createRouter(json& mediaCodecs, json& appData = json::object());

private:
	SubProcess* _child{ nullptr };

	uint32_t _pid{ 0 };

	Channel* _channel{ nullptr };

	PayloadChannel* _payloadChannel{ nullptr };

	bool _closed = false;

	json _appData{ json() };

	// Routers set.
	std::set<Router*> _routers;

	EnhancedEventEmitter* _observer{ nullptr };
};
