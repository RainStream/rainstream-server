#pragma once

#include "Logger.hpp"

namespace rs
{
	class Consumer;
	class Producer;

	using EventListener = std::function<void(Json)>;
	using NotifyListener = std::function<void(std::string, Json)>;

	class EventEmitter
	{
	public:
		EventEmitter() = default;
		~EventEmitter();

	public:
		void addEventListener(std::string event, EventListener eventListener);
		void addNotifyListener(NotifyListener notifyListener);

		uint32_t listeners(std::string event);

		void doEvent(std::string event, const Json& data = Json::object());
		void doNotify(std::string method, const Json& data);

	private:
		std::unordered_multimap<std::string, EventListener> events;
		std::vector<NotifyListener> notifys;
	};

	class ChannelListener
	{
	public:
		virtual void onEvent(std::string event, Json data) = 0;
	};
}
