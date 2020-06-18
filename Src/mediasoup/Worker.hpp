#pragma once

#include "utils.hpp"
#include "common.hpp"
#include "EventEmitter.hpp"

class EnhancedEventEmitter;

namespace rs
{
	class Router;
	class Channel;
	class Request;
	class SubProcess;

	class Worker : public EventEmitter
	{
	public:
		Worker(json settings);

		void close();

		Defer dump();

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
}
