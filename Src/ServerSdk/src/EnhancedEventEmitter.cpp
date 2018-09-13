#include "RainStream.hpp"
#include "EnhancedEventEmitter.hpp"
#include "Logger.hpp"

namespace rs
{
	EventBus::~EventBus()
	{
		events.clear();
		notifys.clear();
	}

	void EventBus::addEventListener(std::string event, EventListener eventListener)
	{
		events.insert(std::make_pair(event, eventListener));
	}

	void EventBus::addNotifyListener(NotifyListener notifyListener)
	{
		notifys.push_back(notifyListener);
	}

	void EventBus::doEvent(std::string event, const Json& data)
	{
		if (events.count(event))
		{
			auto range = events.equal_range(event);

			for (auto it = range.first; it != range.second; ++it)
			{
				EventListener eventListener= it->second;
				eventListener(data);
			}
		}
	}

	void EventBus::doNotify(std::string method, const Json& data)
	{
		for (auto &notify : notifys)
		{
			notify(method, data);
		}
	}


	EnhancedEventEmitter::EnhancedEventEmitter()
	{
		//this->setMaxListeners(Infinity);

		this->_logger = new Logger("EnhancedEventEmitter");
	}

}
