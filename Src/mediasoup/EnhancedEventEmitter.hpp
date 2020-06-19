#pragma once 

#include <EventEmitter.hpp>
#include "Logger.hpp"



class EnhancedEventEmitter : public EventEmitter
{
public:

	EnhancedEventEmitter()
		: EventEmitter()
		, logger(new Logger("EnhancedEventEmitter"))
	{
		
	}

	template <typename ... Arg>
	bool safeEmit(std::string event, Arg&& ... args)
	{
		const numListeners = this->listener_count(event);

		try
		{
			this->emit(event, ...args);
			return true;
		}
		catch (error)
		{
			logger->error(
				"safeEmit() | event listener threw an error [event:%s]:%o",
				event, error);

			return numListeners;
		}
	}

// 	async safeEmitAsPromise(event: string, ...args: any[]): Promise<any>
// 	{
// 		return new Promise((resolve, reject) => (
// 			this->safeEmit(event, ...args, resolve, reject)
// 		));
// 	}

protected:
	Logger* logger;

};
