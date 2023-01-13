#pragma once 

#include "Logger.h"
#include "common.h"
#include <EventEmitter.hpp>

namespace mediasoup {


class MS_EXPORT EnhancedEventEmitter : public EventEmitter
{
public:

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
			MSC_ERROR(
				"safeEmit() | event listener threw an error [event:%s]:%s",
				event.c_str(), error.what());

			return numListeners;
		}
	}

// 	async_simple::coro::Lazy<json> safeEmitAsPromise(event, ...args: any[])
// 	{
// 		return new Promise((resolve, reject) => (
// 			this->safeEmit(event, ...args, resolve, reject)
// 		));
// 	}

};

}
