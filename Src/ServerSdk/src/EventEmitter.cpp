#include "RainStream.hpp"
#include "EventEmitter.hpp"
#include "Logger.hpp"

namespace rs
{
	EventEmitter::~EventEmitter()
	{
		events.clear();
		notifys.clear();
	}

	void EventEmitter::addEventListener(std::string event, EventListener eventListener)
	{
		if (event != "newListener" && event != "removeListener")
		{
			doEvent("newListener", event);
		}

		events.insert(std::make_pair(event, eventListener));
	}

	void EventEmitter::addNotifyListener(NotifyListener notifyListener)
	{
		notifys.push_back(notifyListener);
	}

	void EventEmitter::removeListener(std::string event, EventListener eventListener)
	{
		if (events.count(event))
		{
			auto range = events.equal_range(event);

			for (auto it = range.first; it != range.second; ++it)
			{
				EventListener listener = it->second;
				if (listener.target<void(Json)>() == eventListener.target<void(Json)>())
				{
					events.erase(range.first);
					break;
				}
			}
		}

		if (event != "newListener" && event != "removeListener")
		{
			doEvent("removeListener", event);
		}
	}

	uint32_t EventEmitter::listeners(std::string event)
	{
		return events.count(event);
	}

	void EventEmitter::doEvent(std::string event, const Json& data)
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

	void EventEmitter::doNotify(std::string method, const Json& data)
	{
		for (auto &notify : notifys)
		{
			notify(method, data);
		}
	}

}
