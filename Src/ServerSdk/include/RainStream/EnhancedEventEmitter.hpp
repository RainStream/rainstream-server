#pragma once

#include "Logger.hpp"

namespace rs
{
	class Consumer;
	class Producer;

	using EventListener = std::function<void(Json)>;
	using NotifyListener = std::function<void(std::string, Json)>;

	class EventBus
	{
	public:
		EventBus() = default;
		~EventBus();

	public:
		void addEventListener(std::string event, EventListener eventListener);
		void addNotifyListener(NotifyListener notifyListener);

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

	class ConsumerListener
	{
	public:
		virtual void onNotify(Consumer* consumer, std::string method, Json data) = 0;
		virtual void onEvent(Consumer* consumer, std::string event, Json data) = 0;
	};

	class ProducerListener
	{
	public:
		virtual void onNotify(Producer* producer, std::string method, Json data) = 0;
		virtual void onEvent(Producer* producer, std::string event, Json data) = 0;
	};

	class EnhancedEventEmitter : public EventEmitter
	{
	public:
		EnhancedEventEmitter();

		template <typename ... Arg>
		void safeEmit(const std::string& event, Arg&& ... args)
		{
			try
			{
				this->emit(event, std::forward<Arg>(args)...);
			}
			catch (std::exception error)
			{
				LOG(ERROR) << 
					"safeEmit() | event listener threw an error [event:"
					<< event << ":" << error.what() << "]";
			}
		}
	};
}
