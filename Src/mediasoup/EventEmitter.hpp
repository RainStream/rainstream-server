#pragma once

#include "Logger.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

using json = nlohmann::json;

namespace rs
{
	using EventListener = std::function<void(json)>;
	using NotifyListener = std::function<void(std::string, json)>;

	class EventEmitter
	{
	public:
		EventEmitter() = default;
		~EventEmitter();

	public:
		void addEventListener(std::string event, EventListener eventListener);
		void addNotifyListener(NotifyListener notifyListener);

		void removeListener(std::string event, EventListener eventListener);

		uint32_t listeners(std::string event);

		void doEvent(std::string event, const json& data = json::object());
		void doNotify(std::string method, const json& data);

	private:
		std::unordered_multimap<std::string, EventListener> events;
		std::vector<NotifyListener> notifys;
	};

	class ChannelListener
	{
	public:
		virtual void onEvent(std::string event, json data) = 0;
	};
}
