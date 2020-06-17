#pragma once

#include <set>
#include "EventEmitter.hpp"
#include "StringUtils.hpp"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

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
		Worker(std::string id, AStringVector parameters);

		void close();

		//Defer dump();

		//Defer updateSettings(json& options);

		/**
		 * Create a Room instance.
		 *
		 * @return {Room}
		 */
		//Router* createRouter(const json& data);


	private:
		std::string _id;

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
