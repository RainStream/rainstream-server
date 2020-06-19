#pragma once

#include <future>
#include "utils.hpp"
#include "common.hpp"
#include "EnhancedEventEmitter.hpp"


class Router;
class Channel;
class Request;
class SubProcess;

class Worker : public EnhancedEventEmitter
{
public:
	Worker(json settings);

	void close();

	std::future<json> dump();

	//Defer updateSettings(json& options);

	/**
	 * Create a Room instance.
	 *
	 * @return {Room}
	 */
	 //Router* createRouter(const json& data);


private:
	SubProcess* _child{ nullptr };

	uint32_t _pid{ 0 };

	Channel* _channel{ nullptr };

	Channel* _payloadChannel{ nullptr };

	json _appData{ json() };

	bool _closed = false;

	// Set of Room instances.
	std::set<Router*> _routers;

	EnhancedEventEmitter* _observer{ nullptr };
};
