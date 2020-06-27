#pragma once 

#include "Logger.hpp"
#include <EventEmitter.hpp>


class EnhancedEventEmitter : public EventEmitter
{
public:

	EnhancedEventEmitter()
		: logger(new Logger("EnhancedEventEmitter"))
	{
		
	}

	template <typename ... Arg>
	bool safeEmit(const std::string& event, Arg&& ... args)
	{
		int numListeners = this->listener_count(event);

		try
		{
			this->emit(event, std::forward<Arg>(args)...);
			return true;
		}
		catch (std::exception& error)
		{
			logger->error(
				"safeEmit() | event listener threw an error [event:%s]:%s",
				event.c_str(), error.what());

			return numListeners;
		}
	}

// 	std::future<json> safeEmitAsPromise(event, ...args: any[])
// 	{
// 		return new Promise((resolve, reject) => (
// 			this->safeEmit(event, ...args, resolve, reject)
// 		));
// 	}

	void removeAllListeners(std::string event)
	{
		std::unique_lock<std::mutex> locker(_events_mtx);

		events.erase(event);
	}

protected:
	Logger* logger;

};
